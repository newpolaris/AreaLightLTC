#include <GL/glew.h>
#include <glfw3.h>
#include <string.h>

// GLM for matrix transformation
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

#include <tools/gltools.hpp>
#include <tools/Profile.h>
#include <tools/imgui.h>
#include <tools/TCamera.h>

#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsData.h>
#include <GLType/OGLDevice.h>
#include <GLType/ProgramShader.h>
#include <GLType/GraphicsFramebuffer.h>

#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>
#include <GLType/OGLCoreFramebuffer.h>

#include <GraphicsTypes.h>
#include <Light.h>
#include <SkyBox.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include <string.h>
#include <chrono>
#include <thread>

#include <math.h>
#include <stdio.h>

const char *vertexSource = "#version 450 core\n\
in vec3 point;\n\
in vec2 texcoord;\n\
out vec2 UV;\n\
void main()\n\
{\n\
  gl_Position = vec4(point, 1.0);\n\
  UV = texcoord;\n\
}";

const char *fragmentSource = "#version 450 core\n\
in vec2 UV;\n\
out vec3 fragColor;\n\
uniform sampler2D tex;\n\
void main()\n\
{\n\
  fragColor = texture(tex, UV).rgb;\n\
}";

GLuint vao;
GLuint vbo;
GLuint idx;
GLuint tex;
GLuint program;
GLuint vertexShader = 0;
GLuint fragmentShader = 0;

int width = 320;
int height = 240;

void onResize(int w, int h)
{
	width = w; height = h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}

void printError(const char *context)
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		fprintf(stderr, "%s: %s\n", context, gluErrorString(error));
	};
}

void printStatus(const char *step, GLuint context, GLuint status)
{
	GLint result = GL_FALSE;
	if (status == GL_LINK_STATUS)
		glGetProgramiv(context, GL_LINK_STATUS, &result);
	else
		glGetShaderiv(context, status, &result);

	if (result == GL_FALSE) {
		char buffer[1024];
		if (status == GL_COMPILE_STATUS)
			glGetShaderInfoLog(context, 1024, NULL, buffer);
		else
			glGetProgramInfoLog(context, 1024, NULL, buffer);
		if (buffer[0])
			fprintf(stderr, "%s: %s\n", step, buffer);
	};
}

void printCompileStatus(const char *step, GLuint context)
{
	printStatus(step, context, GL_COMPILE_STATUS);
}

void printLinkStatus(const char *step, GLuint context)
{
	printStatus(step, context, GL_LINK_STATUS);
}

GLfloat vertices[] = {
   0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
  -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
  -0.5f, -0.5f,  0.0f, 0.0f, 0.0f
};

unsigned int indices[] = { 0, 1, 2 };

float pixels[] = {
	0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f
};

bool bRotate = true;
float rotx, roty = 0;
float global_ambient_light[4] = { 0.2, 0.2, 0.2, 1.0 };
float ambient_light[4] = { 0.338, 0.338, 0.338, 1.0};
float diffuse_light[4] = { 1.0, 1.0, 1.0, 1.0 };
float materialColor[] = { 91.f/255, 0.0, 0.0, 1.0 };
float lightpos[4] = {-5, 5, 10, 0};

namespace 
{
    static const uint32_t NumSamples = 4;

    bool s_bSampleReset = false;
    bool s_bUiChanged = false;
    float s_CpuTick = 0.f;
    float s_GpuTick = 0.f;
    int32_t s_SampleCount = 0;
}

class AreaLight final : public gamecore::IGameApp
{
public:
	AreaLight() noexcept;
	virtual ~AreaLight() noexcept;

	virtual void startup() noexcept override;
	virtual void closeup() noexcept override;
	virtual void update() noexcept override;
    virtual void updateHUD() noexcept override;
	virtual void render() noexcept override;

	virtual void keyboardCallback(uint32_t c, bool bPressed) noexcept override;
	virtual void framesizeCallback(int32_t width, int32_t height) noexcept override;
	virtual void motionCallback(float xpos, float ypos, bool bPressed) noexcept override;
	virtual void mouseCallback(float xpos, float ypos, bool bPressed) noexcept override;

	GraphicsDevicePtr createDevice(const GraphicsDeviceDesc& desc) noexcept;

    ShaderPtr submitPerFrameUniformLight(ShaderPtr& shader) noexcept;

private:
};

CREATE_APPLICATION(AreaLight);

AreaLight::AreaLight() noexcept
{
}

AreaLight::~AreaLight() noexcept
{
}

void AreaLight::startup() noexcept
{
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	printCompileStatus("Vertex shader", vertexShader);

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	printCompileStatus("Fragment shader", fragmentShader);

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	printLinkStatus("Shader program", program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &idx);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // glGetAttribLocation requires program
    glUseProgram(program);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glVertexAttribPointer(glGetAttribLocation(program, "point"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
	glVertexAttribPointer(glGetAttribLocation(program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx);

	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

    // uniform1i requires program
    glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_BGR, GL_FLOAT, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);

}

void AreaLight::closeup() noexcept
{
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &tex);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &idx);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vbo);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);

	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteProgram(program);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void AreaLight::update() noexcept
{
}

void AreaLight::updateHUD() noexcept
{
    bool bUpdated = false;
    float width = (float)getWindowWidth(), height = (float)getWindowHeight();

    ImGui::SetNextWindowPos(
        ImVec2(width - width / 4.f - 10.f, 10.f),
        ImGuiSetCond_FirstUseEver);

    ImGui::Begin("Settings",
        NULL,
        ImVec2(width / 4.0f, height - 20.0f),
        ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PushItemWidth(180.0f);
    ImGui::Indent();
    {
        // global
        {
            ImGui::Text("CPU %s: %10.5f ms\n", "Main", s_CpuTick);
            ImGui::Text("GPU %s: %10.5f ms\n", "Main", s_GpuTick);
            ImGui::Separator();
            bUpdated |= ImGui::Checkbox("Rotate", &bRotate);
            ImGui::Separator();
            ImGui::ColorWheel("global ambient light color:", global_ambient_light, 0.6f);
            ImGui::ColorWheel("ambient light color:", ambient_light, 0.6f);
            ImGui::ColorWheel("diffuse light color:", diffuse_light, 0.6f);
			ImGui::ColorWheel("material ambient/diffuse color:", materialColor, 0.6f);
        }
        ImGui::Separator();
        ImGui::Separator();
        // Local
        {
        }
    }
    ImGui::Unindent();
    ImGui::End();
}

void AreaLight::render() noexcept
{
	std::this_thread::sleep_for(std::chrono::milliseconds(33));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glDrawElements requires array object
	glUseProgram(program);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void *)0);
	glBindVertexArray(0);
}

void AreaLight::keyboardCallback(uint32_t key, bool isPressed) noexcept
{
	switch (key)
	{
	case GLFW_KEY_UP:
		break;

	case GLFW_KEY_DOWN:
		break;

	case GLFW_KEY_LEFT:
		break;

	case GLFW_KEY_RIGHT:
		break;
    }
}

void AreaLight::framesizeCallback(int32_t width, int32_t height) noexcept
{
	glViewport(0, 0, width, height);
}

void AreaLight::motionCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
}

void AreaLight::mouseCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
}

GraphicsDevicePtr AreaLight::createDevice(const GraphicsDeviceDesc& desc) noexcept
{
	return nullptr;
}

ShaderPtr AreaLight::submitPerFrameUniformLight(ShaderPtr& shader) noexcept
{
	return nullptr;
}


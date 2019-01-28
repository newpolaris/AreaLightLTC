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
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_LIGHT0);
}

void AreaLight::closeup() noexcept
{
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

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient_light);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -3);
	glRotatef(rotx, 1, 0, 0);
	glRotatef(roty, 0, 1, 1);

	if (bRotate) {
		rotx += 0.5;
		roty += 0.5;
	}

	glBegin(GL_QUADS);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialColor);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialColor);

	glNormal3f(0, 0, 1);
	glVertex3f(-1, -1, 1);
	glVertex3f(1, -1, 1);
	glVertex3f(1, 1, 1);
	glVertex3f(-1, 1, 1);

	glNormal3f(0, 0, -1);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, 1, -1);
	glVertex3f(1, 1, -1);
	glVertex3f(1, -1, -1);

	glNormal3f(0, 1, 0);
	glVertex3f(-1, 1, -1);
	glVertex3f(-1, 1, 1);
	glVertex3f(1, 1, 1);
	glVertex3f(1, 1, -1);

	glNormal3f(0, -1, 0);
	glVertex3f(-1, -1, -1);
	glVertex3f(1, -1, -1);
	glVertex3f(1, -1, 1);
	glVertex3f(-1, -1, 1);

	glNormal3f(1, 0, 0);
	glVertex3f(1, -1, -1);
	glVertex3f(1, 1, -1);
	glVertex3f(1, 1, 1);
	glVertex3f(1, -1, 1);

	glNormal3f(-1, 0, 0);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, -1, 1);
	glVertex3f(-1, 1, 1);
	glVertex3f(-1, 1, -1);

	glEnd();
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(-1, 1, -1, 1, 1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
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

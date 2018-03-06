#include <GL/glew.h>
#include <glfw3.h>

// GLM for matrix transformation
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

#include <tools/gltools.hpp>
#include <tools/SimpleProfile.h>
#include <tools/imgui.h>
#include <tools/TCamera.h>

#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsData.h>
#include <GLType/OGLDevice.h>
#include <GLType/ProgramShader.h>
#include <GLType/Framebuffer.h>

#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>

#include <GraphicsTypes.h>
#include <Light.h>
#include <SkyBox.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>

// #define DRAW_LTC_MAT

struct _Light
{
    float m_RotY = 0.f;
    float m_RotZ = 0.f;
};

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
    virtual void scrollCallback(float xoffset, float yoffset) noexcept override;

	GraphicsDevicePtr createDevice(const GraphicsDeviceDesc& desc) noexcept;

private:

    float m_Zoom;
    int32_t m_RotX, m_RotY;
	TCamera m_Camera;
    light::Light m_Light;
    glm::mat4 m_View;
    FullscreenTriangleMesh m_ScreenTraingle;
    PlaneMesh m_Plane;
    CubeMesh m_Cube;
    ProgramShader m_Shader;
    ProgramShader m_BlitShader;
    GraphicsDevicePtr m_Device;
    GraphicsTexturePtr m_FilteredTex;
};

CREATE_APPLICATION(AreaLight);

AreaLight::AreaLight() noexcept
    : m_RotX(0), m_RotY(0), m_Zoom(0.f)
{
}

AreaLight::~AreaLight() noexcept
{
}

void AreaLight::startup() noexcept
{
	m_Camera.setViewParams(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f) );
	m_Camera.setMoveCoefficient(0.35f);

	GraphicsDeviceDesc deviceDesc;
#if __APPLE__
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGL);
#else
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif
	m_Device = createDevice(deviceDesc);
	assert(m_Device);

	light::initialize(m_Device);
	
	m_Shader.setDevice(m_Device);
	m_Shader.initialize();
	m_Shader.addShader(GL_VERTEX_SHADER, "Flat.Vertex");
	m_Shader.addShader(GL_FRAGMENT_SHADER, "Flat.Fragment");
	m_Shader.link();

	m_BlitShader.setDevice(m_Device);
	m_BlitShader.initialize();
	m_BlitShader.addShader(GL_VERTEX_SHADER, "BlitTexture.Vertex");
	m_BlitShader.addShader(GL_FRAGMENT_SHADER, "BlitTexture.Fragment");
	m_BlitShader.link();

    m_ScreenTraingle.create();
	m_Plane.create();
    m_Cube.create();

    // TODO: move to texture descriptor
    auto type = m_Device->getGraphicsDeviceDesc().getDeviceType();
    auto SetParameter = [type](GraphicsTexturePtr& tex, GLenum pname, GLint param)
    {
        if(type == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
        {
            auto texture = tex->downcast_pointer<OGLCoreTexture>();
            if(texture) texture->parameter(pname, param);
        }
        else if(type == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
        {
            auto texture = tex->downcast_pointer<OGLTexture>();
            if(texture) texture->parameter(pname, param);
        }
    };

    GraphicsTextureDesc filteredDesc;
    filteredDesc.setFilename("resources/stained_glass_filtered.dds");
    m_FilteredTex = m_Device->createTexture(filteredDesc);
    SetParameter(m_FilteredTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    SetParameter(m_FilteredTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    SetParameter(m_FilteredTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    SetParameter(m_FilteredTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    GraphicsTextureDesc source;
    source.setFilename("resources/stained_glass.dds");
    auto lightSource = m_Device->createTexture(source);

	// rotate toward ground and tilt slightly
	// auto rot = glm::angleAxis(glm::pi<float>(), glm::vec3(1, 0, 0));
	// rot = glm::angleAxis(glm::pi<float>()*0.25f, glm::vec3(0, 0, 1)) * rot;
	auto rot = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0));

	m_Light.setRotation(rot);
	m_Light.setPosition(glm::vec3(0, -1, 2));
    m_Light.setLightFilterd(m_FilteredTex);
    m_Light.create();
}

void AreaLight::closeup() noexcept
{
    m_ScreenTraingle.destroy();
    light::shutdown();
}

void AreaLight::update() noexcept
{
    m_Camera.update();
}

void AreaLight::updateHUD() noexcept
{
    float width = (float)getFrameWidth(), height = (float)getFrameHeight();

    ImGui::SetNextWindowPos(
        ImVec2(width - width / 4.f - 10.f, 10.f),
        ImGuiSetCond_FirstUseEver);

    ImGui::Begin("Settings",
        NULL,
        ImVec2(width / 4.0f, height - 20.0f),
        ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PushItemWidth(180.0f);
    ImGui::Indent();
        ImGui::SliderFloat("Roughness", &m_Light.m_Roughness, 0.01f, 1.f);
        ImGui::ColorWheel("Diffuse Color:", glm::value_ptr(m_Light.m_Diffuse), 0.6f);
        ImGui::ColorWheel("Specular Color:", glm::value_ptr(m_Light.m_Specular), 0.6f);
        ImGui::SliderFloat("Intensity", &m_Light.m_Intensity, 0.f, 10.f);
        ImGui::SliderFloat("Width", &m_Light.m_Width, 0.1f, 15.f);
        ImGui::SliderFloat("Height", &m_Light.m_Height, 0.1f, 15.f);
        // ImGui::SliderFloat("Rotation Y", &m_Light.m_RotY, 0.f, 1.f);
        // ImGui::SliderFloat("Rotation Z", &m_Light.m_RotZ, 0.f, 1.f);
        ImGui::Checkbox("Tow sided", &m_Light.m_bTwoSided);
        ImGui::Checkbox("Textured Light", &m_Light.m_bTexturedLight);
    ImGui::Unindent();
    ImGui::End();
}

void AreaLight::render() noexcept
{
	// Rendering
	glViewport(0, 0, getFrameWidth(), getFrameHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, isWireframe() ? GL_LINE : GL_FILL);

    glm::vec2 resolution = glm::vec2((float)getFrameWidth(), (float)getFrameHeight());
	glm::mat4 projection = m_Camera.getProjectionMatrix();
	glm::mat4 view = m_Camera.getViewMatrix();
	glm::vec4 mat_ambient = glm::vec4(glm::vec3(0.1f), 1.f);
	glm::vec4 mat_diffuse = glm::vec4(glm::vec3(0.8f), 1.f);
	glm::vec4 mat_specular = glm::vec4(glm::vec3(0.8f), 1.f);;
	glm::vec4 mat_emissive = glm::vec4(0.0f);
	float mat_shininess = 10.0;

    glm::vec3 dcolor = glm::vec3(1.f, 1.f, 1.f);
    glm::vec3 scolor = glm::vec3(1.f, 1.f, 1.f);

	// plane
	{
		glm::mat4 model = glm::mat4(1.f);
		model = glm::translate(model, glm::vec3(3.0f, -3.5f, 0.0));
        m_Shader.bind();
        m_Shader.setUniform("projection", projection);
        m_Shader.setUniform("view", view);
		m_Shader.setUniform("model", model);
        m_Shader.setUniform("color", glm::vec3(1.f));
		m_Plane.draw();
	}

	m_Light.draw(m_Camera, resolution);
}

void AreaLight::keyboardCallback(uint32_t key, bool isPressed) noexcept
{
	switch (key)
	{
	case GLFW_KEY_UP:
		m_Camera.keyboardHandler(MOVE_FORWARD, isPressed);
		break;

	case GLFW_KEY_DOWN:
		m_Camera.keyboardHandler(MOVE_BACKWARD, isPressed);
		break;

	case GLFW_KEY_LEFT:
		m_Camera.keyboardHandler(MOVE_LEFT, isPressed);
		break;

	case GLFW_KEY_RIGHT:
		m_Camera.keyboardHandler(MOVE_RIGHT, isPressed);
		break;
	}
}

void AreaLight::framesizeCallback(int32_t width, int32_t height) noexcept
{
	float aspectRatio = (float)width/height;
	m_Camera.setProjectionParams(45.0f, aspectRatio, 0.1f, 100.0f);
}

void AreaLight::motionCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
	if (!mouseOverGui && bPressed) m_Camera.motionHandler(int(xpos), int(ypos), false);    
}

void AreaLight::mouseCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
	if (!mouseOverGui && bPressed) m_Camera.motionHandler(int(xpos), int(ypos), true); 
}

void AreaLight::scrollCallback(float xoffset, float yoffset) noexcept
{
    m_Zoom += yoffset;
}

GraphicsDevicePtr AreaLight::createDevice(const GraphicsDeviceDesc& desc) noexcept
{
	GraphicsDeviceType deviceType = desc.getDeviceType();

#if __APPLE__
	assert(deviceType != GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif

	if (deviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGL ||
		deviceType == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto device = std::make_shared<OGLDevice>();
        if (device->create(desc))
            return device;
        return nullptr;
    }
    return nullptr;
}

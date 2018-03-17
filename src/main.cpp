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
#include <GLType/GraphicsFramebuffer.h>

#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>
#include <GLType/OGLCoreFramebuffer.h>

#include <GraphicsTypes.h>
#include <SkyBox.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include <LtcTables.h>

// #define DRAW_LTC_MAT

struct Light
{
    float m_Width = 8.f;
    float m_Height = 10.f;
    float m_RotY = 0.f;
    float m_RotZ = 0.f;
    float m_Roughness = 0.25f;
    glm::vec4 m_Diffuse = glm::vec4(1.f);
    glm::vec4 m_Specular = glm::vec4(1.f);
    float m_Intensity = 4.0f;
    bool m_bTwoSided = false;
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

    int32_t m_RotX, m_RotY;
    float m_Zoom;
    Light m_Light;
    glm::mat4 m_View;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_Shader;
    ProgramShader m_BlitShader;
    GraphicsDevicePtr m_Device;
    GraphicsTexturePtr m_Ltc1Tex;
    GraphicsTexturePtr m_Ltc2Tex;
    GraphicsTexturePtr m_Texture;
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
	GraphicsDeviceDesc deviceDesc;
#if __APPLE__
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGL);
#else
	deviceDesc.setDeviceType(GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore);
#endif
	m_Device = createDevice(deviceDesc);
	assert(m_Device);

	m_Shader.setDevice(m_Device);
	m_Shader.initialize();
	m_Shader.addShader(GL_VERTEX_SHADER, "Ltc.Vertex");
	m_Shader.addShader(GL_FRAGMENT_SHADER, "Ltc.Fragment");
	m_Shader.link();

	m_BlitShader.setDevice(m_Device);
	m_BlitShader.initialize();
	m_BlitShader.addShader(GL_VERTEX_SHADER, "BlitTexture.Vertex");
	m_BlitShader.addShader(GL_FRAGMENT_SHADER, "BlitTexture.Fragment");
	m_BlitShader.link();

    m_ScreenTraingle.create();
	
	GraphicsTextureDesc filteredDesc;
    filteredDesc.setFilename("resources/stained_glass_filtered.dds");
    filteredDesc.setWrapS(GL_CLAMP_TO_EDGE);
    filteredDesc.setWrapT(GL_CLAMP_TO_EDGE);
    filteredDesc.setMinFilter(GL_NEAREST);
    filteredDesc.setMagFilter(GL_NEAREST);
    filteredDesc.setAnisotropyLevel(16);
    m_Texture = m_Device->createTexture(filteredDesc);

    GraphicsTextureDesc ltcMatDesc;
    ltcMatDesc.setFilename("resources/ltc_1.dds");
    ltcMatDesc.setWrapS(GL_CLAMP_TO_EDGE);
    ltcMatDesc.setWrapT(GL_CLAMP_TO_EDGE);
    ltcMatDesc.setMinFilter(GL_NEAREST);
    ltcMatDesc.setMagFilter(GL_LINEAR);
    m_Ltc1Tex = m_Device->createTexture(ltcMatDesc);

    GraphicsTextureDesc ltcMagDesc;
    ltcMagDesc.setFilename("resources/ltc_2.dds");
    ltcMagDesc.setWrapS(GL_CLAMP_TO_EDGE);
    ltcMagDesc.setWrapT(GL_CLAMP_TO_EDGE);
    ltcMagDesc.setMinFilter(GL_NEAREST);
    ltcMagDesc.setMagFilter(GL_LINEAR);
    m_Ltc2Tex = m_Device->createTexture(ltcMagDesc);
}

void AreaLight::closeup() noexcept
{
    m_ScreenTraingle.destroy();
}

void AreaLight::update() noexcept
{
    // Add in camera controller's rotation
    m_View = glm::mat4(1.f);
    m_View = glm::translate(m_View, glm::vec3(0, 6, 0.5f*m_Zoom - 0.5f));
    m_View = glm::rotate(m_View, float(m_RotX + 10)/25, glm::vec3(1, 0, 0));
    m_View = glm::rotate(m_View, float(m_RotY)/25, glm::vec3(0, 1, 0));
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
        ImGui::SliderFloat("Rotation Y", &m_Light.m_RotY, 0.f, 1.f);
        ImGui::SliderFloat("Rotation Z", &m_Light.m_RotZ, 0.f, 1.f);
        ImGui::Checkbox("Tow sided", &m_Light.m_bTwoSided);
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
	glm::vec4 mat_ambient = glm::vec4(glm::vec3(0.1f), 1.f);
	glm::vec4 mat_diffuse = glm::vec4(glm::vec3(0.8f), 1.f);
	glm::vec4 mat_specular = glm::vec4(glm::vec3(0.8f), 1.f);;
	glm::vec4 mat_emissive = glm::vec4(0.0f);
	float mat_shininess = 10.0;

    glm::vec3 dcolor = glm::vec3(1.f, 1.f, 1.f);
    glm::vec3 scolor = glm::vec3(1.f, 1.f, 1.f);

	m_Shader.bind();
    m_Shader.setUniform("ubTwoSided", m_Light.m_bTwoSided);
    m_Shader.setUniform("ubTextured", true);
    m_Shader.setUniform("uIntensity", m_Light.m_Intensity);
    m_Shader.setUniform("uView", m_View);
    m_Shader.setUniform("uResolution", resolution);
    m_Shader.setUniform("uDcolor", glm::vec3(m_Light.m_Diffuse));
    m_Shader.setUniform("uScolor", glm::vec3(m_Light.m_Specular));
    m_Shader.setUniform("uWidth", m_Light.m_Width);
    m_Shader.setUniform("uHeight", m_Light.m_Height);
    m_Shader.setUniform("uRotY", m_Light.m_RotY);
    m_Shader.setUniform("uRotZ", m_Light.m_RotZ);
    m_Shader.setUniform("uRoughness", m_Light.m_Roughness);
    m_Shader.bindTexture("uLtc1", m_Ltc1Tex, 0);
    m_Shader.bindTexture("uLtc2", m_Ltc2Tex, 1);
    m_Shader.bindTexture("tex", m_Texture, 2);
    m_ScreenTraingle.draw();
}

void AreaLight::keyboardCallback(uint32_t key, bool isPressed) noexcept
{
	switch (key)
	{
	case GLFW_KEY_UP:
        m_RotX--;
		break;

	case GLFW_KEY_DOWN:
        m_RotX++;
		break;

	case GLFW_KEY_LEFT:
        m_RotY--;
		break;

	case GLFW_KEY_RIGHT:
        m_RotY++;
		break;
	}
}

void AreaLight::framesizeCallback(int32_t width, int32_t height) noexcept
{
}

void AreaLight::motionCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
}

void AreaLight::mouseCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
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

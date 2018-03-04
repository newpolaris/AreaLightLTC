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
#include <GLType/OGLCoreTexture.h>
#include <GLType/Framebuffer.h>

#include <GraphicsTypes.h>
#include <SkyBox.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>
#include <LtcTables.h>

class AreaLight final : public gamecore::IGameApp
{
public:
	AreaLight() noexcept;
	virtual ~AreaLight() noexcept;

	virtual void startup() noexcept;
	virtual void closeup() noexcept;
	virtual void update() noexcept;
	virtual void render() noexcept;

	virtual void keyboardCallback(uint32_t c, bool bPressed) noexcept;
	virtual void framesizeCallback(int32_t width, int32_t height) noexcept;
	virtual void motionCallback(float xpos, float ypos, bool bPressed) noexcept;
	virtual void mouseCallback(float xpos, float ypos, bool bPressed) noexcept;

	GraphicsDevicePtr createDevice(const GraphicsDeviceDesc& desc) noexcept;

private:

    int32_t m_RotX, m_RotY;
    glm::mat4 m_View;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_Shader;
    ProgramShader m_BlitShader;
    GraphicsDevicePtr m_Device;
    GraphicsTexturePtr m_WoodTex;
    GraphicsTexturePtr m_LtcMatTex;
    GraphicsTexturePtr m_LtcMagTex;
};

CREATE_APPLICATION(AreaLight);

AreaLight::AreaLight() noexcept
    : m_RotX(0), m_RotY(0)
{
}

AreaLight::~AreaLight() noexcept
{
}

void AreaLight::startup() noexcept
{
	// App Objects

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

    GraphicsTextureDesc woodDesc;
    woodDesc.setFilename("resources/wood.png");
    m_WoodTex = m_Device->createTexture(woodDesc);

    GraphicsTextureDesc ltcMatDesc;
    ltcMatDesc.setTarget(gli::TARGET_2D);
    ltcMatDesc.setFormat(gli::FORMAT_RGBA32_SFLOAT_PACK32);
    ltcMatDesc.setWidth(64);
    ltcMatDesc.setHeight(64);
    ltcMatDesc.setStream((uint8_t*)g_ltc_mat);
    ltcMatDesc.setStreamSize(sizeof(g_ltc_mat));
    m_LtcMatTex = m_Device->createTexture(ltcMatDesc);

    GraphicsTextureDesc ltcMagDesc;
    ltcMagDesc.setTarget(gli::TARGET_2D);
    ltcMagDesc.setFormat(gli::FORMAT_R32_SFLOAT_PACK32);
    ltcMagDesc.setWidth(64);
    ltcMagDesc.setHeight(64);
    ltcMagDesc.setStream((uint8_t*)g_ltc_mag);
    ltcMagDesc.setStreamSize(sizeof(g_ltc_mag));
    m_LtcMagTex = m_Device->createTexture(ltcMagDesc);
}

void AreaLight::closeup() noexcept
{
    m_ScreenTraingle.destroy();
}

void AreaLight::update() noexcept
{
    // Add in camera controller's rotation
    m_View = glm::mat4(1.f);
    m_View = glm::translate(m_View, glm::vec3(0, 6, 3));
    m_View = glm::rotate(m_View, float(m_RotY)/25, glm::vec3(1, 0, 0));
    m_View = glm::rotate(m_View, float(m_RotX)/25, glm::vec3(0, 1, 0));
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

#if 1
	m_Shader.bind();
    m_Shader.setUniform("uTwoSided", false);
    m_Shader.setUniform("uIntensity", 4.f);
    m_Shader.setUniform("uView", m_View);
    m_Shader.setUniform("uResolution", resolution);
    m_Shader.setUniform("uDcolor", dcolor);
    m_Shader.setUniform("uScolor", scolor);
    m_Shader.setUniform("uRoughness", 0.2f);
    m_Shader.bindTexture("ltc_mat", m_LtcMatTex, 0);
    m_Shader.bindTexture("ltc_mag", m_LtcMagTex, 1);
#else
    m_BlitShader.bind();
    m_BlitShader.bindTexture("uTexSource", m_LtcMatTex, 0);
#endif
    m_ScreenTraingle.draw();
}

void AreaLight::keyboardCallback(uint32_t key, bool isPressed) noexcept
{
	switch (key)
	{
	case GLFW_KEY_UP:
        m_RotY--;
		break;

	case GLFW_KEY_DOWN:
        m_RotY++;
		break;

	case GLFW_KEY_LEFT:
        m_RotX--;
		break;

	case GLFW_KEY_RIGHT:
        m_RotX++;
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

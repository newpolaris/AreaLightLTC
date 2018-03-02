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

    TCamera m_Camera;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_Shader;
    GraphicsDevicePtr m_Device;
    GraphicsTexturePtr m_Texture;
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
	// App Objects
	m_Camera.setViewParams(glm::vec3( 0.0f, 0.0f, 3.0f), glm::vec3( 0.0f, 0.0f, 0.0f));
	m_Camera.setMoveCoefficient(0.35f);

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
	m_Shader.addShader(GL_VERTEX_SHADER, "BlitTexture.Vertex");
	m_Shader.addShader(GL_FRAGMENT_SHADER, "BlitTexture.Fragment");
	m_Shader.link();

    m_ScreenTraingle.create();

    GraphicsTextureDesc desc;
    desc.setFilename("resources/wood.png");
    m_Texture = m_Device->createTexture(desc);
}

void AreaLight::closeup() noexcept
{
    m_ScreenTraingle.destroy();
}

void AreaLight::update() noexcept
{
	m_Camera.update();
}

void AreaLight::render() noexcept
{
	// Rendering
	glViewport(0, 0, GetFrameWidth(), GetFrameHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, isWireframe() ? GL_LINE : GL_FILL);

	glm::mat4 projection = m_Camera.getProjectionMatrix();
	glm::mat4 view = m_Camera.getViewMatrix();
	glm::vec4 mat_ambient = glm::vec4(glm::vec3(0.1f), 1.f);
	glm::vec4 mat_diffuse = glm::vec4(glm::vec3(0.8f), 1.f);
	glm::vec4 mat_specular = glm::vec4(glm::vec3(0.8f), 1.f);;
	glm::vec4 mat_emissive = glm::vec4(0.0f);
	float mat_shininess = 10.0;

	m_Shader.bind();
    m_Shader.bindTexture("uTexSource", m_Texture, 0);
    m_ScreenTraingle.draw();
}

void AreaLight::keyboardCallback(uint32_t key, bool isPressed) noexcept
{
	switch (key)
	{
	case GLFW_KEY_UP:
		m_Camera.keyboardHandler( MOVE_FORWARD, isPressed);
		break;

	case GLFW_KEY_DOWN:
		m_Camera.keyboardHandler( MOVE_BACKWARD, isPressed);
		break;

	case GLFW_KEY_LEFT:
		m_Camera.keyboardHandler( MOVE_LEFT, isPressed);
		break;

	case GLFW_KEY_RIGHT:
		m_Camera.keyboardHandler( MOVE_RIGHT, isPressed);
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
	if (!mouseOverGui && bPressed) m_Camera.motionHandler( int(xpos), int(ypos), false);    
}

void AreaLight::mouseCallback(float xpos, float ypos, bool bPressed) noexcept
{
	const bool mouseOverGui = ImGui::MouseOverArea();
	if (!mouseOverGui && bPressed) m_Camera.motionHandler( int(xpos), int(ypos), true); 
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

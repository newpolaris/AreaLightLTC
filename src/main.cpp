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
#include <Light.h>
#include <SkyBox.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <GameCore.h>

typedef std::shared_ptr<class Model> ModelPtr;
typedef std::shared_ptr<class Mesh> MeshPtr;
typedef std::shared_ptr<class Light> LightPtr;
typedef std::vector<MeshPtr> MeshList;
typedef std::vector<ModelPtr> ModelList;
typedef std::vector<LightPtr> LightList;

class Model final
{
public:

    Model() noexcept;
    virtual ~Model() noexcept;

    void appendMesh(MeshPtr&& mesh) noexcept;
    void setWorld(const glm::mat4& world) noexcept;
    void submit(const ShaderPtr& shader) noexcept;

private:

    glm::mat4 m_World;
    MeshList m_Meshes;
};

Model::Model() noexcept
    : m_World(1.f)
{
}

Model::~Model() noexcept
{
}

void Model::appendMesh(MeshPtr&& mesh) noexcept
{
    m_Meshes.emplace_back(std::move(mesh));
}

void Model::setWorld(const glm::mat4& world) noexcept
{
    m_World = world;
}

void Model::submit(const ShaderPtr& shader) noexcept
{
    shader->setUniform("uWorld", m_World);
    for (auto& mesh : m_Meshes)
        mesh->draw();
}

template <typename T, typename... Args>
ModelPtr createPrimitive(const glm::mat4& world, Args&&... args)
{
    auto mesh = std::make_shared<T>(std::forward<Args>(args)...);
    mesh->create();

    ModelPtr model = std::make_shared<Model>();;
    model->appendMesh(mesh);
    model->setWorld(world);
    return model;
}

struct SceneSettings
{
    int32_t m_SampleCount = 0;
    uint32_t m_LightIndex = 0;
    float m_Roughness = 0.25f;
    float m_F0 = 0.04f; // fresnel
    glm::vec4 m_Albedo = glm::vec4(0.5f, 0.5f, 0.5f, 1.f); // additional albedo
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

	GraphicsDevicePtr createDevice(const GraphicsDeviceDesc& desc) noexcept;

    ShaderPtr submitPerFrameUniformLight(ShaderPtr& shader) noexcept;

private:

    SceneSettings m_Settings;
	TCamera m_Camera;
    LightList m_Lights;
    ModelList m_Models;
    FullscreenTriangleMesh m_ScreenTraingle;
    ProgramShader m_BlitShader;
    GraphicsTexturePtr m_ScreenColorTex;
    GraphicsFramebufferPtr m_ColorRenderTarget;
    GraphicsDevicePtr m_Device;
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
	m_Camera.setViewParams(glm::vec3(0.0f, 5.0f, 15.0f), glm::vec3(0.0f, 0.0f, 0.0f));
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
    filteredDesc.setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
    filteredDesc.setMagFilter(GL_LINEAR);
    filteredDesc.setAnisotropyLevel(16);
    auto filteredTex = m_Device->createTexture(filteredDesc);

    GraphicsTextureDesc source;
    source.setFilename("resources/stained_glass.dds");
    source.setAnisotropyLevel(16);
    auto lightSource = m_Device->createTexture(source);

	auto rot = glm::angleAxis(glm::half_pi<float>(), glm::vec3(1, 0, 0));
    auto light = std::make_shared<Light>();
	light->setRotation(glm::vec3(90.f, 0, 0));
	light->setPosition(glm::vec3(0, -1, 2));
    light->setTexturedLight(true);
    light->setLightSource(lightSource);
    light->setLightFilterd(filteredTex);
    m_Lights.emplace_back(std::move(light));

    // Ground plane
    m_Models.emplace_back(createPrimitive<PlaneMesh>(glm::mat4(1.f)));

    // Simple cube
    {
        glm::mat4 world = glm::mat4(1.f);
        world = glm::translate(world, glm::vec3(-2.f, 1.f, 8.f));
        m_Models.emplace_back(createPrimitive<CubeMesh>(world));
    }
    // Simple sphere
    {
        glm::mat4 world = glm::mat4(1.f);
        world = glm::translate(world, glm::vec3(2.f, 0.f, 8.f));
        m_Models.emplace_back(createPrimitive<SphereMesh>(world, 32));
    }
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
    {
        // global
        {
            ImGui::SliderFloat("Roughness", &m_Settings.m_Roughness, 0.03f, 1.f);
            ImGui::SliderFloat("Fresnel 0", &m_Settings.m_F0, 0.01f, 1.f);
            ImGui::ColorWheel("Albedo Color:", glm::value_ptr(m_Settings.m_Albedo), 0.6f);
            ImGui::Separator();
        }
        if (m_Lights.size() > 1)
        {
            auto idx = (float)m_Settings.m_LightIndex;
            ImGui::SliderFloat("Roughness", &idx, 0.0f, (float)m_Lights.size() - 1);
            m_Settings.m_LightIndex = (uint32_t)idx;
            ImGui::Separator();
        }
        // Local
        {
            auto idx = m_Settings.m_LightIndex;
            ImGui::SliderFloat("Intensity", &m_Lights[idx]->m_Intensity, 0.f, 10.f);
            ImGui::SliderFloat("Width", &m_Lights[idx]->m_Width, 0.1f, 15.f);
            ImGui::SliderFloat("Height", &m_Lights[idx]->m_Height, 0.1f, 15.f);
            ImGui::SliderFloat("Position X", &m_Lights[idx]->m_Position.x, -10.f, 10.f);
            ImGui::SliderFloat("Position Y", &m_Lights[idx]->m_Position.y, -10.f, 10.f);
            ImGui::SliderFloat("Position Z", &m_Lights[idx]->m_Position.z, -10.f, 10.f);
            ImGui::SliderFloat("Rotation X", &m_Lights[idx]->m_Rotation.x, -180.f, 179.f);
            ImGui::SliderFloat("Rotation Y", &m_Lights[idx]->m_Rotation.y, -180.f, 179.f);
            ImGui::SliderFloat("Rotation Z", &m_Lights[idx]->m_Rotation.z, -180.f, 179.f);
            ImGui::Checkbox("Tow sided", &m_Lights[idx]->m_bTwoSided);
            ImGui::Checkbox("Textured Light", &m_Lights[idx]->m_bTexturedLight);
        }
    }
    ImGui::Unindent();
    ImGui::End();
}

void AreaLight::render() noexcept
{
    RenderingData renderData { 
        m_Camera.getPosition(),
        m_Camera.getViewMatrix(),
        m_Camera.getProjectionMatrix(),
    };

    m_Device->setFramebuffer(m_ColorRenderTarget);
	glViewport(0, 0, getFrameWidth(), getFrameHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, isWireframe() ? GL_LINE : GL_FILL);

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_ONE, GL_ONE); // additive
    auto areaProgram = Light::BindAreaProgram(renderData);
    for (auto& light : m_Lights)
        light->submit(areaProgram);

    auto lightProgram = Light::BindLightProgram(renderData);
    lightProgram = submitPerFrameUniformLight(lightProgram);
    for (auto& light : m_Lights)
    {
        lightProgram = light->submitPerLightUniforms(lightProgram);
        for (auto& model : m_Models)
            model->submit(lightProgram);
    }
    // glDisable(GL_BLEND);

    // TODO: default frame buffer with/without depth test
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, getFrameWidth(), getFrameHeight());

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); // additive
    if (m_Settings.m_SampleCount == 0)
        glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    m_BlitShader.bind();
    m_BlitShader.bindTexture("uTexSource", m_ScreenColorTex, 0);
    m_ScreenTraingle.draw();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
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

    GraphicsTextureDesc colorDesc;
    colorDesc.setWidth(width);
    colorDesc.setHeight(height);
    colorDesc.setFormat(gli::FORMAT_RGBA16_SFLOAT_PACK16);
    m_ScreenColorTex = m_Device->createTexture(colorDesc);

    GraphicsTextureDesc depthDesc;
    depthDesc.setWidth(width);
    depthDesc.setHeight(height);
    depthDesc.setFormat(gli::FORMAT_D24_UNORM_S8_UINT_PACK32);
    auto depthTex = m_Device->createTexture(depthDesc);

    GraphicsFramebufferDesc desc;  
    desc.addComponent(GraphicsAttachmentBinding(m_ScreenColorTex, GL_COLOR_ATTACHMENT0));
    desc.addComponent(GraphicsAttachmentBinding(depthTex, GL_DEPTH_ATTACHMENT));
    
    m_ColorRenderTarget = m_Device->createFramebuffer(desc);;
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

ShaderPtr AreaLight::submitPerFrameUniformLight(ShaderPtr& shader) noexcept
{
    shader->setUniform("uRoughness", m_Settings.m_Roughness);
    shader->setUniform("uAlbedo2", m_Settings.m_Albedo);
    shader->setUniform("uF0", m_Settings.m_F0);
    return shader;
}

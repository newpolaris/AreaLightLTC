#include <Light.h>
#include <Mesh.h>
#include <GLType/ProgramShader.h>
#include <GL/glew.h>
#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>
#include <GLType/GraphicsDevice.h>
#include <GLType/GraphicsTexture.h>
#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLCoreGraphicsData.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace light
{
    PlaneMesh m_AreaMesh(1.0, 1.0);
    FullscreenTriangleMesh m_ScreenTraingle;
    GraphicsTexturePtr m_LtcMatTex;
    GraphicsTexturePtr m_LtcMagTex;
    GraphicsTexturePtr m_WhiteTex;
    ProgramShader m_ShaderLight;
    ProgramShader m_ShaderLTC;

    void initialize(const GraphicsDevicePtr& device)
    {
        m_ShaderLight.setDevice(device);
        m_ShaderLight.initialize();
        m_ShaderLight.addShader(GL_VERTEX_SHADER, "TexturedLight.Vertex");
        m_ShaderLight.addShader(GL_FRAGMENT_SHADER, "TexturedLight.Fragment");
        m_ShaderLight.link();

        m_ShaderLTC.setDevice(device);
        m_ShaderLTC.initialize();
        m_ShaderLTC.addShader(GL_VERTEX_SHADER, "Ltc.Vertex");
        m_ShaderLTC.addShader(GL_FRAGMENT_SHADER, "Ltc.Fragment");
        m_ShaderLTC.link();

        m_AreaMesh.create();
        m_ScreenTraingle.create();

        // TODO: move to texture descriptor
        auto type = device->getGraphicsDeviceDesc().getDeviceType();
        auto SetParameter = [type](GraphicsTexturePtr& tex, GLenum pname, GLint param)
        {
            if (type == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
            {
                auto texture = tex->downcast_pointer<OGLCoreTexture>();
                if(texture) texture->parameter(pname, param);
            }
            else if (type == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
            {
                auto texture = tex->downcast_pointer<OGLTexture>();
                if(texture) texture->parameter(pname, param);
            }
        };

        GraphicsTextureDesc source;
        source.setFilename("resources/white.png");
        m_WhiteTex = device->createTexture(source);

        GraphicsTextureDesc ltcMatDesc;
        ltcMatDesc.setFilename("resources/ltc_mat.dds");
        m_LtcMatTex = device->createTexture(ltcMatDesc);
        SetParameter(m_LtcMatTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        SetParameter(m_LtcMatTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GraphicsTextureDesc ltcMagDesc;
        ltcMagDesc.setFilename("resources/ltc_amp.dds");
        m_LtcMagTex = device->createTexture(ltcMagDesc);
        SetParameter(m_LtcMagTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        SetParameter(m_LtcMagTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void shutdown()
    {
        m_AreaMesh.destroy();
        m_ScreenTraingle.destroy();
    }
}

using namespace light;

Light::Light() noexcept
    : m_Position(glm::vec3(0.f))
    , m_Rotation(1, 0, 0, 0)
    , m_Diffuse(1.f)
    , m_Specular(1.f)
    , m_Width(8.f)
    , m_Height(8.f)
    , m_Intensity(4.f)
    , m_Roughness(0.25f)
    , m_bTwoSided(false)
    , m_bTexturedLight(false)
{
}

void Light::create()
{
    if (!m_bTexturedLight)
        m_LightSourceTex = m_WhiteTex;
}

void Light::draw(const TCamera& camera, const glm::vec2& resolution)
{
    glm::mat4 projection = camera.getProjectionMatrix();
    glm::mat4 view = camera.getViewMatrix();

    glm::mat4 model = glm::mat4(1.f);
    model = glm::translate(model, glm::vec3(m_Position));
    model = model * glm::toMat4(m_Rotation);
    model = glm::scale(model, glm::vec3(m_Width, 1, m_Height));

    // Render light source
    {
        m_ShaderLight.bind();
        m_ShaderLight.bindTexture("uTexColor", m_LightSourceTex, 0);
        m_ShaderLight.setUniform("uIntensity", m_Intensity);
        m_ShaderLight.setUniform("uModelViewProj", projection*view*model);

        m_AreaMesh.draw();
    }

    // Area Light
    {
        m_ShaderLTC.bind();
        m_ShaderLTC.setUniform("uTwoSided", m_bTwoSided);
        m_ShaderLTC.setUniform("uTexturedLight", m_bTexturedLight);
        m_ShaderLTC.setUniform("uIntensity", m_Intensity);
        m_ShaderLTC.setUniform("uView", view);
        m_ShaderLTC.setUniform("uProj", projection);
        m_ShaderLTC.setUniform("uDcolor", glm::vec3(m_Diffuse));
        m_ShaderLTC.setUniform("uScolor", glm::vec3(m_Specular));
        m_ShaderLTC.setUniform("uRoughness", m_Roughness);
        m_ShaderLTC.setUniform("uWidth", m_Width);
        m_ShaderLTC.setUniform("uHeight", m_Height);
        // m_Shader.setUniform("uRotY", m_RotY);
        // m_Shader.setUniform("uRotZ", m_RotZ);
        m_ShaderLTC.setUniform("uResolution", resolution);
        m_ShaderLTC.bindTexture("uLtcMat", m_LtcMatTex, 0);
        m_ShaderLTC.bindTexture("uLtcMag", m_LtcMagTex, 1);
        m_ShaderLTC.bindTexture("uFilteredMap", m_LightFilteredTex, 2);

        // m_ScreenTraingle.draw();
    }
}

void Light::update(const GraphicsDataPtr& buffer)
{
    // const auto makeYaxisFoward = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0));
    // auto rotation = glm::toMat3(m_Rotation * makeYaxisFoward);
    auto rotation = glm::toMat3(m_Rotation);
    auto lightEyePosition = glm::vec4(m_Position, 1.0f);

#if 0
    light.position = lightEyePosition;
    light.width = m_width;
    light.height = m_height;
    light.right = rotation[0];
    light.up = rotation[1];
    light.spotDirection = rotation[2];
#endif
}

const glm::vec3& Light::getPosition() noexcept
{
    return m_Position;
}

void Light::setPosition(const glm::vec3& position) noexcept
{
    m_Position = position;
}

const glm::quat& Light::getRotation() noexcept
{
    return m_Rotation;
}

void Light::setRotation(const glm::quat& quaternion) noexcept
{
    m_Rotation = quaternion;
}

const float Light::getIntensity() noexcept
{
    return m_Intensity;
}

void Light::setIntensity(float intensity) noexcept
{
    m_Intensity = intensity;
}

GraphicsTexturePtr light::Light::getLightSource() const noexcept
{
    return m_LightSourceTex;
}

void Light::setLightSource(const GraphicsTexturePtr& texture) noexcept
{
    m_LightSourceTex = texture;
}

void Light::setLightFilterd(const GraphicsTexturePtr& texture) noexcept
{
    m_LightFilteredTex = texture;
}

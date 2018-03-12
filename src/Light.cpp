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
    PlaneMesh m_LightMesh(2.0, 1.0);
    GraphicsTexturePtr m_LtcMatTex;
    GraphicsTexturePtr m_LtcMagTex;
    GraphicsTexturePtr m_WhiteTex;
    ShaderPtr m_ShaderLight;
    ShaderPtr m_ShaderLTC;

    void initialize(const GraphicsDevicePtr& device)
    {
        m_ShaderLight = std::make_shared<ProgramShader>();
        m_ShaderLight->setDevice(device);
        m_ShaderLight->initialize();
        m_ShaderLight->addShader(GL_VERTEX_SHADER, "TexturedLight.Vertex");
        m_ShaderLight->addShader(GL_FRAGMENT_SHADER, "TexturedLight.Fragment");
        m_ShaderLight->link();

        m_ShaderLTC = std::make_shared<ProgramShader>();
        m_ShaderLTC->setDevice(device);
        m_ShaderLTC->initialize();
        m_ShaderLTC->addShader(GL_VERTEX_SHADER, "Ltc.Vertex");
        m_ShaderLTC->addShader(GL_FRAGMENT_SHADER, "Ltc.Fragment");
        m_ShaderLTC->link();

        m_LightMesh.create();

        GraphicsTextureDesc source;
        source.setFilename("resources/white.png");
        m_WhiteTex = device->createTexture(source);

        GraphicsTextureDesc ltcMatDesc;
        ltcMatDesc.setFilename("resources/ltc_mat.dds");
        ltcMatDesc.setWrapS(GL_CLAMP_TO_EDGE);
        ltcMatDesc.setWrapT(GL_CLAMP_TO_EDGE);
        m_LtcMatTex = device->createTexture(ltcMatDesc);

        GraphicsTextureDesc ltcMagDesc;
        ltcMagDesc.setFilename("resources/ltc_amp.dds");
        ltcMagDesc.setWrapS(GL_CLAMP_TO_EDGE);
        ltcMagDesc.setWrapT(GL_CLAMP_TO_EDGE);
        m_LtcMagTex = device->createTexture(ltcMagDesc);
    }

    void shutdown()
    {
        m_LightMesh.destroy();
    }
}

using namespace light;

Light::Light() noexcept
    : m_Position(0.f)
    , m_Rotation(0.f)
    , m_Albedo(1.f)
    , m_Width(8.f)
    , m_Height(8.f)
    , m_Intensity(4.f)
    , m_bTwoSided(false)
    , m_bTexturedLight(false)
{
}

ShaderPtr Light::BindLightProgram(const RenderingData& data)
{
    m_ShaderLTC->bind();
    m_ShaderLTC->setUniform("uView", data.View);
    m_ShaderLTC->setUniform("uProjection", data.Projection);
    m_ShaderLTC->setUniform("uViewPositionW", data.Position);
    m_ShaderLTC->bindTexture("uLtcMat", m_LtcMatTex, 0);
    m_ShaderLTC->bindTexture("uLtcMag", m_LtcMagTex, 1);
    return m_ShaderLTC;
}

ShaderPtr Light::BindAreaProgram(const RenderingData& data)
{
    m_ShaderLight->bind();
    m_ShaderLight->setUniform("uViewProj", data.Projection*data.View);
    return m_ShaderLight;
}

ShaderPtr Light::submit(ShaderPtr& shader)
{
    glm::mat4 world = getWorld();

    auto& sourceTex = m_bTexturedLight ? m_LightSourceTex : m_WhiteTex;

    m_ShaderLight->setUniform("uWorld", world);
    m_ShaderLight->setUniform("ubTexturedLight", m_bTexturedLight);
    m_ShaderLight->setUniform("uIntensity", m_Intensity);
    m_ShaderLight->bindTexture("uTexColor", sourceTex, 0);

    m_LightMesh.draw();

    return shader;
}

ShaderPtr Light::submitPerLightUniforms(ShaderPtr& shader)
{
    // local
    glm::mat4 model = getWorld();
    // area light rect poinsts in world space
    glm::vec4 points[] = 
    {
        { -1.f, 0.f, -1.f, 1.f },
        { +1.f, 0.f, -1.f, 1.f },
        { +1.f, 0.f, +1.f, 1.f },
        { -1.f, 0.f, +1.f, 1.f },
    };

    points[0] = model * points[0];
    points[1] = model * points[1];
    points[2] = model * points[2];
    points[3] = model * points[3];

    shader->setUniform("uTwoSided", m_bTwoSided);
    shader->setUniform("uTexturedLight", m_bTexturedLight);
    shader->setUniform("uIntensity", m_Intensity);
    shader->setUniform("uAlbedo", glm::vec3(m_Albedo));
    shader->setUniform("uQuadPoints", points, 4);
    shader->bindTexture("uFilteredMap", m_LightFilteredTex, 2);

    return shader;
}

glm::mat4 Light::getWorld() const
{
    glm::mat4 identity = glm::mat4(1.f);
    glm::mat4 translate = glm::translate(identity, glm::vec3(m_Position));
    glm::mat4 rotateZ = glm::rotate(identity, glm::radians(m_Rotation.z), glm::vec3(0, 0, 1));
    glm::mat4 rotateY = glm::rotate(identity, glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
    glm::mat4 rotateX = glm::rotate(identity, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
    glm::mat4 scale = glm::scale(identity, glm::vec3(m_Width, 1, m_Height));
    return translate*rotateX*rotateY*rotateZ*scale;
}

const glm::vec3& Light::getPosition() noexcept
{
    return m_Position;
}

void Light::setPosition(const glm::vec3& position) noexcept
{
    m_Position = position;
}

const glm::vec3& Light::getRotation() noexcept
{
    return m_Rotation;
}

void Light::setRotation(const glm::vec3& rotation) noexcept
{
    m_Rotation = rotation;
}

const float Light::getIntensity() noexcept
{
    return m_Intensity;
}

void Light::setIntensity(float intensity) noexcept
{
    m_Intensity = intensity;
}

void Light::setTexturedLight(bool bTextured) noexcept
{
    m_bTexturedLight = bTextured;
}

void Light::setLightSource(const GraphicsTexturePtr& texture) noexcept
{
    m_LightSourceTex = texture;
}

void Light::setLightFilterd(const GraphicsTexturePtr& texture) noexcept
{
    m_LightFilteredTex = texture;
}

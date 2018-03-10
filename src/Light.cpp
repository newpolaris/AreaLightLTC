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
    : m_Position(glm::vec3(0.f))
    , m_Rotation(1, 0, 0, 0)
    , m_Diffuse(1.f)
    , m_Specular(1.f)
    , m_Width(8.f)
    , m_Height(8.f)
    , m_Intensity(4.f)
    , m_bTwoSided(false)
    , m_bTexturedLight(false)
{
}

void Light::draw(const TCamera& camera)
{
    glm::mat4 projection = camera.getProjectionMatrix();
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 world = getWorld();

    auto& sourceTex = m_bTexturedLight ? m_LightSourceTex : m_WhiteTex;

    m_ShaderLight->bind();
    m_ShaderLight->bindTexture("uTexColor", sourceTex, 0);
    m_ShaderLight->setUniform("ubTexturedLight", m_bTexturedLight);
    m_ShaderLight->setUniform("uDiffuseColor", m_Diffuse);
    m_ShaderLight->setUniform("uIntensity", m_Intensity);
    m_ShaderLight->setUniform("uModelViewProj", projection*view*world);

    m_LightMesh.draw();
}

ShaderPtr Light::bindLightProgram(const TCamera& camera)
{
    glm::mat4 projection = camera.getProjectionMatrix();
    glm::mat4 view = camera.getViewMatrix();
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

    m_ShaderLTC->bind();
    // global
    m_ShaderLTC->setUniform("uView", view);
    m_ShaderLTC->setUniform("uProjection", projection);
    m_ShaderLTC->setUniform("uViewPositionW", camera.getPosition());
    m_ShaderLTC->setUniform("uTwoSided", m_bTwoSided);
    m_ShaderLTC->setUniform("uTexturedLight", m_bTexturedLight);
    m_ShaderLTC->bindTexture("uLtcMat", m_LtcMatTex, 0);
    m_ShaderLTC->bindTexture("uLtcMag", m_LtcMagTex, 1);
    // local
    m_ShaderLTC->setUniform("uIntensity", m_Intensity);
    m_ShaderLTC->setUniform("uDcolor", glm::vec3(m_Diffuse));
    m_ShaderLTC->setUniform("uScolor", glm::vec3(m_Specular));
    m_ShaderLTC->setUniform("uQuadPoints", points, 4);
    m_ShaderLTC->bindTexture("uFilteredMap", m_LightFilteredTex, 2);

    return m_ShaderLTC;
}

glm::mat4 Light::getWorld() const
{
    glm::mat4 model = glm::mat4(1.f);
    model = glm::translate(model, glm::vec3(m_Position));
    model = model * glm::toMat4(m_Rotation);
    return glm::scale(model, glm::vec3(m_Width, 1, m_Height));
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

GraphicsTexturePtr Light::getLightSource() const noexcept
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

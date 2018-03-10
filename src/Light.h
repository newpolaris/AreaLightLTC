#pragma once

#include <tools/TCamera.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <GraphicsTypes.h>

typedef std::shared_ptr<class ProgramShader> ShaderPtr;

namespace light
{
    void initialize(const GraphicsDevicePtr& device);
    void shutdown();
}

class Light
{
public:

    Light() noexcept;

    void draw(const TCamera& camera);
    ShaderPtr bindLightProgram(const TCamera& camera);

    glm::mat4 getWorld() const;

    const glm::vec3& getPosition() noexcept;
    void setPosition(const glm::vec3& position) noexcept;
    const glm::quat& getRotation() noexcept;
    void setRotation(const glm::quat& quaternion) noexcept;
    const float getIntensity() noexcept;
    void setIntensity(float intensity) noexcept;
    GraphicsTexturePtr getLightSource() const noexcept;
    void setLightSource(const GraphicsTexturePtr& texture) noexcept;
    void setLightFilterd(const GraphicsTexturePtr& texture) noexcept;

    glm::vec3 m_Position; // where are we
    glm::quat m_Rotation;
    glm::vec4 m_Diffuse;
    glm::vec4 m_Specular;

    float m_Width;
    float m_Height;
    float m_Intensity;
    bool m_bTwoSided;
    bool m_bTexturedLight;

    GraphicsTexturePtr m_LightSourceTex;
    GraphicsTexturePtr m_LightFilteredTex;
};
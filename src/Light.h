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

    static ShaderPtr BindLightProgram(const TCamera& camera);
    static ShaderPtr BindAreaProgram(const TCamera& camera);

    Light() noexcept;

    ShaderPtr submit(ShaderPtr& shader);
    ShaderPtr submitPerLightUniforms(ShaderPtr& shader);

    glm::mat4 getWorld() const;

    const glm::vec3& getPosition() noexcept;
    void setPosition(const glm::vec3& position) noexcept;
    const glm::vec3& getRotation() noexcept;
    void setRotation(const glm::vec3& rotation) noexcept;
    const float getIntensity() noexcept;
    void setIntensity(float intensity) noexcept;
    void setTexturedLight(bool bTextured) noexcept;
    void setLightSource(const GraphicsTexturePtr& texture) noexcept;
    void setLightFilterd(const GraphicsTexturePtr& texture) noexcept;

    glm::vec3 m_Position; // where are we
    glm::vec3 m_Rotation;
    glm::vec4 m_Albedo;
    glm::vec4 m_Specular;

    float m_Width;
    float m_Height;
    float m_Intensity;
    bool m_bTwoSided;
    bool m_bTexturedLight;

    GraphicsTexturePtr m_LightSourceTex;
    GraphicsTexturePtr m_LightFilteredTex;
};
#include <tools/TCamera.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <GraphicsTypes.h>

namespace light
{
    void initialize();
    void shutdown();

    struct LightBlock
    {
        float enabled;
        float type; // 0 = pointlight, 1 = arealight
        float pad0[2];
        glm::vec4 ambient;
        glm::vec4 position; // where are we
        glm::vec4 diffuse; // how diffuse
        glm::vec4 specular; // what kinda specular stuff we got going on?
        // attenuation
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
        float pad1;
        // spot and area
        glm::vec3 spotDirection;
        float pad2;
        // only for area
        float width;
        float height;
        float pad3[2];
        glm::vec3 right;
        float pad4;
        glm::vec3 up;
        float pad5;
    };

    class Light
    {
    public:

        Light();

        void draw(const TCamera& camera);
        void update(const GraphicsDataPtr& buffer);

        const glm::vec3& getPosition() noexcept;
        void setPosition(const glm::vec3& position) noexcept;
        const glm::quat& getRotation() noexcept;
        void setRotation(const glm::quat& quaternion) noexcept;
        float getType() noexcept;
        void setType(float type) noexcept;
        void setAttenuation(const glm::vec3& attenuation) noexcept;
        const glm::vec3& getAttenuation() noexcept;

        glm::vec3 m_position;
        glm::vec3 m_attenuation;
        glm::quat m_rotation;
        float m_type;
        float m_width;
        float m_height;
    };
}

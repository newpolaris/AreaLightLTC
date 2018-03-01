#include <Light.h>
#include <Mesh.h>
#include <GLType/ProgramShader.h>
#include <GL/glew.h>
#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLCoreGraphicsData.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace light
{
    SphereMesh m_sphereMesh(16);
    PlaneMesh m_areaMesh(1.0);
    ProgramShader m_shaderFlat;

    void initialize()
    {
        m_shaderFlat.initialize();
        m_shaderFlat.addShader(GL_VERTEX_SHADER, "Flat.Vertex");
        m_shaderFlat.addShader(GL_FRAGMENT_SHADER, "Flat.Fragment");
        m_shaderFlat.link();

        m_areaMesh.init();
        m_sphereMesh.init();
    }

    void shutdown()
    {
        m_areaMesh.destroy();
        m_sphereMesh.destroy();
    }
}

using namespace light;

Light::Light() :
	m_position(glm::vec3(0.f)),
	m_rotation(1, 0, 0, 0),
	m_attenuation(glm::vec3(1.f, 0.f, 0.f)),
	m_type(0.f),
	m_width(5.f),
	m_height(5.f)
{
}

void Light::draw(const TCamera& camera)
{
	glm::mat4 projection = camera.getProjectionMatrix();
	glm::mat4 view = camera.getViewMatrix();

	glDisable(GL_CULL_FACE);

	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(m_position));
	model = model * glm::toMat4(m_rotation);

	if (m_type == 0.f)
		model = glm::scale(model, glm::vec3(1.f));
	else if (m_type == 1.f)
		model = glm::scale(model, glm::vec3(m_width, 1, m_height));

	m_shaderFlat.bind();
	m_shaderFlat.setUniform("projection", projection);
	m_shaderFlat.setUniform("view", view);
	m_shaderFlat.setUniform("model", model);
	m_shaderFlat.setUniform("color", glm::vec3(1.f));

	if (m_type == 0.f)
		m_sphereMesh.draw();
	else if (m_type == 1.f)
		m_areaMesh.draw();

	glEnable(GL_CULL_FACE);
}

void Light::update(const GraphicsDataPtr& buffer)
{
	const auto makeYaxisFoward = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0));
	auto rotation = glm::toMat3(m_rotation * makeYaxisFoward);
	auto lightEyePosition = glm::vec4(m_position, 1.0f);

	LightBlock light = { 0, };
	light.enabled = true;
	light.type = m_type;
	light.position = lightEyePosition;
	light.ambient = glm::vec4(glm::vec3(0.1f), 1.0f);
	light.diffuse = glm::vec4(1.0f);
	light.specular = glm::vec4(1.0f);
	light.constantAttenuation = m_attenuation.x;
	light.linearAttenuation = m_attenuation.y;
	light.quadraticAttenuation = m_attenuation.z;
	light.width = m_width;
	light.height = m_height;
	light.right = rotation[0];
	light.up = rotation[1];
	light.spotDirection = rotation[2];

	buffer->update(0, sizeof(light), &light);
}

const glm::vec3& Light::getPosition() noexcept
{
	return m_position;
}

void Light::setPosition(const glm::vec3& position) noexcept
{
	m_position = position;
}

const glm::quat& Light::getRotation() noexcept
{
	return m_rotation;
}

void Light::setRotation(const glm::quat& quaternion) noexcept
{
	m_rotation = glm::toMat4(quaternion);
}

float Light::getType() noexcept
{
	return m_type;
}

void Light::setType(float type) noexcept
{
	m_type = type;
}

void Light::setAttenuation(const glm::vec3& attenuation) noexcept
{
	m_attenuation = attenuation;
}

const glm::vec3& Light::getAttenuation() noexcept
{
	return m_attenuation;
}

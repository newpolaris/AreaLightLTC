#include "Framebuffer.h"
#include <GL/glew.h>
#include <GLType/BaseTexture.h>
#include <cassert>

FramebufferPtr Framebuffer::Create(const FramebufferDesc& desc) noexcept
{
    auto buffer = std::make_shared<Framebuffer>();
    if (buffer->create(desc))
        return buffer;
    return nullptr;
}

Framebuffer::Framebuffer() noexcept :
    m_FBO(GL_NONE)
{
}

Framebuffer::~Framebuffer() noexcept
{
    destroy();
}

void Framebuffer::bind() noexcept
{
    assert(m_FBO != GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

bool Framebuffer::create(const FramebufferDesc& desc)
{
    assert(m_FBO == GL_NONE);

    glCreateFramebuffers(1, &m_FBO);

	GLsizei drawCount = 0;
    GLenum drawBuffers[GL_COLOR_ATTACHMENT15 - GL_COLOR_ATTACHMENT0] = { GL_NONE, };

    const auto& components = desc.getComponents();
    for (const auto& c : components)
    {
        glNamedFramebufferTexture(m_FBO, c.m_Attachment, c.m_Texture->getTextureID(), c.m_MipLevel);
        if (c.m_Attachment != GL_DEPTH_ATTACHMENT)
            drawBuffers[drawCount++] = c.m_Attachment;
    }
    glNamedFramebufferDrawBuffers(m_FBO, drawCount, drawBuffers);

    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer::destroy() noexcept
{
    if (m_FBO != GL_NONE)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

FramebufferDesc::FramebufferDesc() noexcept
{
}

FramebufferDesc::~FramebufferDesc() noexcept
{
}

void FramebufferDesc::addComponent(const AttachmentBinding& component) noexcept
{
    m_Bindings.push_back(component);
}

const AttachmentBindings& FramebufferDesc::getComponents() const noexcept
{
    return m_Bindings;
}

AttachmentBinding::AttachmentBinding(const BaseTexturePtr& texture, std::uint32_t attachment, std::uint32_t mipLevel, std::uint32_t layer) noexcept :
    m_Texture(texture),
    m_Attachment(attachment),
    m_MipLevel(mipLevel),
    m_Layer(layer)
{
}

AttachmentBinding::~AttachmentBinding() noexcept
{
}
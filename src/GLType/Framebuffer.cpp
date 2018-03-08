#include "Framebuffer.h"
#include <GL/glew.h>
#include <GLType/OGLCoreTexture.h>
#include <cassert>

__ImplementSubInterface(GraphicsFramebuffer, rtti::Interface)

GraphicsFramebufferPtr GraphicsFramebuffer::Create(const GraphicsFramebufferDesc& desc) noexcept
{
    auto buffer = std::make_shared<GraphicsFramebuffer>();
    if (buffer->create(desc))
        return buffer;
    return nullptr;
}

GraphicsFramebuffer::GraphicsFramebuffer() noexcept :
    m_FBO(GL_NONE)
{
}

GraphicsFramebuffer::~GraphicsFramebuffer() noexcept
{
    destroy();
}

void GraphicsFramebuffer::bind() noexcept
{
    assert(m_FBO != GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

bool GraphicsFramebuffer::create(const GraphicsFramebufferDesc& desc)
{
    assert(m_FBO == GL_NONE);

    glCreateFramebuffers(1, &m_FBO);

	GLsizei drawCount = 0;
    GLenum drawBuffers[GL_COLOR_ATTACHMENT15 - GL_COLOR_ATTACHMENT0] = { GL_NONE, };

    const auto& components = desc.getComponents();
    for (const auto& c : components)
    {
        auto attachment = c.getAttachment();
        auto texture = c.getTexture()->downcast_pointer<OGLCoreTexture>();
        auto levels = c.getMipLevel();
        auto layer = c.getLayer();
        if (layer > 0)
            glNamedFramebufferTextureLayer(m_FBO, attachment, texture->getTextureID(), levels, layer);
        else
            glNamedFramebufferTexture(m_FBO, attachment, texture->getTextureID(), levels);

        if (attachment != GL_DEPTH_ATTACHMENT)
            drawBuffers[drawCount++] = attachment;
    }
    glNamedFramebufferDrawBuffers(m_FBO, drawCount, drawBuffers);

    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void GraphicsFramebuffer::destroy() noexcept
{
    if (m_FBO != GL_NONE)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

GraphicsFramebufferDesc::GraphicsFramebufferDesc() noexcept
{
}

GraphicsFramebufferDesc::~GraphicsFramebufferDesc() noexcept
{
}

void GraphicsFramebufferDesc::addComponent(const GraphicsAttachmentBinding& component) noexcept
{
    m_Bindings.push_back(component);
}

const AttachmentBindings& GraphicsFramebufferDesc::getComponents() const noexcept
{
    return m_Bindings;
}

GraphicsAttachmentBinding::GraphicsAttachmentBinding(const OGLCoreTexturePtr& texture, std::uint32_t attachment, std::uint32_t mipLevel, std::uint32_t layer) noexcept
    : m_Texture(texture)
    , m_Attachment(attachment)
    , m_MipLevel(mipLevel)
    , m_Layer(layer)
{
}

GraphicsAttachmentBinding::~GraphicsAttachmentBinding() noexcept
{
}

OGLCoreTexturePtr GraphicsAttachmentBinding::getTexture() const noexcept
{
    return m_Texture;
}

void GraphicsAttachmentBinding::setTexture(const OGLCoreTexturePtr& texture) noexcept
{
    m_Texture = texture;
}

std::uint32_t GraphicsAttachmentBinding::getAttachment() const noexcept
{
    return m_Attachment;
}

void GraphicsAttachmentBinding::setAttachment(std::uint32_t attachment) noexcept
{
    m_Attachment = attachment;
}

std::uint32_t GraphicsAttachmentBinding::getMipLevel() const noexcept
{
    return m_MipLevel;
}

void GraphicsAttachmentBinding::setMipLevel(std::uint32_t mipLevel) noexcept
{
    m_MipLevel = mipLevel;
}

std::uint32_t GraphicsAttachmentBinding::getLayer() const noexcept
{
    return m_Layer;
}

void GraphicsAttachmentBinding::setLayer(std::uint32_t layer) noexcept
{
    m_Layer = layer;
}

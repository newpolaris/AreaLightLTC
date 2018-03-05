#include <GLType/OGLDevice.h>
#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLCoreGraphicsData.h>
#include <GLType/OGLTexture.h>
#include <GLType/OGLCoreTexture.h>

__ImplementSubInterface(OGLDevice, GraphicsDevice)

OGLDevice::OGLDevice() noexcept
{
}

OGLDevice::~OGLDevice() noexcept
{
}

bool OGLDevice::create(const GraphicsDeviceDesc& desc) noexcept
{
    m_Desc = desc;
    return true;
}

void OGLDevice::destoy() noexcept
{
}

GraphicsDataPtr OGLDevice::createGraphicsData(const GraphicsDataDesc& desc) noexcept
{
    if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto data = std::make_shared<OGLCoreGraphicsData>();
        if (!data) return nullptr;
		data->setDevice(this->downcast_pointer<OGLDevice>());
        if (data->create(desc))
            return data;
        return nullptr;
    }
    else if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto data = std::make_shared<OGLGraphicsData>();
        if (!data) return nullptr;
		data->setDevice(this->downcast_pointer<OGLDevice>());
        if (data->create(desc))
            return data;
        return nullptr;
    }
    return nullptr;
}

GraphicsTexturePtr OGLDevice::createTexture(const GraphicsTextureDesc& desc) noexcept
{
    if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGLCore)
    {
        auto texture = std::make_shared<OGLCoreTexture>();
        if (!texture) return nullptr;
		texture->setDevice(this->downcast_pointer<OGLDevice>());
        if (texture->create(desc))
            return texture;
        texture->parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        texture->parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        texture->parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return nullptr;
    }
    else if (m_Desc.getDeviceType() == GraphicsDeviceType::GraphicsDeviceTypeOpenGL)
    {
        auto texture = std::make_shared<OGLTexture>();
        if (!texture) return nullptr;
		texture->setDevice(this->downcast_pointer<OGLDevice>());
        if (texture->create(desc))
            return texture;
        return nullptr;
    }
    return nullptr;
}

const GraphicsDeviceDesc& OGLDevice::getGraphicsDeviceDesc() const noexcept
{
    return m_Desc;
}

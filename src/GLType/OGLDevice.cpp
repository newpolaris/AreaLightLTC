#include <GLType/OGLDevice.h>
#include <GLType/OGLGraphicsData.h>
#include <GLType/OGLCoreGraphicsData.h>

__ImplementSubInterface(OGLDevice, GraphicsDevice, "OGLDevice")

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

const GraphicsDeviceDesc& OGLDevice::getGraphicsDeviceDesc() const noexcept
{
    return m_Desc;
}

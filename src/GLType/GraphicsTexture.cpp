#include <GLType/GraphicsTexture.h>

__ImplementSubInterface(GraphicsTexture, rtti::Interface)

GraphicsTextureDesc::GraphicsTextureDesc() noexcept
    : m_Width(1)
    , m_Height(1)
    , m_Depth(1)
    , m_Levels(1)
    , m_Target(GraphicsTarget2D)
    , m_Data(nullptr)
	, m_DataSize(0)
{
}

GraphicsTextureDesc::~GraphicsTextureDesc() noexcept
{
}

std::string GraphicsTextureDesc::getName() const noexcept
{
    return m_Name;
}

void GraphicsTextureDesc::setName(const std::string& name) noexcept
{
    m_Name = name;
}

std::string GraphicsTextureDesc::getFileName() const noexcept
{
    return m_Filename;
}

void GraphicsTextureDesc::setFilename(const std::string& filename) noexcept
{
    m_Filename = filename;
}

int32_t GraphicsTextureDesc::getWidth() const noexcept
{
    return m_Width;
}

void GraphicsTextureDesc::setWidth(int32_t width) noexcept
{
    m_Width = width;
}

int32_t GraphicsTextureDesc::getHeight() const noexcept
{
    return m_Height;
}

void GraphicsTextureDesc::setHeight(int32_t height) noexcept
{
    m_Height = height;
}

int32_t GraphicsTextureDesc::getDepth() const noexcept
{
    return m_Depth;
}

void GraphicsTextureDesc::setDepth(int32_t depth) noexcept
{
    m_Depth = depth;
}

std::uint8_t* GraphicsTextureDesc::getStream() const noexcept
{
    return m_Data;
}

void GraphicsTextureDesc::setStream(std::uint8_t* data) noexcept
{
    m_Data = data;
}

std::uint32_t GraphicsTextureDesc::getStreamSize() const noexcept
{
    return m_DataSize;
}

void GraphicsTextureDesc::setStreamSize(std::uint32_t size) noexcept
{
    m_DataSize = size;
}

GraphicsTexture::GraphicsTexture() noexcept
{
}

GraphicsTexture::~GraphicsTexture() noexcept
{
}

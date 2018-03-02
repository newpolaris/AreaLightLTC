#pragma once

#include <tools/Rtti.h>
#include <string>
#include <GraphicsTypes.h>

class GraphicsTextureDesc final
{
public:

    GraphicsTextureDesc() noexcept;
    ~GraphicsTextureDesc() noexcept;

    std::string getName() const noexcept;
    void setName(const std::string& name) noexcept;

    std::string getFileName() const noexcept;
    void setFilename(const std::string& filename) noexcept;

    int32_t getWidth() const noexcept;
    void setWidth(int32_t width) noexcept;

    int32_t getHeight() const noexcept;
    void setHeight(int32_t height) noexcept;

    int32_t getDepth() const noexcept;
    void setDepth(int32_t depth) noexcept;

    int32_t getLevels() const noexcept;
    void setLevels(int32_t levels) noexcept;

	std::uint8_t* getStream() const noexcept;
	void setStream(std::uint8_t* data) noexcept;

	std::uint32_t getStreamSize() const noexcept;
	void setStreamSize(std::uint32_t size) noexcept;

    GraphicsTarget getTarget() const noexcept;
    void setTarget(GraphicsTarget target) noexcept;

    GraphicsFormat getFormat() const noexcept;
    void setFormat(GraphicsFormat format) noexcept;

private:

    std::string m_Name;
    std::string m_Filename;
    int32_t m_Width;
    int32_t m_Height;
    int32_t m_Depth;
    int32_t m_Levels;
    GraphicsTarget m_Target;
    GraphicsFormat m_Format;
	std::uint8_t* m_Data;
	std::uint32_t m_DataSize;
};

class GraphicsTexture : public rtti::Interface
{
    __DeclareSubInterface(GraphicsTexture, rtti::Interface)
public:

    GraphicsTexture() noexcept;
    virtual ~GraphicsTexture() noexcept;

    virtual const GraphicsTextureDesc& getGraphicsTextureDesc() const noexcept = 0;

private:

    GraphicsTexture(const GraphicsTexture& texture) = delete;
    GraphicsTexture& operator=(const GraphicsTexture& texture) = delete;
};

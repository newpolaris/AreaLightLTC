#pragma once

#include <GL/glew.h>
#include <string>
#include <GraphicsTypes.h>
#include <tools/Rtti.h>
#include <GLType/GraphicsTexture.h>

class OGLCoreTexture final : public GraphicsTexture
{
	__DeclareSubInterface(OGLCoreTexture, GraphicsTexture)
public:

	OGLCoreTexture();
    virtual ~OGLCoreTexture();

    bool create(const GraphicsTextureDesc& desc) noexcept;
	bool create(const std::string& filename) noexcept;
	bool create(GLint width, GLint height, GLenum target, GraphicsFormat format, GLuint levels, const uint8_t* data, uint32_t size) noexcept;
	void destroy() noexcept;
	void bind(GLuint unit) const;
	void unbind(GLuint unit) const;
	void generateMipmap();

    GLuint getTextureID() const noexcept;
    GLenum getFormat() const noexcept;

    const GraphicsTextureDesc& getGraphicsTextureDesc() const noexcept override;

private:

    void applyParameters(const GraphicsTextureDesc& desc);
	void parameteri(GLenum pname, GLint param);
	void parameterf(GLenum pname, GLfloat param);

    bool createFromMemory(const char* data, size_t dataSize) noexcept;
    bool createFromMemoryDDS(const char* data, size_t dataSize) noexcept; // DDS, KTX
    bool createFromMemoryHDR(const char* data, size_t dataSize) noexcept; // HDR
    bool createFromMemoryLDR(const char* data, size_t dataSize) noexcept; // JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC files
    bool createFromMemoryZIP(const char* data, size_t dataSize) noexcept; // ZLIB

private:

	friend class OGLDevice;
	void setDevice(const GraphicsDevicePtr& device) noexcept;
	GraphicsDevicePtr getDevice() noexcept;

private:

	OGLCoreTexture(const OGLCoreTexture&) noexcept = delete;
	OGLCoreTexture& operator=(const OGLCoreTexture&) noexcept = delete;

private:

    GraphicsTextureDesc m_TextureDesc;

	GLuint m_TextureID;
	GLenum m_Target;
	GLenum m_Format;
	GraphicsDeviceWeakPtr m_Device;
};


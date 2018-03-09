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

    bool create(const GraphicsTextureDesc& desc);
	bool create(const std::string& filename);
	bool create(GLint width, GLint height, GLenum target, GraphicsFormat format, GLuint levels, uint8_t* data, uint32_t size);
	void destroy();
	void bind(GLuint unit) const;
	void unbind(GLuint unit) const;
	void generateMipmap();

    GLuint getTextureID() const noexcept;
    GLenum getFormat() const noexcept;

    const GraphicsTextureDesc& getGraphicsTextureDesc() const noexcept;

private:

    void applyParameters(const GraphicsTextureDesc& desc);
	void parameter(GLenum pname, GLint param);

    bool createFromFileGLI(const std::string& filename);
    bool createFromFileSTB(const std::string& filename);

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


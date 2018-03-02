#pragma once

#include <GL/glew.h>
#include <string>
#include <GraphicsTypes.h>
#include <GLType/GraphicsTexture.h>

class OGLTexture final : public GraphicsTexture
{
public:

	OGLTexture();
    virtual ~OGLTexture();

    static GraphicsTexturePtr Create(GLint width, GLint height, GLenum target, GLenum format, GLuint levels);
    static GraphicsTexturePtr Create(const std::string& filename);

    bool create(const GraphicsTextureDesc& desc);
	bool create(const std::string& filename);
	bool create(GLint width, GLint height, GLenum target, GLenum format, GLuint levels);
	void destroy();
	void bind(GLuint unit) const;
	void unbind(GLuint unit) const;
	void generateMipmap();
	void parameter(GLenum pname, GLint param);

    bool createFromFileGLI(const std::string& filename);
    bool createFromFileSTB(const std::string& filename);

    const GraphicsTextureDesc& getGraphicsTextureDesc() const noexcept;

private:

	friend class OGLDevice;
	void setDevice(const GraphicsDevicePtr& device) noexcept;
	GraphicsDevicePtr getDevice() noexcept;

private:

	OGLTexture(const OGLTexture&) noexcept = delete;
	OGLTexture& operator=(const OGLTexture&) noexcept = delete;

private:

    GraphicsTextureDesc m_TextureDesc;

	GLuint m_TextureID;
	GLenum m_Target;
	GLenum m_Format;

	GLint m_Width;
	GLint m_Height;
	GLint m_Depth;
	GLint m_MipCount;
    GraphicsDeviceWeakPtr m_Device;
};


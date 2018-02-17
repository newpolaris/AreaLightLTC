#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <Math/Common.h>
#include <string>
#include <GraphicsTypes.h>
#include <vector>

class ProgramShader
{
public:
    ProgramShader() : m_id(0u) {}
    virtual ~ProgramShader() {destroy();}
    
    /** Generate the program id */
    void initalize();
    
    /** Destroy the program id */
    void destroy();        
    
    /** Add a shader and compile it */
    void addShader(GLenum shaderType, const std::string &tag);
    
    //bool compile(); //static (with param)?
    
    bool link(); //static (with param)?
    
    void bind() const { glUseProgram( m_id ); }
    void unbind() const { glUseProgram( 0u ); }
    
    /** Return the program id */
    GLuint getId() const { return m_id; }
    
    bool setUniform(const std::string &name, GLint v) const;
    bool setUniform(const std::string &name, GLfloat v) const;
    bool setUniform(const std::string &name, const glm::vec3 &v) const;
    bool setUniform(const std::string &name, const glm::vec4 &v) const;
    bool setUniform(const std::string &name, const glm::mat3 &v) const;
    bool setUniform(const std::string &name, const glm::mat4 &v) const;
    bool bindTexture(const std::string &name, const BaseTexturePtr& texture, GLint unit);

    // Compute
    bool bindImage(const std::string &name, const BaseTexturePtr &texture, GLint unit, GLint level, GLboolean layered, GLint layer, GLenum access);

    void Dispatch( GLuint GroupCountX = 1, GLuint GroupCountY = 1, GLuint GroupCountZ = 1 );
    void Dispatch1D( GLuint ThreadCountX, GLuint GroupSizeX = 64);
    void Dispatch2D( GLuint ThreadCountX, GLuint ThreadCountY, GLuint GroupSizeX = 8, GLuint GroupSizeY = 8);
    void Dispatch3D( GLuint ThreadCountX, GLuint ThreadCountY, GLuint ThreadCountZ, GLuint GroupSizeX = 4, GLuint GroupSizeY = 4, GLuint GroupSizeZ = 4 );
  
    static bool setIncludeFromFile(const std::string &includeName, const std::string &filename);
    static std::vector<char> readTextFile(const std::string &filename);

protected:

    static std::vector<std::string> directory;

    GLuint m_id;
};

inline void ProgramShader::Dispatch( size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ )
{
    glDispatchCompute(GroupCountX, GroupCountY, GroupCountZ);
}

inline void ProgramShader::Dispatch1D( size_t ThreadCountX, size_t GroupSizeX )
{
    Dispatch( Math::DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1 );
}

inline void ProgramShader::Dispatch2D( size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY), 1);
}

inline void ProgramShader::Dispatch3D( size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY),
        Math::DivideByMultiple(ThreadCountZ, GroupSizeZ));
}

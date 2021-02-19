#include "shader.h"
#include "scope_guard.h"
#include <glad/glad.h>
#include <cassert>

ShaderProgram::ShaderProgram()
: program_(0)
{
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(program_);
}

bool ShaderProgram::Init(const char *vertexShaderSource, 
                         const char *fragmentShaderSource, 
                         std::string &errorLog)
{
    assert(program_ == 0);

    int success;
    char info_log[1024];

    // vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    auto vertex_guard = scopeGuard([&vertex]{ glDeleteShader(vertex); });
    glShaderSource(vertex, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 1024, NULL, info_log);
        errorLog = "compile vertex shader: ";
        errorLog += info_log;
        return false;
    }

    // fragment Shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragment_guard = scopeGuard([&fragment]{ glDeleteShader(fragment); });
    glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 1024, NULL, info_log);
        errorLog = "compile fragment shader: ";
        errorLog += info_log;
        return false;
    }
    
    // shader Program
    program_ = glCreateProgram();
    auto program_guard = scopeGuard([&]{ glDeleteProgram(program_); program_ = 0; });
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program_, 1024, NULL, info_log);
        errorLog = "link: ";
        errorLog += info_log;
        return false;
    }
    program_guard.dismiss();
    
    return true;
}

void ShaderProgram::Use(bool use)
{
    assert(program_ != 0);
    GLuint prog = use ? program_ : 0;
    glUseProgram(prog);
}

int ShaderProgram::GetUniformLocation(const char *name)
{
    assert(program_ != 0);
    return glGetUniformLocation(program_, name);
}

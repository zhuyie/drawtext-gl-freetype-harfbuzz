#ifndef __SHADER_H__
#define __SHADER_H__

#include <string>

class ShaderProgram
{
public:
    ShaderProgram();
    ~ShaderProgram();

    bool Init(const char *vertexShaderSource, 
              const char *fragmentShaderSource, 
              std::string &errorLog);
    
    void Use(bool use);
    
    int GetUniformLocation(const char *name);

    unsigned int ID() const { return program_; }
    
private:
    unsigned int program_;
};

#endif // !__SHADER_H__

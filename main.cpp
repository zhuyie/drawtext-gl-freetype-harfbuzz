#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <memory>

#include "scope_guard.h"

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static const char* opengl_version_string(char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%d.%d", GLVersion.major, GLVersion.minor);
    return buf;
}

static const char* freetype_version_string(FT_Library library, char* buf, size_t buf_size)
{
    FT_Int major, minor, patch;
    FT_Library_Version(library, &major, &minor, &patch);
    snprintf(buf, buf_size, "%d.%d.%d", major, minor, patch);
    return buf;
}

const char* vertex_shader_string =
"#version 330 core\n"
"layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
"out vec2 TexCoords;\n"
"\n"
"uniform mat4 projection;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
"    TexCoords = vertex.zw;\n"
"}\n";

const char* fragment_shader_string =
"#version 330 core\n"
"in vec2 TexCoords;\n"
"out vec4 color;\n"
"\n"
"uniform sampler2D text;\n"
"uniform vec3 textColor;\n"
"\n"
"void main()\n"
"{\n"
"    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
"    color = vec4(textColor, 1.0) * sampled;\n"
"}\n";

static bool create_shader_program(GLuint& programID)
{
    int success;
    char info_log[1024];

    // vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    auto vertex_guard = scopeGuard([&vertex]{ glDeleteShader(vertex); });
    glShaderSource(vertex, 1, &vertex_shader_string, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 1024, NULL, info_log);
        fprintf(stderr, "create_shader_program: vertex shader compile failed: %s\n", info_log);
        return false;
    }

    // fragment Shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragment_guard = scopeGuard([&fragment]{ glDeleteShader(fragment); });
    glShaderSource(fragment, 1, &fragment_shader_string, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 1024, NULL, info_log);
        fprintf(stderr, "create_shader_program: fragment shader compile failed: %s\n", info_log);
        return false;
    }
    
    // shader Program
    programID = glCreateProgram();
    auto program_guard = scopeGuard([&programID]{ glDeleteProgram(programID); });
    glAttachShader(programID, vertex);
    glAttachShader(programID, fragment);
    glLinkProgram(programID);
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programID, 1024, NULL, info_log);
        fprintf(stderr, "create_shader_program: link failed: %s\n", info_log);
        return false;
    }
    
    program_guard.dismiss();
    return true;
}

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

typedef std::map<char, Character> CharacterMap;
CharacterMap Characters;

bool get_character(FT_Face face, char c, Character& x)
{
    CharacterMap::iterator iter = Characters.find(c);
    if (iter != Characters.end())
    {
        x = iter->second;
        return true;
    }

    // load character glyph 
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
    {
        return false;
    }
    // generate texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer
    );
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // now store character for later use
    Character character = {
        texture, 
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        (unsigned int)face->glyph->advance.x
    };
    Characters.insert(std::pair<char, Character>(c, character));

    return true;
}

void render_text(
    GLuint shader_program, GLuint vao, GLuint vbo, int framebuffer_w, int framebuffer_h,
    FT_Face face, const std::string& text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    glUseProgram(shader_program);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(framebuffer_w), 0.0f, static_cast<float>(framebuffer_h));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shader_program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch;
        if (!get_character(face, *c, ch))
        {
            fprintf(stderr, "get_character %c failed\n", *c);
            break;
        }

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static bool create_font(FT_Library ft, const char* font_path, float size_in_points, float content_scale, FT_Face* face)
{
    if (FT_New_Face(ft, font_path, 0, face))
    {
        return false;
    }
#if defined(_WIN32)
    const int default_logic_dpi_x = 96;
    const int default_logic_dpi_y = 96;
#elif defined(__APPLE__)
    const int default_logic_dpi_x = 72;
    const int default_logic_dpi_y = 72;
#else
    #error "not implemented"
#endif
    FT_Set_Char_Size(
        *face, 
        0,                                  // same as character height
        size_in_points*64,                  // char_height in 1/64th of points
        default_logic_dpi_x*content_scale,  // horizontal device resolution
        default_logic_dpi_y*content_scale   // vertical device resolution
    );
    return true;
}

int main(int argc, char* agrv[])
{
    char version[100] = { 0 };

    fprintf(stdout, "GLFW Version: %s\n", glfwGetVersionString());

    // Initialize GLFW
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
    {
        fprintf(stderr, "glfwInit failed\n");
        return 1;
    }
    auto glfw_guard = scopeGuard([]{ glfwTerminate(); });
    
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // https://www.glfw.org/faq.html#41---how-do-i-create-an-opengl-30-context
#ifdef __APPLE__    
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // https://www.glfw.org/docs/3.3/window_guide.html#GLFW_SCALE_TO_MONITOR
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "drawtext-gl-freetype-harfbuzz", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "glfwCreateWindow failed\n");
        return 1;
    }
    glfwMakeContextCurrent(window);

    float content_scale;
    glfwGetWindowContentScale(window, NULL, &content_scale);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "gladLoadGLLoader failed\n");
        return 1;
    }
    fprintf(stdout, "OpenGL Version: %s\n", opengl_version_string(version, sizeof(version)));

    // Create shader program
    GLuint shader_program = 0;
    if (!create_shader_program(shader_program))
    {
        return 1;
    }
    
    // Create VAO && VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // OpenGL states
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize FreeType
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        fprintf(stderr, "FT_Init_FreeType failed\n");
        return 1;
    }
    auto ft_guard = scopeGuard([&ft]{ FT_Done_FreeType(ft); });
    fprintf(stdout, "FreeType Version: %s\n", freetype_version_string(ft, version, sizeof(version)));

    FT_Face font;
    if (!create_font(ft, "../fonts/NotoSans-Regular.ttf", 72, content_scale, &font))
    {
        fprintf(stderr, "create_font failed\n");
        return 1;
    }

    fprintf(stdout, "HarfBuzz Version: %s\n", hb_version_string());

    // Event loop
    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        render_text(
            shader_program, VAO, VBO, width, height, font, 
            "This is a test.", 25.0f*content_scale, 25.0f*content_scale, 
            1.0f, glm::vec3(1.0f, 0.f, 0.f)
        );

        glfwSwapBuffers(window);

        glfwWaitEvents();
    }

    return 0;
}

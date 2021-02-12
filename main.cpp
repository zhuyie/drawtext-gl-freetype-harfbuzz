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

#include "font.h"
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

struct Glyph {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
};

typedef std::map<unsigned int, Glyph> GlyphCache;
GlyphCache Glyphs;

bool get_glyph(FT_Face face, unsigned int glyph_index, Glyph& x)
{
    GlyphCache::iterator iter = Glyphs.find(glyph_index);
    if (iter != Glyphs.end())
    {
        x = iter->second;
        return true;
    }

    // load glyph 
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER))
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
    // now store Glyph for later use
    x = Glyph {
        texture, 
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top)
    };
    Glyphs.insert(GlyphCache::value_type(glyph_index, x));

    return true;
}

void render_text(
    GLuint shader_program, GLuint vao, GLuint vbo, int framebuffer_w, int framebuffer_h,
    Font& font, const std::string& text, float x, float y, glm::vec3 color)
{
    // activate corresponding render state	
    glUseProgram(shader_program);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(framebuffer_w), 0.0f, static_cast<float>(framebuffer_h));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shader_program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    // Create a hb_buffer.
    hb_buffer_t *buf = hb_buffer_create();
    auto buf_guard = scopeGuard([&buf]{ hb_buffer_destroy(buf); });
    // Put text in.
    hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);
    // Set the script, language and direction of the buffer.
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));
    // Shape
    hb_shape(font.getHBFont(), buf, NULL, 0);
    // Get the glyph and position information.
    unsigned int glyph_count;
    hb_glyph_info_t *glyph_info    = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
    
    // Iterate over each glyph.
    for (unsigned int i = 0; i < glyph_count; i++)
    {
        hb_codepoint_t glyphid  = glyph_info[i].codepoint;
        hb_position_t x_offset  = glyph_pos[i].x_offset / 64;
        hb_position_t y_offset  = glyph_pos[i].y_offset / 64;
        hb_position_t x_advance = glyph_pos[i].x_advance / 64;
        hb_position_t y_advance = glyph_pos[i].y_advance / 64;

        Glyph g;
        if (!get_glyph(font.getFTFont(), glyphid, g))
        {
            fprintf(stderr, "get_glyph %d failed\n", glyphid);
            break;
        }

        float x_origin = x + g.Bearing.x;
        float y_origin = y - (g.Size.y - g.Bearing.y);
        float x_pos = x_origin + x_offset;
        float y_pos = y_origin - y_offset;
        float w = g.Size.x;
        float h = g.Size.y;

        // update VBO for each glyph
        float vertices[6][4] = {
            { x_pos,     y_pos + h,   0.0f, 0.0f },            
            { x_pos,     y_pos,       0.0f, 1.0f },
            { x_pos + w, y_pos,       1.0f, 1.0f },

            { x_pos,     y_pos + h,   0.0f, 0.0f },
            { x_pos + w, y_pos,       1.0f, 1.0f },
            { x_pos + w, y_pos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, g.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // advance cursors for next glyph
        x += x_advance;
        y -= y_advance;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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

    fprintf(stdout, "HarfBuzz Version: %s\n", hb_version_string());

    Font font0(ft, "../fonts/NotoSans-Regular.ttf", 72, content_scale);
    if (!font0.initOK())
    {
        fprintf(stderr, "create font0 failed\n");
        return 1;
    }
    Font font1(ft, "../fonts/NotoSerifSC-Regular.otf", 72, content_scale);
    if (!font1.initOK())
    {
        fprintf(stderr, "create font1 failed\n");
        return 1;
    }

    // Event loop
    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        render_text(
            shader_program, VAO, VBO, width, height, font0, 
            u8"This is a test.", 25.0f*content_scale, 25.0f*content_scale, 
            glm::vec3(1.0f, 0.f, 0.f)
        );
        render_text(
            shader_program, VAO, VBO, width, height, font1, 
            u8"天地玄黄，宇宙洪荒。", 25.0f*content_scale, 125.0f*content_scale, 
            glm::vec3(0.f, 0.f, 1.f)
        );

        glfwSwapBuffers(window);

        glfwWaitEvents();
    }

    return 0;
}

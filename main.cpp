#include "text_render.h"
#include "scope_guard.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <functional>

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

std::function<void(GLFWwindow*)> draw;

static void window_refresh_callback(GLFWwindow* window)
{
    draw(window);
    glfwSwapBuffers(window);
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
    glfwSetWindowRefreshCallback(window, window_refresh_callback);
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

    // Create TextRender
    TextRender render;
    if (!render.Init(4, 256))
    {
        fprintf(stderr, "TextRender Init failed\n");
        return 1;
    }

    // Create fonts
    Font font0(ft, "../fonts/NotoSans-Regular.ttf", 56, content_scale, false, true);
    if (!font0.Ok())
    {
        fprintf(stderr, "create font0 failed\n");
        return 1;
    }
    Font font1(ft, "../fonts/NotoSerifSC-Regular.otf", 32, content_scale, false, false);
    if (!font1.Ok())
    {
        fprintf(stderr, "create font1 failed\n");
        return 1;
    }
    Font font2(ft, "../fonts/NotoSansArabic-Regular.ttf", 56, content_scale, true, false);
    if (!font2.Ok())
    {
        fprintf(stderr, "create font2 failed\n");
        return 1;
    }

    // Create TextRuns
    TextRun text0(font0, u8"This is a test.", HB_DIRECTION_LTR, HB_SCRIPT_LATIN, hb_language_from_string("en", -1));
    TextRun text1(font1, u8"天地玄黄，宇宙洪荒。", HB_DIRECTION_TTB, HB_SCRIPT_HAN, hb_language_from_string("zh", -1));
    TextRun text2(font2, u8"أسئلة و أجوبة", HB_DIRECTION_RTL, HB_SCRIPT_ARABIC, hb_language_from_string("ar", -1));

    unsigned int drawCount = 0;
    double drawTime = 0.0;
    draw = [&](GLFWwindow* window)
    {
        double startTime = glfwGetTime();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        auto DP_X = [](float x) -> float { return x; };
        auto DP_Y = [&height](float y) -> float { return height - y; };

        render.Begin(width, height);
        render.DrawText(
            text0, 
            DP_X(10.0f*content_scale), DP_Y(60.0f*content_scale), 
            glm::vec3(1.0f, 0.f, 0.f)
        );
        render.DrawText(
            text1, 
            DP_X(325.0f*content_scale), DP_Y(100.0f*content_scale), 
            glm::vec3(0.f, 0.f, 1.f)
        );
        render.DrawText(
            text2, 
            DP_X(450.0f*content_scale), DP_Y(575.0f*content_scale), 
            glm::vec3(0.f, 1.f, 0.f)
        );
        render.End();

        double duration = glfwGetTime() - startTime;
        drawTime += duration;
        drawCount++;
    };

    // Event loop
    while (!glfwWindowShouldClose(window))
    {
        draw(window);
        glfwSwapBuffers(window);

        glfwWaitEvents();
    }

    render.PrintStats();
    fprintf(stdout, "----draw time stats----\n");
    fprintf(stdout, "draw count   : %u\n", drawCount);
    fprintf(stdout, "avg draw time: %f ms\n", drawTime / drawCount * 1000.0);
    fprintf(stdout, "\n");

    return 0;
}

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <stdlib.h>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char* agrv[])
{
    int ret_code;
    int width, height;
    GLFWwindow* window = NULL;
    FT_Library ft_library = NULL;
    FT_Int ft_version_major, ft_version_minor, ft_version_patch;

    fprintf(stdout, "GLFW Version: %s\n", glfwGetVersionString());

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {
        fprintf(stderr, "gladInit failed\n");
        ret_code = 1;
        goto Exit0;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // https://www.glfw.org/faq.html#41---how-do-i-create-an-opengl-30-context
#ifdef __APPLE__    
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(800, 600, "drawtext-gl-freetype-harfbuzz", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "glfwCreateWindow failed\n");
        ret_code = 1;
        goto Exit0;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "gladLoadGLLoader failed\n");
        ret_code = 1;
        goto Exit0;
    }
    fprintf(stdout, "OpenGL Version: %d.%d\n", GLVersion.major, GLVersion.minor);

    if (FT_Init_FreeType(&ft_library))
    {
        fprintf(stderr, "FT_Init_FreeType failed\n");
        ret_code = 1;
        goto Exit0;
    }
    FT_Library_Version(ft_library, &ft_version_major, &ft_version_minor, &ft_version_patch);
    fprintf(stdout, "FreeType Version: %d.%d.%d\n", ft_version_major, ft_version_minor, ft_version_patch);

    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);

        glfwWaitEvents();
    }

    ret_code = 0;

Exit0:
    if (ft_library != NULL)
        FT_Done_FreeType(ft_library);
    glfwTerminate();
    return ret_code;
}

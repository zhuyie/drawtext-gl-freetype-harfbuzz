#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char* agrv[])
{
    int ret_code = 0;
    GLFWwindow* window = NULL;
    GLuint WIDTH = 800, HEIGHT = 600;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {
        fprintf(stderr, "gladInit failed\n");
        ret_code = 1;
        goto Exit0;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // https://www.glfw.org/faq.html#41---how-do-i-create-an-opengl-30-context
#ifdef __APPLE__    
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "drawtext-gl-freetype-harfbuzz", NULL, NULL);
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
    fprintf(stdout, "OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

    glViewport(0, 0, WIDTH, HEIGHT);

    while (!glfwWindowShouldClose(window))
    {
        glfwWaitEvents();
    }

Exit0:
    glfwTerminate();
    return 0;
}

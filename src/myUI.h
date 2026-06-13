#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

struct Mouse {
    double x = 0;
    double y = 0;

    bool leftClick = false;
    bool rightClick = false;
};

namespace myUI {
    int init();

    GLFWwindow* createWindow(
        int width,
        int height,
        const char* title
    );

    void close(GLFWwindow* window);

    void getMouseData(
        GLFWwindow* window,
        Mouse& mouse
    );

    std::string readFile(
        const std::string& path
    );

    GLuint compileShader(
        GLenum type,
        const std::string& path
    );

    GLuint createShaderProgram(
        const std::string& vertexPath,
        const std::string& fragmentPath
    );

    GLuint loadTexture(
        const std::string& path
    );
}

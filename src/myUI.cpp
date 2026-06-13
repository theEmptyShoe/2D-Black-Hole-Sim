/*
1. Initialize GLFW:
    if (myUI::init() != 0) return -1;

2. Create a window:
    GLFWwindow* window = myUI::createWindow(1280, 720, "My Window");
    if (!window) return -1;

3. Load shaders:
    GLuint shader = myUI::createShaderProgram("shaders/vertex.glsl", "shaders/fragment.glsl");

4. Main loop:
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);
        // draw here
        glfwSwapBuffers(window);
    }

5. Cleanup:
    glDeleteProgram(shader);
    myUI::close(window);

int init()
    Initializes GLFW.
    Sets OpenGL version to 3.3 Core Profile.
    Returns:
        0  -> success
        -1 -> failure
----------------------------------------------------------------------------
GLFWwindow* createWindow(int width, int height, const char* title)
    Creates a GLFW window.
    Creates an OpenGL context.
    Loads GLAD.
    Enables alpha blending.
Returns:
    GLFWwindow* on success
    nullptr on failure
----------------------------------------------------------------------------
void close(GLFWwindow* window)
    Destroys the window and terminates GLFW.
----------------------------------------------------------------------------
void getMouseData(GLFWwindow* window, Mouse& mouse)
    Updates mouse position and button states.
Mouse fields:
    mouse.x
    mouse.y
    mouse.leftClick
    mouse.rightClick
----------------------------------------------------------------------------
std::string readFile(const std::string& path)
    Reads an entire text file into a string.
Returns:
    File contents
----------------------------------------------------------------------------
GLuint compileShader(GLenum type, const std::string& path)
    Loads and compiles a shader from file.
Supported types:
        GL_VERTEX_SHADER
        GL_FRAGMENT_SHADER
Returns:
    OpenGL shader ID
----------------------------------------------------------------------------
GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
    Creates a complete shader program.
Internally:
        - Loads vertex shader
        - Loads fragment shader
        - Compiles both
        - Links program
        - Deletes temporary shaders
Returns:
    OpenGL program ID
----------------------------------------------------------------------------
GLuint loadTexture(const std::string& path)
    Loads an image using stb_image.
Returns:
    OpenGL texture ID
Bind:
    glBindTexture(GL_TEXTURE_2D, texture);
----------------------------------------------------------------------------

struct Mouse {

    double x;
    double y;

    bool leftClick;
    bool rightClick;
};

----------------------------------------------------------------------------
REQUIRED FILES

myUI.h
myUI.cpp

stb_image.cpp:
    #define STB_IMAGE_IMPLEMENTATION
    #include <stb_image.h>

*/

#include "myUI.h"

namespace myUI {

int init() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return 0;
}

GLFWwindow* createWindow(
    int width,
    int height,
    const char* title
) {
    GLFWwindow* window =
        glfwCreateWindow(
            width,
            height,
            title,
            nullptr,
            nullptr
        );

    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(
        (GLADloadproc)glfwGetProcAddress
    )) {
        std::cerr << "Failed to load GLAD\n";
        return nullptr;
    }

    glEnable(GL_BLEND);
    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    return window;
}

void close(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void getMouseData(
    GLFWwindow* window,
    Mouse& mouse
) {
    glfwGetCursorPos(
        window,
        &mouse.x,
        &mouse.y
    );

    mouse.leftClick =
        glfwGetMouseButton(
            window,
            GLFW_MOUSE_BUTTON_LEFT
        ) == GLFW_PRESS;

    mouse.rightClick =
        glfwGetMouseButton(
            window,
            GLFW_MOUSE_BUTTON_RIGHT
        ) == GLFW_PRESS;
}

std::string readFile(
    const std::string& path
) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr
            << "Failed to open "
            << path
            << "\n";

        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

GLuint compileShader(
    GLenum type,
    const std::string& path
) {
    std::string source =
        readFile(path);

    const char* src =
        source.c_str();

    GLuint shader =
        glCreateShader(type);

    glShaderSource(
        shader,
        1,
        &src,
        nullptr
    );

    glCompileShader(shader);

    GLint success;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success) {
        char buffer[1024];

        glGetShaderInfoLog(
            shader,
            1024,
            nullptr,
            buffer
        );

        std::cerr
            << buffer
            << "\n";
    }

    return shader;
}

GLuint createShaderProgram(
    const std::string& vertexPath,
    const std::string& fragmentPath
) {
    GLuint vertex =
        compileShader(
            GL_VERTEX_SHADER,
            vertexPath
        );

    GLuint fragment =
        compileShader(
            GL_FRAGMENT_SHADER,
            fragmentPath
        );

    GLuint program =
        glCreateProgram();

    glAttachShader(
        program,
        vertex
    );

    glAttachShader(
        program,
        fragment
    );

    glLinkProgram(
        program
    );

    glDeleteShader(
        vertex
    );

    glDeleteShader(
        fragment
    );

    return program;
}

GLuint loadTexture(
    const std::string& path
) {
    stbi_set_flip_vertically_on_load(
        true
    );

    int width;
    int height;
    int channels;

    unsigned char* data =
        stbi_load(
            path.c_str(),
            &width,
            &height,
            &channels,
            4
        );

    if (!data) {
        std::cerr
            << "Failed to load texture: "
            << path
            << "\n";

        return 0;
    }

    GLuint texture;

    glGenTextures(
        1,
        &texture
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(
        GL_TEXTURE_2D
    );

    stbi_image_free(data);

    return texture;
}

}

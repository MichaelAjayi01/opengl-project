#include <glew.h>
#include <glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>




void initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void initializeGLEW() {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void loadModelWithAssimp(const std::string& modelPath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return;
    }

    std::cout << "Model loaded successfully: " << modelPath << std::endl;
    std::cout << "Number of meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "Number of materials: " << scene->mNumMaterials << std::endl;
}

int main() {
    // Initialize GLFW
    initializeGLFW();
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    initializeGLEW();

    // Print OpenGL version
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Load a test model with Assimp
    loadModelWithAssimp("path/to/your/model.obj");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Render something
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
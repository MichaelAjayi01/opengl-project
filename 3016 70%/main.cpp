#include <iostream>
#include <vector>
#include <glew.h>
#include <glfw3.h>
#include <FastNoiseLite.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "shaders/LoadShaders.h"

// Vertex structure for the terrain
struct Vertex {
    glm::vec3 position; // x, y, z
    glm::vec3 color;    // RGB color
};

// Biome colors
glm::vec3 getBiomeColor(float noiseValue) {
    if (noiseValue < -0.3f) {
        return glm::vec3(0.4f, 0.4f, 0.4f); // Ruins: Dark gray
    }
    else if (noiseValue < 0.3f) {
        return glm::vec3(0.6f, 0.4f, 0.2f); // Desolate Plains: Brown
    }
    else {
        return glm::vec3(0.0f, 0.5f, 0.0f); // Forest: Green
    }
}

// Generate the terrain vertices and indices
void generateTerrain(int gridSize, float scale, FastNoiseLite& noise, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            // Generate height using FastNoise
            float noiseValue = noise.GetNoise((float)x, (float)z);
            float height = noiseValue * scale;

            // Determine vertex position and biome color
            glm::vec3 position = glm::vec3((float)x, height, (float)z);
            glm::vec3 color = getBiomeColor(noiseValue);

            vertices.push_back({ position, color });
        }
    }

    // Generate indices for the plane (two triangles per grid cell)
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int topLeft = z * (gridSize + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridSize + 1) + x;
            int bottomRight = bottomLeft + 1;

            // Triangle 1
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Triangle 2
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

// OpenGL setup and main rendering loop
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Request OpenGL 4.6 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Procedural Terrain", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Set up FastNoise
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.05f);

    // Terrain generation
    int gridSize = 100;
    float scale = 5.0f;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    generateTerrain(gridSize, scale, noise, vertices, indices);

    // Create VAO, VBO, and EBO
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Shader setup
    ShaderInfo shaders[] = {
        { GL_VERTEX_SHADER, "shaders/vertex_shader.glsl" },
        { GL_FRAGMENT_SHADER, "shaders/fragment_shader.glsl" },
        { GL_NONE, NULL } // Sentinel value
    };

    GLuint shaderProgram = LoadShaders(shaders);
    if (shaderProgram == 0) {
        std::cerr << "Failed to load shaders!" << std::endl;
        return -1;
    }
    glUseProgram(shaderProgram);

    // Set up camera parameters (view and projection matrices)
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(50.0f, 50.0f, 50.0f), glm::vec3(50.0f, 0.0f, 50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Rendering loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // Draw the terrain
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();
    return 0;
}

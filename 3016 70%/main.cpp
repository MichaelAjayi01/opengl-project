#include <iostream>
#include <vector>
#include <glew.h>
#include <glfw3.h>
#include <FastNoiseLite.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Vertex.h"
#include "Terrain.h"
#include "Sword.h"
#include "shaders/LoadShaders.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main() {
    // GLFW and GLEW Initialization
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

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

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Shader setup for terrain
    ShaderInfo terrainShaders[] = {
        { GL_VERTEX_SHADER, "shaders/terrain_vertex_shader.glsl" },
        { GL_FRAGMENT_SHADER, "shaders/terrain_fragment_shader.glsl" },
        { GL_NONE, NULL }
    };
    GLuint terrainShaderProgram = LoadShaders(terrainShaders);

    // Shader setup for swords
    ShaderInfo swordShaders[] = {
        { GL_VERTEX_SHADER, "shaders/sword_vertex_shader.glsl" },
        { GL_FRAGMENT_SHADER, "shaders/sword_fragment_shader.glsl" },
        { GL_NONE, NULL }
    };
    GLuint swordShaderProgram = LoadShaders(swordShaders);

    // Terrain generation
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.05f);

    int gridSize = 100;
    float scale = 5.0f;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    Terrain terrain(gridSize, scale, noise);
    terrain.generateTerrain(vertices, indices);

    GLuint terrainVAO, terrainVBO, terrainEBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glGenBuffers(1, &terrainEBO);

    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(3);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(50.0f, 50.0f, 150.0f), glm::vec3(50.0f, 0.0f, 50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 500.0f);

    GLuint terrainModelLoc = glGetUniformLocation(terrainShaderProgram, "model");
    GLuint terrainViewLoc = glGetUniformLocation(terrainShaderProgram, "view");
    GLuint terrainProjLoc = glGetUniformLocation(terrainShaderProgram, "projection");
    GLuint terrainLightPosLoc = glGetUniformLocation(terrainShaderProgram, "lightPos");
    GLuint terrainViewPosLoc = glGetUniformLocation(terrainShaderProgram, "viewPos");
    GLuint terrainLightColorLoc = glGetUniformLocation(terrainShaderProgram, "lightColor");

    glUseProgram(terrainShaderProgram);
    glUniformMatrix4fv(terrainViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(terrainProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 lightPos(100.0f, 100.0f, 100.0f);
    glm::vec3 viewPos(50.0f, 50.0f, 150.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    glUniform3fv(terrainLightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(terrainViewPosLoc, 1, glm::value_ptr(viewPos));
    glUniform3fv(terrainLightColorLoc, 1, glm::value_ptr(lightColor));

    // Sword scattering
    Sword sword("models/Swords/fbx/_sword_1.fbx");

    std::vector<glm::mat4> swordTransforms;
    float swordScaleFactor = 0.2f; // Example scale factor
    float offset = 7.0f; // Example offset value to control embedding depth
    sword.scatterSwords(10, gridSize, scale, swordScaleFactor, offset, noise, swordTransforms);

    GLuint swordModelLoc = glGetUniformLocation(swordShaderProgram, "model");
    GLuint swordViewLoc = glGetUniformLocation(swordShaderProgram, "view");
    GLuint swordProjLoc = glGetUniformLocation(swordShaderProgram, "projection");

    glUseProgram(swordShaderProgram);
    glUniformMatrix4fv(swordViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(swordProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Main rendering loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the terrain
        glUseProgram(terrainShaderProgram);
        glUniformMatrix4fv(terrainModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Render the swords
        glUseProgram(swordShaderProgram);
        sword.renderSwords(swordTransforms, swordShaderProgram);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up and exit
    glfwTerminate();
    return 0;
}

#include <iostream>
#include <vector>
#include <random>
#include <glew.h>
#include <glfw3.h>
#include <FastNoiseLite.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Vertex.h"
#include "Terrain.h"
#include "Sword.h"
#include "Key.h"
#include "Camera.h"
#include "shaders/LoadShaders.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Function to generate a random float between min and max
float randomFloat(float min, float max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

// Create a Camera object
Camera camera(glm::vec3(50.0f, 50.0f, 150.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

float lastX = 400, lastY = 300;
bool firstMouse = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Function to calculate delta time
float calculateDeltaTime() {
    static float lastFrameTime = 0.0f;
    float currentFrameTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;
    return deltaTime;
}

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

    // Set the mouse callback
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    // Shader setup for keys
    ShaderInfo keyShaders[] = {
        { GL_VERTEX_SHADER, "shaders/key_vertex_shader.glsl" },
        { GL_FRAGMENT_SHADER, "shaders/key_fragment_shader.glsl" },
        { GL_NONE, NULL }
    };
    GLuint keyShaderProgram = LoadShaders(keyShaders);

    // Shader setup for quad (signature)
    const char* quadVertexShaderSource = R"(
    #version 460 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    void main() {
        TexCoords = aTexCoords;
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
    )";

    const char* quadFragmentShaderSource = R"(
    #version 460 core
    out vec4 FragColor;

    in vec2 TexCoords;

    uniform sampler2D texture1;

    void main() {
        FragColor = texture(texture1, TexCoords);
    }
    )";

    GLuint quadVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(quadVertexShader, 1, &quadVertexShaderSource, NULL);
    glCompileShader(quadVertexShader);

    GLuint quadFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(quadFragmentShader, 1, &quadFragmentShaderSource, NULL);
    glCompileShader(quadFragmentShader);

    GLuint quadShaderProgram = glCreateProgram();
    glAttachShader(quadShaderProgram, quadVertexShader);
    glAttachShader(quadShaderProgram, quadFragmentShader);
    glLinkProgram(quadShaderProgram);

    glDeleteShader(quadVertexShader);
    glDeleteShader(quadFragmentShader);

    // Create a quad
    float quadVertices[] = {
        // positions   // texCoords
        -0.9f,  0.9f,  0.0f, 0.0f,
        -0.9f,  0.7f,  0.0f, 1.0f,
        -0.7f,  0.7f,  1.0f, 1.0f,

        -0.9f,  0.9f,  0.0f, 0.0f,
        -0.7f,  0.7f,  1.0f, 1.0f,
        -0.7f,  0.9f,  1.0f, 0.0f
    };


    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    GLuint signatureTexture = loadTexture("Signature/signature.jpg");

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
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 500.0f);

    GLuint terrainModelLoc = glGetUniformLocation(terrainShaderProgram, "model");
    GLuint terrainViewLoc = glGetUniformLocation(terrainShaderProgram, "view");
    GLuint terrainProjLoc = glGetUniformLocation(terrainShaderProgram, "projection");
    GLuint terrainLightPosLoc = glGetUniformLocation(terrainShaderProgram, "lightPos");
    GLuint terrainViewPosLoc = glGetUniformLocation(terrainShaderProgram, "viewPos");
    GLuint terrainLightColorLoc = glGetUniformLocation(terrainShaderProgram, "lightColor");

    glUseProgram(terrainShaderProgram);
    glUniformMatrix4fv(terrainProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 lightPos(100.0f, 100.0f, 100.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    glUniform3fv(terrainLightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(terrainLightColorLoc, 1, glm::value_ptr(lightColor));

    // Sword scattering
    Sword sword("models/Swords/fbx/_sword_1.fbx", "models/Swords/fbx/_sword_2.fbx");

    std::vector<glm::mat4> swordTransforms1;
    std::vector<glm::mat4> swordTransforms2;
    float swordScaleFactor = 0.2f; // Example scale factor
    float offset = 7.0f; // Example offset value to control embedding depth
    sword.scatterSwords(15, gridSize, scale, swordScaleFactor, offset, noise, swordTransforms1, swordTransforms2);

    GLuint swordViewLoc = glGetUniformLocation(swordShaderProgram, "view");
    GLuint swordProjLoc = glGetUniformLocation(swordShaderProgram, "projection");

    glUseProgram(swordShaderProgram);
    glUniformMatrix4fv(swordProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Key scattering
    Key key("models/Key/FBX/rust_key.FBX");

    std::vector<glm::mat4> keyTransforms;
    float keyScaleFactor = 0.25f;
    float keyHeightOffset = 1.0f;
    for (int i = 0; i < 10; ++i) {
        glm::mat4 transform = glm::mat4(1.0f);
        float x = randomFloat(0.0f, static_cast<float>(gridSize));
        float z = randomFloat(0.0f, static_cast<float>(gridSize));
        float y = terrain.getHeightAt(x, z) + keyHeightOffset;
        transform = glm::translate(transform, glm::vec3(x, y, z));
        transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(keyScaleFactor));
        key.addKeyTransform(transform);
    }

    GLuint keyViewLoc = glGetUniformLocation(keyShaderProgram, "view");
    GLuint keyProjLoc = glGetUniformLocation(keyShaderProgram, "projection");

    glUseProgram(keyShaderProgram);
    glUniformMatrix4fv(keyProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Generate random starting position for the camera
    float startX = randomFloat(0.0f, static_cast<float>(gridSize));
    float startZ = randomFloat(0.0f, static_cast<float>(gridSize));
    float startY = terrain.getHeightAt(startX, startZ) + 2.0f; // Add an offset to the height

    // Update camera position
    camera.Position = glm::vec3(startX, startY, startZ);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Main rendering loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        camera.ProcessKeyboard(window, deltaTime);

        // Constrain camera position to the terrain bounds
        camera.Position.x = glm::clamp(camera.Position.x, 0.0f, static_cast<float>(gridSize));
        camera.Position.z = glm::clamp(camera.Position.z, 0.0f, static_cast<float>(gridSize));

        // Update camera position based on terrain height
        float terrainHeight = terrain.getHeightAt(camera.Position.x, camera.Position.z);
        camera.Position.y = glm::mix(camera.Position.y, terrainHeight + 2.0f, 0.1f); // Smoothly interpolate to the target height

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the terrain
        glUseProgram(terrainShaderProgram);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(terrainViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(terrainModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(terrainViewPosLoc, 1, glm::value_ptr(camera.Position));
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Render the swords
        glUseProgram(swordShaderProgram);
        glUniformMatrix4fv(swordViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        sword.renderSwords(swordTransforms1, swordTransforms2, swordShaderProgram);

        // Render the keys
        glUseProgram(keyShaderProgram);
        glUniformMatrix4fv(keyViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        key.render(view, projection, keyShaderProgram);

        // Render the signature quad
        glUseProgram(quadShaderProgram);
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, signatureTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up and exit
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);
    glDeleteProgram(terrainShaderProgram);
    glDeleteProgram(swordShaderProgram);
    glDeleteProgram(keyShaderProgram);

    glfwTerminate();
    return 0;
    }

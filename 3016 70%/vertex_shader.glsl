#version 330 core

layout(location = 0) in vec3 position; // Vertex position
layout(location = 1) in vec3 color;    // Vertex color

out vec3 vertexColor; // Pass color to fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    vertexColor = color;
}

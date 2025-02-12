#version 330 core

layout (location = 0) in vec3 aPos;

uniform float time; // Time variable
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float size; // size of the plane

out vec2 texCoords;

void main() {
    texCoords = (aPos.xz + size / 2) / size;

    gl_Position = projection * view * model * vec4(aPos, 1.0);

//    texCoords = (aPos + 1.0) / 2.0; // Map [-1, 1] to [0, 1]
//    gl_Position = vec4(aPos, 0.0, 1.0);
}
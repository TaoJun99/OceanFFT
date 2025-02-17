#version 330 core

layout (location = 0) in vec3 aPos;

uniform float time; // Time variable
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float size; // size of the plane
uniform sampler2D inputTexture;

out vec2 texCoords;

void main() {
    texCoords = (aPos.xz + size / 2) / size;
    vec2 tex = (aPos.xz + size / 2) / size;
    float height = length(texture(inputTexture, tex).rg);
    float scale = 1.0;
    height *= scale;
    vec3 position = vec3(aPos.x, height , aPos.z);
    gl_Position = projection * view * model * vec4(position, 1.0);

}
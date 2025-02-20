#version 330 core

uniform sampler2D ifftTexture;

in vec2 texCoords;

out vec4 fragColor;

void main() {
    float scale = 20000.0;
    fragColor = vec4((texture(ifftTexture, texCoords).x + texture(ifftTexture, texCoords).y)* scale, 0.0, 0.0, 1.0);
}
#version 330 core

uniform sampler2D ifftTexture;

in vec2 texCoords;

out vec4 fragColor;

void main() {
    float height = texture(ifftTexture, texCoords).x + texture(ifftTexture, texCoords).y;
    float scale = 500.0;

    fragColor = vec4(height * scale, 0.0, 0.0, 1.0);




}
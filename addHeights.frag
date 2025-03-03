#version 330 core

uniform sampler2D oceanHeightTexture;
uniform sampler2D sweHeightTexture;

in vec2 texCoords;

out vec4 fragColor;

void main() {
    fragColor = vec4(texture(oceanHeightTexture, texCoords).x + texture(sweHeightTexture, texCoords).x, 0.0, 0.0, 0.0);
}
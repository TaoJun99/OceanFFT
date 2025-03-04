#version 330 core

uniform sampler2D v_x;
uniform sampler2D v_y;

in vec2 texCoords;

out vec4 fragColor;

void main() {
    fragColor = vec4(texture(v_x, texCoords).x, texture(v_y, texCoords).x, 0.0, 0.0);
}
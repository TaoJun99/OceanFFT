#version 330 core

uniform sampler2D inputTexture;

in vec2 texCoords;

out vec4 fragColor;

void main() {

        // Sample texture for real and imaginary components
        vec2 complexValue = texture(inputTexture, texCoords).rg;

        // Optionally, visualize real and imaginary parts separately
        float realPart = complexValue.x;
        float imagPart = complexValue.y;

        // Display the magnitude of the complex number
        float magnitude = length(complexValue) * 100000;
//        float magnitude = realPart * 1000;

        fragColor = vec4(magnitude, magnitude, magnitude, 1.0);

}
#version 330 core

uniform sampler2D fftTexture;
uniform bool isReal;
uniform int N; // grid size
uniform float L; // size of the water plane

in vec2 texCoords;

out vec4 fragColor;

void main() {
    ivec2 gridPos = ivec2(floor(texCoords * (N - 1)));
    int k_x = (gridPos.x < (N / 2)) ? gridPos.x : gridPos.x - N; // left to right: 0, +ve, -ve
    int k_y = (gridPos.y < (N / 2)) ? gridPos.y : gridPos.y - N; // bottom to top: 0, +ve, -ve

    vec2 k = (2.0 * 3.14159265359 / L) * vec2(float(k_x), float(k_y));
    float k_mag = length(k);

    if (k_mag == 0.0) {
        fragColor = vec4(0.0);
        return;
    }

    vec2 h = texture(fftTexture, texCoords).xy;

    if (isReal) {
        fragColor = -k_x / k_mag * 500 * vec4(h.y, h.x, 0.0, 0.0);
    } else {
        fragColor = -k_y / k_mag * 500 * vec4(h.y, h.x, 0.0, 0.0);
    }


}
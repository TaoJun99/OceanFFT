#version 330 core

uniform sampler2D fftTexture;
uniform float time;
uniform int N; // grid size
uniform float L; // size of the water plane

in vec2 texCoords;

out vec4 fragColor;


void main() {
    ivec2 gridPos = ivec2(floor(texCoords * (N - 1)));

    int k_x = (gridPos.x < (N / 2)) ? gridPos.x : gridPos.x - N; // left to right: 0, +ve, -ve
    int k_y = (gridPos.y < (N / 2)) ? -(gridPos.y + 1) : -(gridPos.y - N + 1); // top to bottom: 0, +ve, -ve

    vec2 k = (2.0 * 3.14159265359 / L) * vec2(float(k_x), float(k_y));
    float k_mag = length(k);

    // Dispersion relation: exp^(iwt), w = sqrt(k_mag * g);
    // (a + ib) * (cos(wt) + isin(wt)) = [a*cos(wt) - b*sin(wt)] + i[a*sin(wt) + b*cos(wt)]

    vec2 fourierAmplitude = texture(fftTexture, texCoords).rg;
    float a = fourierAmplitude.x;
    float b = fourierAmplitude.y;
    float omega = sqrt(k_mag * 9.81);
    float sinTerm = sin(omega*time);
    float cosTerm = cos(omega*time);

    fragColor = vec4(a*cosTerm - b*sinTerm, a*sinTerm + b*cosTerm, 0.0, 0.0);

}
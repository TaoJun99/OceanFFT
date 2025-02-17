#version 330 core

uniform float alpha;
uniform float g;
uniform float k_p;
uniform float gamma;
uniform int N; // grid size
uniform float L; // size of the water plane

in vec2 texCoords;

out vec4 fragColor;


float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 randomGaussian(vec2 uv) {
    // Generate random numbers
    float u1 = max(1e-6, hash(uv));                // Clamp u1 to avoid log(0)
    float u2 = hash(uv + vec2(1.0, 0.0));          // Use a different seed for the second random number

    // Box-Muller transform to generate two independent normal random variables
    float radius = sqrt(-2.0 * log(u1));
    float theta = 2.0 * 3.14159265358979323846 * u2;

    // Generate z0 and z1
    float z0 = radius * cos(theta);
    float z1 = radius * sin(theta);

    return vec2(z0, z1);
}


void main() {
    // Convert normalized UV coordinates to grid index (0 to N-1)
    ivec2 gridPos = ivec2(floor(texCoords * (N - 1)));

    // Compute discrete wave numbers k_x, k_y using FFT ordering

//    int k_x = gridPos.x - N / 2; // left: negative, right: positive
//    int k_y = (gridPos.y < (N / 2)) ? gridPos.y + N / 2 : -gridPos.y; // top: negative, bottom: positive
//    int k_y = -(gridPos.y + 1) + N / 2;
    //    int k_y = gridPos.y + N / 2;


    int k_x = (gridPos.x < (N / 2)) ? gridPos.x : gridPos.x - N; // left to right: 0, +ve, -ve
    int k_y = (gridPos.y < (N / 2)) ? -(gridPos.y + 1) : -(gridPos.y - N + 1); // top to bottom: 0, +ve, -ve

//    int k_x = gridPos.x - N / 2; // left to right: -ve, 0, +ve
//    int k_y = gridPos.y - N / 2; // bottom to top: -ve, 0, +ve

    // Compute k magnitude |k| and scale it
    vec2 k = (2.0 * 3.14159265359 / L) * vec2(float(k_x), float(k_y));
    float k_mag = length(k);

    // Avoid division by zero for k = 0
    if (k_mag == 0.0) {
        fragColor = vec4(0.0);
        return;
    }

    float sigma = (k_mag <= k_p) ? 0.07 : 0.09;

    // Compute JONSWAP spectrum
    float term1 = alpha * g * g / pow(k_mag, 5.0);
    float term2 = exp(-1.25 * pow(k_p / k_mag, 4.0));
    float term3 = pow(gamma, exp(-0.5 * pow((k_mag / k_p - 1.0) / sigma, 2.0)));

    float S_k = term1 * term2 * term3;

    vec2 gaussianvec = randomGaussian(texCoords);

    if (isnan(gaussianvec.x) || isnan(gaussianvec.y)) {
        gaussianvec.x = 0.0;
        gaussianvec.y = 0.0;
    }

    float h_r = gaussianvec.x * sqrt(S_k / 2.0);
    float h_i = gaussianvec.y * sqrt(S_k / 2.0);

    fragColor = vec4(h_r, h_i, 0.0, 0.0);

}
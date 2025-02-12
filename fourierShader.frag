#version 330 core

uniform float alpha;
uniform float g;
uniform float k_p;
uniform float gamma;
uniform int N; // grid size
uniform float L; // size of the water plane

in vec2 texCoords;

out vec4 fragColor;

vec2 generateGaussianRandom(vec2 uv) {
    // Generate two uniform random numbers between 0 and 1
    float u1 = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
    float u2 = fract(sin(dot(uv, vec2(39.797, 12.791))) * 43758.5453);

    // Box-Muller transform
    float z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265359 * u2);
    float z1 = sqrt(-2.0 * log(u1)) * sin(2.0 * 3.14159265359 * u2);

    return vec2(z0, z1);  // Return the two random Gaussian values
}


void main() {
    // Convert normalized UV coordinates to grid index (0 to N-1)
    ivec2 gridPos = ivec2(floor(texCoords * (N - 1)));

    // Compute discrete wave numbers k_x, k_y using FFT ordering
    int k_x = (gridPos.x < N / 2) ? gridPos.x : gridPos.x - N;
    int k_y = (gridPos.y < N / 2) ? gridPos.y : gridPos.y - N;

    // Compute k magnitude |k| and scale it
    vec2 k = (2.0 * 3.14159265359 / L) * vec2(k_x, k_y);
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

    vec2 xi = generateGaussianRandom(texCoords);

    float h_r = 1 / sqrt(2) * xi.x * sqrt(S_k);
    float h_i = 1 / sqrt(2) * xi.y * sqrt(S_k);

    fragColor = vec4(h_r, h_i, 0.0, 0.0);
}
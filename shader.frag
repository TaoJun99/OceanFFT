#version 330 core

in vec3 ecNormal;
in vec3 ecPosition;
in vec2 texCoords;

// Light info
uniform vec4 LightPosition; // in eye space
uniform vec4 LightAmbient;
uniform vec4 LightDiffuse;
uniform vec4 LightSpecular;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int gridSize;
uniform sampler2D inputTexture;

//uniform samplerCube EnvMap;

out vec4 fragColor;

const vec4 k_a = vec4(0.5, 0.5, 0.5, 1.0);
const vec4 k_d = vec4(0.1, 0.3, 0.5, 1.0);
const vec4 k_s = vec4(1.0, 1.0, 1.0, 1.0);
const float n = 50.0;

vec3 computeSurfaceNormal() {
    ivec2 texel = ivec2(floor(texCoords * float(gridSize - 1)));

    float delta_x = 1.0 / gridSize;

    // Get the height values at the current texel and its neighbors (using central difference)
    float hL = texelFetch(inputTexture, texel - ivec2(1, 0), 0).x; // Left neighbor
    float hR = texelFetch(inputTexture, texel + ivec2(1, 0), 0).x; // Right neighbor
    float hD = texelFetch(inputTexture, texel - ivec2(0, 1), 0).x; // Down neighbor
    float hU = texelFetch(inputTexture, texel + ivec2(0, 1), 0).x; // Up neighbor

    // Compute the partial derivatives using central difference
    float dHdx = (hR - hL) / (2.0 * delta_x); // Gradient in the x direction
    float dHdz = (hU - hD) / (2.0 * delta_x); // Gradient in the z direction

    // Construct the normal vector (negate the gradient and set z to 1)
    vec3 normal = normalize(vec3(-dHdx, 1.0, -dHdz));

    return normal; //world-space normal
}


void main() {

    // Get view vector
    vec3 viewVec = -normalize(ecPosition);

    // Get light vector: from surface to light source
    vec3 lightVec;
    if (LightPosition.w == 0.0 )
    lightVec = normalize(LightPosition.xyz);
    else
    lightVec = normalize(LightPosition.xyz - ecPosition);

    // Normalize normal vector
    vec3 N = normalize(ecNormal);

    // Compute Phong Lighting
    vec3 reflectVec = reflect(-lightVec, N);

    float L_dot_N = max(0.0, dot(-lightVec, N));
    float R_dot_V = max(0.0, dot(reflectVec, viewVec));

    vec4 phongColor = (LightAmbient * k_a) + (LightDiffuse * k_d * L_dot_N) + (LightSpecular * k_s * pow(R_dot_V, n));

    // ENV MAPPING
    // Incident ray for environment mapping is view vector
//    vec3 eyeReflectVec = reflect(viewVec, N);
//    vec3 wcReflectVec = normalize(vec3(transpose(view) * vec4(eyeReflectVec, 0.0))); // Transform reflect vector to world space
//    vec4 envColor = texture(EnvMap, wcReflectVec);
//
    vec3 L = normalize(lightVec);  // Light direction
    vec3 V = normalize(viewVec);    // View direction
    vec3 H = normalize(L + V);      // Half-vector

    float V_dot_H = max(0.0, dot(V, H));
    float exponential = pow(max(0.0, (1 - V_dot_H)), 1.0);
    float F0 = 0.02;
    float fresnel = F0 + (1.0 - F0) * exponential;
    float specularIntensity = pow(max(0.0, dot(H, N)), 16.0);


//    fragColor = mix(envColor+k_s * specularIntensity,  + (LightAmbient * k_a) + (LightDiffuse * k_d * L_dot_N), 0.4);
    fragColor = (LightAmbient * k_a) + (LightDiffuse * k_d * L_dot_N) + (LightSpecular * k_s * specularIntensity);
//    fragColor = phongColor;
    fragColor.a = 1.0;
//    fragColor = vec4(0.0, 0.5, 0.5, 1.0);

}
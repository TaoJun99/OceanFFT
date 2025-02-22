#version 330 core

in vec3 ecNormal;
in vec3 ecPosition;
in vec2 texCoords;
in float waveHeight;

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

uniform samplerCube envMap;

out vec4 fragColor;

//const vec4 k_a = vec4(0.5, 0.5, 0.5, 1.0);
//const vec4 k_d = vec4(0.1, 0.3, 0.5, 1.0);
//const vec4 k_s = vec4(1.0, 1.0, 1.0, 1.0);
//const float n = 50.0;

//const vec4 k_a = vec4(0.0, 0.1, 0.2, 1.0);  // Deep blue-green ambient reflection
//const vec4 k_d = vec4(0.0, 0.3, 0.6, 1.0);  // Diffuse reflection with more blue
//const vec4 k_s = vec4(0.8, 0.9, 1.0, 1.0);  // Slightly bluish specular highlights
//const float n = 100.0;                      // Higher shininess for glossier reflections

const vec4 k_a = vec4(0.2, 0.4, 0.5, 0.4);   // More transparent ambient
const vec4 k_d = vec4(0.1, 0.4, 0.5, 0.3);   // Transparent diffuse
const vec4 k_s = vec4(1.0, 0.9, 0.9, 1.0);   // Transparent specular
const float n = 100.0;                        // Sharp reflections

//const vec4 k_a = vec4(0.5, 0.5, 0.5, 1.0);
//const vec4 k_d = vec4(0.1, 0.3, 0.5, 1.0);
//const vec4 k_s = vec4(1.0, 1.0, 1.0, 1.0);
//const float n = 50.0;


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
    vec3 viewVec = normalize(ecPosition);

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

    float L_dot_N = max(0.0, dot(lightVec, N));
    float R_dot_V = max(0.0, dot(reflectVec, viewVec));

//    vec4 phongColor = (LightAmbient * k_a) + (LightDiffuse * k_d * L_dot_N) + (LightSpecular * k_s * pow(R_dot_V, n));

    // ENV MAPPING
    // Incident ray for environment mapping is view vector
    vec3 eyeReflectVec = reflect(-viewVec, N);
    vec3 wcReflectVec = normalize(vec3(transpose(view) * vec4(eyeReflectVec, 0.0))); // Transform reflect vector to world space
    vec4 envColor = texture(envMap, wcReflectVec);

    vec3 L = normalize(lightVec);  // Light direction
    vec3 V = normalize(viewVec);    // View direction
    vec3 H = normalize(L + V);      // Half-vector

    float V_dot_H = max(0.0, dot(V, H));
    float exponential = pow(1 - V_dot_H, 2.0);
    float F0 = 0.02;
    float fresnel = F0 + (1.0 - F0) * exponential;
    float specularIntensity = pow(max(0.0, dot(H, N)), n);

    vec4 blinnPhong = k_s * specularIntensity + (LightAmbient * k_a) + (LightDiffuse * k_d * L_dot_N);

    // light scattering
    float firstTerm = 0.8 * max(0.05, waveHeight) * pow(max(0.0, dot(L, V)), 4) * pow(0.5 - 0.5 * L_dot_N, 3); //scatter due to waveheight
    float secondTerm = 0.6 * pow(max(0.0, dot(V, N)), 2); // how visible normal is to camera
    float thirdTerm = 1.0 * L_dot_N; // Lambert cosine
    vec3 ambient = 0.4 * k_a.xyz; // ambient light

    vec4 scatterAmbient = vec4((firstTerm + secondTerm + thirdTerm) * k_d.xyz + ambient, 0.7);


    // env map w blinn phong
//    fragColor = mix(fresnel * envColor, k_s * specularIntensity + (LightAmbient * k_a) + (LightDiffuse * k_d * L_dot_N), 0.6);
//    fragColor = blinnPhong;
//    fragColor = fresnel * envColor;
    // env map w light scatter
    fragColor = mix(fresnel * envColor , fresnel * k_s * LightSpecular * specularIntensity + scatterAmbient, 1.0);
//    fragColor = mix(fresnel * envColor, k_s * specularIntensity, 0.5) + scatterAmbient;


    fragColor.a = 1.0;

}
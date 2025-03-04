#version 330 core

uniform sampler2D velocityTexture; //vector field
uniform sampler2D heightField;
uniform float halfrdx;
uniform int gridSize;
uniform float timeStep;

in vec2 texCoords;
out vec4 fragColor;

bool isBoundary(ivec2 gridCellIndex) {
    return gridCellIndex.x <= 1 || gridCellIndex.x >= gridSize - 3 ||
    gridCellIndex.y <= 1 || gridCellIndex.y >= gridSize - 3;
}

void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * (gridSize - 1)));  // Convert normalized to integer coordinates

    float h = texture(heightField, texCoords).x;

    vec4 v = texture(velocityTexture, texCoords);
    vec4 vL = texture(velocityTexture, texCoords + vec2(-1.0 / gridSize, 0));
    vec4 vR = texture(velocityTexture, texCoords + vec2(1.0 / gridSize, 0));
    vec4 vB = texture(velocityTexture, texCoords + vec2(0, -1.0 / gridSize));
    vec4 vT = texture(velocityTexture, texCoords + vec2(0,  1.0 / gridSize));

    float hL = texture(heightField, texCoords + vec2( -1.0 / gridSize, 0)).x;// Left
    float hR = texture(heightField, texCoords + vec2(1.0 / gridSize, 0)).x;// Right
    float hB = texture(heightField, texCoords + vec2(0,-1.0 / gridSize)).x;// Bottom
    float hT = texture(heightField, texCoords + vec2(0, 1.0 / gridSize)).x;// Top


    if (!isBoundary(gridCellIndex)) {
        vec2 grad_h = vec2((hR - hL) * halfrdx, (hT - hB) * halfrdx);
        float dv_dx = (vR.x - vL.x) * halfrdx;
        float dv_dy = (vT.y - vB.y) * halfrdx;

        float div_v = dv_dx + dv_dy;
        fragColor = vec4(h - 5 * timeStep * (dot(v.xy, grad_h) + h * div_v), 0.0, 0.0, 0.0);
    } else {
        fragColor = vec4(h, 0.0, 0.0, 0.0);  // Preserve height at boundary
    }

}
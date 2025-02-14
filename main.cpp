#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#include "OpenCLFFT.h"
#include <clFFT.h>


GLuint waterVAO, waterVBO, waterEBO, waterShader;
GLuint projectionLoc, viewLoc, modelLoc;
glm::mat4 projection, view;
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 2.5f);
float cameraWidth = 800.0f;
float cameraHeight = 600.0f;
Camera camera(cameraWidth, cameraHeight, cameraPos);
int waterPlaneIndexCount;
GLuint oceanHeightTexture;
GLuint fftTexture;

GLuint fourierShader;

GLuint framebuffer;

OpenCLFFT fftProcessor;


// Grid size
const int gridSize = 1000; // Number of segments in each direction
const float size = 25.0f;  // Size of the plane

float quadVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f
};

unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
};

// Function to read shader source from file
std::string readShaderSource(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to compile shader
GLuint compileShader(const std::string& source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }

    return shader;
}

// Function to create shader program
GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = readShaderSource(vertexPath);
    std::string fragmentSource = readShaderSource(fragmentPath);

    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}



void generatePlane(float** vertices, unsigned int** indices, int* indexCount) {
    // Generate vertices
    *vertices = new float[(gridSize + 1) * (gridSize + 1) * 3]; // 3 for x, y, z
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            (*vertices)[(z * (gridSize + 1) + x) * 3 + 0] = (x / (float)gridSize) * size - size / 2; // x
            (*vertices)[(z * (gridSize + 1) + x) * 3 + 1] = 0.0f; // y (initially flat)
            (*vertices)[(z * (gridSize + 1) + x) * 3 + 2] = (z / (float)gridSize) * size - size / 2; // z
        }
    }

    // Generate indices
    *indexCount = gridSize * gridSize * 6; // Each quad consists of 2 triangles
    *indices = new unsigned int[*indexCount];
    int offset = 0;
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            (*indices)[offset++] = (z * (gridSize + 1)) + x;         // Top left
            (*indices)[offset++] = ((z + 1) * (gridSize + 1)) + x;   // Bottom left
            (*indices)[offset++] = (z * (gridSize + 1)) + (x + 1);   // Top right

            (*indices)[offset++] = ((z + 1) * (gridSize + 1)) + x;   // Bottom left
            (*indices)[offset++] = ((z + 1) * (gridSize + 1)) + (x + 1); // Bottom right
            (*indices)[offset++] = (z * (gridSize + 1)) + (x + 1);   // Top right
        }
    }
}

void setupWater() {
    // Vertex data for a detailed grid plane
    float* vertices;
    unsigned int* indices;

    generatePlane(&vertices, &indices, &waterPlaneIndexCount);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glGenBuffers(1, &waterEBO);

    // Bind VAO
    glBindVertexArray(waterVAO);

    // Bind and set VBO
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, (gridSize + 1) * (gridSize + 1) * 3 * sizeof(float), vertices, GL_STATIC_DRAW);

    // Bind and set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterPlaneIndexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind VAO
    glBindVertexArray(0);

    delete[] vertices;
    delete[] indices;
}

void drawWater() {
    glUseProgram(waterShader);

    // Matrices
    glm::mat4 model = glm::mat4(1.0f);  // Identity matrix (no transformation)

    float currentTime = glfwGetTime();

    GLuint modelLoc = glGetUniformLocation(waterShader, "model");
    GLuint viewLoc = glGetUniformLocation(waterShader, "view");
    GLuint projectionLoc = glGetUniformLocation(waterShader, "projection");
//    GLuint timeLoc = glGetUniformLocation(waterShader, "time");
//    GLuint lightPosLoc = glGetUniformLocation(waterShader, "LightPosition");
//    GLuint lightAmbientLoc = glGetUniformLocation(waterShader, "LightAmbient");
//    GLuint lightDiffuseLoc = glGetUniformLocation(waterShader, "LightDiffuse");
//    GLuint lightSpecularLoc = glGetUniformLocation(waterShader, "LightSpecular");
    GLuint textureLoc = glGetUniformLocation(waterShader, "inputTexture");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
//    glUniform1f(timeLoc, currentTime);  // Pass current time to shader
//    glUniform4f(lightPosLoc, lightPosition[0], lightPosition[1], lightPosition[2], lightPosition[3]);
//    glUniform4f(lightAmbientLoc, lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]);
//    glUniform4f(lightDiffuseLoc, lightDiffuse[0], lightDiffuse[1], lightDiffuse[2], lightDiffuse[3]);
//    glUniform4f(lightSpecularLoc, lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]);
    glUniform1i(textureLoc, 1);



    glBindVertexArray(waterVAO);
//    glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxtid);
    glDrawElements(GL_TRIANGLES, waterPlaneIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}

void computeFourier() {
    glUseProgram(fourierShader);

    GLuint alphaLoc = glGetUniformLocation(fourierShader, "alpha");
    GLuint gLoc = glGetUniformLocation(fourierShader, "g");
    GLuint k_pLoc = glGetUniformLocation(fourierShader, "k_p");
    GLuint gammaLoc = glGetUniformLocation(fourierShader, "gamma");
    GLuint NLoc = glGetUniformLocation(fourierShader, "N");
    GLuint LLoc = glGetUniformLocation(fourierShader, "L");

    glUniform1f(alphaLoc, 0.0081);
    glUniform1f(gLoc, 9.81);
    glUniform1f(k_pLoc, 0.03);
    glUniform1f(gammaLoc, 3.3);
    glUniform1i(NLoc, gridSize);
    glUniform1f(LLoc, size);

    glViewport(0, 0, gridSize, gridSize);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fftTexture, 0);

    glBindVertexArray(waterVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void cleanup() {
    // Clean up resources
    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteBuffers(1, &waterEBO);
    glDeleteProgram(waterShader);
//    glDeleteProgram(skyboxShader);
}


int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // OpenGL version and core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required for macOS

    // Create a window and OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Plane with Water Movement", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Print OpenGL version
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Load and compile shaders
    waterShader = createShaderProgram("../shader.vert", "../shader.frag");
    fourierShader = createShaderProgram("../shader.vert", "../fourierShader.frag");

    GLuint VAO, VBO, EBO;
    // Create VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Textures
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &oceanHeightTexture);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, gridSize, gridSize, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &fftTexture);
    glBindTexture(GL_TEXTURE_2D, fftTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, gridSize, gridSize, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create framebuffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fftTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
    } else {
        std::cout << "Framebuffer complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    setupWater(); // Create vertices for the water height plane
    computeFourier();

    fftProcessor.setup();
    fftProcessor.performFFT();
    fftProcessor.performIFFT();  // Execute the inverse FFT

    while (!glfwWindowShouldClose(window)) {
        // Clear screen and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.Inputs(window);
        view = camera.getViewMatrix();
        projection = camera.getProjMatrix(70.0f, 0.1f, 100.0f);

        // Use shader program and bind VAO
        glDisable(GL_DEPTH_TEST);
//        drawSkybox();
        glEnable(GL_DEPTH_TEST);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        drawWater();


        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    clfftTeardown();

    cleanup();
    glfwTerminate();

    return 0;
}

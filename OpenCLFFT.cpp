#include "OpenCLFFT.h"
#include <OpenCL/cl.h>
#include <clFFT.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <OpenGL/gl3.h>
#include <OpenCL/cl_gl.h>  // Header for OpenGL-OpenCL interoperability
#include <cfloat>

OpenCLFFT::OpenCLFFT() : inputBuffer(nullptr), outputBuffer(nullptr), queue(nullptr), context(nullptr), fftPlan(0) {}

OpenCLFFT::~OpenCLFFT() {
    cleanup();
}

void OpenCLFFT::cleanup() {
    if (inputBuffer) clReleaseMemObject(inputBuffer);
    if (outputBuffer) clReleaseMemObject(outputBuffer);
    if (queue) clReleaseCommandQueue(queue);
    if (context) clReleaseContext(context);
    if (fftPlan) clfftDestroyPlan(&fftPlan);
    clfftTeardown();
}

void OpenCLFFT::checkError(cl_int err, const char* operation) {
    if (err != CL_SUCCESS) {
        std::cerr << "OpenCL error during " << operation << ": " << err << std::endl;
        cleanup();
        exit(1);
    }
}

void OpenCLFFT::setup(size_t gridSize) {
    cl_int err;
    static bool clfft_initialized = false;
    if (!clfft_initialized) {
        clfftSetupData fftSetup;
        clfftInitSetupData(&fftSetup);
        checkError(clfftSetup(&fftSetup), "clfftSetup");
        clfft_initialized = true;
    }

    cl_platform_id platform;
    checkError(clGetPlatformIDs(1, &platform, nullptr), "clGetPlatformIDs");

    cl_device_id device;
    checkError(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr), "clGetDeviceIDs");

    // Use default context creation without CGL-specific properties
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    checkError(err, "clCreateContext");

    queue = clCreateCommandQueue(context, device, 0, &err);
    checkError(err, "clCreateCommandQueue");

    // Create FFT plan
    size_t fftDims[2] = {gridSize, gridSize};
    checkError(clfftCreateDefaultPlan(&fftPlan, context, CLFFT_2D, fftDims), "clfftCreateDefaultPlan");
    checkError(clfftSetPlanPrecision(fftPlan, CLFFT_SINGLE), "clfftSetPlanPrecision");
    checkError(clfftSetLayout(fftPlan, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED), "clfftSetLayout");
    checkError(clfftSetResultLocation(fftPlan, CLFFT_OUTOFPLACE), "clfftSetResultLocation");
    checkError(clfftBakePlan(fftPlan, 1, &queue, nullptr, nullptr), "clfftBakePlan");

    inputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float2) * gridSize * gridSize, nullptr, &err);
    checkError(err, "clCreateBuffer (input)");

    outputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float2) * gridSize * gridSize, nullptr, &err);
    checkError(err, "clCreateBuffer (output)");

    // Initialize a periodic sine wave input
    std::vector<cl_float2> inputData(gridSize * gridSize);
    size_t N = gridSize;
    float frequency = 5.0f;  // Example frequency

    for (size_t y = 0; y < N; y++) {
        for (size_t x = 0; x < N; x++) {
            size_t index = y * N + x;
            float phase = 2.0f * M_PI * frequency * x / N;
            inputData[index].x = 100.0f * sin(phase);  // Real part
            inputData[index].y = 0.0f;        // Imaginary part
        }
    }

    // Upload input data to GPU
    checkError(clEnqueueWriteBuffer(queue, inputBuffer, CL_TRUE, 0, sizeof(cl_float2) * 256 * 256, inputData.data(), 0, nullptr, nullptr), "clEnqueueWriteBuffer");
}


void OpenCLFFT::performFFT() {
    cl_int err;
    std::cout << "Performing FFT...\n";

    // Execute Forward FFT
    err = clfftEnqueueTransform(fftPlan, CLFFT_FORWARD, 1, &queue, 0, nullptr, nullptr, &inputBuffer, &outputBuffer, nullptr);
    checkError(err, "clfftEnqueueTransform (Forward FFT)");

    checkError(clFinish(queue), "clFinish");

    // Read and print first 5 values of FFT output
    std::vector<cl_float2> outputData(256 * 256);
    checkError(clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0, outputData.size() * sizeof(cl_float2), outputData.data(), 0, nullptr, nullptr), "clEnqueueReadBuffer");

    std::cout << "FFT Output (first 5 values):\n";
    for (int i = 0; i < 10; i++) {
        std::cout << outputData[i].x << " + " << outputData[i].y << "i\n";
    }
}

void OpenCLFFT::performIFFT() {
    cl_int err;
    std::cout << "Performing IFFT...\n";

    // Execute IFFT
    err = clfftEnqueueTransform(fftPlan, CLFFT_BACKWARD, 1, &queue, 0, nullptr, nullptr, &outputBuffer, &inputBuffer, nullptr);
    checkError(err, "clfftEnqueueTransform (Inverse FFT)");

    checkError(clFinish(queue), "clFinish");

    // Read and normalize output data
    std::vector<cl_float2> outputData(256 * 256);
    checkError(clEnqueueReadBuffer(queue, inputBuffer, CL_TRUE, 0, outputData.size() * sizeof(cl_float2), outputData.data(), 0, nullptr, nullptr), "clEnqueueReadBuffer");

    // Normalize
    size_t N = 256 * 256;
    for (size_t i = 0; i < outputData.size(); i++) {
        outputData[i].x /= N;
        outputData[i].y /= N;
    }

    // Print first 5 values
    std::cout << "IFFT Output (first 5 values):\n";
    for (int i = 0; i < 5; i++) {
        std::cout << outputData[i].x << " + " << outputData[i].y << "i\n";
    }
}

void checkForNaNs(const cl_float2* debugBuffer, size_t gridSize) {
    bool hasNaN = false;
    for (size_t i = 0; i < gridSize * gridSize; ++i) {
        if (std::isnan(debugBuffer[i].x) || std::isnan(debugBuffer[i].y)) {
            std::cerr << "NaN detected at index " << i << " -> ("
                      << debugBuffer[i].x << ", " << debugBuffer[i].y << ")" << std::endl;
            hasNaN = true;
        }
    }

    if (!hasNaN) {
        std::cout << "No NaNs detected in debugBuffer." << std::endl;
    } else {
        std::cerr << "Warning: NaNs were found in debugBuffer!" << std::endl;
    }
}

GLfloat* OpenCLFFT::performIFFTFromOpenGLTexture(GLfloat* textureData, size_t gridSize) {
    cl_int err;

    // Step 1: Create an OpenCL buffer for FFT input data
    cl_mem inputClBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float2) * gridSize * gridSize, nullptr, &err);
    checkError(err, "clCreateBuffer (input)");

    std::vector<cl_float2> inputData(gridSize * gridSize);
    for (size_t i = 0; i < gridSize * gridSize; ++i) {
        inputData[i].x = textureData[2 * i];       // real part
        inputData[i].y = textureData[2 * i + 1];   // imaginary part
    }

    // Step 2: Copy the texture data into the OpenCL buffer
    err = clEnqueueWriteBuffer(queue, inputClBuffer, CL_TRUE, 0, sizeof(cl_float2) * gridSize * gridSize, inputData.data(), 0, nullptr, nullptr);
    checkError(err, "clEnqueueWriteBuffer (input)");

    if (err == CL_SUCCESS) {
        std::cout << "clEnqueueWriteBuffer success!" << std::endl;
    } else {
        std::cerr << "clEnqueueWriteBuffer failed with error: " << err << std::endl;
    }
// Allocate memory to store the read-back buffer as cl_float2 (real and imaginary parts)
    cl_float2* debugBuffer = new cl_float2[gridSize * gridSize];

// Read back the OpenCL buffer contents into debugBuffer
    err = clEnqueueReadBuffer(queue, inputClBuffer, CL_TRUE, 0,
                              sizeof(cl_float2) * gridSize * gridSize,
                              debugBuffer, 0, nullptr, nullptr);
    checkError(err, "clEnqueueReadBuffer (debug input)");

// Ensure the read operation completes
    clFinish(queue);

// Debug: Print the contents of the debugBuffer after reading
//    std::cout << "Contents of inputClBuffer after writing:" << std::endl;
//    for (size_t i = 0; i < std::min<size_t>(100, gridSize * gridSize); ++i) {
//        std::cout << "debugBuffer[" << i << "] = (" << debugBuffer[i].x
//                  << ", " << debugBuffer[i].y << ")" << std::endl;  // print real and imaginary parts
//    }

    checkForNaNs(debugBuffer, gridSize);

// Free allocated memory
    delete[] debugBuffer;


//    for (size_t i = 0; i < gridSize * gridSize; ++i) {
//        inputData[i].x /= (gridSize * gridSize);
//        inputData[i].y /= (gridSize * gridSize);
//    }

    float minReal = FLT_MAX, maxReal = -FLT_MAX;
    float minImag = FLT_MAX, maxImag = -FLT_MAX;

    for (size_t i = 0; i < gridSize * gridSize; ++i) {
        minReal = std::min(minReal, inputData[i].x);
        maxReal = std::max(maxReal, inputData[i].x);
        minImag = std::min(minImag, inputData[i].y);
        maxImag = std::max(maxImag, inputData[i].y);
    }

    std::cout << "FFT Input Data Range:" << std::endl;
    std::cout << "Real: Min = " << minReal << ", Max = " << maxReal << std::endl;
    std::cout << "Imag: Min = " << minImag << ", Max = " << maxImag << std::endl;

    // Step 3: Create an OpenCL buffer for the IFFT result
    cl_mem outputClBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float2) * gridSize * gridSize, nullptr, &err);
    checkError(err, "clCreateBuffer (output)");

    std::vector<cl_float2> zeroData(gridSize * gridSize, {0.0f, 0.0f});
    err = clEnqueueWriteBuffer(queue, outputClBuffer, CL_TRUE, 0, sizeof(cl_float2) * gridSize * gridSize, zeroData.data(), 0, nullptr, nullptr);
    checkError(err, "clEnqueueWriteBuffer (zeroing output buffer)");

    // Step 4: Perform the IFFT using OpenCL FFT
    err = clfftEnqueueTransform(fftPlan, CLFFT_BACKWARD, 1, &queue, 0, nullptr, nullptr, &inputClBuffer, &outputClBuffer, nullptr);
    checkError(err, "clfftEnqueueTransform (IFFT)");

    // Step 5: Wait for the IFFT to finish
    err = clFinish(queue);
    checkError(err, "clFinish");


    // Step 6: Read back the result from the OpenCL buffer into a new buffer
    cl_float2* ifftData = new cl_float2[gridSize * gridSize]; // Allocate memory to store the IFFT result
    err = clEnqueueReadBuffer(queue, outputClBuffer, CL_TRUE, 0, sizeof(cl_float2) * gridSize * gridSize, ifftData, 0, nullptr, nullptr);
    checkError(err, "clEnqueueReadBuffer (output)");

    // Step 8: Debug - Print min, max, and average of the IFFT result
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    float sum = 0.0f;

    for (size_t i = 0; i < gridSize * gridSize; ++i) {
        minVal = std::min(minVal, ifftData[i].x);
        maxVal = std::max(maxVal, ifftData[i].x);
        sum += ifftData[i].x;
    }

    std::cout << "IFFT Result Summary:" << std::endl;
    std::cout << "Min: " << minVal << ", Max: " << maxVal << ", Avg: " << (sum / (gridSize * gridSize)) << std::endl;


    GLfloat* outputData = new GLfloat[gridSize * gridSize * 2]; // Each entry has two components (real and imaginary)

    for (size_t i = 0; i < gridSize * gridSize; ++i) {
        outputData[2 * i] = ifftData[i].x * 10000;   // real part
        outputData[2 * i + 1] = ifftData[i].y; // imaginary part
    }

    // Step 7: Clean up OpenCL buffers
    err = clReleaseMemObject(inputClBuffer);
    checkError(err, "clReleaseMemObject (input)");

    err = clReleaseMemObject(outputClBuffer);
    checkError(err, "clReleaseMemObject (output)");


    // Step 8: Return the IFFT data to the caller
    return outputData;  // Return the IFFT result
}


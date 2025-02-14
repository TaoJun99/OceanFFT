#include "OpenCLFFT.h"
#include <OpenCL/cl.h>
#include <clFFT.h>
#include <iostream>
#include <vector>
#include <cmath>

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

void OpenCLFFT::setup() {
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

    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    checkError(err, "clCreateContext");

    queue = clCreateCommandQueue(context, device, 0, &err);
    checkError(err, "clCreateCommandQueue");

    // Create FFT plan
    size_t fftDims[2] = {256, 256};
    checkError(clfftCreateDefaultPlan(&fftPlan, context, CLFFT_2D, fftDims), "clfftCreateDefaultPlan");
    checkError(clfftSetPlanPrecision(fftPlan, CLFFT_SINGLE), "clfftSetPlanPrecision");
    checkError(clfftSetLayout(fftPlan, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED), "clfftSetLayout");
    checkError(clfftSetResultLocation(fftPlan, CLFFT_OUTOFPLACE), "clfftSetResultLocation");
    checkError(clfftBakePlan(fftPlan, 1, &queue, nullptr, nullptr), "clfftBakePlan");

    inputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float2) * 256 * 256, nullptr, &err);
    checkError(err, "clCreateBuffer (input)");

    outputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float2) * 256 * 256, nullptr, &err);
    checkError(err, "clCreateBuffer (output)");

    // Initialize a periodic sine wave input
    std::vector<cl_float2> inputData(256 * 256);
    size_t N = 256;
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




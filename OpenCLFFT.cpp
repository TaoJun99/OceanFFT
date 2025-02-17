#include "OpenCLFFT.h"
#include <OpenCL/cl.h>
#include <clFFT.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <OpenGL/gl3.h>
#include <OpenCL/cl_gl.h>
#include <cfloat>

OpenCLFFT::OpenCLFFT() : queue(nullptr), context(nullptr), fftPlan(0) {}

OpenCLFFT::~OpenCLFFT() {
    cleanup();
}

void OpenCLFFT::cleanup() {
//    if (inputBuffer) clReleaseMemObject(inputBuffer);
//    if (outputBuffer) clReleaseMemObject(outputBuffer);
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

    // Get platform and device info
    cl_platform_id platform;
    checkError(clGetPlatformIDs(1, &platform, nullptr), "clGetPlatformIDs");

    cl_device_id device;
    checkError(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr), "clGetDeviceIDs");

    cl_device_type deviceType;
    clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, nullptr);

    // Use default context creation
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
}

GLfloat* OpenCLFFT::performIFFTFromOpenGLTexture(GLfloat* textureData, size_t gridSize) {
    cl_int err;

    // Step 1: Create an OpenCL buffer for FFT input data
    cl_mem inputClBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(GLfloat) * 2 * gridSize * gridSize, nullptr, &err);
    checkError(err, "clCreateBuffer (input)");

    // Step 2: Copy the texture data into the OpenCL buffer
    err = clEnqueueWriteBuffer(queue, inputClBuffer, CL_TRUE, 0, sizeof(GLfloat) * 2 * gridSize * gridSize, textureData, 0, nullptr, nullptr);
    checkError(err, "clEnqueueWriteBuffer (input)");

    // Step 3: Create an OpenCL buffer for the IFFT result
    cl_mem outputClBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(GLfloat) * 2 * gridSize * gridSize, nullptr, &err);
    checkError(err, "clCreateBuffer (output)");

    // Step 4: Perform the IFFT using OpenCL FFT
    err = clfftEnqueueTransform(fftPlan, CLFFT_BACKWARD, 1, &queue, 0, nullptr, nullptr, &inputClBuffer, &outputClBuffer, nullptr);
    checkError(err, "clfftEnqueueTransform (IFFT)");

    // Step 5: Wait for the IFFT to finish
    err = clFinish(queue);
    checkError(err, "clFinish");

    // Step 6: Read back the result from the OpenCL buffer into a new buffer
    GLfloat* ifftData = new GLfloat[gridSize * gridSize * 2]; // Allocate memory to store the IFFT result
    err = clEnqueueReadBuffer(queue, outputClBuffer, CL_TRUE, 0, sizeof(GLfloat) * 2 * gridSize * gridSize, ifftData, 0, nullptr, nullptr);
    checkError(err, "clEnqueueReadBuffer (output)");

    // Normalize results if necessary
    GLfloat minVal = std::numeric_limits<GLfloat>::max();
    GLfloat maxVal = std::numeric_limits<GLfloat>::lowest();

    for (size_t i = 0; i < gridSize * gridSize * 2; ++i) {
        minVal = std::min(minVal, ifftData[i]);
        maxVal = std::max(maxVal, ifftData[i]);
    }

    GLfloat range = maxVal - minVal;
    if (range > 0.0f) {
        for (size_t i = 0; i < gridSize * gridSize * 2; ++i) {
            ifftData[i] = (ifftData[i] - minVal) / range;
        }
    }

    // Step 7: Clean up OpenCL buffers
    err = clReleaseMemObject(inputClBuffer);
    checkError(err, "clReleaseMemObject (input)");

    err = clReleaseMemObject(outputClBuffer);
    checkError(err, "clReleaseMemObject (output)");

    // Step 8: Return the IFFT data to the caller
    return ifftData;  // Return the IFFT result
}



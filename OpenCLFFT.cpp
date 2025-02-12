#include "OpenCLFFT.h"
#include <OpenCL/cl.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <clFFT.h>

OpenCLFFT::OpenCLFFT() {
    // Constructor (no specific initialization here)
}

OpenCLFFT::~OpenCLFFT() {
    // Cleanup OpenCL resources
    if (program) {
        clReleaseProgram(program);
    }
    if (fftKernel) {
        clReleaseKernel(fftKernel);
    }
    if (inputBuffer) {
        clReleaseMemObject(inputBuffer);
    }
    if (outputBuffer) {
        clReleaseMemObject(outputBuffer);
    }
    if (queue) {
        clReleaseCommandQueue(queue);
    }
    if (context) {
        clReleaseContext(context);
    }
}

void OpenCLFFT::setup() {
    // Set up OpenCL context, queue, and program

    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);

    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);

    // Create OpenCL context and command queue
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
    queue = clCreateCommandQueue(context, device, 0, nullptr);

    // Load OpenCL program (kernel code)
    std::ifstream kernelFile("fft_kernel.cl");  // OpenCL kernel source file
    std::string kernelCode((std::istreambuf_iterator<char>(kernelFile)),
                           std::istreambuf_iterator<char>());

    const char* kernelSource = kernelCode.c_str();
    size_t kernelSourceSize = kernelCode.size();

    program = clCreateProgramWithSource(context, 1, &kernelSource, &kernelSourceSize, nullptr);
    cl_int buildStatus = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);

    if (buildStatus != CL_SUCCESS) {
        char buildLog[2048];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, nullptr);
        std::cerr << "OpenCL Build Error: " << buildLog << std::endl;
        exit(1);
    }

    // Create buffers for input and output (example: 256x256 complex FFT)
    inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * 256 * 256, nullptr, nullptr);
    outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * 256 * 256, nullptr, nullptr);

    // Create kernel for FFT (this would need to be defined in your OpenCL program)
    fftKernel = clCreateKernel(program, "inverse_fft_kernel", nullptr);
}

void OpenCLFFT::performIFFT() {
    // Set arguments for the OpenCL kernel (e.g., input/output buffers, other params)
    clSetKernelArg(fftKernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(fftKernel, 1, sizeof(cl_mem), &outputBuffer);

    // Execute the kernel with appropriate work group sizes
    size_t globalWorkSize[] = { 256, 256 };  // Global work size
    size_t localWorkSize[] = { 16, 16 };     // Local work size

    clEnqueueNDRangeKernel(queue, fftKernel, 2, nullptr, globalWorkSize, localWorkSize, 0, nullptr, nullptr);
    clFinish(queue);  // Wait for completion
}

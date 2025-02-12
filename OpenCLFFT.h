#ifndef OPENCL_FFT_H
#define OPENCL_FFT_H

#include <OpenCL/cl.h>  // Include OpenCL C API headers

class OpenCLFFT {
public:
    OpenCLFFT();
    ~OpenCLFFT();

    // Setup OpenCL context, queue, and buffers
    void setup();

    // Perform Inverse FFT (iFFT)
    void performIFFT();

private:
    cl_context context;          // OpenCL context
    cl_command_queue queue;      // OpenCL command queue
    cl_program program;          // OpenCL program (compiled from source)
    cl_mem inputBuffer;          // OpenCL buffer for input data
    cl_mem outputBuffer;         // OpenCL buffer for output data

    cl_kernel fftKernel;         // OpenCL kernel for FFT operations
};

#endif


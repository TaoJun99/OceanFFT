#ifndef OPENCLFFT_H
#define OPENCLFFT_H

#include <OpenCL/cl.h>
#include <clFFT.h>

class OpenCLFFT {
public:
    OpenCLFFT();
    ~OpenCLFFT();

    void setup();
    void performFFT();
    void performIFFT();

private:
    cl_context context;
    cl_command_queue queue;
    cl_mem inputBuffer;
    cl_mem outputBuffer;
    clfftPlanHandle fftPlan;

    void cleanup();
    void checkError(cl_int err, const char* operation);
};

#endif // OPENCLFFT_H



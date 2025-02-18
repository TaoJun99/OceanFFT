#include "IFFT.h"
#include <Accelerate/Accelerate.h>
#include <iostream>
#include <vector>

IFFT::IFFT() {}

IFFT::~IFFT() {}


#include <Accelerate/Accelerate.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <GL/glew.h>

std::vector<GLfloat> IFFT::performIFFTFromTextureData(GLfloat* textureData, size_t gridSize) {
    // Ensure gridSize is a power of 2
    if ((gridSize & (gridSize - 1)) != 0) {
        std::cerr << "gridSize must be a power of 2!" << std::endl;
        exit(1);
    }

//    std::cout << "Texture Data (First 10 values): " << std::endl;
//    for (size_t i = 0; i < 10 && i < gridSize * gridSize * 2; ++i) {
//        std::cout << "Before: textureData[" << i << "] = " << textureData[i] << std::endl;
//    }


    // Prepare a vector for the IFFT result (real, imag, real, imag, ...)
    std::vector<GLfloat> outputData(gridSize * gridSize * 2);  // Interleaved result (real, imag, real, imag, ...)

    // Prepare the input in DSPSplitComplex format
    DSPSplitComplex splitComplexInput;
    splitComplexInput.realp = (float*)malloc(gridSize * gridSize * sizeof(float));  // Real part
    splitComplexInput.imagp = (float*)malloc(gridSize * gridSize * sizeof(float));  // Imaginary part

    vDSP_ctoz((DSPComplex*)textureData, 2, &splitComplexInput, 1, gridSize * gridSize);

    // Prepare the output in DSPSplitComplex format (same as input for in-place operation)
    DSPSplitComplex splitComplexOutput;
    splitComplexOutput.realp = outputData.data();           // Real part
    splitComplexOutput.imagp = outputData.data() + gridSize * gridSize;  // Imaginary part

    // Create an FFT setup (necessary for vDSP_fft_zrip)
    FFTSetup fftSetup = vDSP_create_fftsetup(log2(gridSize), FFT_RADIX2);
    if (fftSetup == nullptr) {
        std::cerr << "Failed to create FFT setup!" << std::endl;
        exit(1);
    }

    // Perform 2D FFT using vDSP_fft2d_zop
    vDSP_fft2d_zop(
            fftSetup,
            &splitComplexInput, 1, 0,   // Input data
            &splitComplexOutput, 1, 0,  // Output data
            log2(gridSize),  // Row dimension
            log2(gridSize),  // Column dimension
            FFT_INVERSE               // Perform inverse FFT
    );

    // Normalize the result by 1 / (gridSize * gridSize)
    float normalizationFactor = 1.0f / static_cast<float>(gridSize * gridSize);
    vDSP_vsmul(outputData.data(), 1, &normalizationFactor, outputData.data(), 1, gridSize * gridSize * 2);

    // Normalize results between 0 and 1 if necessary
//    size_t totalSize = gridSize * gridSize * 2;
//
//    GLfloat minVal, maxVal;
//    vDSP_minv(outputData.data(), 1, &minVal, totalSize); // Find min
//    vDSP_maxv(outputData.data(), 1, &maxVal, totalSize); // Find max
//
//    GLfloat range = maxVal - minVal;
//    if (range > 0.0f) {
//        GLfloat scale = 1.0f / range;
//        GLfloat negMin = -minVal; // Offset by min
//        vDSP_vsmsa(outputData.data(), 1, &scale, &negMin, outputData.data(), 1, totalSize);
//    }



    // Print a few values of outputData (first 10 values)
//    std::cout << "First 10 values of outputData:" << std::endl;
//    for (size_t i = 0; i < std::min<size_t>(10, outputData.size()); ++i) {
//        std::cout << "outputData[" << i << "] = " << outputData[i] << std::endl;
//    }

    // Clean up FFT setup
    vDSP_destroy_fftsetup(fftSetup);
    free(splitComplexInput.realp);
    free(splitComplexInput.imagp);

    return outputData;
}



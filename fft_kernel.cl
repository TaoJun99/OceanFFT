// fft_kernel.cl - 2D Inverse FFT (cIFFT)
__kernel void cifft_kernel(__global float* inputReal, __global float* inputImaginary,
                            __global float* outputReal, __global float* outputImaginary,
                            const unsigned int width, const unsigned int height) {

    int x = get_global_id(0);
    int y = get_global_id(1);

    // Perform the inverse FFT on the rows first
    float realSum = 0.0f;
    float imaginarySum = 0.0f;

    for (int k = 0; k < width; ++k) {
        // Complex exponential term e^(-2πikx/N) for the rows
        float angle = -2.0f * M_PI * x * k / width;
        float realPart = cos(angle);
        float imaginaryPart = sin(angle);

        realSum += (inputReal[k + y * width] * realPart - inputImaginary[k + y * width] * imaginaryPart);
        imaginarySum += (inputImaginary[k + y * width] * realPart + inputReal[k + y * width] * imaginaryPart);
    }

    // Now perform the FFT on the columns
    float realFinal = 0.0f;
    float imaginaryFinal = 0.0f;

    for (int k = 0; k < height; ++k) {
        // Complex exponential term e^(-2πiky/N) for the columns
        float angle = -2.0f * M_PI * y * k / height;
        float realPart = cos(angle);
        float imaginaryPart = sin(angle);

        realFinal += (realSum * realPart - imaginarySum * imaginaryPart);
        imaginaryFinal += (imaginarySum * realPart + realSum * imaginaryPart);
    }

    // Apply scaling factor for inverse FFT
    outputReal[x + y * width] = realFinal / (width * height);
    outputImaginary[x + y * width] = imaginaryFinal / (width * height);
}

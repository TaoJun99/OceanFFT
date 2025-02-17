#ifndef IFFT_H
#define IFFT_H

#include <vector>
#include <GL/glew.h>

class IFFT {
public:
    IFFT();
    ~IFFT();

    std::vector<GLfloat> performIFFTFromTextureData(float* textureData, size_t gridSize);

};

#endif // IFFT_H


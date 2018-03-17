#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <gli/gli.hpp>

using namespace std;

#include "CImg.h"
using namespace cimg_library;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void filter(CImg<float>& imageInput, CImg<float>& imageOutput, const int level, const int Nlevels)
{
    // distance to texture plane
    // in shader: LOD = log(powf(2.0f, Nlevels - 1.0f) * dist) / log(3)
    const float dist = powf(3.0f, level) / powf(2.0f, Nlevels - 1.0f);

    // filter size
    // at distance 1 ~= Gaussian of std 0.75
    const float filterStd = 0.75f * dist * imageInput.width();

    cout << "level " << level << endl;
    cout << "distance to texture plane = " << dist << endl;
    cout << "filterStd = " << filterStd << endl << endl;

    CImg<float> tmp = imageInput;
    tmp.blur(filterStd, filterStd, filterStd, false);

    // rescale image
    imageOutput = tmp.resize(imageOutput, 5); // 5 = cubic interpolation
    return;
}

// filtered textures

CImg<float> * imageInputPrefiltered;

// STD of gaussian filter applied on 2D image imageInputPrefiltered(:, :, level)
float level2gaussianFilterSTD(int level)
{
    float filterSTD = 0.5f * powf(1.3f, level);
    return filterSTD;
}

// inverse function
float gaussianFilterSTD2level(int filterSTD)
{
    float level = (logf(filterSTD) - logf(0.5f)) / logf(1.3f);
    level = std::max<float>(0.0f, std::min<float>(float((*imageInputPrefiltered).depth()) - 1.0f, level));
    return level;
}



int main(int argc, char* argv[])
{

    // Skip executable argument
    argc--;
    argv++;

    if (argc < 1)
    {
        printf("Syntax: <input file>\n");
        return -1;
    }

    string filenameInput(argv[0]);

	size_t pos = filenameInput.find_last_of(".");
    string filename  = filenameInput.substr(0, pos);
    string extension = filenameInput.substr(pos + 1, string::npos);

    // input image
    int x, y, n;
    float* data = stbi_loadf(filenameInput.c_str(), &x, &y, &n, 3);
    if (data == nullptr)
    {
        cerr << "can't find file " << filenameInput.c_str() << endl;
        return -1;
    }

    int offset = 0;
    CImg<float> imageInput(x, y, 1, 3);
    for (int j = 0; j < imageInput.height(); ++j)
    for (int i = 0; i < imageInput.width();  ++i)
    {
        for (int k = 0; k < imageInput.spectrum(); ++k)
            imageInput(i, j, 0, k) = data[offset++];
    }

    // filtered levels
    unsigned int Nlevels;
    for (Nlevels = 1; (imageInput.width() >> Nlevels) > 0; ++Nlevels);

    size_t levels = static_cast<size_t>(Nlevels);
    gli::extent3d extent(imageInput.width(), imageInput.height(), 1); 
    gli::texture texture(gli::TARGET_2D, gli::FORMAT_RGBA32_SFLOAT_PACK32, extent, 1, 1, levels);

    // borders
    stringstream filenameOutput (stringstream::in | stringstream::out);
    filenameOutput << filename << "_filtered" << ".dds";

    for (unsigned int level = 0; level < Nlevels; ++level)
    {
        cout << "processing file " << filenameOutput.str() << " Level: " << level << endl;
        unsigned int width = imageInput.width() >> level;
        unsigned int height = imageInput.height() >> level;

        if (width <= 0 || height <= 0)
            break;

        CImg<float> imageOutput(width, height, 1, 3);

        filter(imageInput, imageOutput, level, Nlevels);

        offset = 0;
        float* dest = reinterpret_cast<float*>(texture.data(0, 0, level));
        for (int j = 0; j < imageOutput.height(); ++j)
        for (int i = 0; i < imageOutput.width();  ++i)
        {
            dest[offset++] = imageOutput(i, j, 0, 0);
            dest[offset++] = imageOutput(i, j, 0, 1);
            dest[offset++] = imageOutput(i, j, 0, 2);
            dest[offset++] = 1.f;
        }
    }
    gli::save(texture, filenameOutput.str());

    return 0;
}

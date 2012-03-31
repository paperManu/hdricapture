#ifndef HDRIBUILDER_H
#define HDRIBUILDER_H

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

namespace paper
{
struct LDRi
{
    Mat image;
    float EV;
};

class hdriBuilder
{
public:
    hdriBuilder();
    ~hdriBuilder();

    // Adds an LDR image to the list
    // LDRi must be of type RGB8u
    bool addLDR(Mat* pImage, float pEV);

    // Retrieves the HDRI
    // To call after the HDRI generation
    Mat getHDRI();

    // Generate the HDRI
    // Empties the LDRi list
    bool computeHDRI();

private:
    /*****************/
    // Attributes
    // LDR images list
    vector<LDRi> mLDRi;

    // Computed HDRi
    Mat mHDRi;

    // Minimum sum used in the HDRi computation
    float mMinSum;

    // Images with highest and lowest exposures
    unsigned int mMinExposureIndex;
    unsigned int mMaxExposureIndex;

    /****************/
    // Methods
    float getGaussian(float pValue);
};
}

#endif // HDRIBUILDER_H

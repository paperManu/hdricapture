#include "hdribuilder.h"

using namespace paper;

/*******************************************/
hdriBuilder::hdriBuilder()
{
    mMinSum = 0.5f;
    mMinExposureIndex = 0;
    mMaxExposureIndex = 0;

    mHDRi.create(0, 0, CV_32FC3);
}

/*******************************************/
hdriBuilder::~hdriBuilder()
{
}

/*******************************************/
bool hdriBuilder::addLDR(Mat *pImage, float pEV)
{
    LDRi lLDRi;

    lLDRi.EV = pEV;
    lLDRi.image = *pImage;

    // Check if this is the first image
    if(mLDRi.size() == 0)
    {
        mLDRi.push_back(lLDRi);
        return true;
    }
    // If not, we must check the width and height
    // as they must be the same
    else
    {
        if(lLDRi.image.cols != mLDRi[0].image.cols)
        {
            return false;
        }
        if(lLDRi.image.rows != mLDRi[0].image.rows)
        {
            return false;
        }

        mLDRi.push_back(lLDRi);
        return true;
    }
}

/*******************************************/
Mat hdriBuilder::getHDRI()
{
    return mHDRi;
}

/*******************************************/
bool hdriBuilder::computeHDRI()
{
    // If no LDRi were submitted
    if(mLDRi.size() == 0)
        return false;

    // HDRi the same size as LDRi, but RGB32f
    mHDRi.create(mLDRi[0].image.rows, mLDRi[0].image.cols, CV_32FC3);

    // Checking the highest and lowest exposure images
    float lHighEV = numeric_limits<float>::min();
    float lLowEV = numeric_limits<float>::max();
    for(unsigned int i=0; i<mLDRi.size(); i++)
    {
        if(mLDRi[i].EV > lHighEV)
        {
            lHighEV = mLDRi[i].EV;
            mMaxExposureIndex = i;
        }
        if(mLDRi[i].EV < lLowEV)
        {
            lLowEV = mLDRi[i].EV;
            mMinExposureIndex = i;
        }
    }

    // Calculation of each HDR pixel
    for(unsigned int x=0; x<(unsigned int)mHDRi.cols; x++)
    {
        for(unsigned int y=0; y<(unsigned int)mHDRi.rows; y++)
        {
            float lHDRPixel_R = 0.f;
            float lHDRPixel_G = 0.f;
            float lHDRPixel_B = 0.f;
            float lSum_R = 0.f;
            float lSum_G = 0.f;
            float lSum_B = 0.f;

            // We add the contribution of each image
            for(unsigned int index=0; index<mLDRi.size(); index++)
            {
                unsigned char lLDRPixel_R = mLDRi[index].image.at<Vec3b>(y, x)[0];
                unsigned char lLDRPixel_G = mLDRi[index].image.at<Vec3b>(y, x)[1];
                unsigned char lLDRPixel_B = mLDRi[index].image.at<Vec3b>(y, x)[2];

                float lCoeff;
                // Red
                lCoeff = getGaussian(lLDRPixel_R);
                lSum_R += lCoeff;
                lHDRPixel_R += lCoeff*(float)lLDRPixel_R/127.f*pow(2.0f, mLDRi[index].EV);

                // Green
                lCoeff = getGaussian(lLDRPixel_G);
                lSum_G += lCoeff;
                lHDRPixel_G += lCoeff*(float)lLDRPixel_G/127.f*pow(2.0f, mLDRi[index].EV);

                // Blue
                lCoeff = getGaussian(lLDRPixel_B);
                lSum_B += lCoeff;
                lHDRPixel_B += lCoeff*(float)lLDRPixel_B/127.f*pow(2.0f, mLDRi[index].EV);
            }

            // We divide by the sum of gaussians to get the final values
            lHDRPixel_R /= lSum_R;
            lHDRPixel_G /= lSum_G;
            lHDRPixel_B /= lSum_B;

            mHDRi.at<Vec3f>(y, x)[0] = lHDRPixel_R;
            mHDRi.at<Vec3f>(y, x)[1] = lHDRPixel_G;
            mHDRi.at<Vec3f>(y, x)[2] = lHDRPixel_B;
        }
    }

    mLDRi.clear();
    return true;
}

/*******************************************/
float hdriBuilder::getGaussian(float pValue)
{
    float lSigma = 40;
    float lMu = 127;

    float lValue = 1/(lSigma*2*M_PI)*exp(-(pValue-lMu)*(pValue-lMu)/(2*pow(lSigma,2)));
    // We want the mean value (lMu) to return 1
    lValue /= 1/(lSigma*2*M_PI);

    return lValue;
}

#ifndef CAMERA_H
#define CAMERA_H

#include "opencv2/opencv.hpp"

using namespace cv;

namespace paper
{
enum cameraType
{
    none,
    sony
};

class camera
{
public:
    camera();
    ~camera();

    // Open and close the camera
    bool open(cameraType pCamera);
    void close();

    // Set camera parameters
    bool setAperture(float pAperture);
    bool setShutter(float pShutter); // pShutter = 1/t
    bool setGain(float pGain); // in dB
    void setDefaultISO(float pISO);
    bool setFrameRate(float pRate);
    bool setBrightness(float pBrightness);
    bool setGamma(float pGamma);
    bool setColorBalance(float pRed, float pBlue); // between 0.f and 2.f

    bool setWidth(unsigned int pWidth);
    bool setHeight(unsigned int pHeight);

    // Returns camera parameters
    float getAperture();
    float getShutter();
    float getGain();

    // Return the exposure value (from the current settings)
    float getEV();

    // Capture images
    Mat getImage();

private:
    VideoCapture mCamera;
    cameraType mCameraType;

    float mAperture;
    float mShutter;
    float mDefaultISO;
    float mGain;
};
}

#endif // CAMERA_H

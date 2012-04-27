#ifndef CAMERA_H
#define CAMERA_H

#include "opencv2/opencv.hpp"
#include "lcms2.h"

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
    bool setICCProfiles(const char* pInProfile, const char* pOutProfile = "sRGB"); // set an ICC profile for color correction
    bool setCalibration(const char* pCalibFile); // set a calibration file containing deformation informations on the camera+lense

    bool setWidth(unsigned int pWidth);
    bool setHeight(unsigned int pHeight);
    void setFOV(float pFOV); // set the field of view of the camera. Automatically calculated if calibration is set

    // Returns camera parameters
    float getAperture();
    float getShutter();
    float getGain();
    float getFOV();

    // Return the exposure value (from the current settings)
    float getEV();

    // Capture images
    Mat getImage();

private:
    VideoCapture mCamera;
    cameraType mCameraType;

    // Camera parameters
    float mAperture;
    float mShutter;
    float mDefaultISO;
    float mGain;
    float mFOV;

    // ICC related attributes
    cmsHTRANSFORM mICCTransform;

    // Lense deformation correction
    Mat mCameraMat, mDistortionMat;
    Mat mRectifyMap1, mRectifyMap2;
};
}

#endif // CAMERA_H

#include "camera.h"

using namespace paper;

/*******************************************/
camera::camera()
{
    mCameraType = none;
    mAperture = 2.f;
    mShutter = 60.f;
    mGain = 0.f;
    mDefaultISO = 100.f;
}

/*******************************************/
camera::~camera()
{
}

/*******************************************/
bool camera::open(cameraType pCamera)
{
    switch(pCamera)
    {
    case sony:
        mCamera.open(CV_CAP_IEEE1394);
        if(!mCamera.isOpened())
        {
            mCameraType = none;
            return false;
        }
        else
        {
            mCameraType = pCamera;
            setShutter(mShutter);
            setGain(mGain);
            return true;
        }
        break;
    default:
        return false;
    }
}

/*******************************************/
void camera::close()
{
    if(mCameraType != none)
        mCamera.release();
}

/*******************************************/
bool camera::setAperture(float pAperture)
{
    bool lResult = true;

    switch(mCameraType)
    {
    case sony:
        mAperture = max(pAperture, 0.f);
        break;
    default:
        lResult = false;
    }

    return lResult;
}

/*******************************************/
bool camera::setShutter(float pShutter)
{
    bool lResult = true;
    float lValue;

    switch(mCameraType)
    {
    case sony:
        if(pShutter < 0.5)
        {
            lValue = 2048-7.5/0.5;
            mShutter = 0.5;
        }
        else if(pShutter <= 7.5)
        {
            lValue = 2048.f-7.5/pShutter;
            mShutter = pShutter;
        }
        else if(pShutter <= 5882)
        {
            lValue = 3116.4 - 7999/pShutter;
            mShutter = pShutter;
        }
        else if(pShutter <= 10000)
        {
            lValue = 3116.f;
            mShutter = 10000.f;
        }
        else
        {
            lValue = 3117.f;
            mShutter = 20000.f;
        }

        lResult &= mCamera.set(CV_CAP_PROP_EXPOSURE, lValue);
        break;
    default:
        lResult = false;
    }

    return true;
}

/*******************************************/
bool camera::setGain(float pGain)
{
    bool lResult = true;
    float lValue;

    switch(mCameraType)
    {
    case sony:
        mGain = max(min(pGain, 18.f), 0.f);
        lValue = 2048 + mGain*10.f;
        lResult &= mCamera.set(CV_CAP_PROP_GAIN, lValue);
        break;
    default:
        lResult = false;
    }

    return lResult;
}

/*******************************************/
void camera::setDefaultISO(float pISO)
{
    mDefaultISO = max(pISO, 0.f);
}

/*******************************************/
bool camera::setFrameRate(float pRate)
{
    bool lResult = true;

    switch(mCameraType)
    {
    case sony:
        lResult &= mCamera.set(CV_CAP_PROP_FPS, pRate);
        break;
    default:
        lResult = false;
    }

    return lResult;
}

/*******************************************/
bool camera::setBrightness(float pBrightness)
{
    bool lResult = true;
    float lValue = max(min(pBrightness, 1.0f), 0.0f);

    switch(mCameraType)
    {
    case sony:
        lValue = lValue*(223.f-96.f)+96.f;
        lResult &= mCamera.set(CV_CAP_PROP_GAMMA, lValue);
        break;
    default:
        lResult = false;
    }

    return lResult;
}

/*******************************************/
bool camera::setGamma(float pGamma)
{
    bool lResult = true;

    switch(mCameraType)
    {
    case sony:
        if(pGamma == 2.2f)
            lResult &= mCamera.set(CV_CAP_PROP_GAMMA, 129);
        else if(pGamma > 2.2f)
            lResult &= mCamera.set(CV_CAP_PROP_GAMMA, 130);
        else
            lResult &= mCamera.set(CV_CAP_PROP_GAMMA, 128);
        break;
    default:
        lResult = false;
    }

    return lResult;
}

/*******************************************/
bool camera::setColorBalance(float pRed, float pBlue)
{
    bool lResult = true;

    switch(mCameraType)
    {
    case sony:
        lResult &= mCamera.set(CV_CAP_PROP_WHITE_BALANCE_RED_V, (unsigned char)(pRed*128));
        lResult &= mCamera.set(CV_CAP_PROP_WHITE_BALANCE_BLUE_U, (unsigned char)(pBlue*128));
        break;
    default:
        lResult = false;
    }

    return lResult;
}

/*******************************************/
bool camera::setWidth(unsigned int pWidth)
{
    return mCamera.set(CV_CAP_PROP_FRAME_WIDTH, pWidth);
}

/*******************************************/
bool camera::setHeight(unsigned int pHeight)
{
    return mCamera.set(CV_CAP_PROP_FRAME_HEIGHT, pHeight);
}

/*******************************************/
float camera::getAperture()
{
    return mAperture;
}

/*******************************************/
float camera::getShutter()
{
    return mShutter;
}

/*******************************************/
float camera::getGain()
{
    return mGain;
}

/*******************************************/
float camera::getEV()
{
    return log2(mAperture*mAperture*mShutter*100/mDefaultISO)-mGain/6.f;
}

/*******************************************/
Mat camera::getImage()
{
    Mat lFrame;
    mCamera >> lFrame;
    return lFrame;
}
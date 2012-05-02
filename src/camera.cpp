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
    mFOV = 50.f;

    mICCTransform = NULL;
    mIsLabD65 = false;
}

/*******************************************/
camera::~camera()
{
    if(mICCTransform != NULL)
        cmsDeleteTransform(mICCTransform);
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

            // Get some informations
            mWidth = mCamera.get(CV_CAP_PROP_FRAME_WIDTH);
            mHeight = mCamera.get(CV_CAP_PROP_FRAME_HEIGHT);
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

    return lResult;
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
bool camera::setICCProfiles(const char *pInProfile, const char *pOutProfile)
{
    cmsHPROFILE lInProfile, lOutProfile;
    cmsUInt32Number lOutType;

    // Open input profile
    lInProfile = cmsOpenProfileFromFile(pInProfile, "r");

    // Open output profile
    if(strcmp(pOutProfile, "sRGB") == 0)
    {
        lOutType = TYPE_BGR_8;
        lOutProfile = cmsCreate_sRGBProfile();
        mIsLabD65 = false;
    }
    else if(strcmp(pOutProfile, "RGB_E") == 0)
    {
        lOutType = TYPE_BGR_8;

        // We create a CIE RGB with an E white point
        // and NO gamma curve
        cmsCIExyY lWhitePoint = {1.f/3.f, 1.f/3.f, 1.f};
        cmsCIExyYTRIPLE lPrimaries = {{0.7347f, 0.2653f, 1.f},{0.2738f, 0.7174f, 1.f}, {0.1666, 0.0089f, 1.f}};

        cmsToneCurve* lCurve[3];
        lCurve[0] = cmsBuildGamma(NULL, 1.f);
        lCurve[2] = lCurve[1] = lCurve[0];

        lOutProfile = cmsCreateRGBProfile(&lWhitePoint, &lPrimaries, lCurve);
        mIsLabD65 = false;
    }
    else if(strcmp(pOutProfile, "RGB_D65") == 0)
    {
        lOutType = TYPE_BGR_8;

        // We create a CIE RGB with a D65 white point
        // and NO gamma curve
        cmsCIExyY lWhitePoint = {0.31271f, 0.32902f, 1.f};
        cmsCIExyYTRIPLE lPrimaries = {{0.64f, 0.33f, 1.f},{0.3f, 0.6f, 1.f}, {0.15, 0.06f, 1.f}};

        cmsToneCurve* lCurve[3];
        lCurve[0] = cmsBuildGamma(NULL, 1.f);
        lCurve[2] = lCurve[1] = lCurve[0];

        lOutProfile = cmsCreateRGBProfile(&lWhitePoint, &lPrimaries, lCurve);
        mIsLabD65 = false;
    }
    else if(strcmp(pOutProfile, "Lab") == 0)
    {
        lOutType = TYPE_Lab_8;
        lOutProfile = cmsCreateLab2Profile(NULL);
        mIsLabD65 = false;
    }
    else if(strcmp(pOutProfile, "Lab_E") == 0)
    {
        lOutType = TYPE_Lab_8;

        cmsCIExyY lWhitePoint = {1.f/3.f, 1.f/3.f, 1.f};
        lOutProfile = cmsCreateLab2Profile(&lWhitePoint);
        mIsLabD65 = false;
    }
    else if(strcmp(pOutProfile, "Lab_D65") == 0)
    {
        lOutType = TYPE_Lab_8;
        cmsCIExyY lWhitePoint = {0.31271f, 0.32902f, 1.f};

        lOutProfile = cmsCreateLab2Profile(&lWhitePoint);
        mIsLabD65 = true;
    }
    else
    {
        lOutProfile = cmsOpenProfileFromFile(pOutProfile, "r");
        lOutType = TYPE_BGR_8;
        mIsLabD65 = false;
    }

    if(lInProfile == NULL || lOutProfile == NULL)
    {
        // If profiles are NULL, we deactivate current profile
        if(mICCTransform != NULL)
        {
            cmsDeleteTransform(mICCTransform);
            mICCTransform = NULL;
        }

        cmsCloseProfile(lInProfile);
        cmsCloseProfile(lOutProfile);
        mIsLabD65 = false;
        return false;
    }

    // Creating the transform
    mICCTransform = cmsCreateTransform(lInProfile, TYPE_BGR_8, lOutProfile, lOutType, INTENT_ABSOLUTE_COLORIMETRIC, 0);

    // Closing the profiles
    cmsCloseProfile(lInProfile);
    cmsCloseProfile(lOutProfile);

    return true;
}

/*******************************************/
bool camera::setCalibration(const char *pCalibFile)
{
    FileStorage lFile;
    lFile.open(pCalibFile, FileStorage::READ);
    if(!lFile.isOpened())
    {
        std::cerr << "Error while opening calibration file." << std::endl;
        return false;
    }

    lFile["Camera_Matrix"] >> mCameraMat;
    lFile["Distortion_Coefficients"] >> mDistortionMat;

    lFile.release();

    return true;
}

/*******************************************/
bool camera::setWidth(unsigned int pWidth)
{
    bool lReturn;
    lReturn =  mCamera.set(CV_CAP_PROP_FRAME_WIDTH, pWidth);
    mWidth = mCamera.get(CV_CAP_PROP_FRAME_WIDTH);
    return lReturn;
}

/*******************************************/
bool camera::setHeight(unsigned int pHeight)
{
    bool lReturn;
    lReturn = mCamera.set(CV_CAP_PROP_FRAME_HEIGHT, pHeight);
    mHeight = mCamera.get(CV_CAP_PROP_FRAME_HEIGHT);
    return lReturn;
}

/*******************************************/
void camera::setFOV(float pFOV)
{
    if(pFOV > 0.f)
        mFOV = pFOV;
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
float camera::getFOV()
{
    return mFOV;
}

/*******************************************/
Mat camera::getImage()
{
    Mat lFrame;

    // Capturing the frame
    mCamera >> lFrame;

    // If the frame is empty (error while capturing), we create a black frame
    if(lFrame.rows == 0 || lFrame.cols == 0)
        lFrame = Mat::zeros(mHeight, mWidth, CV_8UC3);

    // If specified so, correct the colorimetry
    if(mICCTransform != NULL)
    {
        for(int i=0; i<lFrame.rows; i++)
        {
            Mat lRow = lFrame.row(i);
            cmsDoTransform(mICCTransform, lRow.data, lRow.data, lRow.step/lRow.channels());
        }

        if(mIsLabD65)
        {
            cvtColor(lFrame, lFrame, CV_Lab2BGR);
        }
    }

    // Same for the distortion
    if(mCameraMat.rows != 0)
    {
        // Check if the mapping matrices have already been calculated
        if(mRectifyMap1.rows == 0 || mRectifyMap2.rows == 0)
        {
            Mat lTempMat;
            Size lImageSize(lFrame.cols, lFrame.rows);
            initUndistortRectifyMap(mCameraMat, mDistortionMat, Mat(), mCameraMat, lImageSize, CV_16SC2, mRectifyMap1, mRectifyMap2);

            mFOV = 2*atan(((float)lImageSize.width/2.f)/(mCameraMat.at<double>(0,0)));
        }

        // Correct distortion
        if(mRectifyMap1.rows != 0 && mRectifyMap2.rows != 0)
        {
            Mat lBuffer;
            remap(lFrame, lBuffer, mRectifyMap1, mRectifyMap2, INTER_LINEAR, BORDER_CONSTANT, Scalar(0,0,0));
            lFrame = lBuffer;
        }
    }

    return lFrame.clone();
}

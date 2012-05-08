#include "chromedsphere.h"

using namespace paper;

/*******************************************/
chromedSphere::chromedSphere()
{
    mTrackingLength = 5;
    mThreshold = 2;
    mSphereReflectance = 1.f;
}

/*******************************************/
chromedSphere::~chromedSphere()
{
}

/*******************************************/
bool chromedSphere::setProbe(Mat pImage, float pFOV, Vec3f pSphere)
{
    bool lReturn = true;

    if(pImage.rows == 0)
        return false;
    if(pFOV <= 0)
        return false;

    // Copying the input image
    mImage = pImage;
    mFOV = pFOV*M_PI/180.f;

    // If the sphere is not specified, we detect it
    if(pSphere[2] == 0.f)
        mSphere = detectSphere();
    else
        mSphere = pSphere;

    // If no sphere is detected, we indicate so.
    if(mSphere[2] == 0.f)
        lReturn &= false;

    // Filter the position of the sphere;
    mSphere = filterSphere(mSphere);

    // Position and size of the sphere on the image are given
    // We use that to estimate its distance from the camera
    distanceFromCamera();

    // Now we crop around the probe
    mSphereImage = cropImage();

    // Compute the transformation map
    createTransformationMap();

    return lReturn;
}

/*******************************************/
bool chromedSphere::setProbe(Mat pImage, bool pFixed)
{
    if(pImage.rows == 0)
        return false;

    mImage = pImage;

    distanceFromCamera();
    mSphereImage = cropImage();

    if(!pFixed)
        createTransformationMap();

    return true;
}

/*******************************************/
Mat chromedSphere::getConvertedProbe()
{
    Mat lImage, lProbe;

    // If no sphere was detected
    if(mSphere[2] == 0.f)
        return Mat::zeros(256, 512, mImage.type());

    // Set the (0,0) pixel to black
    Mat lPixel = mSphereImage(Rect(0, 0, 1, 1));
    lPixel.setTo(0);

    // Correct the probe
    remap(mSphereImage, lImage, mMap, Mat(), CV_INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));

    // Crop and resize the probe
    switch(mProjection)
    {
    case eEquirectangular:
        resize(lImage, lImage, Size(512, 512));
        lProbe = lImage(Rect(0, 128, 512, 256));
        break;
    default:
        break;
    }

    // And apply the reflectance coefficient
    lProbe *= 1/mSphereReflectance;

    return lProbe;
}

/*******************************************/
void chromedSphere::setSphereSize(float pSize)
{
    mSphereDiameter = pSize;
}

/*******************************************/
void chromedSphere::setSphereReflectance(float pReflectance)
{
    mSphereReflectance = pReflectance;
}

/*******************************************/
void chromedSphere::setProjection(projection pProjection)
{
    mProjection = pProjection;
}

/*******************************************/
void chromedSphere::setTrackingLength(unsigned int pLength, float pThreshold)
{
    mTrackingLength = pLength;
    mThreshold = pThreshold;
}

/*******************************************/
Vec3f chromedSphere::detectSphere()
{
    Mat lImage, lBuffer;

    // Convert the source image to grayscale
    cvtColor(mImage, lBuffer, CV_BGR2GRAY);
    GaussianBlur(lBuffer, lBuffer, Size(3,3), 2, 2);
    equalizeHist(lBuffer,lBuffer);

    // Detect the circles
    vector<Vec3f> lCircles;
    HoughCircles(lBuffer, lCircles, CV_HOUGH_GRADIENT, 2, lBuffer.cols, 200, 150, lBuffer.rows/8, lBuffer.rows);

    // If no sphere has ben detected ...
    if(lCircles.size() == 0)
        return Vec3f(0.f, 0.f, 0.f);

    // We check that the circle fits in the image
    if(lCircles[0][0]+lCircles[0][2] > mImage.cols || lCircles[0][0]-lCircles[0][2] < 0
            || lCircles[0][1]+lCircles[0][2] > mImage.rows || lCircles[0][1]-lCircles[0][2] < 0)
        return Vec3f(0.f, 0.f, 0.f);

#ifdef _DEBUG
    // Draw the circles on the input image
    lImage = mImage.clone();
    for(size_t i=0; i<lCircles.size(); i++)
    {
        circle(lImage, Point(lCircles[i][0], lCircles[i][1]), 3, Scalar(0, 255, 0));
        circle(lImage, Point(lCircles[i][0], lCircles[i][1]), lCircles[i][2], Scalar(0, 0, 255));
    }
    imwrite("_debugCircles.png", lImage);
    imwrite("_debugCirclesGray.png", lBuffer);
#endif

    return lCircles[0];
}

/*******************************************/
Vec3f chromedSphere::filterSphere(Vec3f pNewSphere)
{
    // Added the new value to the vector
    // We test if the new value is valid
    if(pNewSphere[2] != 0.f)
    {
        if(mSpherePositions.size() == (size_t)mTrackingLength)
            mSpherePositions.erase(mSpherePositions.begin());

        mSpherePositions.push_back(pNewSphere);
    }
    // If no sphere was detected till now,
    // and the new one is not ok
    else if(mSpherePositions.size() == (size_t)0)
    {
        return Vec3f(0.f, 0.f, 0.f);
    }

    // Calculating the standard deviation
    Vec3f lSum = Vec3f(0.f, 0.f, 0.f);
    Vec3f lSum2 = Vec3f(0.f, 0.f, 0.f);
    for(unsigned int index=0; index<(unsigned int)mSpherePositions.size(); index++)
    {
        lSum += mSpherePositions[index];
        lSum2 += mSpherePositions[index].mul(mSpherePositions[index]);
    }
    lSum *= (1.f/(float)mSpherePositions.size());
    lSum2 *= (1.f/(float)mSpherePositions.size());

    Vec3f lSigma;
    lSigma[0] = sqrtf(lSum2[0]-lSum[0]*lSum[0]);
    lSigma[1] = sqrtf(lSum2[1]-lSum[1]*lSum[1]);
    lSigma[2] = sqrtf(lSum2[2]-lSum[2]*lSum[2]);

    // We eliminate values too far from the mean, according to sigma
    Vec3f lFiltered = Vec3f(0.f, 0.f, 0.f);
    for(unsigned int index=0; index<(unsigned int)mSpherePositions.size(); index++)
    {
        if(mSpherePositions[index][0]-lSum[0] > 2*lSigma[0]
                || mSpherePositions[index][1]-lSum[1] > 2*lSigma[1]
                || mSpherePositions[index][2]-lSum[2] > 2*lSigma[2])
        {
#ifdef _DEBUG
            std::cout << "Erased!" << std::endl;
#endif
            mSpherePositions.erase(mSpherePositions.begin()+index);
            index--;
        }
        else
        {
            lFiltered += mSpherePositions[index];
        }
    }
#ifdef _DEBUG
    std::cout << "Length: " << (int)mSpherePositions.size() << std::endl;
#endif
    lFiltered *= (1.f/(float)mSpherePositions.size());

    // We check if the new position is farther than mThreshold*lSigma to the mean center
    // No check on the radius as the hough transform is not precise enough for it
    if(pNewSphere[2] != 0.f)
    {
        float lDistance = sqrtf(powf(pNewSphere[0]-lFiltered[0], 2.f)+powf(pNewSphere[1]-lFiltered[1], 2.f));
        float lSigmaDist = sqrtf(lSigma[0]*lSigma[0]+lSigma[1]*lSigma[1]);
        if((lDistance > mThreshold*lSigmaDist) && !(mThreshold == 0))
        {
            // We moved too far away, reset averager
#ifdef _DEBUG
            std::cout << "Reset!" << std::endl;
#endif
            mSpherePositions.clear();
            mSpherePositions.push_back(pNewSphere);
            return pNewSphere;
        }
    }

    // Else, we return the mean value
    return lFiltered;
}

/*******************************************/
void chromedSphere::distanceFromCamera()
{
    // It has been detected, so it's projected as a sphere
    // We consider here that the FOV is not to wide so that it's not
    // too much deformed.
    // Anyway, we compute a coeff to correct for not seeing the sphere
    // with an ortho projection
    // This is not a full correction but should suffice for normal lenses.

    // Computing the angle occupied by the sphere
    float lLongitudeCoeff = tan(mFOV/2.f)/((float)mImage.cols/2.f);

    float lXLeft = (mSphere[0]-mSphere[2]-mImage.cols/2.f)*lLongitudeCoeff;
    float lXRight = (mSphere[0]+mSphere[2]-mImage.cols/2.f)*lLongitudeCoeff;

    float lAlpha = atan(lXRight) - atan(lXLeft);
    lAlpha = max(lAlpha, -lAlpha);
    mCroppedFOV = lAlpha;

    // Now we compute the "real" radius from the viewed one
    float lPixelRadius = mSphere[2]/cos(lAlpha/2.f);

    // Calculation of the positionning error we will get next
    // as the new FOV is the one for the ball if slightly nearer
    float lCorrection = mSphereDiameter/2.f*sin(lAlpha/2.f);

    // And compute the real FOV occupied by the equator of the sphere
    lXLeft = (mSphere[0]-lPixelRadius-mImage.cols/2.f)*lLongitudeCoeff;
    lXRight = (mSphere[0]+lPixelRadius-mImage.cols/2.f)*lLongitudeCoeff;
    lAlpha = atan(lXRight) - atan(lXLeft);
    lAlpha = max(lAlpha, -lAlpha);

    mCameraDistance = mSphereDiameter/(2*tan(lAlpha/2.f)) + lCorrection;
}

/*******************************************/
Mat chromedSphere::cropImage()
{
    Mat lCroppedImage = mImage(Rect(mSphere[0]-mSphere[2], mSphere[1]-mSphere[2], mSphere[2]*2, mSphere[2]*2));

#ifdef _DEBUG
    imwrite("_debugCrop.png", lCroppedImage);
#endif

    return lCroppedImage;
}

/*******************************************/
void chromedSphere::createTransformationMap()
{
    // this is a mirror ball, height = width
    float lWidth = (float)mSphereImage.cols;
    float lHeight = lWidth;

    // Allocating the map
    mMap.create(lHeight, lWidth, CV_32FC2);

#ifdef _DEBUG
    Mat lDebugAngle = Mat(lHeight, lWidth, CV_8UC3);
#endif

    // We scan through the light probe
    for(float x=0; x<lWidth; x++)
    {
        for(float y=0; y<lHeight; y++)
        {
            float lAlpha, lBeta; // alpha along width, beta along height
            // Calculating the view angle
            // Check if the pixel is on the mirror ball
            if(sqrtf(powf(x-lWidth/2.f, 2.f)+powf(y-lHeight/2.f, 2.f)) > lWidth/2.f)
            {
                // Set the value to a constant, corresponding to the back of the sphere
                mMap.at<Vec2f>(y, x) = Vec2f(M_PI, M_PI);
            }
            else
            {
                lAlpha = atanf(((x-lWidth/2.f)*mSphereDiameter/lWidth)/mCameraDistance);
                lBeta = -atanf(((y-lHeight/2.f)*mSphereDiameter/lHeight)/mCameraDistance);

#ifdef _DEBUG
                lDebugAngle.at<Vec3b>(y, x)[0] = (unsigned char)roundf(((lAlpha+mCroppedFOV/2.f)*255.f/mCroppedFOV));
                lDebugAngle.at<Vec3b>(y, x)[1] = (unsigned char)roundf(((lBeta+mCroppedFOV/2.f)*255.f/mCroppedFOV));
#endif

                mMap.at<Vec2f>(y, x) = directionFromViewAngle(Vec2f(lAlpha, lBeta));
            }
        }
    }

#ifdef _DEBUG
    imwrite("_debugAngle.png", lDebugAngle);
#endif

#ifdef _DEBUG
    {
        Mat lDebugMap = Mat::zeros(lHeight, lWidth, CV_32FC3);
        int fromTo[] = {0, 0, 1, 1};
        mixChannels(&mMap, 1, &lDebugMap, 1, fromTo, 2);
        lDebugMap.convertTo(lDebugMap, CV_8UC3, 255.f/(2*M_PI), 127);
        imwrite("_debugMap.png", lDebugMap);
    }
#endif

    // Calculate the desired projection
    projectionMapFromDirections();

#ifdef _DEBUG
    {
        Mat lDebugMap = Mat::zeros(lHeight, lWidth, CV_32FC3);
        int fromTo[] = {0, 0, 1, 1};
        mixChannels(&mMap, 1, &lDebugMap, 1, fromTo, 2);
        lDebugMap.convertTo(lDebugMap, CV_8UC3, 255.f/lWidth, 0.f);
        imwrite("_debugMapEqui.png", lDebugMap);
    }
#endif
}

/*******************************************/
Vec2f chromedSphere::directionFromViewAngle(Vec2f pAngle)
{
    // All formulas are from the mathematica file "chromed_sphere"

    float lA, lB; // for lisibility, we will stock the angles in these vars
    float lC; // ... and the distance to camera in this one ...
    float lR; // ... and the sphere radius
    lA = pAngle[0];
    lB = pAngle[1];
    lC = -mCameraDistance;
    lR = mSphereDiameter/2.f;

    // Precomputing some useful values
    float lCosA = cosf(lA);
    float lCosB = cosf(lB);
    float lCos2A = cosf(2*lA);
    float lCos2B = cosf(2*lB);

    // First we verify that the pixel is on the sphere
    // (although this test should have been done before)
    float lDelta = lCosA*lCosA*lCosB*lCosB*(-2*lC*lC+3*lR*lR+lR*lR*lCos2B+lCos2A*(lR*lR+(2*lC*lC-lR*lR)*lCos2B));
    if(lDelta < 0.f)
    {
        // not on the sphere: no projection!
        return Vec2f(0.f, 0.f);
    }

    // We can safely calculate the t parameter
    float lNum, lDenom;
    lNum = 4*lC+4*lC*lCos2A+2*lC*cosf(2*lA-2*lB)+4*lC*lCos2B+2*lC*cosf(2*(lA+lB));
    lDenom = 2*(-6-2*lCos2A+cosf(2*lA-2*lB)-2*lCos2B+cosf(2*(lA+lB)));
    float lT;
    if(lDenom > 0.f)
        lT = (lNum-8*sqrtf(lDelta))/lDenom;
    else
        lT = (lNum+8*sqrtf(lDelta))/lDenom;

    // And we calculte the point of impact
    float lX, lY, lZ;
    lX = lC+lT;
    lY = lT*tanf(lA);
    lZ = lT*tanf(lB);

    // Now, we create a base to project the ray from the camera on
    Vec3f lU, lV, lW;
    lU = Vec3f(lX, lY, lZ)*(1/lR); // This is the normal to the sphere
    lV = Vec3f(lU[1], -lU[0], 0);
    lW = lU.cross(lV);

    // Projection on this new base
    Vec3f lNewCoords;
    Vec3f lInDir = Vec3f(lX-lC, lY, lZ);
    lNewCoords[0] = lInDir.dot(lU);
    lNewCoords[1] = lInDir.dot(lV);
    lNewCoords[2] = lInDir.dot(lW);

    // The reflected ray has the same direction, except on the axis
    // which is normal to the sphere: there it goes the opposite
    lNewCoords[0] = -lNewCoords[0];

    // We can reproject the direction in the old base
    Vec3f lOutDir = lNewCoords[0]*lU + lNewCoords[1]*lV + lNewCoords[2]*lW;

    // Calculate Euler angles from this new direction
    lOutDir = lOutDir*(1/sqrtf(lOutDir.dot(lOutDir)));

    float lYaw, lPitch;
    // First, yaw
    if(lOutDir[0] == 0.f)
    {
        if(lOutDir[1] > 0.f)
            lYaw = M_PI_2;
        else
            lYaw = -M_PI_2;
    }
    else if(lOutDir[0] > 0.f)
    {
        if(lOutDir[1] < 0.f)
            lYaw = asinf(-lOutDir[1]/sqrt(lOutDir[0]*lOutDir[0]+lOutDir[1]*lOutDir[1]));
        else
            lYaw = 2*M_PI+asinf(-lOutDir[1]/sqrt(lOutDir[0]*lOutDir[0]+lOutDir[1]*lOutDir[1]));
    }
    else
    {
        lYaw = M_PI-asinf(-lOutDir[1]/sqrt(lOutDir[0]*lOutDir[0]+lOutDir[1]*lOutDir[1]));
    }

    // Then pitch
    lPitch = asinf(lOutDir[2]);

    return Vec2f(lYaw-M_PI, lPitch);
}

/*******************************************/
void chromedSphere::projectionMapFromDirections()
{
    switch(mProjection)
    {
    case eEquirectangular:
        // If equirectangular, we have to scale the projection to the dimension
        // of the source probe image
    {
        float lWCoeff = (float)mSphereImage.cols/(2*M_PI);
        float lHCoeff = (float)mSphereImage.rows/(2*M_PI);

        for(int x=0; x<mSphereImage.cols; x++)
        {
            for(int y=0; y<mSphereImage.rows; y++)
            {
                mMap.at<Vec2f>(y,x)[0] = (mMap.at<Vec2f>(y,x)[0]+M_PI)*lHCoeff;
                mMap.at<Vec2f>(y,x)[1] = (mMap.at<Vec2f>(y,x)[1]+M_PI)*lWCoeff;
            }
        }
    }
        break;
    default:
        return;
    }

    // We have a forward map: cv::remap needs a backward one
    // So we will create it, as well as a mask for non-set values
    // Going through the probe ...
    Mat lBackMap = Mat::zeros(mMap.rows, mMap.cols, mMap.type());
    Mat lMask = Mat::zeros(mMap.rows, mMap.cols, CV_8U);

    for(int x=0; x<mMap.cols; x++)
    {
        for(int y=0; y<mMap.rows; y++)
        {
            // We set (in chromedSphere::createTransformationMap()) values for the
            // angles to M_PI. We don't want to consider them, so ...
            if(mMap.at<Vec2f>(y, x)[0] < mSphereImage.rows-1 && mMap.at<Vec2f>(y, x)[1] < mSphereImage.cols-1)
            {
                int u, v;
                u = round(mMap.at<Vec2f>(y,x)[0]);
                v = round(mMap.at<Vec2f>(y,x)[1]);

                lBackMap.at<Vec2f>(v,u)[0] = mMap.cols-x;
                lBackMap.at<Vec2f>(v,u)[1] = mMap.rows-y;

                lMask.at<unsigned char>(v,u) = 255;
            }
        }
    }

    // Mask inversion
    lMask = 255-lMask;

    // We need to filter the result to fill the holes
    Mat lBuffer = Mat::zeros(mMap.rows, mMap.cols, mMap.type());

    // We dilate the map in a temp buffer
    dilate(lBackMap, lBuffer, Mat(), Point(-1, -1), 4);

    // And add it mask-wise
    mMap = lBackMap;
    add(mMap, lBuffer, mMap, lMask, mMap.type());
    mMap = lBuffer;
}

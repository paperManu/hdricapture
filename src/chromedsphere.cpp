#include "chromedsphere.h"

using namespace paper;

/*******************************************/
chromedSphere::chromedSphere()
{
}

/*******************************************/
chromedSphere::~chromedSphere()
{
}

/*******************************************/
bool chromedSphere::setProbe(Mat *pImage, float pFOV, Vec3f pSphere)
{
    if(pImage == NULL)
        return false;
    if(pFOV <= 0)
        return false;

    // Copying the input image
    mImage = pImage->clone();
    mFOV = pFOV*M_PI/180.f;
    mSphere = pSphere;

    // Position and size of the sphere on the image are given
    // We use that to estimate its distance from the camera
    distanceFromCamera();

    // Now we crop around the probe
    mSphereImage = cropImage();

    // Compute the transformation map
    createTransformationMap();

    return true;
}

/*******************************************/
Mat chromedSphere::getConvertedProbe()
{
    Mat lImage;

    // Correct the probe
    remap(mSphereImage, lImage, mMap, Mat(), CV_INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));

    return lImage;
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
Mat chromedSphere::detectSphere()
{
    Mat lImage;
    return lImage;
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
                lBeta = atanf(((y-lHeight/2.f)*mSphereDiameter/lHeight)/mCameraDistance);

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

    // First we verify that the pixel is on the sphere
    // (although this test should have been done before)
    float lDelta = cos(lA)*cos(lA)*cos(lB)*cos(lB)*(-2*lC*lC+3*lR*lR+lR*lR*cos(2*lB)+cos(2*lA)*(lR*lR+(2*lC*lC-lR*lR)*cos(2*lB)));
    if(lDelta < 0.f)
    {
        // not on the sphere: no projection!
        return Vec2f(0.f, 0.f);
    }

    // We can safely calculate the t parameter
    float lNum, lDenom;
    lNum = 4*lC+4*lC*cos(2*lA)+2*lC*cos(2*lA-2*lB)+4*lC*cos(2*lB)+2*lC*cos(2*(lA+lB));
    lDenom = 2*(-6-2*cos(2*lA)+cos(2*lA-2*lB)-2*cos(2*lB)+cos(2*(lA+lB)));
    float lT;
    if(lDenom > 0.f)
        lT = (lNum-8*sqrt(lDelta))/lDenom;
    else
        lT = (lNum+8*sqrt(lDelta))/lDenom;

    // And we calculte the point of impact
    float lX, lY, lZ;
    lX = lC+lT;
    lY = lT*tan(lA);
    lZ = lT*tan(lB);

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

    // And we calculate final values
    float lBeta1, lBeta2;
    if(lX == 0.f)
    {
        if(lY > 0.f)
            lBeta1 = -M_PI/2.f;
        else
            lBeta1 = M_PI/2.f;

        if(lZ > 0.f)
            lBeta2 = M_PI/2.f;
        else
            lBeta2 = -M_PI/2.f;
    }
    else
    {
        lBeta1 = atan(lY/lX);
        lBeta2 = atan(lZ/lX)/cos(lBeta1);
    }

    float lR1 = sqrt(lX*lX+lY*lY);
    float lR2 = sqrt(lX*lX+lZ*lZ);

    float lLambda1, lLambda2;
    // Here we need to take approximation into account
    float lBuffer = (lX*(lC-lX)-lY*lY)/(lR1*sqrt((lC-lX)*(lC-lX)+lY*lY));
    if(lBuffer < -1.f)
        lBuffer = -1.f;
    else if(lBuffer > 1.f)
        lBuffer = 1.f;
    lLambda1 = acos(lBuffer);

    lBuffer = (lX*(lC-lX)-lZ*lZ)/(lR2*sqrt((lC-lX)*(lC-lX)+lZ*lZ));
    if(lBuffer < -1.f)
        lBuffer = -1.f;
    else if(lBuffer > 1.f)
        lBuffer = 1.f;
    lLambda2 = acos(lBuffer);

    float lDelta1, lDelta2;
    if(lY > 0.f)
        lDelta1 = lBeta1-lLambda1;
    else
        lDelta1 = lBeta1+lLambda1;

    if(lZ > 0.f)
        lDelta2 = lBeta2-lLambda2;
    else
        lDelta2 = lBeta2+lLambda2;

    return Vec2f(lDelta1, lDelta2);
}

/*******************************************/
void chromedSphere::projectionMapFromDirections()
{
    switch(mProjection)
    {
    case eEquirectangular:
        // If equirectangular, we have to scale the projection to the dimension
        // of the source probe image
        for(int x=0; x<mSphereImage.cols; x++)
        {
            for(int y=0; y<mSphereImage.rows; y++)
            {
                mMap.at<Vec2f>(y,x)[0] = (mMap.at<Vec2f>(y,x)[0]+M_PI)*(float)mSphereImage.rows/(2*M_PI);
                mMap.at<Vec2f>(y,x)[1] = (mMap.at<Vec2f>(y,x)[1]+M_PI)*(float)mSphereImage.cols/(2*M_PI);
            }
        }
        break;
    default:
        return;
    }

    // We have a forward map: cv::remap needs a backward one
    // So we will create it
    // Going through the probe ...
    Mat lBackMap = Mat::zeros(mMap.rows, mMap.cols, mMap.type());
    for(int x=0; x<mMap.cols; x++)
    {
        for(int y=0; y<mMap.rows; y++)
        {
            // We set (in chromedSphere::createTransformationMap()) values for the
            // angles to M_PI. We don't want to consider them, so ...
            if(mMap.at<Vec2f>(y, x)[0] < mSphereImage.rows && mMap.at<Vec2f>(y, x)[1] < mSphereImage.cols)
            {
                int u, v;
                u = round(mMap.at<Vec2f>(y,x)[0]);
                v = round(mMap.at<Vec2f>(y,x)[1]);

                lBackMap.at<Vec2f>(v,u)[0] = mMap.cols-x;
                lBackMap.at<Vec2f>(v,u)[1] = mMap.rows-y;
            }
        }
    }
    // We need to filter the result to fill the holes
    Mat lBuffer = Mat::zeros(mMap.rows, mMap.cols, mMap.type());
    for(int x=1; x<mMap.cols-1; x++)
    {
        for(int y=1; y<mMap.rows-1; y++)
        {
            if(lBackMap.at<Vec2f>(y,x) == Vec2f(0.f, 0.f))
            {
                Vec2f lValue = Vec2f(0.f, 0.f);
                float lIndex = 0.f;
                for(int u=-1; u<2; u++)
                    for(int v=-1; v<2; v++)
                    {
                        if(lBackMap.at<Vec2f>(y+v, x+u) != Vec2f(0.f, 0.f))
                        {
                            lValue[0] = lValue[0]*(lIndex)/(lIndex+1) + lBackMap.at<Vec2f>(y+v, x+u)[0]/(lIndex+1);
                            lValue[1] = lValue[1]*(lIndex)/(lIndex+1) + lBackMap.at<Vec2f>(y+v, x+u)[1]/(lIndex+1);
                            lIndex++;
                        }
                    }
                mMap.at<Vec2f>(y,x) = lValue;
            }
            else
            {
                mMap.at<Vec2f>(y,x) = lBackMap.at<Vec2f>(y,x);
            }
        }
    }
    mMap = lBackMap.clone();
}

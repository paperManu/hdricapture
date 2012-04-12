// The class will convert the lightprobe captured by a chromed sphere
// to a standard panoramic image, using an equirectangular projection
// as of yet.

#ifndef CHROMEDSPHERE_H
#define CHROMEDSPHERE_H

#include <opencv2/opencv.hpp>

using namespace cv;

enum projection
{
    eEquirectangular = 0
};

namespace paper
{
class chromedSphere
{
public:
    chromedSphere();
    ~chromedSphere();

    // Set and retrieve lightprobe
    bool setProbe(Mat* pImage, float pFOV, Vec2f pSpherePosition, float pSphereSize);
    Mat getConvertedProbe();

    // Sets various parameters
    void setSphereSize(float pSize); // Chromed sphere size, in mm
    void setSphereReflectance(float pReflectance); // % of reflected light
    void setProjection(projection pProjection);

private:
    // Attributes
    Mat mImage;
    Mat mSphereImage; // image cropped to contain only the chromed sphere

    float mSphereRealSize;
    float mSphereReflectance;

    float mCameraDistance;
    float mSphereSize;

    projection mProjection;

    // Methods
    // Detect the chromed sphere
    Mat detectSphere();

    // Calculating illumination direction from the position
    // of pixels on the chrome sphere
    float directionFromViewAngle(Vec2f pAngle);

    // Converting chrome sphere image to equirectangular panorama
    Mat convertToEquirectangular();
};
}

#endif // CHROMEDSPHERE_H

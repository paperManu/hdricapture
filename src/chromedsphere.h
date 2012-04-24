// The class will convert the lightprobe captured by a chromed sphere
// to a standard panoramic image, using an equirectangular projection
// as of yet.

#ifndef CHROMEDSPHERE_H
#define CHROMEDSPHERE_H

#include <opencv2/opencv.hpp>

//#define _DEBUG

using namespace cv;

namespace paper
{
enum projection
{
    eEquirectangular = 0
};

class chromedSphere
{
public:
    chromedSphere();
    ~chromedSphere();

    // Set and retrieve lightprobe
    bool setProbe(Mat* pImage, float pFOV, Vec3f pSphere = Vec3f(0.f, 0.f, 0.f)); // pFOV in degree, see mSphere for pSphere
    Mat getConvertedProbe();

    // Sets various parameters
    void setSphereSize(float pSize); // Chromed sphere size, in mm
    void setSphereReflectance(float pReflectance); // % of reflected light
    void setProjection(projection pProjection);

private:
    /***************************/
    // Attributes
    Mat mImage;
    Mat mSphereImage; // image cropped to contain only the chromed sphere

    float mFOV, mCroppedFOV; // FOV of the whole image, FOV of the cropped one
    Vec3f mSphere; // position and radius of the sphere projection (in the image)

    float mSphereDiameter; // Real radius (in mm)
    float mSphereReflectance;

    float mCameraDistance;

    projection mProjection; // projection type, default to equirectangular
    Mat mMap; // map of the geometrical transformation

    /***************************/
    // Methods
    // Detect the chromed sphere
    Vec3f detectSphere();

    // Distance from position and size of the sphere on the image
    void distanceFromCamera();

    // Crops the view to keep only the sphere
    Mat cropImage();

    // Creates the transformation map (mMap)
    void createTransformationMap();

    // Calculating illumination direction from the position
    // of pixels on the chrome sphere
    Vec2f directionFromViewAngle(Vec2f pAngle);

    // Convert the map according to the chosen projection
    void projectionMapFromDirections();
};
}

#endif // CHROMEDSPHERE_H

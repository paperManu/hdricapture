#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "rgbe.h"

#include "hdribuilder.h"
#include "camera.h"
#include "chromedsphere.h"

using namespace std;
using namespace cv;
using namespace paper;

boost::mutex gMutex;
Mat gFrame;
bool gFrameUpdated;
bool gStopAll;
bool gFixSphere;

/*************************************/
void probe()
{
    chromedSphere lSphere;

    lSphere.setProjection(eEquirectangular);
    lSphere.setSphereSize(50.8f);

    lSphere.setTrackingLength(30, 3);

    bool lStop = false;
    bool lUpdated = false;
    bool lFixSphere = false;
    Mat lFrame;
    for(;;)
    {
        gMutex.lock();
        lUpdated = gFrameUpdated;
        lFixSphere = gFixSphere;
        if(gFrameUpdated)
        {
            lFrame = gFrame.clone();
            gFrameUpdated = false;
        }
        gMutex.unlock();

        if(lUpdated)
        {
            if(lFixSphere)
                lSphere.setProbe(gFrame);
            else
                lSphere.setProbe(gFrame, 52.8f);
        }

        Mat lPano = lSphere.getConvertedProbe();
        imshow("probe", lPano);
        usleep(1);

        gMutex.lock();
        lStop = gStopAll;
        gMutex.unlock();
        if(lStop)
            break;
    }
}

/*************************************/
int main(int argc, char** argv)
{
    bool lResult;
    bool lViewMode = false;
    bool lCreateHDRi = false;
    bool lProbeMode = false;
    int lLdrNbr = 1;

    // Camera parameters
    camera lCamera;
    float lAperture = 5.6f;
    float lISO = 100.0f;
    float lStopSteps = 1.0f;
    float lGain = 0.0f;
    bool lGamma = false;

    if(argc < 2)
    {
        cout << "No argument - Running in view mode." << endl;
        lViewMode = true;
    }
    else
    {
        for(int i=1; i<argc; i++)
        {
            if(strcmp(argv[i], "--view") == 0)
            {
                lViewMode = true;
            }
            else if(strcmp(argv[i], "--gain") == 0)
            {
                lGain = boost::lexical_cast<float>(argv[i+1]);
            }
            else if(strcmp(argv[i], "--ldr") == 0)
            {
                lLdrNbr = boost::lexical_cast<int>(argv[i+1]);
            }
            else if(strcmp(argv[i], "--hdr") == 0)
            {
                lCreateHDRi = true;
            }
            else if(strcmp(argv[i], "--stop") == 0)
            {
                lStopSteps = boost::lexical_cast<float>(argv[i+1]);
            }
            else if(strcmp(argv[i], "--probe") == 0)
            {
                lProbeMode = true;
            }
            else if(strcmp(argv[i], "--profile") == 0)
            {
                lCamera.setICCProfiles("profile.icc", "sRGB");
            }
        }
    }

    if(!lCamera.open(sony))
        return 1;

    lCamera.setAperture(lAperture);
    lCamera.setWidth(1280);
    lCamera.setColorBalance(1.f, 1.f);
    lCamera.setGamma(1.f);
    lCamera.setDefaultISO(lISO);
    lCamera.setGain(lGain);
    lCamera.setFrameRate(7.5);

    Mat lFrame, lProbe;

    if(lViewMode || lProbeMode)
    {
        double lShutterSpeed = 15;

        lCamera.setShutter(lShutterSpeed);

        boost::thread lThread;

        if(lProbeMode)
        {
            gStopAll = false;
            gFixSphere = false;

            lThread = boost::thread(&probe);
        }

        for(;;)
        {
            // Image capture
            gMutex.lock();
            gFrame = lCamera.getImage();
            gFrameUpdated = true;
            imshow("frame", gFrame);
            gMutex.unlock();

            int lKey = waitKey(5);
            if(lKey >= 0)
            {
                if(lKey == 'f')
                {
                    gMutex.lock();
                    gFixSphere = !gFixSphere;
                    gMutex.unlock();
                }
                else if(lKey == 'm')
                {
                    lShutterSpeed = min(lShutterSpeed*2.0, 5000.0);
                    lCamera.setShutter(lShutterSpeed);
                }
                else if(lKey == 'p')
                {
                    lShutterSpeed = max(lShutterSpeed/2.0, 7.5);
                    lCamera.setShutter(lShutterSpeed);
                }
                else if(lKey == 's')
                {
                    cout << "Shot!" << endl;
                    gMutex.lock();
                    imwrite("capture.png", gFrame);
                    gMutex.unlock();
                }
                else if(lKey == 'g')
                {
                    if(lGamma)
                    {
                        lGamma = !lGamma;
                        lCamera.setGamma(1.f);
                    }
                    else
                    {
                        lGamma = !lGamma;
                        lCamera.setGamma(2.2f);
                    }
                }
                else
                {
                    gMutex.lock();
                    gStopAll = true;
                    gMutex.unlock();
                    break;
                }
            }
        }

        if(lProbeMode)
            lThread.join();
    }
    else if(!lViewMode)
    {
        hdriBuilder lHDRiBuilder;
        double lShutterSpeed = 1.f;

        // Set gamma to 1
        lCamera.setGamma(1.f);

        for(int i=0; i<lLdrNbr; i++)
        {
            lCamera.setShutter(lShutterSpeed);
            // Real shutter speed might differ from the specified one
            lShutterSpeed = lCamera.getShutter();

            // Loop to empty the buffer
            for(int j=0; j<5; j++)
            {
                lCamera.getImage();
                usleep(10);
            }
            lFrame = lCamera.getImage();

            if(lCreateHDRi == true)
            {
                Mat lFrame_RGB(lFrame.size(), lFrame.type());
                cvtColor(lFrame, lFrame_RGB, CV_BGR2RGB);
                lResult = lHDRiBuilder.addLDR(&lFrame_RGB, lCamera.getEV());
                if(lResult)
                    cout << "LDRi successfully added, f=" << lAperture << ", 1/t=" << lShutterSpeed << endl;
                else
                    cout << "Error while adding LDRi." << endl;
            }

            string lStr = "img_" + boost::lexical_cast<std::string>(i) + ".bmp";
            lResult = imwrite(lStr, lFrame);
            if(!lResult)
            {
                cout << "Error while writing image n°" << i << endl;
            }

            lShutterSpeed *= pow(2, lStopSteps);
        }

        if(lCreateHDRi)
        {
            cout << "Computing HDRi ..." << endl;
            if(lHDRiBuilder.computeHDRI())
                cout << "HDRi successfully computed." << endl;
            else
                cout << "Error while computing HDRi." << endl;

            // Saving in Radiance HDR format
            Mat lHDRi = lHDRiBuilder.getHDRI();
            FILE *lFile = fopen("hdri.hdr", "wb");
            if(lHDRi.isContinuous())
            {
                RGBE_WriteHeader(lFile, lHDRi.cols, lHDRi.rows, NULL);
                RGBE_WritePixels(lFile, (float*)lHDRi.data, lHDRi.rows*lHDRi.cols);
            }
            fclose(lFile);
        }
    }

    lCamera.close();
    return 0;
}

#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <boost/lexical_cast.hpp>
#include "rgbe.h"

#include "hdribuilder.h"
#include "camera.h"

using namespace std;
using namespace cv;
using namespace paper;

int main(int argc, char** argv)
{
    bool lResult;
    bool lViewMode = false;
    bool lCreateHDRi = false;
    int lLdrNbr = 1;

    // Camera parameters
    camera lCamera;
    float lAperture = 4.0f;
    float lISO = 100.0f;

    if(argc > 1)
    {
        if(strcmp(argv[1], "--view") == 0)
            lViewMode = true;
        else if(strcmp(argv[1], "--ldr") == 0)
        {
            if(argc == 2)
                lLdrNbr = 5;
            else
                lLdrNbr = boost::lexical_cast<int>(argv[2]);

            if(argc == 4 && strcmp(argv[3], "--hdr") == 0)
                lCreateHDRi = true;
        }
        else
            return 0;
    }

    if(!lCamera.open(sony))
        return 1;

    lCamera.setAperture(4.f);
    lCamera.setWidth(1280);
    lCamera.setColorBalance(1.f, 1.f);
    lCamera.setGamma(1.f);
    lCamera.setGain(0.f);

    Mat lFrame;

    if(lViewMode == false)
    {
        hdriBuilder lHDRiBuilder;
        double lShutterSpeed = 1.f;

        // Set gamma to 1
        lCamera.setGamma(1.f);

        for(int i=0; i<lLdrNbr; i++)
        {
            lCamera.setShutter(lShutterSpeed);
            lShutterSpeed *= pow(2, 1.0);

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
                lResult = lHDRiBuilder.addLDR(&lFrame_RGB, log2(lAperture*lAperture*lShutterSpeed*100/lISO));
                if(lResult)
                    cout << "LDRi successfully added." << endl;
                else
                    cout << "Error while adding LDRi." << endl;
            }

            string lStr = "img_" + boost::lexical_cast<std::string>(i) + ".bmp";
            lResult = imwrite(lStr, lFrame);
            if(!lResult)
            {
                cout << "Error while writing image n°" << i << endl;
            }
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
    else
    {
        double lShutterSpeed = 60;

        lCamera.setShutter(lShutterSpeed);
        lCamera.setGamma(2.2f);

        for(;;)
        {
            lFrame = lCamera.getImage();
            imshow("frame", lFrame);

            int lKey = waitKey(5);
            if(lKey >= 0)
            {
                if(lKey == 'm')
                {
                    lShutterSpeed = min(lShutterSpeed*2.0, 5000.0);
                    lCamera.setShutter(lShutterSpeed);
                }
                else if(lKey == 'p')
                {
                    lShutterSpeed = max(lShutterSpeed/2.0, 7.5);
                    lCamera.setShutter(lShutterSpeed);
                }
                else
                {
                    break;
                }
            }
        }
    }

    lCamera.close();
    return 0;
}

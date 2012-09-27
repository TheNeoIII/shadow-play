#include "ofMain.h"
#include "ofxOpenCv.h"
#include "recording.h"

class camera
{
public:
    int width;
    int height;
    bool isTest;

    recording rec;

    ofxCvColorImage	camFrame;

    ofxCvGrayscaleImage gsCamFrame;
    ofxCvGrayscaleImage gsScaledCamFrame;

    ofVideoGrabber vg;


    float scaleFactor;

    camera(bool isTest);

    void update();

    int getWidth();

    int getHeight();

    void drawFrame();

    ofxCvGrayscaleImage* getFrame();
};

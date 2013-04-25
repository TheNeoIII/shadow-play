#include "ofMain.h"
#include "ofxOpenCv.h"

class recording
{
public:
    int width;
    int height;
    float speed;
    ofxCvGrayscaleImage frame;
    int frameIndex;
    list<ofxCvGrayscaleImage> frameCache;
    list<ofxCvGrayscaleImage>::iterator it;
    int numFrames;

    recording();
    void init(string framespath);
    void update();
    int getWidth();
    int getHeight();
    void drawFrame(int x, int y, int w, int h);
    unsigned char* getPixels();
    ofxCvGrayscaleImage* getFrame();
};

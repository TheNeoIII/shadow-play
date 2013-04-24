#include "camera.h"

camera::camera(bool isTest)
{
        this->isTest = isTest;
        if(isTest)
        {
            rec.init("recordings/camera_test/");
            this->width = rec.getWidth();
            this->height = rec.getHeight();
        }
        else
        {
            vg.setDeviceID(1);
            vg.initGrabber(320,240,false);
            this->width = vg.getWidth();
            this->height = vg.getHeight();
        }
        camFrame.allocate(width,height);
        gsCamFrame.allocate(width,height);
}

void camera::update()
{
        if(isTest)
        {
            rec.update();
            gsCamFrame.setFromPixels(rec.getPixels(),rec.getWidth(),rec.getHeight());
        }
        else
        {

            vg.update();
            vg.grabFrame();

            camFrame.setFromPixels(vg.getPixels(),vg.getWidth(),vg.getHeight());
            gsCamFrame = camFrame;

        }

}

int camera::getWidth()
{
        return width;
}

int camera::getHeight()
{
        return height;
}

void camera::drawFrame()
{
        gsCamFrame.draw(0,0);
}

ofxCvGrayscaleImage* camera::getFrame()
{
        return &gsCamFrame;
}


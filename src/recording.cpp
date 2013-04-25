#include "recording.h"

recording::recording(){
}

void recording::init(string framespath)
{
	cout << framespath << endl;
        ofDirectory dir(framespath);

        dir.listDir();
	dir.sort();

        numFrames = dir.numFiles();
        cout << numFrames << endl;

        frameIndex = 0;

        if(numFrames >0)
        {
            ofImage inFrame;
            inFrame.loadImage(dir.getPath(0));
            width = inFrame.getWidth();
            height = inFrame.getHeight();

            frame.allocate(width,height);
            ofxCvColorImage cFrame;
            cFrame.allocate(width,height);
            cFrame.setFromPixels(inFrame.getPixels(),width,height);
            frame = cFrame;

            frameCache.push_back(frame);

            cout << "Loading " << numFrames << " frames from " << framespath << "...";
            //go through and print out all the paths
            for(int i = 1; i < numFrames; i++)
            {
                inFrame.loadImage(dir.getPath(i));
                cFrame.setFromPixels(inFrame.getPixels(),width,height);
                frame = cFrame;
                frameCache.push_back(frame);
            }
            it = frameCache.begin();


            cout << "   done!" << endl;

        }
        else
        {
            this->width = 0;
            this->height = 0;
            frame.allocate(width,height);
            frameCache.push_back(frame);

        }

}

void recording::update()
{

        if(it==frameCache.end())
        {
            it = frameCache.begin();
            frameIndex = 0;
        }
        frame = *(it);
        it++;
        frameIndex++;
}



int recording::getWidth()
{
        return width;
}

int recording::getHeight()
{
        return height;
}

void recording::drawFrame(int x, int y, int w, int h)
{
        frame.draw(x,y,w,h);
}

unsigned char* recording::getPixels()
{
        return frame.getPixels();
}

ofxCvGrayscaleImage* recording::getFrame()
{
        return &frame;
}


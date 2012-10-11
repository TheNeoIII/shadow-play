/*
* shadowPlay.h
* by btparker
*
* 
* 
* 
* 
*
*/


#pragma once
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "camera.h"
#include "ofxFensterManager.h"
#include "ofxXmlSettings.h"


#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768

enum { CALIBRATION, REAL_SHADOW, NO_SHADOW, COMPUTED_SHADOW, RECORDED_SHADOW, TRACKED_RECORDED_SHADOW};
enum {CONTROL_WINDOW, WINDOW_A,WINDOW_B};

enum { MOUSE_NONE};//, MOUSE_MOVED, MOUSE_DRAGGED, MOUSE_PRESSED, MOUSE_RELEASED};

/** BEGIN CHILD WINDOW **/
class childWindow
{
public:
  childWindow(string title, int id, int width, int height){
      win = ofxFensterManager::get()->createFenster(0, 0, width, height, OF_WINDOW);
      
      win->setWindowTitle(title);
      
      this->id = id;
      
      buffer.allocate(width, height);
      

      corners[0].x = 0.0;
      corners[0].y = 0.0;
      
      corners[1].x = 1.0;
      corners[1].y = 0.0;
      
      corners[2].x = 1.0;
      corners[2].y = 1.0;
      
      corners[3].x = 0.0;
      corners[3].y = 1.0;
            
  }
  
  void setWarpPoints(ofPoint* pts){
    corners[0] = pts[0];
    corners[1] = pts[1];
    corners[2] = pts[2];
    corners[3] = pts[3];
  }
  
  ofPoint* getWarpPoints(){
    ofPoint* pts;
    pts[0] = corners[0];
    pts[1] = corners[1];
    pts[2] = corners[2];
    pts[3] = corners[3];
    return pts;
  }
  
  ofFbo* getBuffer()
    {
      return &buffer;
    }
  
  ofxFenster* getWin()
    {
        return win;
    }
  
  ofPoint* getCorners(){
    return corners;
  }

  void draw(){
    win->draw();
  }

  void drawBoundingBox()
    {
        ofNoFill();
        ofSetHexColor(0xFF00FF);
        ofRect(1, 1, DISPLAY_WIDTH-2, DISPLAY_HEIGHT-2);
        ofSetHexColor(0xFFFFFF);
    }

    void bumpCorner(int x, int y)
    {
        float scaleX = (float)x / DISPLAY_WIDTH;
        float scaleY = (float)y / DISPLAY_HEIGHT;
        if(whichCorner >= 0)
        {
            corners[whichCorner].x += scaleX;
            corners[whichCorner].y += scaleY;
        }
    }

    void dragCorner(int x, int y)
    {

        float scaleX = (float)x / DISPLAY_WIDTH;
        float scaleY = (float)y / DISPLAY_HEIGHT;

        if(whichCorner >= 0)
        {
            corners[whichCorner].x = scaleX;
            corners[whichCorner].y = scaleY;
        }
    }

    void selectCorner(int x, int y)
    {
        float smallestDist = 1.0;
        whichCorner = -1;

        for(int i = 0; i < 4; i++)
        {
            float distx = corners[i].x - (float)x/DISPLAY_WIDTH;
            float disty = corners[i].y - (float)y/DISPLAY_HEIGHT;
            float dist  = sqrt( distx * distx + disty * disty);

            if(dist < smallestDist && dist < 0.1)
            {
                whichCorner = i;
                smallestDist = dist;
            }
        }
    }

    void windowDistort()
    {
        //lets make a matrix for openGL
        //this will be the matrix that peforms the transformation
        GLfloat myMatrix[16];

        //we set it to the default - 0 translation
        //and 1.0 scale for x y z and w
        for(int i = 0; i < 16; i++)
        {
            if(i % 5 != 0) myMatrix[i] = 0.0;
            else myMatrix[i] = 1.0;
        }

        //we need our points as opencv points
        //be nice to do this without opencv?
        CvPoint2D32f cvsrc[4];
        CvPoint2D32f cvdst[4];

        //we set the warp coordinates
        //source coordinates as the dimensions of our window
        cvsrc[0].x = 0;
        cvsrc[0].y = 0;
        cvsrc[1].x = DISPLAY_WIDTH;
        cvsrc[1].y = 0;
        cvsrc[2].x = DISPLAY_WIDTH;
        cvsrc[2].y = DISPLAY_HEIGHT;
        cvsrc[3].x = 0;
        cvsrc[3].y = DISPLAY_HEIGHT;

        //corners are in 0.0 - 1.0 range
        //so we scale up so that they are at the window's scale
        for(int i = 0; i < 4; i++)
        {
            cvdst[i].x = corners[i].x  * (float)DISPLAY_WIDTH;
            cvdst[i].y = corners[i].y * (float)DISPLAY_HEIGHT;
        }

        //we create a matrix that will store the results
        //from openCV - this is a 3x3 2D matrix that is
        //row ordered
        CvMat * translate = cvCreateMat(3,3,CV_32FC1);

        //this is the slightly easier - but supposidly less
        //accurate warping method
        //cvWarpPerspectiveQMatrix(cvsrc, cvdst, translate);


        //for the more accurate method we need to create
        //a couple of matrixes that just act as containers
        //to store our points  - the nice thing with this
        //method is you can give it more than four points!

        CvMat* src_mat = cvCreateMat( 4, 2, CV_32FC1 );
        CvMat* dst_mat = cvCreateMat( 4, 2, CV_32FC1 );

        //copy our points into the matrixes
        cvSetData( src_mat, cvsrc, sizeof(CvPoint2D32f));
        cvSetData( dst_mat, cvdst, sizeof(CvPoint2D32f));

        //figure out the warping!
        //warning - older versions of openCV had a bug
        //in this function.
        cvFindHomography(src_mat, dst_mat, translate);

        //get the matrix as a list of floats
        float *matrix = translate->data.fl;


        //we need to copy these values
        //from the 3x3 2D openCV matrix which is row ordered
        //
        // ie:   [0][1][2] x
        //       [3][4][5] y
        //       [6][7][8] w

        //to openGL's 4x4 3D column ordered matrix
        //        x  y  z  w
        // ie:   [0][3][ ][6]
        //       [1][4][ ][7]
        //		 [ ][ ][ ][ ]
        //       [2][5][ ][9]
        //

        myMatrix[0]		= matrix[0];
        myMatrix[4]		= matrix[1];
        myMatrix[12]	= matrix[2];

        myMatrix[1]		= matrix[3];
        myMatrix[5]		= matrix[4];
        myMatrix[13]	= matrix[5];

        myMatrix[3]		= matrix[6];
        myMatrix[7]		= matrix[7];
        myMatrix[15]	= matrix[8];


        //finally lets multiply our matrix
        //wooooo hoooo!
        glMultMatrixf(myMatrix);
    }



    ofxFenster* win;
    int id;
    ofFbo buffer;

    ofPoint corners[4];
    int whichCorner;


};





/** END CHILD WINDOW **/

class shadowPlay : public ofBaseApp{
	public:
  
		childWindow* winA;
        childWindow* winB;
        
        ofPoint defaultWarpCoords[4];

        ofPoint cornersA[4];
        ofPoint cornersB[4];

		camera* cam;

        ofxXmlSettings settings;

		ofImage calibImageA;
		ofImage calibImageB;

		ofImage backgroundFrame;

		int outFrame;

		ofxCvGrayscaleImage frame;
		ofxCvGrayscaleImage maskA;
		ofxCvGrayscaleImage maskB;
		ofxCvGrayscaleImage diff;
		ofxCvGrayscaleImage diffBg;
		ofxCvGrayscaleImage dilate;
		ofxCvGrayscaleImage blur;
		ofxCvGrayscaleImage thresh;
		ofxCvGrayscaleImage computedShadow;
		ofxCvGrayscaleImage shadow;
		ofxCvGrayscaleImage recordedShadowThresh;
		ofxCvGrayscaleImage recordedShadowFrame;

		ofxCvContourFinder shadowCF;
		ofxCvContourFinder recordedShadowCF;

		ofVec2f shadowPos;

		bool isRecording;

		recording recordedShadow;

		ofImage out;
		ofxCvColorImage outTemp;

		int state;
        
        bool testMode;

		int thresholdValue;
		int dilateValue;
		int blurValue;

		int focus;

		float scaleFactor;

        bool trackingShadow;


		shadowPlay();
		void setup();
        void loadSettings();
        void saveSettings();
        void assignDefaultSettings();
        ofPoint* loadPoints(string s);
        void savePoints(string s, childWindow*);
		void update();
		void draw();

		void mousePressed(int x, int y, int button);
		void mouseDragged(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseMoved(int x, int y);
		void keyPressed(int key);

		void drawFrameInverse();
		void drawFrame();
		void drawShadow();

		int getState();

		void drawCalibrationFrames();
		void drawNoShadow();
		void drawRealShadow();
		void drawComputedShadow();
		void drawRecordedShadow();

		void setFocus(int id);
		int getFocus();

		void setState(int state);

		void generateMask();
		void generateComputedShadow();

		void trackShadow();
		void recordShadow();
		void recordShadow(ofxCvGrayscaleImage im,string outputname, bool numberframes);


		void exit();

};

class childWindowListener: public ofxFensterListener
{
public:

    childWindowListener(shadowPlay* mw, ofFbo* buffer, int id)
    {
        this->buffer = buffer;
        this->mw = mw;
        this->id = id;
    }


    void draw()
    {
        buffer->draw(0,0);
    }

    //--------------------------------------------------------------
    void mouseDragged(int x, int y, int button)
    {
      if(button == 0){
        mw->setFocus(id);
        mw->mouseDragged(x,y,button);
      }
    }

    //--------------------------------------------------------------
    void mousePressed(int x, int y, int button)
    {
        mw->setFocus(id);
        mw->mousePressed(x,y,button);
    }

    //--------------------------------------------------------------
    void mouseReleased(int x, int y, int button)
    {
      mw->setFocus(id);
        mw->mouseReleased(x,y,button);
    }

    void keyReleased(int key)
    {
      //mw->setFocus(id);
        mw->keyReleased(key);
    }

    void keyPressed(int key)
    {
      //mw->setFocus(id);
        mw->keyPressed(key);
    }


    shadowPlay* mw;

    ofFbo* buffer;
    int id;
};

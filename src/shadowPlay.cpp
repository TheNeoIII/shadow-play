#include "shadowPlay.h"
shadowPlay::shadowPlay(){
}

//--------------------------------------------------------------------------------
void shadowPlay::setup(){

  loadSettings();

  /* Creating two window views (one for each projector) */
  winA = new childWindow("Window A",WINDOW_A, DISPLAY_WIDTH, DISPLAY_HEIGHT);//,cornersA);
  winA->getWin()->addListener(new childWindowListener(this,winA->getBuffer(),WINDOW_A));
  winB = new childWindow("Window B",WINDOW_B, DISPLAY_WIDTH, DISPLAY_HEIGHT);//,cornersB);
  winB->getWin()->addListener(new childWindowListener(this,winB->getBuffer(),WINDOW_B));
  
  /* Setting up to receive the camera input (pass 'true' for a sequence of test frames) */
  cam = new camera(true);
  cam->update();
  
  /* Loading helper images */  
  calibImageA.loadImage("calibrationImageA.jpg");
  calibImageA.setImageType(OF_IMAGE_COLOR);
  
  calibImageB.loadImage("calibrationImageB.jpg");
  calibImageB.setImageType(OF_IMAGE_COLOR);
  
  backgroundFrame.loadImage("bg_birch.jpg");
  backgroundFrame.setImageType(OF_IMAGE_COLOR);
  
  /* Setting up intermediary graphics buffers for creating the masks */
  frame.allocate(cam->getWidth(),cam->getHeight());
  diffBg.allocate(cam->getWidth(),cam->getHeight());
  diff.allocate(cam->getWidth(), cam->getHeight());
  dilate.allocate(cam->getWidth(), cam->getHeight());
  blur.allocate(cam->getWidth(), cam->getHeight());
  thresh.allocate(cam->getWidth(), cam->getHeight());
  maskA.allocate(cam->getWidth(), cam->getHeight());
  maskB.allocate(cam->getWidth(), cam->getHeight());

  computedShadow.allocate(cam->getWidth(), cam->getHeight());
  outTemp.allocate(cam->getWidth(), cam->getHeight());
  shadow.allocate(cam->getWidth(), cam->getHeight());
  recordedShadowFrame.allocate(cam->getWidth(), cam->getHeight());
  recordedShadowThresh.allocate(cam->getWidth(), cam->getHeight());
  
  scaleFactor = DISPLAY_WIDTH/cam->getWidth();
  
  recordedShadow.init("recordings/camera_test/");
  isRecording = false;

  focus = CONTROL_WINDOW;  

  diffBg = *(cam->getFrame());
}

//--------------------------------------------------------------------------------
void shadowPlay::loadSettings(){
  state = CALIBRATION;
  testMode = true;
  thresholdValue = 40;
  dilateValue = 5;
  blurValue = 5;
  /**
  cornersA[0].x = 0.0;
  cornersA[0].y = 0.0;
  
  cornersA[1].x = 1.0;
  cornersA[1].y = 0.0;
  
  cornersA[2].x = 1.0;
  cornersA[2].y = 1.0;
  
  cornersA[3].x = 0.0;
  cornersA[3].y = 1.0;
  
  
  cornersB[0].x = cornersA[0].x;
  cornersB[0].y = cornersA[0].y;
  
  cornersB[1].x = cornersA[1].x; 
  cornersB[1].y = cornersA[1].y;
  
  cornersB[2].x = cornersA[2].x;
  cornersB[2].y = cornersA[2].y;
  
  cornersB[3].x = cornersA[3].x;
  cornersB[3].y = cornersA[3].y;
  **/
  
  if(settings.loadFile("settings.xml")){
    state = settings.getValue("settings:startState",state);
    testMode = settings.getValue("settings:testMode",testMode);
    thresholdValue = settings.getValue("settings:thresholdValue",thresholdValue);
    dilateValue = settings.getValue("settings:dilateValue",dilateValue);
    blurValue = settings.getValue("settings:blurValue",blurValue);
    /**

    cornersA[0].x = settings.getValue("settings:cA1x",cornersA[0].x);
    cornersA[0].y = settings.getValue("settings:cA1y",cornersA[0].y);
    
    cornersA[1].x = settings.getValue("settings:cA2x",cornersA[1].x);
    cornersA[1].y = settings.getValue("settings:cA2y",cornersA[1].y);
    
    cornersA[2].x = settings.getValue("settings:cA3x",cornersA[2].x);
    cornersA[2].y = settings.getValue("settings:cA3y",cornersA[2].y);
    
    cornersA[3].x = settings.getValue("settings:cA4x",cornersA[3].x);
    cornersA[3].y = settings.getValue("settings:cA4y",cornersA[3].y);
    
    cornersB[0].x = settings.getValue("settings:cB1x",cornersB[0].x);
    cornersB[0].y = settings.getValue("settings:cB1y",cornersB[0].y);
    
    cornersB[1].x = settings.getValue("settings:cB2x",cornersB[1].x);
    cornersB[1].y = settings.getValue("settings:cB2y",cornersB[1].y);
    
    cornersB[2].x = settings.getValue("settings:cB3x",cornersB[2].x);
    cornersB[2].y = settings.getValue("settings:cB3y",cornersB[2].y);
    
    cornersB[3].x = settings.getValue("settings:cB4x",cornersB[3].x);
    cornersB[3].y = settings.getValue("settings:cB4y",cornersB[3].y);
    **/
  }
}

//--------------------------------------------------------------------------------
void shadowPlay::setFocus(int id)
{
  focus = id;
}

//--------------------------------------------------------------------------------
int shadowPlay::getFocus()
{
  return focus;
}


//--------------------------------------------------------------
void shadowPlay::update()
{
  cam->update();
  if(state==RECORDED_SHADOW || state == TRACKED_RECORDED_SHADOW)
    {
      recordedShadow.update();
      trackShadow();
    }
}

//--------------------------------------------------------------------------------
void shadowPlay::generateMask()
{
  frame = *(cam->getFrame());
  
  
  diff = diffBg;
  diff.absDiff(frame);
  
  shadow = diff;
  
  thresh = diff;
  thresh.threshold(thresholdValue);
    
  dilate = thresh;
  
  for(int i = 0; i < dilateValue; i++)
    {
      dilate.dilate();
    }
  
  blur = dilate;

  blur.blur(blurValue);
  blur.blurGaussian();
  
  
  maskA = blur;
  maskB = maskA;
  maskB.invert();
}

//--------------------------------------------------------------------------------
void shadowPlay::setState(int state)
{
  this->state = state;
}

//--------------------------------------------------------------------------------
int shadowPlay::getState()
{
  return state;
}

//--------------------------------------------------------------------------------
void shadowPlay::trackShadow()
{
  int minArea = 20;
  int maxArea = (int)((computedShadow.width)*(computedShadow.height));
  shadowCF.findContours(thresh,minArea,maxArea,1,false);
  recordedShadowCF.findContours(recordedShadowThresh,minArea,maxArea,1,false);
  
  bool shadowVisible = shadowCF.nBlobs > 0;
  bool recShadowVisible = recordedShadowCF.nBlobs > 0;
  
  if(shadowVisible && recShadowVisible)
    {
      shadowPos = shadowCF.blobs[0].centroid-recordedShadowCF.blobs[0].centroid;
    }
  
}

//--------------------------------------------------------------------------------
void shadowPlay::generateComputedShadow()
{
  computedShadow = shadow;
  computedShadow.invert();
  computedShadow *= computedShadow;
  computedShadow *= computedShadow;
  computedShadow.blurGaussian(1);
  computedShadow += computedShadow;
  
  
}

//--------------------------------------------------------------------------------
void shadowPlay::recordShadow()
{
  recordShadow(frame,"frame",true);
}

//--------------------------------------------------------------------------------
void shadowPlay::recordShadow(ofxCvGrayscaleImage im, string outputname, bool numberframes)
{
  outTemp = im;//computedShadow;
  out.setFromPixels(outTemp.getPixelsRef());
  
  stringstream ss;
  ss  << "recordings/rec/";
  ss << outputname;
  if(numberframes)
    {
      
      ss << "_";
      ss << setfill('0') << setw(6);
      ss << outFrame;
    }
  
  ss << ".bmp";
  
  out.saveImage(ss.str());
  outFrame++;
}

//--------------------------------------------------------------
void shadowPlay::draw()
{
  generateMask();
  generateComputedShadow();
  
  
  if(isRecording)
    {
      recordShadow();
      /**
         recordShadow(diff,"diff",false);
         recordShadow(shadow,"shadow",false);
         recordShadow(thresh,"thresh",false);
         recordShadow(dilate,"dilate",false);
         recordShadow(blur,"blur",false);
         recordShadow(maskA,"maskA",false);
         recordShadow(maskB,"maskB",false);
      **/
    }
  

  ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate()),500,20);

  ofDrawBitmapString("threshold value ([,]): " + ofToString(thresholdValue),10,40);
  thresh.draw(10,60);
  ofDrawBitmapString("dilate value (;,'): " + ofToString(dilateValue),250,40);
  dilate.draw(250,60);
  ofDrawBitmapString("blur value (-,=): " + ofToString(blurValue),500,40);
  blur.draw(500,60);
  frame.draw(10,310);
  backgroundFrame.draw(330,310,320,240);

  switch(state)
    {
    case CALIBRATION:
      ofDrawBitmapString("state: CALIBRATION",10,20);
      drawCalibrationFrames();
      break;
    case REAL_SHADOW:
      ofDrawBitmapString("state: REAL_SHADOW",10,20);
      drawRealShadow();
      break;
    case NO_SHADOW:
      ofDrawBitmapString("state: NO_SHADOW",10,20);
      drawNoShadow();
      break;
    case COMPUTED_SHADOW:
      ofDrawBitmapString("state: COMPUTED_SHADOW",10,20);
      drawComputedShadow();
      break;
    case RECORDED_SHADOW:
      ofDrawBitmapString("state: RECORDED_SHADOW",10,20);
      recordedShadowFrame = *(recordedShadow.getFrame());
      recordedShadowThresh = recordedShadowFrame;
      recordedShadowThresh.invert();
      recordedShadowThresh.threshold(thresholdValue);
      drawRecordedShadow();
      break;
    case TRACKED_RECORDED_SHADOW:
      ofDrawBitmapString("state: TRACKED_RECORDED_SHADOW",10,20);
      recordedShadowFrame = *(recordedShadow.getFrame());
      recordedShadowThresh = recordedShadowFrame;
      recordedShadowThresh.invert();
      recordedShadowThresh.threshold(thresholdValue);
      trackShadow();
      drawRecordedShadow();
      break;
    }
  
  
  
}

//--------------------------------------------------------------------------------
void shadowPlay::drawCalibrationFrames()
{
  // Draw Window A
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  calibImageA.draw(0,0);
  if(focus == WINDOW_A)
    {
      winA->drawBoundingBox();
    }
  
  winA->getBuffer()->end();
  
  //Draw Window B
  winB->draw();
  
  winB->getBuffer()->begin();
  ofClear(0,0,0);
  winB->windowDistort();
  calibImageB.draw(0,0);
  if(focus == WINDOW_B)
    {
      winB->drawBoundingBox();
    }
  winB->getBuffer()->end();
}

//--------------------------------------------------------------------------------
void shadowPlay::drawRealShadow()
{
  // Draw Window A
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  winA->getBuffer()->end();
  
  //Draw Window B
  winB->draw();
  
  winB->getBuffer()->begin();
  ofClear(0,0,0);
  winB->windowDistort();
  winB->getBuffer()->end();
}

//--------------------------------------------------------------------------------
void shadowPlay::drawNoShadow()
{
  
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  maskA.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  
  winA->getBuffer()->end();
  
  //Draw Window B
  winB->draw();
  
  winB->getBuffer()->begin();
  ofClear(0,0,0);
  winB->windowDistort();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  maskB.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  winB->getBuffer()->end();
}

//--------------------------------------------------------------------------------
void shadowPlay::drawComputedShadow()
{
  
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  maskA.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  computedShadow.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  
  winA->getBuffer()->end();
  
  //Draw Window B
  winB->draw();
  
  winB->getBuffer()->begin();
  ofClear(0,0,0);
  winB->windowDistort();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  maskB.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  computedShadow.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  winB->getBuffer()->end();
}

//--------------------------------------------------------------------------------
void shadowPlay::drawRecordedShadow()
{
  
  // Draw Window A
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  maskA.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  recordedShadowFrame.draw(shadowPos.x*scaleFactor,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  
  winA->getBuffer()->end();
  
  //Draw Window B
  winB->draw();
  
  winB->getBuffer()->begin();
  ofClear(0,0,0);
  winB->windowDistort();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  maskB.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  
  recordedShadowFrame.draw(shadowPos.x*scaleFactor,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  winB->getBuffer()->end();
}

void saveSettings(){
  
}

//--------------------------------------------------------------------------------
void shadowPlay::keyPressed(int key)
{
  switch(key)
    {
    case 'd':
      diffBg = frame;
      recordShadow(diffBg,"diffBg",false);
      break;
    case 'f':
      ofxFensterManager::get()->getWindowById(WINDOW_A)->toggleFullscreen();
      ofxFensterManager::get()->getWindowById(WINDOW_B)->toggleFullscreen();
      break;
    case 'r':
      isRecording = !isRecording;
      outFrame = 0;
      break;
    case '1':
      setState(CALIBRATION);
      break;
    case '2':
      setState(REAL_SHADOW);
      break;
    case '3':
      setState(NO_SHADOW);
      break;
    case '4':
      setState(COMPUTED_SHADOW);
      break;
    case '5':
      setState(RECORDED_SHADOW);
      break;
    case '6':
      setState(TRACKED_RECORDED_SHADOW);
      break;
    case '[':
      if(thresholdValue>0)
        {
          thresholdValue--;
        }
      break;
    case ']':
      thresholdValue++;
      break;
    case ';':
      if(dilateValue>0)
        {
          dilateValue--;
        }
      break;
    case '\'':
      dilateValue++;
      break;
    case '-':
      if(blurValue>0)
        {
          blurValue--;
        }
      break;
    case '=':
      blurValue++;
      break;
    case 269:
      if(focus == WINDOW_A)
        {
          winA->bumpCorner(0,-1);
        }
      else if(focus == WINDOW_B)
        {
          winB->bumpCorner(0,-1);
        }
      break;
    case 267:
      if(focus == WINDOW_A)
        {
          winA->bumpCorner(-1,0);
        }
      else if(focus == WINDOW_B)
        {
          winB->bumpCorner(-1,0);
        }
      break;
    case 270:
      if(focus == WINDOW_A)
        {
          winA->bumpCorner(0,1);
        }
      else if(focus == WINDOW_B)
        {
          winB->bumpCorner(0,1);
        }
      break;
    case 268:
      if(focus == WINDOW_A)
        {
          winA->bumpCorner(1,0);
        }
      else if(focus == WINDOW_B)
        {
          winB->bumpCorner(1,0);
        }
      break;
      
    }
}

//--------------------------------------------------------------------------------
void shadowPlay::mouseMoved(int x, int y)
{
}

//--------------------------------------------------------------
void shadowPlay::mouseDragged(int x, int y, int button)
{
  if(state==CALIBRATION)
    {
      switch(focus)
        {
        case WINDOW_A:
          winA->dragCorner(x,y);
          break;
        case WINDOW_B:
          winB->dragCorner(x,y);
          break;
        }
    }
  
}

//--------------------------------------------------------------
void shadowPlay::mouseReleased(int x, int y, int button)
{
}

//--------------------------------------------------------------
void shadowPlay::mousePressed(int x, int y, int button)
{
  if(state==CALIBRATION)
    {
      switch(focus)
        {
        case WINDOW_A:
          winA->selectCorner(x,y);
          break;
        case WINDOW_B:
          winB->selectCorner(x,y);
          break;
        }
    }
}

//--------------------------------------------------------------------------------
void shadowPlay::exit()
{
}

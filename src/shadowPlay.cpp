#include "shadowPlay.h"
shadowPlay::shadowPlay(){
}

//--------------------------------------------------------------------------------
void shadowPlay::setup(){
  /* Creating two window views (one for each projector) */
  winA = new childWindow("Window A",WINDOW_A, DISPLAY_WIDTH, DISPLAY_HEIGHT);//,cornersA);
  winA->getWin()->addListener(new childWindowListener(this,winA->getBuffer(),WINDOW_A));
  winB = new childWindow("Window B",WINDOW_B, DISPLAY_WIDTH, DISPLAY_HEIGHT);//,cornersB);
  winB->getWin()->addListener(new childWindowListener(this,winB->getBuffer(),WINDOW_B));
  
  loadSettings();

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

void shadowPlay::assignDefaultSettings(){
  /* Defaults for parameters */
  state = CALIBRATION;
  testMode = true;
  trackingShadow = true;
  thresholdValue = 40;
  dilateValue = 5;
  blurValue = 5;

  defaultWarpCoords[0].x = 0.0;
  defaultWarpCoords[0].y = 0.0;
  
  defaultWarpCoords[1].x = 1.0;
  defaultWarpCoords[1].y = 0.0;
  
  defaultWarpCoords[2].x = 1.0;
  defaultWarpCoords[2].y = 1.0;
  
  defaultWarpCoords[3].x = 0.0;
  defaultWarpCoords[3].y = 1.0;

  winA->setWarpPoints(defaultWarpCoords);
  winB->setWarpPoints(defaultWarpCoords);

}

//--------------------------------------------------------------------------------
void shadowPlay::loadSettings(){
  assignDefaultSettings();
  
  /* Load settings file */
  if(settings.loadFile("settings.xml")){
    settings.pushTag("settings");
    cout << "*** Loading settings file ***" << endl;
    state = settings.getValue("state",state);
    cout << "state " << state << endl;
    testMode = settings.getValue("testMode",testMode);
    cout << "testMode " << testMode << endl;
    thresholdValue = settings.getValue("thresholdValue",thresholdValue);
    cout << "thresholdValue " << thresholdValue << endl;
    dilateValue = settings.getValue("dilateValue",dilateValue);
    cout << "dilateValue " << dilateValue << endl;
    blurValue = settings.getValue("blurValue",blurValue);
    cout << "blurValue " << blurValue << endl;
    winA->setWarpPoints(loadPoints("warpPtsA"));
    winB->setWarpPoints(loadPoints("warpPtsB"));
    cout << "*****************************" << endl;
    settings.popTag();
  }
  else{
    cout << "No settings file to load." << endl;
  }
}

ofPoint* shadowPlay::loadPoints(string s){
  ofPoint pts[4];
  settings.pushTag(s);
  cout << s << ":" << endl;
  for(int i = 0; i < 4; i++){
    settings.pushTag("warpPt",i);
    
    stringstream ss;
    ss << s;
    ss << i;

    pts[i] = defaultWarpCoords[i];
    pts[i].x = settings.getValue("X", pts[i].x);
    pts[i].y = settings.getValue("Y", pts[i].y); 

    cout << "pts[" << i << "]: " << pts[i] << endl;
    settings.popTag();
    
  }
  settings.popTag();

  return pts;
}

void shadowPlay::savePoints(string s, childWindow* cw){
  ofPoint* pts;
  pts = cw->getCorners();
  settings.addTag(s);
  settings.pushTag(s);
  cout << s << ":" << endl;
  for(int i = 0; i < 4; i++){
    settings.addTag("warpPt");
    settings.pushTag("warpPt",i);
    
    stringstream ss;
    ss << s;
    ss << i;

    settings.setValue("X", pts[i].x);
    settings.setValue("Y", pts[i].y); 

    cout << "pts[" << i << "]: " << defaultWarpCoords[i] << endl;
    settings.popTag();
    
  }
  settings.popTag();


}

//--------------------------------------------------------------------------------
void shadowPlay::saveSettings(){
  settings.clear();
  settings.addTag("settings");
  settings.pushTag("settings");

  cout << "*** Saving settings to file ***" << endl;

  settings.setValue("state",state);
  cout << "state " << state << endl;

  settings.setValue("testMode",testMode);
  cout << "testMode " << testMode << endl;

  settings.setValue("thresholdValue",thresholdValue);
  cout << "thresholdValue " << thresholdValue << endl;

  settings.setValue("dilateValue",dilateValue);
  cout << "thresholdValue " << thresholdValue << endl;

  settings.setValue("blurValue",blurValue);
  cout << "blurValue " << blurValue << endl;

  savePoints("warpPtsA",winA);

  savePoints("warpPtsB",winB);

  settings.popTag();

  settings.saveFile("settings.xml");
  cout << "*** Saved to 'settings.xml' ***" << endl;
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
  if(state==RECORDED_SHADOW || state == TRACKED_RECORDED_SHADOW){
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
  
  for(int i = 0; i < dilateValue; i++){
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
  
  if(shadowVisible && recShadowVisible){
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
  if(numberframes){
    
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
  
  
  if(isRecording){
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
  
  
  ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate()),330,20);
  
  ofDrawBitmapString("thresh value ( [ , ] ): " + ofToString(thresholdValue),10,40);
  thresh.draw(10,60);
  ofDrawBitmapString("dilate value ( ; , ' ): " + ofToString(dilateValue),330,40);
  dilate.draw(330,60);
  ofDrawBitmapString("blur value ( - , = ): " + ofToString(blurValue),660,40);
  blur.draw(660,60);
  frame.draw(10,310);
  backgroundFrame.draw(330,310,320,240);


  if(RECORDED_SHADOW || TRACKED_RECORDED_SHADOW){
    recordedShadowFrame = *(recordedShadow.getFrame());
    recordedShadowThresh = recordedShadowFrame;
    recordedShadowThresh.invert();
    recordedShadowThresh.threshold(thresholdValue);
  }

  if(trackingShadow){
    ofDrawBitmapString("tracking shadow (t): true",660,20);
  }
  else{
    ofDrawBitmapString("tracking shadow (t): false",660,20);
  }

  
  switch(state){
  case CALIBRATION:
    ofDrawBitmapString("state ( 1 - 5 ): CALIBRATION (1)",10,20);
    drawCalibrationFrames();
    break;
  case REAL_SHADOW:
    ofDrawBitmapString("state ( 1 - 5 ): REAL_SHADOW (2)",10,20);
    drawRealShadow();
    break;
  case NO_SHADOW:
    ofDrawBitmapString("state ( 1 - 5 ): NO_SHADOW (3)",10,20);
    drawNoShadow();
    break;
  case COMPUTED_SHADOW:
    ofDrawBitmapString("state ( 1 - 5 ): COMPUTED_SHADOW (4)",10,20);
    drawComputedShadow();
    break;
  case RECORDED_SHADOW:
    ofDrawBitmapString("state ( 1 - 5 ): RECORDED_SHADOW (5)",10,20);
    if(trackingShadow){
      trackShadow();
    }
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


//--------------------------------------------------------------------------------
void shadowPlay::keyPressed(int key)
{
  switch(key){
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
  case 's':
    saveSettings();
    break;
  case 'c':
    assignDefaultSettings();
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
  case '[':
    if(thresholdValue>0){
      thresholdValue--;
    }
    break;
  case ']':
    thresholdValue++;
    break;
  case ';':
    if(dilateValue>0){
      dilateValue--;
    }
    break;
  case '\'':
    dilateValue++;
    break;
  case '-':
    if(blurValue>0 ){
      blurValue-=2;
    }
    else{
      blurValue = 0;
    }
    break;
  case '=':
    blurValue+=2;
    break;
  case 't':
    trackingShadow = !trackingShadow;
      break;
  case 269:
    if(focus == WINDOW_A){
      winA->bumpCorner(0,-1);
    }
    else if(focus == WINDOW_B){
      winB->bumpCorner(0,-1);
    }
    break;
  case 267:
    if(focus == WINDOW_A){
      winA->bumpCorner(-1,0);
    }
    else if(focus == WINDOW_B){
      winB->bumpCorner(-1,0);
    }
      break;
  case 270:
    if(focus == WINDOW_A){
      winA->bumpCorner(0,1);
    }
    else if(focus == WINDOW_B){
      winB->bumpCorner(0,1);
    }
    break;
  case 268:
    if(focus == WINDOW_A){
      winA->bumpCorner(1,0);
    }
      else if(focus == WINDOW_B){
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
  if(state==CALIBRATION){
    switch(focus){
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
  if(state==CALIBRATION){
    switch(focus){
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

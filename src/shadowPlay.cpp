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

  shadowRecDir = "recordings/rec/";
  //ofSetFrameRate(2); 
  
  loadSettings();

  /* Setting up to receive the camera input (pass 'true' for a sequence of test frames) */
  cam = new camera(true); 
  cam->update();
  
  /* Loading helper images */  
  calibImageA.loadImage("calibrationImageA.jpg");
  calibImageA.setImageType(OF_IMAGE_COLOR);
  
  calibImageB.loadImage("calibrationImageB.jpg");
  calibImageB.setImageType(OF_IMAGE_COLOR);
  
  backgroundFrame.loadImage("spooky.jpg");
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
  tween.allocate(cam->getWidth(), cam->getHeight());
  fbo.allocate(cam->getWidth(), cam->getHeight(),GL_LUMINANCE);
  performerFbo.allocate(cam->getWidth(), cam->getHeight(),GL_RGB);
  characterFbo.allocate(cam->getWidth(), cam->getHeight(),GL_RGB);

  performerMask.allocate(cam->getWidth(), cam->getHeight());
  characterMask.allocate(cam->getWidth(), cam->getHeight());


  
  computedShadow.allocate(cam->getWidth(), cam->getHeight());
  outTempC.allocate(cam->getWidth(), cam->getHeight());
  outTempG.allocate(cam->getWidth(), cam->getHeight());
  shadow.allocate(cam->getWidth(), cam->getHeight());
  recordedShadowFrame.allocate(cam->getWidth(), cam->getHeight());
  recordedShadowThresh.allocate(cam->getWidth(), cam->getHeight());
  
  scaleFactor = DISPLAY_WIDTH/cam->getWidth();
  
  recordedShadow.init(shadowRecDir);
  performerProj.init("recordings/eye2/");
  characterProj.init("recordings/eye2/");
  isRecording = false;

  performerFade = 255;
  performerFadeIn = true;
  performerLit = true;

  characterFade = 0;
  characterFadeIn = false;

  focus = CONTROL_WINDOW;  

  diffBg = *(cam->getFrame());

  shadowTransition = true;
  tweenLength = 12;
  tweenFrame = 0;
  shadowBlob = false;
  recordedShadowBlob = false;

  aColor.set(1.0,1.0,1.0);
  bColor.set(1.0,1.0,1.0);


//  //tween.set(1.0);

}

void shadowPlay::assignDefaultSettings(){
  /* Defaults for parameters */
  state = CALIBRATION;
  testMode = true;
  trackingShadow = true;
  thresholdValue = 40;
  dilateValue = 5;
  blurValue = 5;
  scale = 1.0;
  offsetX = 0;
  offsetY = 0;

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
    cout << "scale " << scale << endl;
    cout << "offsetX " << offsetX << endl;
    cout << "offsetY " << offsetY << endl;
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

  settings.setValue("scale",scale);
  cout << "scale " << scale << endl;

  settings.setValue("offsetX",offsetX);
  cout << "offsetX " << offsetX << endl;

  settings.setValue("offsetY",offsetY);
  cout << "offsetY " << offsetY << endl;

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
  performerProj.update();
  characterProj.update();
  trackShadow();
  if(state==RECORDED_SHADOW || state== TWEEN_SHADOW || state == TRACKED_RECORDED_SHADOW){
  recordedShadow.update();
  
  
  }
}

//--------------------------------------------------------------------------------
void shadowPlay::generateMask()
{
  frame = *(cam->getFrame());

  frame.transform(0, 0,0, scale,scale,offsetX,offsetY);
  
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

  //outTempG = performerMask;
  performerMask = thresh;
  //performerMask *= outTempG;
  performerMask.erode();
  performerMask.erode();
  performerMask.blur(5);
  performerMask.blurGaussian();
}

//--------------------------------------------------------------------------------
void shadowPlay::setState(int state)
{
  this->prevState = this->state;
  this->state = state;
}

//--------------------------------------------------------------------------------
int shadowPlay::getState()
{
  return state;
}

//--------------------------------------------------------------------------------
void shadowPlay::contourShadows()
{
  int minArea = 20;
  int maxArea = (int)((computedShadow.width)*(computedShadow.height));
  shadowCF.findContours(thresh,minArea,maxArea,1,false);
  recordedShadowCF.findContours(recordedShadowThresh,minArea,maxArea,1,false);

  if(shadowCF.nBlobs > 0){
    shadowBlob = true;
  }
  else{
    shadowBlob = false;
  }

  if(recordedShadowCF.nBlobs > 0){
    recordedShadowBlob = true;
  }
  else{
    recordedShadowBlob = false;
  }
}

//--------------------------------------------------------------------------------
void shadowPlay::trackShadow()
{
  
  
  bool shadowVisible = shadowCF.nBlobs > 0;
  bool recShadowVisible = recordedShadowCF.nBlobs > 0;
  if(shadowVisible){
    shadowCentroid = shadowCF.blobs[0].centroid;
  }
  if(recShadowVisible){
    recShadowCentroid = recordedShadowCF.blobs[0].centroid;
  }
    
  
  if(shadowVisible && recShadowVisible){
    
    shadowPos = shadowCentroid - recShadowCentroid;
  }
  
}



//--------------------------------------------------------------------------------
void shadowPlay::tweenShadow(bool tweenToRecording, float percent){
  ofPolyline inPL;
  ofPolyline outPL;
  ofPath tweenPath;
  tweenPath.setPolyWindingMode(OF_POLY_WINDING_NONZERO);
  
  
  
  
  if(!shadowBlob || !recordedShadowBlob){
//    tween.set(1.0);
    return;
  }

  ofxCvBlob inBlob;
  ofxCvBlob outBlob;
  if(tweenToRecording){
    inBlob = recordedShadowCF.blobs[0];
    outBlob = shadowCF.blobs[0];
  }
  else{
    outBlob = recordedShadowCF.blobs[0];
    inBlob = shadowCF.blobs[0];
  }

  inPL.addVertexes(inBlob.pts);
  outPL.addVertexes(outBlob.pts);

  int minNPts = inPL.size() < outPL.size() ? inPL.size() : outPL.size();
  inPL = inPL.getResampledByCount(minNPts);
  outPL = outPL.getResampledByCount(minNPts);
  inPL = inPL.getSmoothed(5);
  outPL = outPL.getSmoothed(5);

  ofPoint tweenPt;
  ofPoint inPt;
  ofPoint outPt;
  ofVec2f vec;
  for(int i = 0; i < minNPts; i++){
    inPt = inPL[i];
    outPt = outPL[i];

    vec = ofVec2f(inPt);
    vec.interpolate(outPt,percent);
    tweenPt = ofPoint(vec);
    
    if(i == 0) {  
        tweenPath.newSubPath();  
        tweenPath.moveTo(tweenPt);  
    } else {  
        tweenPath.lineTo( tweenPt );  
    } 
    
  }
  tweenPath.close();
  tweenPath.setFillColor(ofColor(0,0,0));
  tweenPath.setFilled(true);
    
  
  
  fbo.begin();
    ofClear(255);
    tweenPath.draw();

  fbo.end();
  
  ofPixels pixels; 
  fbo.readToPixels(pixels);
  tween.setFromPixels(pixels);
  tween.erode();
  tween.dilate();
  tween.invert();
  tween.blurGaussian(4);
  tween *= diffBg;
  tween *= tween;
  
  tween += tween;
  tween.invert();

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
  recordShadow(computedShadow,"frame",true);
}

//--------------------------------------------------------------------------------
void shadowPlay::recordShadow(ofxCvGrayscaleImage im, string outputname, bool numberframes)
{
  outTempC = im;//computedShadow;
  out.setFromPixels(outTempC.getPixelsRef());
  
  stringstream ss;
  ss  << "recordings/vid/";
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

  //ofSaveFrame();

  
  
  if(isRecording){
    recordShadow();
      //recordShadow(*(cam->getFrame()),"cam",true);
       // recordShadow(diff,"diff",true);
       // recordShadow(shadow,"shadow",true);
       // recordShadow(thresh,"thresh",true);
       // recordShadow(dilate,"dilate",true);
       // recordShadow(blur,"blur",true);
       //recordShadow(maskA,"maskA",true);
       // recordShadow(maskB,"maskB",true);
  }
  
  
  ofDrawBitmapString("fps: " + ofToString(ofGetFrameRate()),330,20);
  
  ofDrawBitmapString("thresh value ( [ , ] ): " + ofToString(thresholdValue),10,40);
  thresh.draw(10,60);
  ofDrawBitmapString("dilate value ( ; , ' ): " + ofToString(dilateValue),330,40);
  dilate.draw(330,60);
  ofDrawBitmapString("blur value ( - , = ): " + ofToString(blurValue),660,40);
  blur.draw(660,60);
  frame.draw(10,310);
  computedShadow.draw(330,310,320,240);
  backgroundFrame.draw(660,310,320,240);
  //tween.draw(10,660,320,240);


  if(RECORDED_SHADOW || TRACKED_RECORDED_SHADOW){
    recordedShadowFrame = *(recordedShadow.getFrame());
    recordedShadowThresh = recordedShadowFrame;
    recordedShadowThresh.invert();
    recordedShadowThresh.threshold(thresholdValue);
  }

  contourShadows();

  if(trackingShadow){
    ofDrawBitmapString("tracking shadow (t): true",660,20);
  }
  else{
    ofDrawBitmapString("tracking shadow (t): false",660,20);
  }

  float fade;
  performerFbo.begin();
    ofClear(1,1,1);
    fade = ofMap(performerFade,0,255,0.0,1.0);
    glBlendColor(fade,fade,fade,1);
    glEnable(GL_BLEND);

    glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);

    if(performerLit){
      ofColor(255,255,255);
      ofRect(0,0,performerFbo.getWidth(), performerFbo.getHeight());
    }
    else{
      performerProj.drawFrame(shadowCentroid.x-performerMask.getWidth()/2.0,shadowCentroid.y-performerMask.getHeight()/2.0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
    }
    
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    performerMask.draw(0,0);
    
    glDisable(GL_BLEND);
  performerFbo.end();

  if(performerFadeIn && performerFade < 255){
    performerFade += 20;
    if(performerFade >= 255){
      performerFade = 255;
    }
  }
  else if(!performerFadeIn && performerFade > 0){
    performerFade -= 20;
    if(performerFade <= 0){
      performerFade = 0;
    }
  }

  characterFbo.begin();
    ofClear(1,1,1);
    fade = ofMap(characterFade,0,255,0.0,1.0);
    glBlendColor(fade,fade,fade,1);
    glEnable(GL_BLEND);

    glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);

    characterProj.drawFrame(shadowCentroid.x-shadow.getWidth()/2.0,shadowCentroid.y-shadow.getHeight()/2.0,shadow.getWidth(),shadow.getHeight());
    
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    shadow.draw(0,0);
    glDisable(GL_BLEND);
  characterFbo.end();

  if(characterFadeIn && characterFade < 255){
    characterFade += 20;
    if(characterFade >= 255){
      characterFade = 255;
    }
  }
  else if(!characterFadeIn && characterFade > 0){
    characterFade -= 20;
    if(characterFade <= 0){
      characterFade = 0;
    }
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
  case TWEEN_SHADOW:
    ofDrawBitmapString("state : TWEEN_SHADOW",10,20);

    if(trackingShadow){
      trackShadow();
    }

    float tweenPercent = (1.0*tweenFrame)/(1.0*tweenLength);
    if(prevState == COMPUTED_SHADOW){
      tweenShadow(true, tweenPercent);  
    }
    else{
      tweenShadow(false, tweenPercent);
    }

    drawTweenShadow();
    if(tweenFrame >= tweenLength){
      tweenFrame = 0;

      if(prevState == COMPUTED_SHADOW){
        setState(RECORDED_SHADOW);  
      }
      else{
        setState(COMPUTED_SHADOW);
      }
      
    }
    else{
      tweenFrame++;
    }
    
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
  glBlendColor(aColor.x,aColor.y,aColor.z,1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
  calibImageA.draw(0,0);

  if(focus == WINDOW_A)
    {
      winA->drawBoundingBox();
    }
  glDisable(GL_BLEND);
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
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  ofClear(0,0,0);
  winA->getBuffer()->end();
  
  //Draw Window B
  winB->draw();
  
  winB->getBuffer()->begin();

  
  winB->windowDistort();
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  winB->getBuffer()->end();
}

//--------------------------------------------------------------------------------
void shadowPlay::drawNoShadow()
{
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  glBlendColor(aColor.x,aColor.y,aColor.z,1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  maskA.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  
  winA->getBuffer()->end();
  
  //Draw Window B
  winB->draw();
  
  winB->getBuffer()->begin();
  ofClear(0,0,0);
  winB->windowDistort();
  
  glEnable(GL_BLEND);
  //glBlendEquation ( GL_FUNC_ADD );
  glBlendFunc(GL_ONE, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  //glBlendEquation(GL_FUNC_SUBTRACT);

  maskB.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

  // glBlendEquation(GL_FUNC_ADD);

  //glBlendFunc(GL_ONE, GL_ONE);

  //performerFbo.draw(offsetX,offsetY,scale*DISPLAY_WIDTH,scale*DISPLAY_HEIGHT);


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
  glBlendColor(aColor.x,aColor.y,aColor.z,1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  maskA.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  computedShadow.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

  glBlendFunc(GL_ONE, GL_ONE);

  characterFbo.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

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
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  maskB.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

  glBlendFunc(GL_ONE, GL_ONE);

  performerFbo.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  winB->getBuffer()->end();
}

//--------------------------------------------------------------------------------
void shadowPlay::drawTweenShadow()
{
  // Draw Window A
  winA->draw();
  
  winA->getBuffer()->begin();
  ofClear(0,0,0);
  winA->windowDistort();
  glBlendColor(aColor.x,aColor.y,aColor.z,1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  maskA.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  tween.draw(shadowPos.x*scaleFactor,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

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
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  maskB.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  tween.draw(shadowPos.x*scaleFactor,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

  glBlendFunc(GL_ONE, GL_ONE);

  performerFbo.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

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

  glBlendColor(aColor.x,aColor.y,aColor.z,1);

  glEnable(GL_BLEND);
  glBlendFunc(GL_CONSTANT_COLOR, GL_ZERO);
  
  backgroundFrame.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  maskA.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
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
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  maskB.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glBlendFunc(GL_DST_COLOR, GL_ZERO);
  
  recordedShadowFrame.draw(shadowPos.x*scaleFactor,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);

  glBlendFunc(GL_ONE, GL_ONE);

  performerFbo.draw(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
  
  glDisable(GL_BLEND);
  winB->getBuffer()->end();
}


//--------------------------------------------------------------------------------
void shadowPlay::keyPressed(int key)
{
  float inc = 0.005;

  switch(key){
  case 'z':
    if(focus == WINDOW_A){

      aColor.y = aColor.y - inc;
      aColor.z = aColor.z - inc;

    }
    else if(focus == WINDOW_B){
      bColor.y = bColor.y - inc;
      bColor.z = bColor.z - inc;

    }
    break;
  case 'x':
    if(focus == WINDOW_A){

      aColor.y = aColor.y + inc;
      aColor.z = aColor.z + inc;

    }
    else if(focus == WINDOW_B){
      bColor.y = bColor.y + inc;
      bColor.z = bColor.z + inc;

    }
    break;
  case 'c':
    if(focus == WINDOW_A){

      aColor.x = aColor.x - inc;
      aColor.z = aColor.z - inc;

    }
    else if(focus == WINDOW_B){
      bColor.x = bColor.x - inc;
      bColor.z = bColor.z - inc;

    }
    break;
  case 'v':
    if(focus == WINDOW_A){

      aColor.x = aColor.x + inc;
      aColor.z = aColor.z + inc;

    }
    else if(focus == WINDOW_B){
      bColor.x = bColor.x + inc;
      bColor.z = bColor.z + inc;

    }
    break;
  case 'b':
    if(focus == WINDOW_A){

      aColor.y = aColor.y - inc;
      aColor.x = aColor.x - inc;

    }
    else if(focus == WINDOW_B){
      bColor.y = bColor.y - inc;
      bColor.x = bColor.x - inc;

    }
    break;
  case 'n':
    if(focus == WINDOW_A){

      aColor.y = aColor.y + inc;
      aColor.x = aColor.x + inc;

    }
    else if(focus == WINDOW_B){
      bColor.y = bColor.y + inc;
      bColor.x = bColor.x + inc;

    }
    break;

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
  case 'a':
    assignDefaultSettings();
    break;
  case 'u':
    characterFadeIn = !characterFadeIn;
    break;
  case 'o':
    performerLit = !performerLit;
    break;
  case 'p':
    performerFadeIn = !performerFadeIn;
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
    if(blurValue>=2 ){
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
  case 'i':
    if(tweenFrame == 0){
      setState(TWEEN_SHADOW);
    }
    
    break;
  case 269:
    if(state == CALIBRATION){
      if(focus == WINDOW_A){
        winA->bumpCorner(0,-1);
      }
      else if(focus == WINDOW_B){
        winB->bumpCorner(0,-1);
      }
    }
  else{
    offsetY -= 1;
  }
    break;
  case 267:
    if(state == CALIBRATION){
      if(focus == WINDOW_A){
        winA->bumpCorner(-1,0);
      }
      else if(focus == WINDOW_B){
        winB->bumpCorner(-1,0);
      }
 
    }
  else{
    offsetX -= 1;
  }
    break;
  case 270:
    if(state == CALIBRATION){
      if(focus == WINDOW_A){
        winA->bumpCorner(0,1);
      }
      else if(focus == WINDOW_B){
        winB->bumpCorner(0,1);
      }
 
    }
  else{
    offsetY += 1;
  }
    break;
  case 268:
    if(state == CALIBRATION){
      if(focus == WINDOW_A){
        winA->bumpCorner(1,0);
      }
        else if(focus == WINDOW_B){
          winB->bumpCorner(1,0);
        }
      }
    else{
      offsetX += 1;
    }
    break;
  case 'k':
    scale -= 0.01;
    break;
  case 'l':
    scale += 0.01;
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
  delete cam;
}

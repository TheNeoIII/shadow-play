#include "ofMain.h"
#include "shadowPlay.h"
#include "ofxFensterManager.h"
#include "ofAppGlutWindow.h"

//--------------------------------------------------------------
int main(){
	ofSetupOpenGL(ofxFensterManager::get(), DISPLAY_WIDTH, DISPLAY_HEIGHT, OF_WINDOW);	
	ofRunFensterApp(new shadowPlay()); // start the app
}

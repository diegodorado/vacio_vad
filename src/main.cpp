#include "ofMain.h"
#include "ofApp.h"
#include <stdio.h>

//========================================================================
int main( ){
	fprintf(stdout, "mainmainmainmain\n");

	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context
	
	fprintf(stdout, "ofSetupOpenGL\n");

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}

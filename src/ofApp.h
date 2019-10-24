#pragma once

#include "ofMain.h"
#include "ofxMaxim.h"
#include "ofxOsc.h"
#include "ofxGui.h"

#include <fvad.h>

#define HOST "localhost"
#define PORT 6448
#define FFT_SIZE 1024
//#define SAMPLE_RATE 44100
#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024
#define N_AVERAGES 12
#define N_COEFF 13
#define N_NOISE 10

class ofApp : public ofBaseApp{

	public:
    void setup();
    void update();
    void draw();
		void audioIn(ofSoundBuffer & buffer);
    void updateFbo();
    void drawOctave(int _x, int _y, int _w, int _h);
    void drawMFCC(int _x, int _y, int _w, int _h);
    void drawFeatures(int _x, int _y, int _w, int _h);
    void drawSpectrum(int _x, int _y, int _w, int _h);
    void drawSpectrogram(int _x, int _y, int _w, int _h);

    double wave;
    ofxMaxiFFT mfft;
    ofxMaxiFFTOctaveAnalyzer oct;
    ofxMaxiMFCC mfcc;
    double *mfccs;
    double *noise_mfccs;
    int noise_mfccs_index = 0;
    double *noise_avg_mfccs;
    Fvad *vad = NULL;

    ofxOscSender sender;

		ofSoundStream soundStream;
    
    int spectrogram_pos = 0;
    ofFbo fbo_spectrum;
    ofFbo fbo_spectrogram;

    string string_device_info;
    int should_update_fbos;


    //GUI STUFF
    bool bHide;
    ofxFloatSlider radius;
    ofxColorSlider color;
    ofxVec2Slider center;
    ofxIntSlider circleResolution;
    ofxToggle filled;
    ofxButton twoCircles;
    ofxButton ringButton;
    ofxLabel screenSize;
    ofxPanel gui;



};

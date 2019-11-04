#pragma once

#include "ofMain.h"
#include "ofxMaxim.h"
#include "ofxOsc.h"
#include "ofxGui.h"
#include "vad.h"

//#define HOST "localhost"
#define HOST "192.168.1.10"
//#define PORT 6448
#define PORT 9000
#define FFT_SIZE 1024
//#define SAMPLE_RATE 44100
#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024
#define N_AVERAGES 12
#define N_COEFF 13
#define N_AVG_SAMPLES 20

typedef enum Status_t{
  IDLE,
  TRANSITIONING,
  SPEAKING,
  NOT_SPEAKING,
  VANISHING
} Status_t;

class ofApp : public ofBaseApp{

	public:
    void setup();
    void update();
    void draw();
		void audioIn(ofSoundBuffer & buffer);
    void updateFbo();
    void drawMFCC(int _x, int _y, int _w, int _h);
    void drawFeatures(int _x, int _y, int _w, int _h);
    void drawSpectrum(int _x, int _y, int _w, int _h);
    void drawSpectrogram(int _x, int _y, int _w, int _h);

    void sendFloat(const char* addr, float val);
    void setStatus(Status_t st);
    void calculateVad();
    char* statusToString();
    void resetButtonPressed();
    void exit();

    double wave;
    ofxMaxiFFT mfft;
    ofxMaxiFFTOctaveAnalyzer oct;
    ofxMaxiMFCC mfcc;
    double *mfccs;
    double *prev_mfccs;
    double *avg_mfccs;
    int prev_mfccs_idx = 0;

    Status_t status = IDLE;

    uint8_t *vad_history;

    ofxOscSender sender;
    Vad vad;

		ofSoundStream soundStream;
    
    int spectrogram_pos = 0;
    ofFbo fbo_spectrum;
    ofFbo fbo_spectrogram;

    string string_device_info;
    int should_update_fbos;


    //GUI STUFF
    bool bHide;
    ofxFloatSlider inputVolume;
    float lastInputVolume = 1.0f;
    float inputVolumeGainFactor = 1.0f;
    ofxIntSlider vadMode;
    int lastVadMode = 0;

    ofxFloatSlider spectrogramBoost;

    ofxFloatSlider minSpeechTime;
    ofxFloatSlider minSilenceTime;
    ofxFloatSlider vadAttack;
    ofxFloatSlider vadRelease;
    ofxFloatSlider speechHoldTime;
    ofxFloatSlider silenceHoldTime;
    ofxFloatSlider maxSpeechTime;
    ofxFloatSlider silenceTime;
    ofxFloatSlider vanishingTime;
    ofxFloatSlider centroidDamp;
    ofxButton resetButton;


    ofxLabel labels[6];
    float vadChangedAt = 0.0f;
    float vadLevel = 0.0f;
    float listeningAt = 0.0f;
    float timeSilenced = 0.0f;
    float timeSpeaking = 0.0f;
    float speechRatio = 0.0f;
    float speechAt = 0.0f;
    float lastElapsedTimef = 0.0f;
    float changedStateAt = 0.0f;
    float centroid = 0.0f;

    ofxPanel gui;

};

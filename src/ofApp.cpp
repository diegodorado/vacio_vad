#include "ofApp.h"







static bool process_sf(SNDFILE *infile, Fvad *vad, size_t framelen, SNDFILE *outfiles[2], FILE *listfile)
{
    double *buf0 = NULL;
    int16_t *buf1 = NULL;
    int vadres, prev = -1;
    long frames[2] = {0, 0};
    long segments[2] = {0, 0};


    while (sf_read_double(infile, buf0, framelen) == (sf_count_t)framelen) {

        // Convert the read samples to int16
        for (size_t i = 0; i < framelen; i++)
            buf1[i] = buf0[i] * INT16_MAX;

        vadres = fvad_process(vad, buf1, framelen);
        if (vadres < 0) {
            fprintf(stderr, "VAD processing failed\n");
        }

        vadres = !!vadres; // make sure it is 0 or 1

        frames[vadres]++;
        if (prev != vadres) segments[vadres]++;
        prev = vadres;
    }

    printf("voice detected in %ld of %ld frames (%.2f%%)\n",
        frames[1], frames[0] + frames[1],
        frames[0] + frames[1] ?
            100.0 * ((double)frames[1] / (frames[0] + frames[1])) : 0.0);
    printf("%ld voice segments, average length %.2f frames\n",
        segments[1], segments[1] ? (double)frames[1] / segments[1] : 0.0);
    printf("%ld non-voice segments, average length %.2f frames\n",
        segments[0], segments[0] ? (double)frames[0] / segments[0] : 0.0);

}





void ofApp::setup(){

    sender.setup(HOST, PORT);

    /* some standard setup stuff*/
    ofEnableAlphaBlending();
    ofSetupScreen();
    ofBackground(0, 0, 0);
    ofSetFrameRate(60);
    ofSetVerticalSync(true);

  	ofSoundStreamSettings settings;


    auto devices = soundStream.getDeviceList(ofSoundDevice::Api::JACK);
  	//auto devices = soundStream.getMatchingDevices("default");
    settings.setInDevice(devices[0]);
  	settings.setInListener(this);
  	settings.sampleRate = SAMPLE_RATE;
  	settings.numOutputChannels = 0;
  	settings.numInputChannels = 2;
  	settings.bufferSize = BUFFER_SIZE;
  	soundStream.setup(settings);

    mfft.setup(FFT_SIZE, BUFFER_SIZE, BUFFER_SIZE);
    oct.setup(SAMPLE_RATE, FFT_SIZE/2, N_AVERAGES);

    mfccs = (double*) malloc(sizeof(double) * N_COEFF);
    noise_mfccs = (double*) malloc(sizeof(double) * N_COEFF * N_NOISE);
    noise_avg_mfccs = (double*) malloc(sizeof(double) * N_COEFF);


    //512 bins, 42 filters, 13 coeffs, min/max freq 20/20000
    mfcc.setup(BUFFER_SIZE, 42, N_COEFF, 100, 4000, SAMPLE_RATE);
    ofxMaxiSettings::setup(SAMPLE_RATE, 2, BUFFER_SIZE);

    fbo_spectrum.allocate(BUFFER_SIZE/2, BUFFER_SIZE/2);
    fbo_spectrogram.allocate(BUFFER_SIZE/2, BUFFER_SIZE/2);


    gui.setup(); // most of the time you don't need a name
	gui.add(filled.setup("fill", true));
	gui.add(radius.setup("radius", 140, 10, 300));
	gui.add(center.setup("center", ofVec2f(ofGetWidth()*.5, ofGetHeight()*.5), ofVec2f(0, 0), ofVec2f(ofGetWidth(), ofGetHeight())));
	gui.add(color.setup("color", ofColor(100, 100, 140), ofColor(0, 0), ofColor(255, 255)));
	gui.add(circleResolution.setup("circle res", 5, 3, 90));
	gui.add(twoCircles.setup("two circles"));
	gui.add(ringButton.setup("ring"));
	gui.add(screenSize.setup("screen size", ofToString(ofGetWidth())+"x"+ofToString(ofGetHeight())));

	bHide = false;


    vad = fvad_new();
    if (!vad) {
        fprintf(stderr, "out of memory\n");
    }
    if (fvad_set_sample_rate(vad, SAMPLE_RATE) < 0) {
        fprintf(stderr, "invalid sample rate: %d Hz\n", SAMPLE_RATE);
        goto fail;
    }


 if (!process_sf(in_sf, vad,
            (size_t)in_info.samplerate / 1000 * frame_ms, out_sf, list_file))
        goto fail;


}

//--------------------------------------------------------------
void ofApp::update(){
    if( should_update_fbos ){
        updateFbo();
        should_update_fbos = 0; 
    }
}


void ofApp::drawSpectrum(int _x, int _y, int _w, int _h){
    // anchor and scale, to better see above 4khz range
    ofSetColor(255);
    fbo_spectrum.draw(_x,_y, _w*2, _h);
    ofNoFill();
    ofDrawRectangle(_x,_y,_w,_h);
}


void ofApp::drawSpectrogram(int _x, int _y, int _w, int _h){
    // anchor and scale, to better see above 4khz range
    ofSetColor(255);
    fbo_spectrogram.setAnchorPercent(0,0.8);
    fbo_spectrogram.draw(_x,_y, _w, _h*5);
}


void ofApp::drawMFCC(int _x, int _y, int _w, int _h){
    ofFill();
    ofSetColor(255);
    float xinc = _w / N_COEFF / 2;
    for(int i=0; i < N_COEFF; i++) {
        float height = mfccs[i] * _h;
        ofDrawRectangle(_x + i*xinc,_y+_h/2 - height,xinc, height);
    }

    for(int i=0; i < N_COEFF; i++) {
        float height = (mfccs[i]-noise_avg_mfccs[i]) * _h;
        ofDrawRectangle(_x + _w/2 + i*xinc,_y+_h/2 - height,xinc, height);
    }

}



void ofApp::drawOctave(int _x, int _y, int _w, int _h){
    ofFill();
    ofSetColor(255);
    float xinc = ((float)_w) / oct.nAverages;
    for(int i=0; i < oct.nAverages; i++) {
        float height = oct.peaks[i] * _h*0.05;
        ofDrawRectangle(_x + i*xinc,_y+_h - height,xinc, height);
    }
    ofNoFill();
    ofDrawRectangle(_x,_y,_w,_h);
}


void ofApp::drawFeatures(int _x, int _y, int _w, int _h){
    ofFill();
    ofSetColor(255);
    float xinc = ((float)_w) / 2;

   
    float height = mfft.spectralFlatness() * _h;
    ofDrawRectangle(_x ,_y+_h - height,xinc, height);

    //	spectralCentroid
    height = mfft.spectralCentroid() * _h/20000;
    ofDrawRectangle(_x + xinc ,_y+_h - height,xinc, height);


    ofNoFill();
    ofDrawRectangle(_x,_y,_w,_h);
}





//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255, 255, 255,255);
    drawSpectrogram(0,ofGetHeight()/2,ofGetWidth()/2, ofGetHeight()/2);
    //draw spectrogram position
    ofSetColor(0, 255, 0,255);
    ofDrawRectangle(((float)spectrogram_pos/mfft.bins)*(ofGetWidth()/2),ofGetHeight()/2, 1, ofGetHeight()/2);

    drawFeatures(ofGetWidth()/2,ofGetHeight()/2,ofGetWidth()/2,ofGetHeight()/2);

    drawMFCC(0,0,ofGetWidth()/2, ofGetHeight()/2);
    drawSpectrum(ofGetWidth()/2,0,ofGetWidth()/2, ofGetHeight()/2);



    ofDrawBitmapString(ofGetFrameRate(), ofGetWidth()-100, 20);

	// auto draw?
	// should the gui control hiding?
	if(!bHide){
		//gui.draw();
	}

    //Send OSC:
    ofxOscMessage m;
    m.setAddress("/wek/inputs");
    for (int i = 0; i < N_COEFF; i++) {
        m.addFloatArg(mfccs[i]);
    }
    sender.sendMessage(m);
}



void ofApp::audioIn(ofSoundBuffer & buffer){
	for (size_t i = 0; i < buffer.getNumFrames(); i++){
        wave = buffer[i*2]*0.5 + buffer[i*2+1]*0.5;
        //Calculate the mfccs if fft buffer is full
        if (mfft.process(wave)) {
            mfft.magsToDB();
            oct.calculate(mfft.magnitudesDB);
            mfcc.mfcc(mfft.magnitudes, mfccs);
            should_update_fbos = 1;
        }
	}
}





void ofApp::updateFbo(){
    int s = mfft.bins;

    fbo_spectrum.begin();
    {
        ofClear(0);
        ofNoFill();
        ofSetColor(255, 255, 255,255);
        ofBeginShape();
        
        for( int i = 0; i < s; i++ ){
            float y = (1-mfft.magnitudes[i]*0.5)*s;
            ofVertex(i, y);
        }
        ofEndShape();
    }
    fbo_spectrum.end();


    fbo_spectrogram.begin();
    glBegin(GL_POINTS);
    for( int i = 0; i < s; i++ ){
        float p = ofMap(mfft.magnitudes[i], 0.0, 10.0, 0, 255);
        if( p > 255 )p = 255;
        if( p < 0 ) p = 0;
        ofSetColor(p);
        glVertex2f(spectrogram_pos,s-i);
    }
    glEnd();
    fbo_spectrogram.end();

    spectrogram_pos++;
    spectrogram_pos %=s;

/*
    if(noise_mfccs_index<N_NOISE){
        for( int i = 0; i < N_COEFF; i++ ){
            noise_mfccs[noise_mfccs_index*N_COEFF+i] = mfccs[i];
        }
        
        if(noise_mfccs_index==(N_NOISE-1)){
            for( int i = 0; i < N_COEFF; i++ ){
                double total = 0;    
                for( int j = 0; j < N_NOISE; j++ )
                    total += noise_mfccs[j*N_COEFF+i];
                
                noise_avg_mfccs[i] = total / N_NOISE;
            }
        }
        
        noise_mfccs_index++;
    }

*/


}

#include "ofApp.h"

void ofApp::setup(){
    sender.setup(HOST, PORT);

    /* some standard setup stuff*/
    ofEnableAlphaBlending();
    ofSetupScreen();
    ofBackground(0, 0, 0);
    ofSetFrameRate(60);
    ofSetVerticalSync(true);


  	ofSoundStreamSettings settings;

    //auto devices = soundStream.getDeviceList(ofSoundDevice::Api::JACK);
    // auto devices = soundStream.getDeviceList(ofSoundDevice::Api::MS_WASAPI);
    //auto devices = soundStream.getDeviceList();
  	//auto devices = soundStream.getMatchingDevices("default");
    auto devices = soundStream.getMatchingDevices("",2);
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
    avg_mfccs = (double*) malloc(sizeof(double) * N_COEFF);
    prev_mfccs = (double*) malloc(sizeof(double) * N_COEFF * N_AVG_SAMPLES);

    //512 bins, 42 filters, 13 coeffs, min/max freq 20/20000
    mfcc.setup(BUFFER_SIZE, 42, N_COEFF, 100, 4000, SAMPLE_RATE);
    ofxMaxiSettings::setup(SAMPLE_RATE, 2, BUFFER_SIZE);

    fbo_spectrum.allocate(BUFFER_SIZE/2, BUFFER_SIZE/2);
    fbo_spectrogram.allocate(BUFFER_SIZE/2, BUFFER_SIZE/2);


    vad_history = (uint8_t*) malloc(sizeof(uint8_t) * BUFFER_SIZE/2 );
    

    vad.setup(SAMPLE_RATE);


    gui.setup(); // most of the time you don't need a name
	gui.add(inputVolume.setup("inputVolume", 1.0f, 0.0f, 1.0f));
	gui.add(spectrogramBoost.setup("spectrogramBoost", 5.0f, 1.0f, 10.0f));
	gui.add(vadMode.setup("vadMode", 0, 0, 3));


	gui.add(vadAttack.setup("vadAttack", 0.1f, 0.1f, 2.0f));
	gui.add(vadRelease.setup("vadRelease", 0.1f, 0.1f, 2.0f));
	gui.add(minSpeechTime.setup("minSpeechTime", 1.0f, 0.1f, 5.0f));
	gui.add(speechHoldTime.setup("speechHoldTime", 5.0f, 1.0f, 20.0f));
	gui.add(maxSpeechTime.setup("maxSpeechTime", 60.0f, 30.0f, 240.0f));
	gui.add(silenceTime.setup("silenceTime", 5.0f, 5.0f, 30.0f));
	gui.add(vanishingTime.setup("vanishingTime", 20.0f, 10.0f, 60.0f));


    gui.setPosition(ofGetWidth() - gui.getWidth(),ofGetHeight()/2);

	bHide = false;



}

//--------------------------------------------------------------
void ofApp::update(){

    if( should_update_fbos ){
        updateFbo();
        should_update_fbos = 0; 
    }

    vadLevel = ofClamp(vadLevel + ofGetLastFrameTime() * (vad.vad_result ? vadAttack : -vadRelease), 0.0f, 1.0f); 

}


void ofApp::drawSpectrum(int _x, int _y, int _w, int _h){
    ofSetColor(255);
    fbo_spectrum.draw(_x,_y, _w, _h);
    ofNoFill();
    ofDrawRectangle(_x,_y,_w,_h);
}


void ofApp::drawSpectrogram(int _x, int _y, int _w, int _h){
    ofSetColor(255);
    fbo_spectrogram.draw(_x,_y, _w, _h);
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
        float height = (avg_mfccs[i]) * _h;
        ofDrawRectangle(_x + _w/2 + i*xinc,_y+_h/2 - height,xinc, height);
    }

}



void ofApp::drawFeatures(int _x, int _y, int _w, int _h){
    ofFill();
    ofSetColor(255);
    float xinc = ((float)_w) / 4;
    float height;


    //vad level
    height = vadLevel * _h;
    ofDrawRectangle(_x ,_y+_h - height,xinc, height);


    //	spectralCentroid
    height = mfft.spectralCentroid() * _h/20000;
    ofDrawRectangle(_x + xinc ,_y+_h - height,xinc, height);

    //	spectralFlatness
    height = mfft.spectralFlatness() * _h;
    ofDrawRectangle(_x + xinc*2,_y+_h - height,xinc, height);



    ofNoFill();
    ofDrawRectangle(_x,_y,_w,_h);




    ofDrawBitmapString("fr[0]: " + ofToString(vad.frames[0]), ofGetWidth()-150, 40);
    ofDrawBitmapString("fr[1]: " + ofToString(vad.frames[1]), ofGetWidth()-150, 60);
    ofDrawBitmapString("fr[1]: " + ofToString(vad.frames[0] + vad.frames[1] ?
            100.0 * ((double)vad.frames[1] / (vad.frames[0] + vad.frames[1])) : 0.0), ofGetWidth()-150, 80);
    ofDrawBitmapString("voiced: " + ofToString(vad.segments[1] ?
             (double)vad.frames[1] / vad.segments[1] : 0.0), ofGetWidth()-150, 100);
    ofDrawBitmapString("non v.: " + ofToString(vad.segments[0] ?
             (double)vad.frames[0] / vad.segments[0] : 0.0), ofGetWidth()-150, 120);
    ofDrawBitmapString("et.: " + ofToString(ofGetElapsedTimef(),1), ofGetWidth()-150, 140);
    ofDrawBitmapString("vadc.: " + ofToString(vadChangedAt,1), ofGetWidth()-150, 160);
    ofDrawBitmapString("speech: " + ofToString(speechAt,1), ofGetWidth()-150, 170);
    ofDrawBitmapString("status: " + ofToString(status), ofGetWidth()-150, 180);




}





//--------------------------------------------------------------
void ofApp::draw(){

    ofSetColor(255, 255, 255,255);
    drawSpectrogram(0,ofGetHeight()/2,ofGetWidth()/2, ofGetHeight()/2);

    //draw spectrogram cursor
    ofSetColor(0, 255, 0,255);
    ofDrawRectangle(((float)spectrogram_pos/mfft.bins)*(ofGetWidth()/2),ofGetHeight()/2, 1, ofGetHeight()/2);

    drawFeatures(ofGetWidth()/2,ofGetHeight()/2,ofGetWidth()/2,ofGetHeight()/2);

    drawMFCC(0,0,ofGetWidth()/2, ofGetHeight()/2);
    drawSpectrum(ofGetWidth()/2,0,ofGetWidth()/2, ofGetHeight()/2);

	ofSetWindowTitle(ofToString("void (fps: ") + ofToString(ofGetFrameRate(),0)+ ofToString(")"));


	// auto draw?
	// should the gui control hiding?
	if(!bHide){
		gui.draw();
	}

    //Send OSC:
    ofxOscMessage m;
    m.setAddress("/wek/inputs");
    for (int i = 0; i < N_COEFF; i++) {
        m.addFloatArg(mfccs[i]);
    }
    sender.sendMessage(m);

    if(lastInputVolume!=inputVolume){
        lastInputVolume=inputVolume;
        // https://www.dr-lex.be/info-stuff/volumecontrols.html#table1
        inputVolumeGainFactor = inputVolume * inputVolume * inputVolume * inputVolume;
    }

    if(lastVadMode!=vadMode){
        lastVadMode=vadMode;
        vad.setMode(vadMode);
    }
        

}



void ofApp::audioIn(ofSoundBuffer & buffer){

	for (size_t i = 0; i < buffer.getNumFrames(); i++){
        wave = buffer[i*2]*0.5 + buffer[i*2+1]*0.5;
        wave *= inputVolumeGainFactor;

        //Calculate vad and see if buffer is full
        if (vad.process(wave)) {

            float et = ofGetElapsedTimef();

            if(vad.changed){
                vadChangedAt = et;
            }

            switch (status)
            {
                case IDLE:
                    if(vad.vad_result==1 && (et-vadChangedAt) > minSpeechTime ){
                        status = LISTENING;
                        speechAt = et;
                    }
                    break;
                
                case LISTENING:
                    if(vad.vad_result==0 && (et-vadChangedAt) > speechHoldTime ){
                        status = IDLE;
                    }
                    break;
                
                case VANISHING:
                    break;
                
                default:
                    break;
            }

            
        }


        //Calculate the mfccs if fft buffer is full
        if (mfft.process(wave)) {
            mfft.magsToDB();
            oct.calculate(mfft.magnitudesDB);
            mfcc.mfcc(mfft.magnitudes, mfccs);
            should_update_fbos = 1;


            
            //copy current mfccs to prevs
            for( int i = 0; i < N_COEFF; i++ ){
                //cache
                prev_mfccs[prev_mfccs_idx*N_COEFF+i] = mfccs[i];

                double total = 0;    
                for( int j = 0; j < N_AVG_SAMPLES; j++ )
                    total += prev_mfccs[j*N_COEFF+i];
                avg_mfccs[i] = total / N_AVG_SAMPLES;
            }

            prev_mfccs_idx++;
            prev_mfccs_idx %= N_AVG_SAMPLES;




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
        float p = ofMap(mfft.magnitudes[i]*spectrogramBoost, 0.0, 10.0, 0, 255);
        p = ofClamp( p, 0, 255 );



        if(i > s*0.1f && i < s*0.2f)
            ofSetColor(p,0,vadLevel*255);
        else if(vad.vad_result && i < s*0.1f)
            ofSetColor(p,255-p,0);
        else
            ofSetColor(p);

        glVertex2f(spectrogram_pos,s-i);
    }


    glEnd();
    fbo_spectrogram.end();


    spectrogram_pos++;
    spectrogram_pos %=s;



}

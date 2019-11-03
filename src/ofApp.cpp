#include "ofApp.h"
#include <stdio.h>



void ofApp::setup(){

    sender.setup(HOST, PORT);

    /* some standard setup stuff*/
    ofEnableAlphaBlending();
    ofSetupScreen();
    ofBackground(0, 0, 0);
    ofSetFrameRate(60);
    ofSetVerticalSync(true);

  	ofSoundStreamSettings settings;


    fprintf(stdout, "opening sound device\n");
    //auto devices = soundStream.getDeviceList(ofSoundDevice::Api::JACK);
  	//auto devices = soundStream.getMatchingDevices("default");
    auto devices = soundStream.getMatchingDevices("",2);
    settings.setInDevice(devices[0]);
  	settings.setInListener(this);
  	settings.sampleRate = SAMPLE_RATE;
  	settings.numOutputChannels = 0;
  	settings.numInputChannels = 2;
  	settings.bufferSize = BUFFER_SIZE;
  	soundStream.setup(settings);
    fprintf(stdout, "setup sound device\n");

    mfft.setup(FFT_SIZE, BUFFER_SIZE, BUFFER_SIZE);
    oct.setup(SAMPLE_RATE, FFT_SIZE/2, N_AVERAGES);

    mfccs = (double*) malloc(sizeof(double) * N_COEFF);
    avg_mfccs = (double*) malloc(sizeof(double) * N_COEFF);
    prev_mfccs = (double*) malloc(sizeof(double) * N_COEFF * N_AVG_SAMPLES);

    //512 bins, 42 filters, 13 coeffs, min/max freq 20/20000
    //mfcc.setup(BUFFER_SIZE, 42, N_COEFF, 100, 4000, SAMPLE_RATE);
    mfcc.setup(BUFFER_SIZE/2, 42, N_COEFF, 20, 20000, SAMPLE_RATE);
    ofxMaxiSettings::setup(SAMPLE_RATE, 2, BUFFER_SIZE);

    fbo_spectrum.allocate(BUFFER_SIZE/2, BUFFER_SIZE/2);
    fbo_spectrogram.allocate(BUFFER_SIZE/2, BUFFER_SIZE/2);


    vad_history = (uint8_t*) malloc(sizeof(uint8_t) * BUFFER_SIZE/2 );
    

    vad.setup(SAMPLE_RATE);


    gui.setup(); // most of the time you don't need a name
	gui.add(inputVolume.setup("inputVolume", 1.0f, 0.0f, 1.0f));
	gui.add(spectrogramBoost.setup("spectrogramBoost", 5.0f, 1.0f, 10.0f));
	gui.add(vadMode.setup("vadMode", 0, 0, 3));


	gui.add(vadAttack.setup("vadAttack", 2.0f, 0.1f, 5.0f));
	gui.add(vadRelease.setup("vadRelease", 2.0f, 0.1f, 5.0f));
	gui.add(minSpeechTime.setup("minSpeechTime", 1.0f, 0.1f, 5.0f));
	gui.add(minSilenceTime.setup("minSilenceTime", 1.0f, 0.1f, 5.0f));
	gui.add(speechHoldTime.setup("speechHoldTime", 5.0f, 1.0f, 20.0f));
	gui.add(silenceHoldTime.setup("silenceHoldTime", 5.0f, 1.0f, 20.0f));
	gui.add(maxSpeechTime.setup("maxSpeechTime", 60.0f, 30.0f, 240.0f));
	gui.add(silenceTime.setup("silenceTime", 20.0f, 5.0f, 60.0f));
	gui.add(vanishingTime.setup("vanishingTime", 20.0f, 10.0f, 60.0f));

    gui.add(labels[0].setup("speechRatio",""));
    gui.add(labels[1].setup("listeningAt",""));
    gui.add(labels[2].setup("timeSilenced",""));
    gui.add(labels[3].setup("timeSpeaking",""));
    gui.add(labels[4].setup("status",""));

    gui.setPosition(ofGetWidth() - gui.getWidth(),0);

	bHide = false;



}

//--------------------------------------------------------------
void ofApp::update(){
    if( should_update_fbos ){
        updateFbo();
        should_update_fbos = 0; 
    }
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

    float xinc = _w / N_COEFF;
    for(int i=0; i < N_COEFF; i++) {
        float height = mfccs[i] * _h;
        ofDrawRectangle(_x + i*xinc,_y+_h/2 - height,xinc, height);
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
        float et = ofGetElapsedTimef();
        labels[0] = ofToString(speechRatio,2);
        labels[1] = ofToString(ofGetElapsedTimef()-listeningAt,1);
        labels[2] = ofToString(timeSilenced,1);
        labels[3] = ofToString(timeSpeaking,1);
        labels[4] = ofToString(statusToString());
	}

    //Send OSC:
    for (int i = 0; i < N_COEFF; i++) {
        sendFloat((ofToString("/mfcc")+ofToString(i)).c_str(),ofClamp((float)(mfccs[i]+1.0)*0.5f, 0.0f, 1.0f));
    }

    sendFloat("/vad",vadLevel);
    sendFloat("/verbo", speechRatio);



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


void ofApp::calculateVad(){

}


void ofApp::sendFloat(const char* addr, float val){
    //Send OSC:
    ofxOscMessage m;
    m.setAddress(addr);
    m.addFloatArg(val);
    sender.sendMessage(m);

}

char* ofApp::statusToString(){
    switch (status)
    {
        case IDLE:
            return "IDLE";

        case TRANSITIONING:
            return "TRANSITIONING";

        case SPEAKING:
            return "SPEAKING";

        case NOT_SPEAKING:
            return "NOT_SPEAKING";

        case VANISHING:
            return "VANISHING";
        
        default:
            return "IMPOSIBLE";
    }
}


void ofApp::audioIn(ofSoundBuffer & buffer){
	for (size_t i = 0; i < buffer.getNumFrames(); i++){
        wave = buffer[i*2]*0.5 + buffer[i*2+1]*0.5;
        wave *= inputVolumeGainFactor;


        //Calculate the mfccs if fft buffer is full
        if (mfft.process(wave)) {
            mfft.magsToDB();
            oct.calculate(mfft.magnitudesDB);
            mfcc.mfcc(mfft.magnitudes, mfccs);
            should_update_fbos = 1;
        }

        //Calculate vad and see if buffer is full
        if (vad.process(wave)) {



            float et = ofGetElapsedTimef();
            float dt = et-lastElapsedTimef;
            lastElapsedTimef = et;
            

            if(vad.changed)
                vadChangedAt = et;

            speechRatio = (timeSilenced + timeSpeaking)==0 ? 0.0f : timeSpeaking/(timeSilenced + timeSpeaking);



            switch (status)
            {
                case IDLE:
                    listeningAt = et;
                    timeSilenced = 0.0f;
                    timeSpeaking = 0.0f;
                    if((vad.vad_result==1) && (et-vadChangedAt) > minSpeechTime ){
                        setStatus(TRANSITIONING);
                    }

                    break;

                case TRANSITIONING:
                    vadLevel = ofClamp(vadLevel + dt * (vad.vad_result ? vadAttack : -vadRelease), 0.0f, 1.0f); 

                    if(vadLevel == 1.0f ){
                        setStatus(SPEAKING);
                    }

                    if(vadLevel == 0.0f ){
                        setStatus(NOT_SPEAKING);
                    }


                    break;

                case SPEAKING:
                    timeSpeaking += dt;

                    // update if not holding
                    if((et-changedStateAt) > speechHoldTime )
                        vadLevel = ofClamp(vadLevel + dt * (vad.vad_result ? vadAttack : -vadRelease), 0.0f, 1.0f); 

                    if((et-listeningAt) > maxSpeechTime)
                        setStatus(VANISHING);
                    
                    if(vadLevel < 1.0f)
                        setStatus(TRANSITIONING);
                    
                    break;

                case NOT_SPEAKING:
                    timeSilenced += dt;

                    if((et-changedStateAt) > silenceHoldTime )
                        vadLevel = ofClamp(vadLevel + dt * (vad.vad_result ? vadAttack : -vadRelease), 0.0f, 1.0f); 

                    if((et-changedStateAt) > silenceTime)
                        setStatus(VANISHING);
                   

                    if( vadLevel > 0.0f )
                        setStatus(TRANSITIONING);
                    

                    break;

                case VANISHING:
                    if( (et-changedStateAt) > vanishingTime ){
                        setStatus(IDLE);
                    }
                    break;
                
                default:
                    break;
            }

            
        }




	}
}

    

void ofApp::setStatus(Status_t st){
    status = st;
    changedStateAt = ofGetElapsedTimef();

    sendFloat("/idle", st == IDLE ? 1.0f: 0.0f);
    sendFloat("/starting", ((st == TRANSITIONING)||(st == SPEAKING)||(st == NOT_SPEAKING)) ? 1.0f: 0.0f);
    sendFloat("/finishing", st == VANISHING ? 1.0f: 0.0f);
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


        float h = (float)i/s;

        if(h > 0.9f)
            // RAW VAD
            ofSetColor(0, vad.vad_result  ? 255 : 0,0);
        else if(h > 0.8f)
            // speaking
            ofSetColor(0,(speechRatio < (h-0.8f)*10.0f) ? 0 : 200,0);
        else if(h > 0.7f)
            // SMOOTH VAD
            ofSetColor(0,0, (vadLevel < (h-0.7f)*10.0f) ? 0 : 255);
        else if(h > 0.6f)
            ofSetColor(0,0, (vadLevel == 1.0f)? 200 : 100);
        else
            ofSetColor(p);

        glVertex2f(spectrogram_pos,s-i);
    }


    glEnd();
    fbo_spectrogram.end();


    spectrogram_pos++;
    spectrogram_pos %=s;



}

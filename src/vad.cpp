#include "vad.h"
#include <stdio.h>
#include <stdlib.h>



void Vad::setup(int sample_rate){
    this->sample_rate = sample_rate;

    vad = fvad_new();
    if (!vad) {
        fprintf(stderr, "out of memory\n");
    }
    if (fvad_set_sample_rate(vad, sample_rate) < 0) {
        fprintf(stderr, "invalid sample rate: %d Hz\n", sample_rate);
    }

    this->frame_length = (size_t) sample_rate / 1000 * frame_ms;

    //initialize buffer    
    buffer = (int16_t*) malloc(sizeof(int16_t) * frame_length);


}

void Vad::setMode(int mode){
    if (fvad_set_mode(vad, mode) != 0) {
        fprintf(stderr, "invalid vad mode: %d Hz\n", mode);
    }
}



bool Vad::process(float sample){
    if (!vad) {
        return false;
    }

    // Convert the read samples to int16
    buffer[buffer_idx++] = sample * INT16_MAX;

    if(buffer_idx >= frame_length){
        //remember previous result
        if (changed)
            vad_prev_result = vad_result;

        //resets buffer
        buffer_idx = 0;

        vad_result = fvad_process(vad, buffer, frame_length);
        if (vad_result < 0) {
            fprintf(stderr, "VAD failed  %d  \n", vad_result);
        }

        // make sure it is 0 or 1
        vad_result = !!vad_result; 
        frames[vad_result]++;
        changed = vad_prev_result != vad_result;
        if (changed)
            segments[vad_result]++;

        return true;
    }else{
        return false;
    }

}


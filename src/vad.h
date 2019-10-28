#pragma once

#include "libvad/fvad.h"

class Vad{

	public:
        void setup(int sample_rate);
        bool process(float sample);
        void setMode(int mode);
        int sample_rate;
        size_t frame_length;
        int frame_ms = 10;
        long frames[2] = {0,0};
        long segments[2] = {0,0};
        int16_t *buffer = NULL;
        uint16_t buffer_idx = 0;
        int vad_result = -1;
        int vad_prev_result= -1;
        bool changed = false;
        Fvad *vad = NULL;

};

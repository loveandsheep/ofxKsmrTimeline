#pragma once
#include "ofMain.h"

enum timelineState {
    STATE_IDLE,
    STATE_PLAYING,
    STATE_PAUSE,
    STATE_FINISHED,
};

struct timelineEvent{
    uint64_t time = 0;
    bool paused = false;
};
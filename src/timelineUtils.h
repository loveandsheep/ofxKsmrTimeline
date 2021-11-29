#pragma once
#include "ofMain.h"

enum timelineState {
    STATE_IDLE,
    STATE_PLAYING,
    STATE_PAUSE,
    STATE_FINISHED,
};

struct timelineEventArgs{
    uint64_t time = 0;
    int chapterIndex = 0;
    bool paused = false;
};
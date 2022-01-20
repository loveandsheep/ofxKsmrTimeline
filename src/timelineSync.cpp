#include "timelineSync.h"

void timelineSyncVideo::setup(ofPtr<timeline> masterPtr, string chap)
{
    chapter = chap;
    timelineSyncBase::setup(masterPtr);
}

void timelineSyncVideo::play(bool byMaster)
{
    timelineSyncBase::play(byMaster);
    if (chapter.length() > 0 && chapter == master->getCurrentChapter()->name)
        video.play();
}

void timelineSyncVideo::setPause(bool b, bool byMaster)
{
    timelineSyncBase::setPause(b, byMaster);
    if (chapter.length() > 0 && chapter == master->getCurrentChapter()->name)
        video.setPaused(b);
}

void timelineSyncVideo::stop(bool byMaster)
{
    timelineSyncBase::stop(byMaster);
    if (chapter.length() > 0 && chapter == master->getCurrentChapter()->name)
        video.stop();
}

void timelineSyncVideo::setPosition(uint64_t millis, bool byMaster)
{
    timelineSyncBase::setPosition(millis, byMaster);
    if (chapter.length() > 0 && chapter == master->getCurrentChapter()->name)
        video.setPosition(float(millis) / (video.getDuration() * 1000));
}

void timelineSyncVideo::setChapter(int index, bool byMaster)
{
    timelineSyncBase::setChapter(index, byMaster);
}

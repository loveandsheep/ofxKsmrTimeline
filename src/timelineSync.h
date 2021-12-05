#pragma once
#include "ofMain.h"
#include "timeline.h"

class timeline;

class timelineSyncBase{
public:
    void setup(ofPtr<timeline> masterPtr)
    {
        master = masterPtr;
        ofAddListener(master->ev_play, this, &timelineSyncBase::playMaster);
        ofAddListener(master->ev_stop, this, &timelineSyncBase::stopMaster);
        ofAddListener(master->ev_pause, this, &timelineSyncBase::pauseMaster);
        ofAddListener(master->ev_seek, this, &timelineSyncBase::positionMaster);
        ofAddListener(master->ev_chapter, this, &timelineSyncBase::chapterMaster);
    }

    void playMaster(timelineEventArgs & arg){ play(true); }
    void stopMaster(timelineEventArgs & arg){ stop(true); }
    void pauseMaster(timelineEventArgs & arg){ setPause(arg.paused, true); }
    void positionMaster(timelineEventArgs & arg){ setPosition(arg.time, true); }
    void chapterMaster(timelineEventArgs & arg){ setChapter(arg.chapterIndex, true); }

    ofPtr<timeline> master;

    virtual void play (bool byMaster = false)
    {
        if (!master) return;
        if (!byMaster) master->play();
    };

    virtual void setPause (bool b, bool byMaster = false)
    {
        if (!master) return;
        if (!byMaster) master->setPause(b);
    };
    virtual void stop (bool byMaster = false)
    {
        if (!master) return;
        if (!byMaster) master->stop();
    };

    virtual void setPosition(uint64_t millis, bool byMaster = false)
    {
        if (!master) return;
        if (!byMaster) master->setPositionByMillis(millis);
    };

    virtual void setChapter(int index, bool byMaster = false)
    {
        if (!master) return;
        if (!byMaster) master->setChapter(index);
    }
};

class timelineSyncIp : public timelineSyncBase{
public:
    ofxOscSender sender;
    void setup(ofPtr<timeline> masterPtr, string addr, int port)
    {
        timelineSyncBase::setup(masterPtr);
        sender.setup(addr, port);
    }

    virtual void play(bool byMaster = false)
    {
        timelineSyncBase::play(byMaster);
        ofxOscMessage m;
        m.setAddress("/play");
        sender.sendMessage(m);
    };
    
    virtual void setPause (bool b, bool byMaster = false)
    {
        timelineSyncBase::setPause(b, byMaster);
        ofxOscMessage m;
        m.setAddress("/pause");
        m.addBoolArg(b);
        sender.sendMessage(m);
    };

    virtual void stop (bool byMaster = false)
    {
        timelineSyncBase::stop(byMaster);
        ofxOscMessage m;
        m.setAddress("/stop");
        sender.sendMessage(m);

    };

    virtual void setPosition(uint64_t millis, bool byMaster = false)
    {
        timelineSyncBase::setPosition(millis, byMaster);
        ofxOscMessage m;
        m.setAddress("/seek");
        m.addInt64Arg(millis);
        sender.sendMessage(m);
    };

    virtual void setChapter(int index, bool byMaster = false)
    {
        timelineSyncBase::setChapter(index, byMaster);
        ofxOscMessage m;
        m.setAddress("/chapter");
        m.addIntArg(index);
        sender.sendMessage(m);        
    }

};

class timelineSyncLocal : public timelineSyncBase{
public:
    ofPtr<timeline> syncTimeline;

    void setup(ofPtr<timeline> masterPtr, ofPtr<timeline> childPtr)
    {
        timelineSyncBase::setup(masterPtr);
        syncTimeline = childPtr;
    }

    virtual void play(bool byMaster = false)
    {
        timelineSyncBase::play(byMaster);
        syncTimeline->play();
    };
    
    virtual void setPause (bool b, bool byMaster = false)
    {
        timelineSyncBase::setPause(b, byMaster);
        syncTimeline->setPause(b);
    };

    virtual void stop (bool byMaster = false)
    {
        timelineSyncBase::stop(byMaster);
        syncTimeline->stop();
    };

    virtual void setPosition(uint64_t millis, bool byMaster = false)
    {
        timelineSyncBase::setPosition(millis, byMaster);
        syncTimeline->setPositionByMillis(millis);
    };
    
    virtual void setChapter(int index, bool byMaster = false)
    {
        timelineSyncBase::setChapter(index, byMaster);
        syncTimeline->setChapter(index);
    }
};

// 再生
// 一時停止
// シーク（setposition）

// メモ：timelineをマスターとする同期クラス
// 動画とリモートタイムラインをここからコントロールできるようにする
// byMasterがfalseの場合はマスターのタイムラインに指示を仰ぐ
// マスターTMから指示を出すときはbyMaster true
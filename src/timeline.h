#pragma once
#include "ofMain.h"
#include "./track.h"
#include "./motorTrack.h"
#include "ofxOsc.h"

class timelineSyncBase;

class chapter {
public:
    string                      name = "chapter";
    uint64_t                    duration = 1000;
    bool                        isLoop = false;
    vector<ofPtr<trackBase> >   tracks;
};

class timeline {
public:
    void load(string path);
    void save(string path);
    void clear(bool completely = false);
    void drawMinimum(int x, int y);

    void setFromJson(ofJson data);
    ofJson getJsonData();

    // タイムラインの再生・シーク
    timelineEventArgs & getTimelineEvArg();
    timelineEventArgs eventArg;
    ofEvent<timelineEventArgs> ev_play;
    ofEvent<timelineEventArgs> ev_stop;
    ofEvent<timelineEventArgs> ev_pause;
    ofEvent<timelineEventArgs> ev_seek;
    ofEvent<timelineEventArgs> ev_chapter;
    void play();
    void stop();
    void setPositionByMillis(uint64_t time);
    void setPosition(float position);
    void togglePause();
    void setPause(bool b);

    //ゲッター
    uint64_t const & getPassed(){return passed;}
    uint64_t const & getDuration(){return getCurrentChapter()->duration;}
    bool getPaused(){return paused;}
    bool getIsPlay(){return isPlay;}
    bool getIsLoop(){return getCurrentChapter()->isLoop;}
    void setIsLoop(bool b){getCurrentChapter()->isLoop = b;}

    //その他メソッド
    void sendSyncJsonData(string host = "", int port = 0);
    ofxOscSender & getSender(){return sender;}
    void sendOsc();

    void setup();
    timelineState const & update();
    void draw();

    vector<string> json_piece;

    //セッター
    void setDuration(uint64_t d){getCurrentChapter()->duration = d;}

    //トラックの追加削除・編集
    ofPtr<trackBase> addTrack(string name, trackType tp, bool newTrack = true);
    void removeTrack(string name);
    void upTrack(ofPtr<trackBase> tr);
    void downTrack(ofPtr<trackBase> tr);

    vector<ofPtr<trackBase> > & getTracks(){return getCurrentChapter()->tracks;}
    ofPtr<trackBase> getTrack(string name)
    {
        for (auto & t : getCurrentChapter()->tracks)
        {
            if (name == t->getName()) return t;
        }
    }
    
    void drawGui();

    tm_event getEventParameter(ofPtr<trackBase> & tr, int paramIndex = 0, int time = -1, int callOrigin = 0);

    bool getParameterExist(string trackName){
        for (auto & t : getCurrentChapter()->tracks)
        {
            if (t->getName() == trackName) return true;
        }
        return false;
    }
    template <typename T> T getParameter(string trackName, int paramIndex = 0, int time = -1)
    {
        if (time < 0) time = getPassed();
        T ret = T();
        for (auto & t : getCurrentChapter()->tracks)
        {
            if (t->getName() == trackName) 
            {
                return getParameter<T>(t, paramIndex, time);
            }
        }

        return ret;
    }

    template<typename T> T getParameter(ofPtr<trackBase> & tr, int paramIndex = 0, int time = -1)
    {
        if (time < 0) time = getPassed();
        T ret = T();
        ret = tr->getParamsRef()[paramIndex]->get<T>(time, getDuration());
        return ret;
    }

    string getCurrentFileName(){return currentFileName;}
    
    void setSenderAddr(string addr);
    void setSenderPort(int port);
    void setReceiverPort(int port);
    string const & getSendAddr(){return sendAddr;}
    int const & getSendPort(){return sendPort;}
    int const & getReceiverPort(){return recvPort;}
    bool sendOnSeek = false;
    bool sendAlways = false;

    vector<string> getChapterNames();
    int getChapterSize(){return chapters.size();}
    int getCurrentChapterIndex(){return currentChapterIndex;}
    ofPtr<chapter> & getCurrentChapter(){return chapters[currentChapterIndex];}
    void createChapter(string name, uint64_t duration, bool setToCurrent = true);
    void setChapter(string name);
    void setChapter(int index);
    void removeChapter(string name);
    void removeChapter(int index);
    void clearChapter(bool completely = false);

    bool edited = false;
protected:

    timelineState currentState;
    ofxOscSender sender;
    ofxOscReceiver receiver;
    string sendAddr = "localhost";
    uint64_t seekOscCheck = 0;
    int sendPort = 7000;
    int recvPort = 7000;

    //チャプタープロパティ
    int currentChapterIndex = 0;
    vector<ofPtr<chapter> > chapters;

    uint64_t started = 0;
    uint64_t passed = 0;
    bool isPlay = false;
    bool paused = false;

    string currentPath = "";
    string currentFileName = "";
};

//イベントのgetParameterのみ特殊化
template <> inline tm_event timeline::getParameter(ofPtr<trackBase> & tr, int paramIndex, int time)
{
    if (time < 0) time = passed;
    return getEventParameter(tr, paramIndex, time);
}
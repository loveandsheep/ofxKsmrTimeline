#pragma once
#include "ofMain.h"
#include "./track.h"
#include "ofxOsc.h"

class timeline {
public:
    void load(string path);
    void save(string path);
    void clear();

    // タイムラインの再生・シーク
    void play();
    void stop();
    void setPositionByMillis(uint64_t time);
    void setPosition(float position);
    void togglePause();
    void setPause(bool b);

    //ゲッター
    uint64_t getPassed(){return passed;}
    uint64_t getDuration(){return duration;}
    bool getPaused(){return paused;}
    bool getIsPlay(){return isPlay;}
    bool isLoop = false;

    //その他メソッド
    ofxOscSender & getSender(){return sender;}
    void sendOsc();

    void setup();
    void update();
    void draw();


    //セッター
    void setDuration(uint64_t d){duration = d;}

    //トラックの追加削除・編集
    ofPtr<trackBase> addTrack(string name, trackType tp, bool newTrack = true);
    void removeTrack(string name);
    void upTrack(ofPtr<trackBase> tr);
    void downTrack(ofPtr<trackBase> tr);

    vector<ofPtr<trackBase> > & getTracks(){return tracks;}
    
    void drawGui();

    tm_event getEventParameter(ofPtr<trackBase> & tr, int paramIndex = 0, int time = -1, int callOrigin = 0);

    bool getParameterExist(string trackName){
        for (auto & t : tracks)
        {
            if (t->getName() == trackName) return true;
        }
        return false;
    }
    template <typename T> T getParameter(string trackName, int paramIndex = 0, int time = -1)
    {
        if (time < 0) time = passed;
        T ret = T();
        for (auto & t : tracks)
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
        if (time < 0) time = passed;
        T ret = T();
        ret = tr->getParamsRef()[paramIndex]->get<T>(time, duration);
        return ret;
    }

    string getCurrentFileName(){return currentFileName;}
    tm_event oscEvent;

    void setSenderAddr(string addr);
    void setSenderPort(int port);
    void setReceiverPort(int port);
    string const & getSendAddr(){return sendAddr;}
    int const & getSendPort(){return sendPort;}
    int const & getReceiverPort(){return recvPort;}
    bool sendOnSeek = false;
    bool sendAlways = false;
protected:
    ofxOscSender sender;
    ofxOscReceiver receiver;
    string sendAddr = "localhost";
    uint64_t seekOscCheck = 0;
    int sendPort = 12400;
    int recvPort = 12500;
    vector<ofPtr<trackBase> > tracks;
    uint64_t duration = 1000;

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
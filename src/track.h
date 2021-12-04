#pragma once
#include "ofMain.h"
#include "./param.h"
#include "./jsonParam.h"
#include "ofxOsc.h"

enum trackType {
    TRACK_FLOAT,
    TRACK_COLOR,
    TRACK_VEC2,
    TRACK_VEC3,
    TRACK_DMX,
    TRACK_IO,
    TRACK_NODE,
    TRACK_MOTOR,
    TRACK_EVENT,
    TRACK_INHERITANCE,
    TRACK_JSONSTREAM,
};

// トラックは複数のparamを持つ
// 時間のレンジはparamで
// 各クラスへの指示もここから出す
class trackBase {
public:
    virtual void setup(string name, trackType type, bool newTrack);
    virtual void update(timelineState state, uint64_t const & passed, uint64_t const & duration){};
    virtual void controlMessage(ofxOscMessage & m, uint64_t passed, uint64_t duration){};
    //

    //
    trackType const & getType(){return myType;}
    ofPtr<param> getParam(string name);
    ofPtr<param> getParam(int index);
    vector<ofPtr<param> > & getParamsRef(){return params;}
    string getName(){return myName;}
    string getUniqueName(){return uniqueName;}
    void setName(string name){myName = name;};
    
    int view_height = 100;
    bool fold = false;
    bool oscSend = false;

    virtual ofJson getJsonData();
    virtual void setJsonData(ofJson j);

    static const int NUM_ORIGIN = 32;
    int eventCallOrigin[NUM_ORIGIN] = {0};

protected:
    string uniqueName;//起動中に使う、変更しても終了まで変更しない名前。主にGUIのIDとして使う
    string myName;
    trackType myType;
    vector<ofPtr<param> > params;
};
#pragma once
#include "ofMain.h"
#include "./param.h"
#include "./jsonParam.h"

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
    void setup(string name, trackType type, bool newTrack);
    virtual void update(){};
    
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
    bool oscSend = false;

    ofJson getJsonData();

    static const int NUM_ORIGIN = 32;
    int eventCallOrigin[NUM_ORIGIN] = {0};

protected:
    string uniqueName;//起動中に使う、変更しても終了まで変更しない名前。主にGUIのIDとして使う
    string myName;
    trackType myType;
    vector<ofPtr<param> > params;
};
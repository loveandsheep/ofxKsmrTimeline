#pragma once
#include "ofMain.h"
#include "./param.h"

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
};

// トラックは複数のparamを持つ
// 時間のレンジはparamで
// 各クラスへの指示もここから出す
class trackBase {
public:
    void setup(string name, trackType type, bool newTrack);
    void update();
    
    //

    //
    trackType const & getType(){return myType;}
    ofPtr<param> getParam(string name);
    ofPtr<param> getParam(int index);
    vector<ofPtr<param> > const & getParamsRef(){return params;}
    string getName(){return myName;}
    string getUniqueName(){return uniqueName;}
    void setName(string name){myName = name;};
    
    int view_height = 100;
    bool oscSend = false;

    ofJson getJsonData();

protected:
    string uniqueName;//起動中に使う、変更しても終了まで変更しない名前。主にGUIのIDとして使う
    string myName;
    trackType myType;
    vector<ofPtr<param> > params;
};
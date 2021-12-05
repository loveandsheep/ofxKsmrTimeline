#pragma once
#include "ofMain.h"
#include "./block.h"

// 特定の型の遷移を管理する
// 数値・Bool・色・Vec3
// paramはブロックを持つ
// ブロックは基本的に全てFloatで、ここで情報を解釈して上に渡す

enum paramType {
    PTYPE_FLOAT,
    PTYPE_BOOL,
    PTYPE_COLOR,
    PTYPE_VEC2,
    PTYPE_VEC3,
    PTYPE_EVENT,
    PTYPE_JSONSTREAM,
};

struct tm_event {
    string label = "";
    int time = -1;
};

class param {
public:

    void setType(paramType tp);
    paramType const & getType(){return myType;}

    template <typename T> T get(uint64_t const & time, uint64_t const & duration){}

    virtual float   getFloat(uint64_t const & time, uint64_t const & duration);
    ofFloatColor getColor(uint64_t const & time, uint64_t const & duration);
    tm_event getEvent(uint64_t const & time, uint64_t const & duration);
    ofVec2f getVec2f(uint64_t const & time, uint64_t const & duration);
    ofVec3f getVec3f(uint64_t const & time, uint64_t const & duration);
    float   getBlockValue(int numBlock, uint64_t const & time, uint64_t const & duration);
    
    int getBlockIndexByTime(uint64_t millis);
    int getBlockIndexByLabel(string const & label);
    vector<ofPtr<block> > pickBlocksByTime(uint64_t millis);
    vector<ofPtr<block> > pickBlocksByLabel(string const & label);
    vector<ofPtr<block> > pickBlocks(int number);

    int  addKeyPoint(uint64_t const & time);
    int  moveKeyPoint(ofPtr<block> const & bl, int const & targetTime, vector<uint64_t> const & snapPt, int snapRange);
    void setKeyPoint(ofPtr<block> const & bl, int const & targetTime);
    void removeKeyPoint(ofPtr<block> const & bl);
    void clearKeyPoints();

    vector<uint64_t> const & getKeyPoints(){return keyPoints;}
    vector<ofPtr<block> > & getBlocks(int num){return blocks[num];}

    string dumpBlocks();
    void        refleshInherit();//継承関係を更新する

    void resetMaxMinRange();
    virtual float getValueMax(int index = -1);
    virtual float getValueMin(int index = -1);

    virtual ofJson getJsonData();

    bool checkEdited();
    protected:
    bool edited = false;
    //プロパティ（編集される固定値）
    paramType   myType = PTYPE_FLOAT;
    vector<uint64_t>        keyPoints;
    vector<ofPtr<block> >   blocks[4];

    //計算される値達
    bool        checkParamMatching(paramType tp);
    int         numUsingBlockLine = 1;
    uint64_t    lastSeekPoint = 0;
    float valueMax[4], valueMin[4];
};

template <> inline float param::get(uint64_t const & time, uint64_t const & duration){return getFloat(time, duration);}
template <> inline ofFloatColor param::get(uint64_t const & time, uint64_t const & duration){return getColor(time, duration);}
template <> inline tm_event param::get(uint64_t const & time, uint64_t const & duration){return getEvent(time, duration);}
template <> inline ofVec2f param::get(uint64_t const & time, uint64_t const & duration){return getVec2f(time, duration);}
template <> inline ofVec3f param::get(uint64_t const & time, uint64_t const & duration){return getVec3f(time, duration);}
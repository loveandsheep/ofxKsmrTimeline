#pragma once
#include "ofMain.h"
#include "timelineUtils.h"

enum complementType {
    CMPL_LINEAR,
    CMPL_CONSTANT,
    CMPL_SIN,
    CMPL_QUAD,
    CMPL_CUBIC,
    CMPL_RAMP,
    CMPL_EXPO,
};

class block {
public:

    void setInherit(bool b, ofPtr<block> ptr = nullptr){
        edited = true;
        Inherit = b;
        if (b && ptr) inheritPtr = ptr;
        else inheritPtr.reset();
    }

    void setKeep(bool b)
    {
        edited = true;
        keep = b;
        if (b) setTo(getFrom());
    }
    bool getKeep(){return keep;}

    void updateInherit(ofPtr<block> ptr) {inheritPtr = ptr;}

    bool getIsInherit(){return Inherit;}
    
    float getFrom(){
        if (inheritPtr && Inherit) from = inheritPtr->getTo();
        return from;
    }

    float getTo(){
        if (keep) return getFrom();
        return to;
    }

    void setFrom(float val){
        edited = true;
        from = val;
        maxVal = MAX(from, to);
        minVal = MIN(from, to);
    }
    
    void setTo(float val){
        edited = true;
        to = val;
        maxVal = MAX(from, to);
        minVal = MIN(from, to);
    }

    float getMax(){return maxVal;}
    float getMin(){return minVal;}

    ofPtr<block> inheritPtr;

    void setNumber(int num){number = num;}
    int getNumber(){return number;}

    ofJson getJsonData() {
        ofJson j;
        j["inherit"] = Inherit;
        j["label"] = label;
        j["keep"] = keep;
        j["from"] = from;
        j["to"] = to;
        j["eventName"] = eventName;
        j["cmplType"] = int(cmplType);
        j["easeIn"] = easeInFlag;
        j["easeOut"] = easeOutFlag;
        j["accel"] = accel;
        j["decel"] = decel;
        j["marker"] = marker;

        return j;
    }

    void setFromJson(ofJson & j_br)
    {
        setInherit(j_br["inherit"].get<bool>());
        setFrom(j_br["from"].get<float>());
        setTo(j_br["to"].get<float>());
        eventName = j_br["eventName"].get<string>();
        setComplement(complementType(j_br["cmplType"].get<int>()));
        easeInFlag = j_br["easeIn"].get<bool>();
        easeOutFlag = j_br["easeOut"].get<bool>();
        if (!j_br["keep"].empty())  setKeep(j_br["keep"].get<bool>());
        if (!j_br["label"].empty()) label = j_br["label"].get<string>();
        if (!j_br["accel"].empty()) accel = j_br["accel"].get<float>();
        if (!j_br["decel"].empty()) decel = j_br["decel"].get<float>();
        if (!j_br["marker"].empty()) marker = j_br["marker"].get<float>();
    }

    complementType const & getComplement(){return cmplType;}
    void setComplement(complementType type){cmplType = type;edited = true;}

    //=====================================Curve
    float getValue(float pos, uint64_t const & length)
    {
        uint8_t easeOpt = (easeInFlag ? EASE_IN : 0) | (easeOutFlag ? EASE_OUT : 0);
        float x = getLinear(pos, easeOpt);

        if (cmplType == CMPL_CONSTANT) x = getConstant(pos, easeOpt);
        if (cmplType == CMPL_SIN)      x = getSin(pos, easeOpt);
        if (cmplType == CMPL_CUBIC)    x = getCubic(pos, easeOpt);
        if (cmplType == CMPL_QUAD)     x = getQuad(pos, easeOpt);
        if (cmplType == CMPL_RAMP)     x = getRampControl(pos, length);
        if (cmplType == CMPL_EXPO)     x = getExpo(pos, easeOpt);

        lastLength = length;

        return ofMap(x, 0, 1, getFrom(), getTo(), true);
    }

    float getLinear(float x, uint8_t easeOption);
    float getConstant(float x, uint8_t easeOption);
    float getSin(float x, uint8_t easeOption);
    float getQuad(float x, uint8_t easeOption);
    float getCubic(float x, uint8_t easeOption);
    float getExpo(float x, uint8_t easeOption);

    uint64_t getLength(){return lastLength;}
    float getPow(float x, uint8_t easeOption, int numPow);
    float getRampControl(float x, uint64_t const & length);

    static const uint8_t EASE_IN = 0x1;
    static const uint8_t EASE_OUT = 0x2;
    static const uint8_t EASE_INOUT = 0x3;

    bool edited = false;
    bool easeInFlag = true;
    bool easeOutFlag = true;

    string eventName = "newEvent";
    string label = "";

    //??????????????????
    float marker = 0;

    //????????????
    float accel = 0.25;
    float decel = 0.25;
    float speed_max = 0;

    float dump_T = 0;
    float dump_spd = 0, dump_topY = 0;
    float dump_maxP = 0, dump_prog = 0, dump_pDiff = 0;
    float dump_t1 = 0, dump_t2 = 0, dump_t3 = 0;
    float dumpRamp = 0;
    string getDump()
    {
        stringstream ret;
        ret << "t1 :" << dump_t1 << endl;
        ret << "t2 :" << dump_t2 << endl;
        ret << "t3 :" << dump_t3 << endl;
        ret << "topY :" << dump_topY << endl;
        ret << "maxProgress :" << dump_maxP << endl;
        ret << "progress :" << dump_prog << endl;
        ret << "pDiff :" << dump_pDiff << endl;
        ret << "speed :" << dump_spd << endl;
        ret << "ramp :" << dumpRamp << endl;
        return ret.str();
    }

protected:
    
    complementType cmplType = CMPL_QUAD;
    uint64_t lastLength = 0;
    bool Inherit = true;//?????????from?????????????????????????????????
    bool keep = false;
    float from = 0;
    float to = 1;
    int number = 0;

    float maxVal = 0;
    float minVal = 0;

    //sqrt????????????
    float ls_pdiff = 0;
    float ls_maxProgress = 0;
    float sq_pdiff = 0;
    float sq_maxProgress = 0;

};
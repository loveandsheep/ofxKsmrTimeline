#include "track.h"
#include "ofxModbusMotorDriver.h"

class motorTrack : public trackBase {
public:
    int motorIndex = 1;
    int lastBlock = -1;
    float stepDeg = 0.018;
    bool doDrive = true;
    
    virtual void setup(string name, trackType type, bool newTrack)
    {
        uniqueName = myName = name;
        myType = type;
        if (type == TRACK_MOTOR)
        {
            auto np = make_shared<param>();
            np->setType(PTYPE_FLOAT);
            params.push_back(np);
        }

        if (newTrack) for (auto & p : params) p->addKeyPoint(0);
    }

    void motorStandby(uint64_t passed)
    {
        auto p = getParamsRef()[0];
        int  index = p->getBlockIndexByTime(passed);
        auto b = p->pickBlocksByTime(passed)[0];
        ofxModbusMotorDriver::instance().goAbs(motorIndex, b->getTo() / stepDeg,
            30 / stepDeg, 1000 / stepDeg,1000 / stepDeg);
    }

    virtual void update(timelineState state, uint64_t passed)
    {
        if (state == STATE_PLAYING)
        {
            auto p = getParamsRef()[0];
            int  index = p->getBlockIndexByTime(passed);
            auto b = p->pickBlocksByTime(passed)[0];
            
            if (lastBlock != index)
            {
                if (lastBlock == -1)
                {
                }
                else
                {
                    //MotorRampかつKeepじゃない時かつ値が有効の時
                    bool drive = true;
                    
                    if (b->getKeep()) drive = false;
                    if (!doDrive) drive = false;

                    if (drive)
                    {
                        ofxModbusMotorDriver::instance().goAbs(motorIndex, b->getTo() / stepDeg,
                            b->speed_max * 1000 / stepDeg,
                            b->accel * 1000 / stepDeg,
                            b->decel * 1000 / stepDeg);
                    }

                }

                lastBlock = index;
            }
        }
        else
        {
            lastBlock = -1;
        }
    }

    virtual void setJsonData(ofJson j){
        trackBase::setJsonData(j);
        if (!j["drive"].empty()) doDrive = j["drive"].get<bool>();
        if (!j["motorID"].empty()) motorIndex = j["motorID"].get<int>();
        if (!j["stepDeg"].empty()) stepDeg = j["stepDeg"].get<float>();
    }

    virtual ofJson getJsonData(){
        ofJson j;
        j["view_height"] = view_height;
        j["name"] = myName;
        j["type"] = int(myType);
        j["oscSend"] = oscSend;
        j["drive"] = doDrive;
        j["motorID"] = motorIndex;
        j["stepDeg"] = stepDeg;

        for (auto & p : params)
            j["params"].push_back(p->getJsonData());

        return j;
    }
};
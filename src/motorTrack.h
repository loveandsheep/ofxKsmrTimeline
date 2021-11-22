#include "track.h"
#include "ofxModbusMotorDriver.h"

class motorTrack : public trackBase {
public:
    int motorIndex = 1;
    int lastBlock = -1;
    bool doDrive = false;
    
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
        ofxModbusMotorDriver::instance().goAbs(motorIndex, b->getTo(),
            5000,1000,1000);
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
                    ofxModbusMotorDriver::instance().goAbs(motorIndex, b->getTo(),
                        b->speed_max * 1000,
                        b->accel * 1000,
                        b->decel * 1000);

                    cout << "run motor " << endl;
                }

                lastBlock = index;
            }
        }
        else
        {
            lastBlock = -1;
        }
    }
};
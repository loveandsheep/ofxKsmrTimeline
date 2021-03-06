#include "track.h"

#define MODBUS

#ifdef MODBUS
#include "ofxModbusMotorDriver.h"
#endif

class motorTrack : public trackBase {
public:
    int motorIndex = 1;
    int lastBlock = -1;
    float stepDeg = 0.018;
    bool doDrive = true;
    bool spin = false;

    int gui_goParam = 0;
    
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
        if (spin) return;
         
        auto p = getParamsRef()[0];
        int  index = p->getBlockIndexByTime(passed);
        auto b = p->pickBlocksByTime(passed)[0];
#ifdef MODBUS
        ofxModbusMotorDriver::instance().goAbs(motorIndex, b->getTo() / stepDeg,
            30 / stepDeg, 50 / stepDeg, 50 / stepDeg);
#endif
    }

    virtual void controlMessage(ofxOscMessage & m, uint64_t passed, uint64_t duration)
    {
#ifdef MODBUS

        if (m.getArgAsString(0) == getName())
        {
            auto & motor = ofxModbusMotorDriver::instance();
            if (m.getAddress() == "/track/standby") motorStandby(passed);
            if (m.getAddress() == "/track/jog/fw") 
                motor.run(motorIndex, 50 / stepDeg, 100 / stepDeg, 100 / stepDeg, 100);

            if (m.getAddress() == "/track/jog/rv") 
                motor.run(motorIndex, -50 / stepDeg, 100 / stepDeg, 100 / stepDeg, 100);

            if (m.getAddress() == "/track/jog/stop") 
                motor.run(motorIndex, 0, 1000, 1000, 100);
            
            if (m.getAddress() == "/track/motor/set")
            {
                logHost = m.getRemoteHost();
                lastLog = "Go to " + ofToString(m.getArgAsInt(1));
                motor.goAbs(motorIndex, m.getArgAsInt(1) / stepDeg, 50 / stepDeg, 100 / stepDeg, 100 / stepDeg);
            }

            if (m.getAddress() == "/track/preset")
            {
                logHost = m.getRemoteHost();
                lastLog = "Preset.";
                motor.setRemote(motorIndex, RIO_PRESET, true);
                motor.setRemote(motorIndex, RIO_PRESET, false);
            }

            if (m.getAddress() == "/track/alarmReset")
            {
                logHost = m.getRemoteHost();
                lastLog = "Alarm reset";
                resetAlarm();
            }
        }

#endif
    }

    void resetAlarm()
    {
        ofxModbusMotorDriver::instance().setRemote(motorIndex, RIO_ALM_RST, true);
        ofxModbusMotorDriver::instance().setRemote(motorIndex, RIO_ALM_RST, false);
    }

    virtual void update(timelineState state, uint64_t const & passed, uint64_t const & duration)
    {
        if (state == STATE_PLAYING)
        {
            auto p      = getParamsRef()[0];
            int  index  = p->getBlockIndexByTime(passed);
            auto b      = p->pickBlocksByTime(passed)[0];

            p->getBlockValue(0, passed, duration);
            
            if (lastBlock != index)
            {
                if (lastBlock == -1)
                {
                }
                else
                {
                    //MotorRamp??????Keep???????????????????????????????????????
                    bool drive = true;
                    
                    if (b->getKeep()) drive = false;
                    if (!doDrive) drive = false;
                    if (!spin && b->getComplement() != CMPL_RAMP) drive = false;
                    if (spin && b->getComplement() != CMPL_LINEAR) drive = false;
#ifdef MODBUS
                    if (drive)
                    {
                        if (spin)
                        {
                            float acc = abs(b->getTo() - b->getFrom()) / (b->getLength() / 1000.0);

                            ofxModbusMotorDriver::instance().run(
                                motorIndex, b->getTo() / stepDeg, 
                                acc / stepDeg, acc / stepDeg, 500);
                        }
                        else
                        {
                            ofxModbusMotorDriver::instance().goAbs(motorIndex, b->getTo() / stepDeg,
                                b->speed_max * 1000 / stepDeg,
                                b->accel * 1000 / stepDeg,
                                b->decel * 1000 / stepDeg);
                        }
                    }
#endif
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
        if (!j["spin"].empty()) spin = j["spin"].get<bool>();
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
        j["spin"] = spin;

        for (auto & p : params)
            j["params"].push_back(p->getJsonData());

        return j;
    }
};
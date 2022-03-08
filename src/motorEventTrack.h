#include "track.h"

#define MODBUS

#ifdef MODBUS
#include "ofxModbusMotorDriver.h"
#endif

class motorEventTrack : public trackBase {
public:
	int motorIndex = 1;
	float stepDeg = 0.018;
    bool doDrive = true;
	int gui_goParam = 0;

	int p_speed = 500;
	int p_accel = 500;
	int p_decel = 500;

	virtual void setup(string name, trackType type, bool newTrack)
	{
		uniqueName = myName = name;
		myType = type;
		if (type == TRACK_MOTOREVENT)
		{
			auto np = make_shared<param>();
			np->setType(PTYPE_EVENT);
			params.push_back(np);
		}

		if (newTrack) for (auto & p : params) p->addKeyPoint(0);
	}

	virtual void update(timelineState state, uint64_t const & passed, uint64_t const & duration)
	{
		tm_event ev = getEventParameter(state, passed, duration, 0, -1, 2);
		if (ev.label.length() > 0)
		{
			if (ev.label.substr(0, 4) == "spd:")
			{
				float spd = ofToFloat(ev.label.substr(4));
				p_speed = spd;
			}
			if (ev.label.substr(0, 4) == "acc:")
			{
				float acc = ofToFloat(ev.label.substr(4));
				p_accel = acc;
			}
			if (ev.label.substr(0, 4) == "dec:")
			{
				float dec = ofToFloat(ev.label.substr(4));
				p_decel = dec;
			}
			if (doDrive)
			{
				if (ev.label.substr(0, 4) == "alm:")
				{
					ofxModbusMotorDriver::instance().setRemote(0, RIO_ALM_RST, true);
					ofxModbusMotorDriver::instance().setRemote(0, RIO_ALM_RST, false);
				}
				if (ev.label.substr(0, 4) == "rnd:")
				{
					float angle = ofToFloat(ev.label.substr(4));
					ofxModbusMotorDriver::instance().roundNear(motorIndex,
						angle / stepDeg, p_speed / stepDeg, p_accel / stepDeg, p_decel / stepDeg);
				}
				if (ev.label.substr(0, 5) == "rndf:")
				{
					float angle = ofToFloat(ev.label.substr(5));
					// ofxModbusMotorDriver::instance().roundFWD(motorIndex,
						// angle / stepDeg, p_speed / stepDeg, p_accel / stepDeg, p_decel / stepDeg);
				}
				if (ev.label.substr(0, 5) == "rndr:")
				{
					float angle = ofToFloat(ev.label.substr(5));
					// ofxModbusMotorDriver::instance().roundRVS(motorIndex,
					// 	angle / stepDeg, p_speed / stepDeg, p_accel / stepDeg, p_decel / stepDeg);
				}
				if (ev.label.substr(0, 4) == "abs:")
				{
					float angle = ofToFloat(ev.label.substr(4));
					// ofxModbusMotorDriver::instance().goAbs(motorIndex,
					// 	angle / stepDeg, p_speed / stepDeg, p_accel / stepDeg, p_decel / stepDeg);
				}
				if (ev.label.substr(0, 4) == "run:")
				{
					float speed = ofToFloat(ev.label.substr(4));
					// ofxModbusMotorDriver::instance().run(motorIndex,
					// 	speed / stepDeg, p_accel / stepDeg, p_decel / stepDeg);
				}
			}
		}
	}

	virtual void setJsonData(ofJson j){
        trackBase::setJsonData(j);
        if (!j["drive"].empty()) doDrive = j["drive"].get<bool>();
        if (!j["motorID"].empty()) motorIndex = j["motorID"].get<int>();
        if (!j["stepDeg"].empty()) stepDeg = j["stepDeg"].get<float>();
		
		setJsonParameter(j, "defSpeed", p_speed);
		setJsonParameter(j, "defAccel", p_accel);
		setJsonParameter(j, "defDecel", p_decel);
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
		j["defSpeed"] = p_speed;
		j["defAccel"] = p_accel;
		j["defDecel"] = p_decel;

        for (auto & p : params)
            j["params"].push_back(p->getJsonData());

        return j;
    }
};
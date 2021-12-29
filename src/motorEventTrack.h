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

	virtual void setup(string name, trackType type, bool newTrack)
	{
		cout << "called motorEvent setup" << endl;
		uniqueName = myName = name;
		myType = type;
		if (type == TRACK_MOTOREVENT)
		{
			cout << "make param"	 << endl;
			auto np = make_shared<param>();
			np->setType(PTYPE_EVENT);
			params.push_back(np);
		}

		if (newTrack) for (auto & p : params) p->addKeyPoint(0);
	}

	virtual void update(timelineState state, uint64_t const & passed, uint64_t const & duration)
	{
		if (state == STATE_PLAYING)
		{
			tm_event ev = getEventParameter(state, passed, duration, 0, -1, 2);
			if (ev.label.length() > 0)
			{
				cout << "motorEvent :" << ev.label << endl;
			}
			
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
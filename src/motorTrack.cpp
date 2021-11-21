#include "track.h"

class motorTrack : public trackBase {
public:
    int motorIndex = 0;
    bool doDrive = false;
    
    virtual void setup(string name, trackType type = TRACK_MOTOR, bool newTrack = true)
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

    virtual void update()
    {

    }
}
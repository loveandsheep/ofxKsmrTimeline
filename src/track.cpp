#include "track.h"

void trackBase::setup(string name, trackType type, bool newTrack)
{
    uniqueName = myName = name;
    myType = type;

    if (type == TRACK_FLOAT)
    {
        auto np = make_shared<param>();
        np->setType(PTYPE_FLOAT);
        params.push_back(np);
    }

    if (type == TRACK_COLOR)
    {
        auto np = make_shared<param>();
        np->setType(PTYPE_COLOR);
        params.push_back(np);
    }

    if (type == TRACK_EVENT)
    {
        auto np = make_shared<param>();
        np->setType(PTYPE_EVENT);
        params.push_back(np);
    }

    if (type == TRACK_VEC2)
    {
        auto np = make_shared<param>();
        np->setType(PTYPE_VEC2);
        params.push_back(np);
    }

    if (type == TRACK_VEC3)
    {
        auto np = make_shared<param>();
        np->setType(PTYPE_VEC3);
        params.push_back(np);
    }

    if (type == TRACK_JSONSTREAM)
    {
        auto np = make_shared<jsonParam>();
        np->setType(PTYPE_JSONSTREAM);
        params.push_back(np);
    }


    if (newTrack) for (auto & p : params) p->addKeyPoint(0);
    //TODO: タイプを増やすよ
}

ofPtr<param> trackBase::getParam(int index)
{
    if (params.size() == 0) return nullptr;
    index = ofClamp(index, 0, params.size() - 1);
    return params[index];
}

void trackBase::setJsonData(ofJson j){
    oscSend = j["oscSend"].get<bool>();
    view_height = j["view_height"].get<float>();
}

ofJson trackBase::getJsonData(){
    ofJson j;
    j["view_height"] = view_height;
    j["name"] = myName;
    j["type"] = int(myType);
    j["oscSend"] = oscSend;

    for (auto & p : params)
        j["params"].push_back(p->getJsonData());

    return j;
}
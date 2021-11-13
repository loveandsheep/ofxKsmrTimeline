#include "timeline.h"

void timeline::setup() {
    sender.setup(sendAddr, sendPort);
    receiver.setup(recvPort);
}

void timeline::update() {
    if (isPlay)
    {
        if (paused)
        {
            // started = 
            started = ofGetElapsedTimeMillis() - passed;
        }
        else
        {
            passed = ofGetElapsedTimeMillis() - started;
        }

        if (passed > duration)
        {
            sendOsc();
            if (isLoop) play();
            else stop();
        }
    }

    sendOsc();
}

void timeline::sendOsc()
{
    bool sendOn = true;
    sendOn &= isPlay && !paused;
    sendOn |= sendOnSeek && (seekOscCheck != passed);
    sendOn |= sendAlways;

    seekOscCheck = passed;

    if (sendOn)
    {
        ofxOscMessage pm;
        pm.setAddress("/time");
        pm.addIntArg(MIN(passed, duration));
        pm.addIntArg(duration);
        sender.sendMessage(pm);

        for (auto & tr : tracks)
        {
            ofxOscMessage m;
            bool sendEnable = tr->oscSend;
            m.setAddress("/" + tr->getName());
            trackType const & t = tr->getType();
            if (t == TRACK_FLOAT) m.addFloatArg(getParameter<float>(tr));
            if (t == TRACK_COLOR)
            {
                ofFloatColor c = getParameter<ofFloatColor>(tr);
                m.addFloatArg(c.r);
                m.addFloatArg(c.g);
                m.addFloatArg(c.b);
                m.addFloatArg(c.a);
            }
            if (t == TRACK_EVENT)
            {
                // FIXME: 再生開始時、直是のイベントが発火してしまう
                // FIXME: イベントが複数トラックになっていると処置しきれない
                tm_event mes = getEventParameter(tr, 0, -1, 1);

                sendEnable = false;
                if (mes.label.length() > 0)
                {
                    m.addStringArg(mes.label);
                    sendEnable = true;
                }
            }
            if (t == TRACK_VEC2)
            {
                ofVec2f val = getParameter<ofVec2f>(tr);
                m.addFloatArg(val.x);
                m.addFloatArg(val.y);
            }
            if (t == TRACK_VEC3)
            {
                ofVec3f val = getParameter<ofVec3f>(tr);
                m.addFloatArg(val.x);
                m.addFloatArg(val.y);
                m.addFloatArg(val.z);
            }

            if (sendEnable) sender.sendMessage(m);
        }
    }
}

void timeline::play()
{
    started = ofGetElapsedTimeMillis();
    isPlay = true;
    setPause(false);
}

void timeline::stop()
{
    isPlay = false;
    passed = 0;
}

void timeline::setPosition(float position)
{
    setPositionByMillis(getDuration() * ofClamp(position, 0, 1));
}

void timeline::setPositionByMillis(uint64_t time)
{
    passed = time;
    play();
    setPause(true);
}

void timeline::togglePause()
{
    setPause(!paused);
}

void timeline::setPause(bool b)
{
    paused = b;
}

ofPtr<trackBase> timeline::addTrack(string name, trackType tp, bool newTrack)
{
    auto nt = make_shared<trackBase>();
    nt->setup(name, tp, newTrack);
    tracks.push_back(nt);
    return nt;
}

void timeline::drawGui()
{
#ifdef USE_IMGUI

#endif
}

void timeline::clear()
{
    currentPath = currentFileName = "";
    tracks.clear();
}

void timeline::load(string path)
{
    clear();

    currentPath = path;
    currentFileName = ofSplitString(ofSplitString(path, "/", true, true).back(), "\\", true, true).back();

    cout << "CurrentPATH :" << currentPath << endl;
    ofJson j = ofLoadJson(path);
    j = j["timeline"];

    setDuration(j["duration"].get<int>());
    if (!j["isLoop"].empty()) isLoop = j["isLoop"].get<bool>();
    if (!j["osc"]["sendAddr"].empty()) sendAddr = j["osc"]["sendAddr"].get<string>();
    if (!j["osc"]["sendPort"].empty()) sendPort = j["osc"]["sendPort"].get<int>();
    if (!j["osc"]["recvPort"].empty()) recvPort = j["osc"]["recvPort"].get<int>();

    //トラックの追加
    for (auto & jtr : j["tracks"])
    {
        //トラックの読み込んだプロパティはここで設定 
        auto nt = addTrack(jtr["name"], trackType(jtr["type"].get<int>()), false);
        nt->oscSend = jtr["oscSend"].get<bool>();
        nt->view_height = jtr["view_height"].get<float>();

        //トラックのparamにキーポイントを追加
        //paramsの数はtrackTypeによって保証されている前提
        auto & params = nt->getParamsRef();
        for (int i = 0;i < params.size(); i++)
        {            
            if (i < jtr["params"].size())
            {
                auto & j_pr = jtr["params"][i];

                //特殊パラメータ型の処置
                if (j_pr["type"].get<int>() == int(PTYPE_JSONSTREAM))
                {
                    jsonParam* jsonpr = (jsonParam*)params[i].get();
                    jsonpr->parseJson(j_pr);
                }

                for (int j = 0;j < j_pr["keyPoints"].size();j++)
                {
                    params[i]->addKeyPoint(j_pr["keyPoints"][j]);
                    //キーポイントの該当ブロックにパラメータを設定
                    for (int o = 0;o < j_pr["blocks"].size();o++)
                    {
                        auto & j_br = j_pr["blocks"][o][j];
                        auto & b = params[i]->getBlocks(o)[j];
                        b->setInherit(j_br["inherit"].get<bool>());
                        if (!j_br["keep"].empty()) b->setKeep(j_br["keep"].get<bool>());
                        b->setFrom(j_br["from"].get<float>());
                        b->setTo(j_br["to"].get<float>());
                        b->eventName = j_br["eventName"].get<string>();
                        b->setComplement(complementType(j_br["cmplType"].get<int>()));
                        b->easeInFlag = j_br["easeIn"].get<bool>();
                        b->easeOutFlag = j_br["easeOut"].get<bool>();
                    }
                }
            }

            params[i]->refleshInherit();
        }

    }

    sender.setup(sendAddr, sendPort);
    receiver.setup(recvPort);
}

void timeline::save(string path)
{
    ofJson j;
    currentPath = path;
    currentFileName = ofSplitString(ofSplitString(path, "/", true, true).back(), "\\", true, true).back();


    ofJson & j_tm = j["timeline"];
    j_tm["duration"] = duration;
    j_tm["isLoop"] = isLoop;
    j_tm["osc"]["sendAddr"] = sendAddr;
    j_tm["osc"]["sendPort"] = sendPort;
    j_tm["osc"]["recvPort"] = recvPort;

    for (auto & t : tracks)
        j_tm["tracks"].push_back(t->getJsonData());

    ofSavePrettyJson(path, j);
}

void timeline::removeTrack(string name)
{
    auto it = tracks.begin();
    while(it != tracks.end())
    {
        if ((*it)->getName() == name) it = tracks.erase(it);
        else ++it;
    }
}
void timeline::upTrack(ofPtr<trackBase> tr)
{
    for (int i = 1;i < tracks.size();i++)
    {
        if (tracks[i] == tr)
        {
            swap(tracks[i], tracks[i-1]);
            break;
        }
    }
}

void timeline::downTrack(ofPtr<trackBase> tr)
{
    for (int i = 0;i < tracks.size()-1;i++)
    {
        if (tracks[i] == tr)
        {
            swap(tracks[i], tracks[i+1]);
            break;
        }
    }
}

void timeline::setSenderAddr(string addr)
{
    sendAddr = addr;
    sender.setup(sendAddr, sendPort);
}

void timeline::setSenderPort(int port)
{
    sendPort = port;
    sender.setup(sendAddr, sendPort);
}

void timeline::setReceiverPort(int port)
{
    recvPort = port;
    receiver.setup(recvPort);
}

tm_event timeline::getEventParameter(ofPtr<trackBase> & tr, int paramIndex, int time, int callOrigin)
{
    if (time < 0) time = passed;
    tm_event ret = tr->getParamsRef()[paramIndex]->get<tm_event>(time, duration);

    if (tr->eventCallOrigin[callOrigin] != ret.time)
    {
        tr->eventCallOrigin[callOrigin] = ret.time;

        if (ret.label != "" && !getPaused()) return ret;
    }
    return tm_event();
}
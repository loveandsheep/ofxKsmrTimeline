#include "timeline.h"

void timeline::setup() {
    sender.setup(sendAddr, sendPort);
    receiver.setup(recvPort);
    clear();
}

void timeline::sendSyncJsonData(string host, int port)
{
    if (host == "") host = getSendAddr();
    if (port == 0) port = getSendPort();

    ofxOscSender s_sync;
    s_sync.setup(host, port);

    ofJson syncJson = getJsonData();
    syncJson["settings"].erase("osc");

    string dat = syncJson.dump();

    int pieceSize = 100;

    for (int i = 0;i <= dat.length() / pieceSize ;i++)
    {
        ofxOscMessage mes;
        mes.setAddress("/json/set");
        int st = i * pieceSize;
        int ed = MIN(dat.length(), (i + 1) * pieceSize);
        mes.addStringArg(dat.substr(st, ed - st));
        mes.addIntArg(i);
        mes.addIntArg(dat.length() / pieceSize + 1);
        s_sync.sendMessage(mes);
    }

    ofxOscMessage joiner;
    joiner.setAddress("/json/set/parse");

    s_sync.sendMessage(joiner);
}

void timeline::drawMinimum(int x, int y)
{
    string info = "[timeline - " + currentFileName + "]\n";
    info += "status :";
    if (currentState == STATE_IDLE) info += "IDLE\n";
    if (currentState == STATE_PLAYING) info += "PLAY\n";
    if (currentState == STATE_PAUSE) info += "PAUSE\n";
    if (currentState == STATE_FINISHED) info += "FINISHED\n";

    info += "time     :" + ofToString(getPassed()) + "\n";
    info += "duration :" + ofToString(getDuration()) + "\n";

    ofDrawBitmapStringHighlight(info, x, y);
}

timelineState const & timeline::update() {
    
    while (receiver.hasWaitingMessages())
    {
        ofxOscMessage m;
        receiver.getNextMessage(m);
        cout << "received :" << m.getAddress() << endl;
        if (m.getAddress().find("/track") != string::npos)
        {
            for (auto & tr : getTracks())
            {
                tr->controlMessage(m, getPassed(), getDuration());
            }
        }

        if (m.getAddress() == "/error")
        {
            lastLog = "err :" + m.getArgAsString(0);
        }
        if (m.getAddress() == "/log")
        {
            lastLog = "log :" + m.getArgAsString(0);
        }

        //JSONデータの読み出し
        if (m.getAddress() == "/json/set")
        {
            json_piece.resize(m.getArgAsInt(2));
            json_piece[m.getArgAsInt(1)] = m.getArgAsString(0);
        }

        if (m.getAddress() == "/json/set/parse")
        {
            string d = "";
            for (auto & jp : json_piece)
            {
                d += jp;
            }
            setFromJson(ofJson::parse(d));
        }

        if (m.getAddress() == "/json/save")
        {
            save(m.getArgAsString(0));
        }

        // ip-Syncモードで送るsyncシグナル
        // 現在時刻その他の情報を返す
        if (m.getAddress() == "/sync")
        {            
            ofxOscSender repSender;
            ofxOscMessage reply;
            repSender.setup(m.getRemoteHost(), sendPort);
            reply.setAddress("/return/sync");
            reply.addIntArg(getPassed());
            repSender.sendMessage(reply);
        }

        if (m.getAddress() == "/json/get")
        {
            sendSyncJsonData(m.getRemoteHost(), m.getArgAsInt(0));
            cout << "Send to :" << m.getRemoteHost() << "::" << m.getArgAsInt(0) << endl;
        } 

        if (m.getAddress() == "/return/json/get")
            setFromJson(ofJson::parse(m.getArgAsString(0)));

        if (m.getAddress() == "/play")      play();
        if (m.getAddress() == "/stop")      stop();
        if (m.getAddress() == "/seek")      setPositionByMillis(m.getArgAsInt64(0));
        if (m.getAddress() == "/pause")     setPause(m.getArgAsBool(0));
        if (m.getAddress() == "/chapter")   setChapter(m.getArgAsInt(0));

    }

    timelineState prev = currentState;
    currentState = STATE_IDLE;

    if (isPlay)
    {
        if (paused)
        {
            currentState = STATE_PAUSE;
            started = ofGetElapsedTimeMillis() - passed;
        }
        else
        {
            currentState = STATE_PLAYING;
            if (prev == STATE_FINISHED) currentState = STATE_LOOPBACK;
            passed = ofGetElapsedTimeMillis() - started;
        }

        if (passed > getDuration())
        {
            sendOsc();
            if (getIsLoop()) play();
            else stop();
            currentState = STATE_FINISHED;
        }
    }

    sendOsc();

    for (auto & tr : getTracks()) 
    {
        tr->update(currentState, getPassed(), getDuration());
        edited |= tr->checkEdited();
    }
    return currentState;
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
        pm.addIntArg(MIN(getPassed(), getDuration()));
        pm.addIntArg(getDuration());
        sender.sendMessage(pm);

        for (auto & tr : getCurrentChapter()->tracks)
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

    ofNotifyEvent(ev_play, getTimelineEvArg());
}

void timeline::stop()
{
    isPlay = false;
    passed = 0;

    ofNotifyEvent(ev_stop, getTimelineEvArg());

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

    ofNotifyEvent(ev_seek, getTimelineEvArg());
}

void timeline::togglePause()
{
    setPause(!paused);
}

void timeline::setPause(bool b)
{
    paused = b;

    ofNotifyEvent(ev_pause, getTimelineEvArg());
}

timelineEventArgs & timeline::getTimelineEvArg()
{
    eventArg.time = getPassed();
    eventArg.paused = paused;
    eventArg.chapterIndex = currentChapterIndex;
    return eventArg;
}

ofPtr<trackBase> timeline::addTrack(string name, trackType tp, bool newTrack)
{
    ofPtr<trackBase> nt;
    if (tp == TRACK_MOTOR)
    {
        nt = make_shared<motorTrack>();
        nt->setup(name, tp, newTrack);
        getCurrentChapter()->tracks.push_back(nt);
    }
    else
    {
        nt = make_shared<trackBase>();
        nt->setup(name, tp, newTrack);
        getCurrentChapter()->tracks.push_back(nt);
    }

    return nt;
}

void timeline::drawGui()
{
#ifdef USE_IMGUI

#endif
}

void timeline::clear(bool completely)
{
    //completely = trueの場合、チャプターが0なのですぐにチャプターを作成する必要がある
    currentPath = currentFileName = "";
    clearChapter(completely);
}

void timeline::load(string path)
{
    setFromJson(ofLoadJson(path));
    currentPath = path;
    currentFileName = ofSplitString(ofSplitString(path, "/", true, true).back(), "\\", true, true).back();
}

void timeline::setFromJson(ofJson j)
{
    if (j.empty())
    {
        clear(false);
        return;
    }

    clear(true);
    ofJson j_tm = j["timeline"];

    //タイムライン全体設定
    if (!j["settings"].empty())
    {
        if (!j["settings"]["osc"]["sendAddr"].empty()) sendAddr = j["settings"]["osc"]["sendAddr"].get<string>();
        if (!j["settings"]["osc"]["sendPort"].empty()) sendPort = j["settings"]["osc"]["sendPort"].get<int>();
        if (!j["settings"]["osc"]["recvPort"].empty()) recvPort = j["settings"]["osc"]["recvPort"].get<int>();
    }

    // チャプターの追加
    for (int ts = 0;ts < j_tm.size();ts++)
    {
        //====================================================旧式（チャプター無し時代）の処理
        if (j_tm.type() == nlohmann::detail::value_t::object)
        {
            if (ts > 0) break;
            j = j_tm;
            if (!j["osc"]["sendAddr"].empty()) sendAddr = j["osc"]["sendAddr"].get<string>();
            if (!j["osc"]["sendPort"].empty()) sendPort = j["osc"]["sendPort"].get<int>();
            if (!j["osc"]["recvPort"].empty()) recvPort = j["osc"]["recvPort"].get<int>();
        }
        if (j_tm.type() == nlohmann::detail::value_t::array)  j = j_tm[ts];

        string chapterName = "";
        if (!j["chapterName"].empty()) chapterName = j["chapterName"].get<string>();
        createChapter(chapterName, j["duration"].get<int>());

        setDuration(j["duration"].get<int>());
        if (!j["isLoop"].empty()) setIsLoop(j["isLoop"].get<bool>());

        //トラックの追加
        for (auto & jtr : j["tracks"])
        {
            //トラックの読み込んだプロパティはここで設定 
            auto nt = addTrack(jtr["name"], trackType(jtr["type"].get<int>()), false);
            nt->setJsonData(jtr);

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
                            if (!j_br["label"].empty()) b->label = j_br["label"].get<string>();
                            if (!j_br["accel"].empty()) b->accel = j_br["accel"].get<float>();
                            if (!j_br["decel"].empty()) b->decel = j_br["decel"].get<float>();
                        }
                    }
                }
                params[i]->refleshInherit();
            }
        }//for (auto & jtr : j["tracks"])

    }// for (int ts = 0;ts < j_tm.size();ts++)

    sender.setup(sendAddr, sendPort);
    receiver.setup(recvPort);
}

void timeline::save(string path)
{
    ofSavePrettyJson(path, getJsonData());
    currentPath = path;
    currentFileName = ofSplitString(ofSplitString(path, "/", true, true).back(), "\\", true, true).back();
}

ofJson timeline::getJsonData()
{
    ofJson j;
    ofJson & j_st = j["settings"];
    ofJson & j_tm = j["timeline"];
    
    j_st["osc"]["sendAddr"] = sendAddr;
    j_st["osc"]["sendPort"] = sendPort;
    j_st["osc"]["recvPort"] = recvPort;

    for (auto & c : chapters)
    {
        ofJson chj;
        cout << "Duration :" << c->name << "::" << c->duration << endl;
        chj["duration"] = c->duration;
        chj["isLoop"] = c->isLoop;
        chj["chapterName"] = c->name;

        for (auto & t : c->tracks) chj["tracks"].push_back(t->getJsonData());
        j_tm.push_back(chj);
    }

    return j;
}

void timeline::removeTrack(string name)
{
    auto it = getCurrentChapter()->tracks.begin();
    while(it != getCurrentChapter()->tracks.end())
    {
        if ((*it)->getName() == name) it = getCurrentChapter()->tracks.erase(it);
        else ++it;
    }
}
void timeline::upTrack(ofPtr<trackBase> tr)
{
    for (int i = 1;i < getCurrentChapter()->tracks.size();i++)
    {
        if (getCurrentChapter()->tracks[i] == tr)
        {
            swap(getCurrentChapter()->tracks[i], getCurrentChapter()->tracks[i-1]);
            break;
        }
    }
}

void timeline::downTrack(ofPtr<trackBase> tr)
{
    for (int i = 0;i < getCurrentChapter()->tracks.size()-1;i++)
    {
        if (getCurrentChapter()->tracks[i] == tr)
        {
            swap(getCurrentChapter()->tracks[i], getCurrentChapter()->tracks[i+1]);
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
    tm_event ret = tr->getParamsRef()[paramIndex]->get<tm_event>(time, getDuration());

    if (tr->eventCallOrigin[callOrigin] != ret.time)
    {
        tr->eventCallOrigin[callOrigin] = ret.time;

        if (ret.label != "" && !getPaused()) return ret;
    }
    return tm_event();
}

void timeline::createChapter(string name, uint64_t duration, bool setToCurrent)
{
    auto ch = make_shared<chapter>();
    if (name == "") name = "chapter-" + ofToString(chapters.size());
    ch->name = name;
    ch->duration = duration;
    chapters.push_back(ch);
    if (setToCurrent) setChapter(chapters.size() - 1);
}

void timeline::setChapter(string name)
{
    for (int i = 0;i < chapters.size();i++)
    {
        if (chapters[i]->name == name) setChapter(i);
    }
}

void timeline::setChapter(int index)
{
    if (index < 0 || chapters.size() <= index) return;
    currentChapterIndex = index;
    ofNotifyEvent(ev_chapter, getTimelineEvArg());
}

void timeline::removeChapter(int index)
{
    if (index < 0 || chapters.size() <= index) return;
    removeChapter(chapters[index]->name);
}

void timeline::removeChapter(string name)
{
    auto it = chapters.begin();
    while (it != chapters.end())
    {
        if ((*it)->name == name) it = chapters.erase(it);
        else ++it;
    }
}

void timeline::clearChapter(bool completely)
{
    for (auto & c : chapters) c.reset();
    chapters.clear();
    
    if (!completely)
    {
        createChapter("chapter-0", 3000);
    }
}

vector<string> timeline::getChapterNames()
{
    vector<string> ret;
    for (auto & c : chapters) ret.push_back(c->name);
    return ret;
}
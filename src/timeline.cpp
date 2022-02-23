#include "timeline.h"

void timeline::setup() {
    sender.setup(sendAddr, sendPort);
    receiver.setup(recvPort);
    syncRecv.setup(syncPort);
    clear();
}

void timeline::sendSyncJsonData(string host, int port)
{
    if (host == "") host = getSendAddr();
    if (port == 0) port = getSendPort();

    ofxOscSender s_sync;
    s_sync.setup(host, syncPort);

    ofJson syncJson = getJsonData();
    syncJson["settings"].erase("osc");

    string  dat = syncJson.dump();
    int     pieceSize = 100;

    for (int i = 0;i <= dat.length() / pieceSize ;i++)
    {
        ofxOscMessage mes;
        mes.setAddress("/json/set");
        int st = i * pieceSize;
        int ed = MIN(dat.length(), (i + 1) * pieceSize);
        
        mes.addStringArg(dat.substr(st, ed - st)); //JSONデータ
        mes.addIntArg(i); //データインデックス
        mes.addIntArg(dat.length() / pieceSize + 1); //総データ数

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
        receivedMessage(m);
    }

    while(syncRecv.hasWaitingMessages())
    {
        ofxOscMessage m;
        syncRecv.getNextMessage(m);
        receivedMessage(m);
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

        if (tr->lastLog.length() > 0)
        {
            stringstream ss;
            ss << tr->getName() << "::" << tr->lastLog;
            sendLog(tr->logHost, ss.str());
            tr->lastLog = "";
        }
    }
    return currentState;
}

void timeline::receivedMessage(ofxOscMessage & m)
{
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
        lastLogRemoteHost = m.getRemoteHost();
    }
    if (m.getAddress() == "/log")
    {
        lastLog = "log :" + m.getArgAsString(0);
        lastLogRemoteHost = m.getRemoteHost();
    }

    //JSONデータの読み出し
    if (m.getAddress() == "/json/set")
    {
        json_piece.resize(m.getArgAsInt(2));
        json_piece[m.getArgAsInt(1)] = m.getArgAsString(0);
        if (m.getArgAsInt(1) == 0) parseCounter = 0;
        parseCounter++;
    }

    if (m.getAddress() == "/json/set/parse")
    {
        string d = "";
        for (auto & jp : json_piece)
        {
            d += jp;
        }
        bool parseSuccess = true;
        
        try
        {
            setFromJson(ofJson::parse(d));
        }
        catch(const std::exception & e)
        {
            sendLog(m.getRemoteHost(), "[sync] sync data error.");
            stringstream ss;
            ss << "[error] data Parsing failed. : " << parseCounter << "/" << json_piece.size();
            lastLog = ss.str();
            parseSuccess = false;
        }
        if (parseSuccess)
        {
            stringstream ss;
            ss << "[sync] sync data success. " << parseCounter << "/" << json_piece.size() << " piece merged." << "\nHash : 0x" << hex << crc16(d.c_str(), d.length());
            sendLog(m.getRemoteHost(), ss.str());
        }

        parseCounter = 0;
    }

    if (m.getAddress() == "/json/save")
    {
        save(m.getArgAsString(0));
        sendLog(m.getRemoteHost(), "[sync] save remote 'main.json'");
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
    } 

    if (m.getAddress() == "/play")
    {
        play();
        if (m.getArgAsString(0) != getCurrentChapter()->name) 
            setChapter(m.getArgAsString(0));

        sendLog(m.getRemoteHost(), "[sync] play.");
    }
    if (m.getAddress() == "/stop")
    {
        stop();
        sendLog(m.getRemoteHost(), "[sync] stop.");
    }
    if (m.getAddress() == "/seek")
    {
        if (m.getArgAsString(1) != getCurrentChapter()->name) 
            setChapter(m.getArgAsString(1));

        setPositionByMillis(m.getArgAsInt64(0));
        sendLog(m.getRemoteHost(), "[sync] seek to :" + ofToString(m.getArgAsInt64(0)));
    }
    if (m.getAddress() == "/pause")
    {
        bool b = m.getArgAsBool(0);
        setPause(b);

        if (m.getArgAsString(1) != getCurrentChapter()->name) 
            setChapter(m.getArgAsString(1));

        sendLog(m.getRemoteHost(), string("[sync] pause :") + (b ? "ON" : "OFF"));
    }
    if (m.getAddress() == "/chapter") 
    {
        int ci = m.getArgAsInt(0);
        if (ci < chapters.size())
        {
            setChapter(ci);
            sendLog(m.getRemoteHost(), "[sync] chapter :" + chapters[ci]->name);
        }
        else
        {
            sendError(m.getRemoteHost(), "[sync] chapter size error");
        }
    } 
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
            if (t == TRACK_MOTOR) m.addFloatArg(getParameter<float>(tr));
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
    ofLogNotice() << ofGetTimestampString("[%H:%M:%S]") << "Play at " << getCurrentChapter()->name;
}

void timeline::stop()
{
    isPlay = false;
    passed = 0;

    ofNotifyEvent(ev_stop, getTimelineEvArg());
    ofLogNotice() << ofGetTimestampString("[%H:%M:%S]") << "stop at " << getCurrentChapter()->name;
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
    if (b)
        ofLogNotice() << ofGetTimestampString("[%H:%M:%S]") << "Paused at " << getCurrentChapter()->name;
    else
        ofLogNotice() << ofGetTimestampString("[%H:%M:%S]") << "Play at " << getCurrentChapter()->name;
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

    if (tp == TRACK_MOTOR) nt = make_shared<motorTrack>();
    else if (tp == TRACK_MOTOREVENT) nt = make_shared<motorEventTrack>();
    else nt = make_shared<trackBase>();

    nt->setup(name, tp, newTrack);
    getCurrentChapter()->tracks.push_back(nt);
    
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

void timeline::importChapter(string path)
{
    ofJson j, js = ofLoadJson(path);
    ofJson j_tm = js["timeline"];

    for (int ts = 0;ts < j_tm.size();ts++)
    {
        if (j_tm.type() == nlohmann::detail::value_t::array)  j = j_tm[ts];
        createChapterFromJson(j);
    }
}

void timeline::createChapterFromJson(ofJson j)
{
    string chapterName = "";
    string suffix = "";
    if (!j["chapterName"].empty()) chapterName = j["chapterName"].get<string>();

    bool namePassed = false;
    int nameCounter = 0;

    while (!namePassed)
    {
        nameCounter++;
        if (nameCounter > 1) suffix = "_" + ofToString(nameCounter);
        namePassed = true;

        for (int i = 0;i < getChapterSize();i++)
        {
            if (getChapter(i)->name == chapterName + suffix)
            {
                namePassed = false;
            }
        }
    }

    chapterName = chapterName + suffix;

    createChapter(chapterName, j["duration"].get<int>());

    setDuration(j["duration"].get<int>());
    if (!j["isLoop"].empty()) setIsLoop(j["isLoop"].get<bool>());
    if (!j["bgColor"].empty())
    {
        getCurrentChapter()->bgColor.r = j["bgColor"][0];
        getCurrentChapter()->bgColor.g = j["bgColor"][1];
        getCurrentChapter()->bgColor.b = j["bgColor"][2];
    }


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
                        b->setFromJson(j_br);
                    }
                }
            }
            params[i]->refleshInherit();
        }
    }//for (auto & jtr : j["tracks"])
}

void timeline::duplicateChapter(int index)
{
    if (index < 0 || index > chapters.size()) return;
    ofJson j = getChapter(index)->getJsonData();
    j["chapterName"] = j["chapterName"].get<string>() + "_copy";
    createChapterFromJson(j);
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

        createChapterFromJson(j);

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
        ofJson chj = c->getJsonData();
        j_tm.push_back(chj);
    }

    ofJson jHash = j;
    jHash["settings"].erase("osc");
    string jhs = jHash.dump();
    jsonHash = crc16(jhs.c_str(), jhs.length());

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

    ofLogNotice() << ofGetTimestampString("[%H:%M:%S]") << "setChapter " << getCurrentChapter()->name;
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

void timeline::sendLog(string host, string message)
{
    ofxOscSender sender;
    ofxOscMessage log;

    log.setAddress("/log");
    log.addStringArg(message);

    sender.setup(host, syncPort);
    sender.sendMessage(log);
}

void timeline::sendError(string host, string message)
{
    ofxOscSender sender;
    ofxOscMessage log;

    log.setAddress("/error");
    log.addStringArg(message);

    sender.setup(host, syncPort);
    sender.sendMessage(log);
}

uint16_t timeline::crc16(const char* data, int numByte)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0;i < numByte;i++)
    {
        crc ^= data[i];
        for (int j = 0;j < 8;j++)
        {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}
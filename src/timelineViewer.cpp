#include "timelineViewer.h"

#if defined(TARGET_WIN32)

void timelineViewer::setup(ofPtr<timeline> tmPtr) {
    tm = tmPtr;
    view_end = tm->getDuration();

    ofAddListener(ofEvents().keyPressed, this, &timelineViewer::keyPressed);
    ofAddListener(ofEvents().keyReleased, this, &timelineViewer::keyReleased);

    ofAddListener(ofEvents().mouseMoved, this, &timelineViewer::mouseMoved);
    ofAddListener(ofEvents().mousePressed, this, &timelineViewer::mousePressed);
    ofAddListener(ofEvents().mouseDragged, this, &timelineViewer::mouseDragged);
    ofAddListener(ofEvents().mouseReleased, this, &timelineViewer::mouseReleased);
    ofAddListener(ofEvents().mouseScrolled, this, &timelineViewer::mouseScrolled);

    //inputTextが変更される度にsender::setupが発火するとまいっちゃうのでこういう風になってる
    strcpy(gui_oscAddrInput, tm->getSendAddr().c_str());
}

void timelineViewer::update() {
    
}

void timelineViewer::draw(ofRectangle area) {
    if (ofGetElapsedTimeMillis() - backupTimer > backupInterval * 1000)
    {
        backupTimer = ofGetElapsedTimeMillis();
        ofSavePrettyJson("backup.json", tm->getJsonData());
    }

    ofPushStyle();
    ofPushMatrix();
    
    float heightSum = peek_height + 30;
    for (auto & tr : tm->getTracks())
        heightSum += (tr->fold ? foldedHeight : tr->view_height) + 5;
    
    area.height = heightSum;

    ofSetColor(tm->getCurrentChapter()->bgColor);
    ofDrawRectangle(area);
    isEditorHovered = area.inside(ofGetMouseX(), ofGetMouseY());
    
    //Brush描画など
    seekLeft = area.x + 180;
    seekWidth = MAX(100, area.getWidth() - 180);
    drawPeek(ofRectangle(seekLeft, area.y, seekWidth, peek_height));
    drawBrush(ofRectangle(seekLeft, area.y + peek_height, seekWidth, 15));

    ofRectangle track_area = ofRectangle(area.x + margin, area.y + peek_height + 30, area.width - margin, 0);

    auto & trs = tm->getTracks();

    //trackBaseの描画
    peekHeight.reset();
    for (int i = 0;i < trs.size(); i++)
    {
        //ビューの高さ調整
        ofRectangle peekArea = track_area;
        peekArea.y += trs[i]->fold ? foldedHeight : trs[i]->view_height;
        peekArea.setHeight(5);
        if (peekArea.inside(ofGetMouseX(), ofGetMouseY()))
        {
            peekHeight = trs[i];
            ofSetHexColor(palette::orange[0]);
            ofDrawRectangle(peekArea);
        }

        //trackBaseの描画本体
        ImGui::PushID(300 + i);
        track_area.y += drawTrack(trs[i], track_area, view_begin, view_end) + 5;
        ImGui::PopID();
    }

    //シークバー描画
    ofSetColor(150);
    float seekPoint = ofMap(tm->getPassed(), view_begin, view_end, seekLeft, seekLeft + seekWidth);
    ofDrawLine(seekPoint, 0, seekPoint, area.height);

    if (snaped >= 0)
    {
        ofSetColor(255,255,0,150);
        float snapPoint = ofMap(snaped, view_begin, view_end, seekLeft, seekLeft + seekWidth);
        ofDrawLine(snapPoint, 0, snapPoint, area.height);
    }

    ofPopMatrix();
    ofPopStyle();
}

void timelineViewer::drawBrush(ofRectangle area)
{
    ofPushStyle();
    ofPushMatrix();
    {
        ofSetHexColor(palette::gray[0]);
        ofDrawRectangle(seekLeft, area.y, seekWidth, area.getHeight());
        float beginPos = ofMap(view_begin, 0, tm->getDuration(), seekLeft, seekLeft + seekWidth); 
        float endPos = ofMap(view_end, 0, tm->getDuration(), seekLeft, seekLeft + seekWidth);
        
        ofSetHexColor(palette::green[1]);
        ofDrawRectangle(beginPos, area.y, endPos - beginPos, area.getHeight());
        ofSetHexColor(palette::orange[1]);
        ofDrawRectangle(beginPos, area.y, 5, area.getHeight());
        ofDrawRectangle(endPos, area.y, -5, area.getHeight());
    }
    ofPopMatrix();
    ofPopStyle();

    if (ofGetMousePressed() && (mouseLock == "free" || mouseLock == "brush"))
    {
        
        if (area.inside(ofGetMouseX(), ofGetMouseY())) mouseLock = "brush";
        
        if (mouseLock == "brush")
        {
            float timedX = ofMap(ofGetMouseX(), seekLeft, seekLeft + seekWidth, 0, tm->getDuration());
            if (abs(view_begin - timedX) < abs(view_end - timedX))
            {
                view_begin = MAX(0, timedX);
            }
            else
            {
                view_end = MIN(tm->getDuration(), timedX);
            }
        }
    }
    else if (mouseLock == "brush") mouseLock = "free";
}

void timelineViewer::drawPeek(ofRectangle area)
{
    if (tm->getDuration() < view_end) zoomOut();

    ofPushStyle();
    ofPushMatrix();
    ofTranslate(area.getPosition());
    ofSetHexColor(0x41484F);
    ofDrawRectangle(0, 0, area.getWidth(), area.getHeight());
    vector<ofVec2f> pts;
    for (int i = 0; i < area.getWidth(); i+=10)
    {
        pts.push_back(ofVec2f(i, area.getHeight() - 5));
        pts.push_back(ofVec2f(i, area.getHeight()));
    }

    ofSetHexColor(0x768491);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &pts[0]);
    glDrawArrays(GL_LINES, 0, pts.size());
    glDisableClientState(GL_VERTEX_ARRAY);

    if (ofGetMousePressed() && (mouseLock == "free" || mouseLock == "peek"))
    {
        ofVec2f mp = ofVec2f(ofGetMouseX(), ofGetMouseY());
        
        if (area.inside(mp)) mouseLock = "peek";
        
        if (mouseLock == "peek")
        {
            uint64_t targTime = MIN(view_end, MAX(view_begin, ofMap(mp.x, seekLeft, seekLeft + seekWidth, view_begin, view_end)));
            tm->setPositionByMillis(targTime);
            seekVideo(tm->getPassed());
        }
    }
    else if (mouseLock == "peek") mouseLock = "free";

    ofPopMatrix();
    ofPopStyle();
}

float timelineViewer::drawTrack(ofPtr<trackBase> tr, ofRectangle area, uint64_t begin, uint64_t end)
{
    ofPushStyle();
    ofPushMatrix();

    area.height = tr->fold ? foldedHeight : tr->view_height;

    ofSetHexColor(0x394150);
    ofDrawRectangle(area);

    auto & prms = tr->getParamsRef();
    ofRectangle param_area = ofRectangle(seekLeft, area.y + 3, seekWidth, area.height - 5);

    for (int i = 0;i < prms.size();i++)
    {
        param_area.y += drawParam(prms[i], param_area, begin, end) + 3;
    }

    ofPopMatrix();
    ofPopStyle();

    bool open = true;
    int tpNum = int(tr->getType());
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ofFloatColor::fromHsb(tpNum / 20.0, tpNum % 2 == 0 ? 0.5 : 0.3, tpNum % 2 == 0 ? 0.3 : 0.2, 1.0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(10, 10));
    ImGui::Begin(tr->getUniqueName().c_str(), &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowPos(ImVec2(area.x, area.y));
    ImGui::SetWindowSize(ImVec2(seekLeft - area.x, area.height));
    
    char val[1024] = {0};
    if (tr->getType() == TRACK_FLOAT) 
    {
        sprintf(val, "[%-8.2f]", tr->getParam(0)->getFloat(tm->getPassed(), tm->getDuration()));
    }

    if (tr->getType() == TRACK_VEC2)
    {
        ofVec2f v = tr->getParam(0)->getVec2f(tm->getPassed(), tm->getDuration());
        sprintf(val, "[%-8.2f, %-8.2f]", v[0], v[1]);
    }

    if (tr->getType() == TRACK_VEC3)
    {
        ofVec3f v = tr->getParam(0)->getVec3f(tm->getPassed(), tm->getDuration());
        sprintf(val, "[%-8.2f, %-8.2f, %-8.2f]", v[0], v[1], v[2]);
    }

    if (tr->getType() == TRACK_JSONSTREAM)
    {
        sprintf(val, "[%-8.2f]", tr->getParam(0)->getFloat(tm->getPassed(), tm->getDuration()));
    }

    if (tr->getType() == TRACK_MOTOR)
    {
        sprintf(val, "[%-8.2f]", tr->getParam(0)->getFloat(tm->getPassed(), tm->getDuration()));
    }

    if (tr->fold)
    {
        if (ImGui::Button("O")) tr->fold = false;
        ImGui::SameLine();
        string tex = tr->getName() + ":" + string(val);
        ImGui::Text(tex.c_str());
    }
    else
    {
        if (ImGui::Button("-")) tr->fold = true;
        ImGui::SameLine();
        if (ImGui::Button("x")) removeTrack(tr);
        ImGui::SameLine();
        if (ImGui::Button("Up")) tm->upTrack(tr);
        ImGui::SameLine();
        if (ImGui::Button("Down")) tm->downTrack(tr);
        ImGui::SameLine();
        ImGui::Checkbox("OSC", &tr->oscSend);
        
        if (tr->getType() == TRACK_JSONSTREAM)
        {
            jsonParam* jtr = (jsonParam*)tr->getParam(0).get();
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                ofFileDialogResult result = ofSystemLoadDialog("load", false, "./bin/data");        
                if (result.bSuccess)
                {
                    jtr->load(result.getPath());
                }
            }
            if (jtr->keyNames.size() > 0)
            {
                if (combo("Key", &jtr->keyIndex, jtr->keyNames))
                {
                    jtr->calcParameters(jtr->keyNames[jtr->keyIndex]);
                }
            }
        }

        strcpy(gui_trackInput, tr->getName().substr(0, numEventText).c_str());
        if (ImGui::InputText("Name", gui_trackInput, numTrackText))
        {
            tr->setName(string(gui_trackInput));
        }
        ImGui::Text(val);

        if (tr->getType() == TRACK_FLOAT)
        {
            if (ImGui::Button("ConvMotor"))
            {
                string name = tr->getName();
                tr->setName(name + "conv");
                ofPtr<trackBase> nt = tm->addTrack(name, TRACK_MOTOR);
                motorTrack* trPtr = (motorTrack*)nt.get();

                vector<ofPtr<block> > & bls = tr->getParamsRef()[0]->getBlocks(0);

                for (int i = 0;i < bls.size();i++)
                {
                    uint64_t kp = tr->getParamsRef()[0]->getKeyPoints()[i];
                    if (kp != 0) trPtr->getParamsRef()[0]->addKeyPoint(kp);
                    auto b = trPtr->getParamsRef()[0]->getBlocks(0)[i];
                    b->setFrom(bls[i]->getFrom());
                    b->setTo(bls[i]->getTo());
                    b->setComplement(CMPL_RAMP);
                    b->accel = 0.05;
                    b->decel = 0.05;
                    b->setKeep(bls[i]->getKeep());
                    b->label = bls[i]->label;
                }
                trPtr->stepDeg = 0.0036;
                removeTrack(tr);
            }
        }

        if (tr->getType() == TRACK_MOTOR)
        {
            motorTrack* trPtr = (motorTrack*)tr.get();
            if (ImGui::InputInt("ID", &trPtr->motorIndex)) trPtr->edited = true;
            if (ImGui::InputFloat("Deg/Step", &trPtr->stepDeg, 0.01, 0.1, "%.4f")) trPtr->edited = true;
            if (ImGui::Button("Standby"))
            {
                trPtr->motorStandby(tm->getPassed());
                ofxOscMessage mes;
                mes.setAddress("/track/standby");
                mes.addStringArg(tr->getName());
                tm->getSender().sendMessage(mes);
            }ImGui::SameLine();
            if (ImGui::Button("Preset"))
            {
                ofxOscMessage mes;
                mes.setAddress("/track/preset");
                mes.addStringArg(tr->getName());
                tm->getSender().sendMessage(mes);
            }
            ImGui::SameLine();
            if (ImGui::Button("ALMRST"))
            {
                trPtr->resetAlarm();
            }
            if (ImGui::Button(" - "))
            {
                ofxOscMessage mes;
                mes.setAddress("/track/jog/fw");
                mes.addStringArg(tr->getName());
                tm->getSender().sendMessage(mes);
            }ImGui::SameLine();
            if (ImGui::Button("STOP"))
            {
                ofxOscMessage mes;
                mes.setAddress("/track/jog/stop");
                mes.addStringArg(tr->getName());
                tm->getSender().sendMessage(mes);
            }ImGui::SameLine();
            if (ImGui::Button(" + "))
            {
                ofxOscMessage mes;
                mes.setAddress("/track/jog/rv");
                mes.addStringArg(tr->getName());
                tm->getSender().sendMessage(mes);
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Drive", &trPtr->doDrive)) trPtr->edited = true;
            if (ImGui::Checkbox("Spin", &trPtr->spin)) trPtr->edited = true;
        }

    }
    


    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    return area.height;
}

float timelineViewer::drawParam(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end)
{
    float waving = (sin(ofGetElapsedTimeMillis() / 180.0) + 1) / 2;
    //drawParamにおけるareaはseekLeft~seekWidthの範囲内
    ofPushStyle();
    ofSetHexColor(pr == hoverParam ? 0x4d565e : 0x41484F);
    ofDrawRectangle(area);
    ofPushMatrix();
    ofTranslate(area.getPosition());

    //スナップガイドの描画
    if (doSnap)
    {
        int snpW = getCurrentSnapRange();
        for (int i = int(view_begin / snpW) * snpW;i < view_end; i+=snpW) 
        {
            float drawX = ofMap(i, view_begin, view_end, 0, seekWidth);
            ofSetColor(20);
            ofDrawLine(drawX, 0, drawX, area.height);
        }
    }

    //各パラメータ種別に合わせた描画
    if (pr->getType() == PTYPE_FLOAT) drawParam_float(pr, area, begin, end);
    if (pr->getType() == PTYPE_JSONSTREAM) drawParam_json(pr, area, begin, end);
    if (pr->getType() == PTYPE_COLOR) drawParam_color(pr, area, begin, end);
    if (pr->getType() == PTYPE_EVENT) drawParam_event(pr, area, begin, end);
    if (pr->getType() == PTYPE_VEC3) drawParam_vector(pr, area, begin, end);
    if (pr->getType() == PTYPE_VEC2) drawParam_vector(pr, area, begin, end);

    auto & kp = pr->getKeyPoints();
    auto & bl = pr->getBlocks(0);
    pr->resetMaxMinRange();

    //ブロックの端描画とホバー判定
    for (int i = 0;i < kp.size();i++)
    {
        bool isInside = true;
        isInside &= kp[i] <= end;
        if (i < kp.size() - 1) isInside &= begin < kp[i + 1];
        if (isInside)
        {
            //ホバー判定領域を計算
            ofRectangle hb;
            hb.x = MAX(seekLeft, ofMap(kp[i], begin, end, area.getLeft(), area.getRight()));
            hb.y = area.y;
            hb.width = area .getRight() - hb.x;
            if (i < kp.size() - 1) hb.width = ofMap(kp[i+1], begin, end, area.getLeft(), area.getRight()) - hb.x;
            hb.height = area.height;
            
            bool blockSelected = false;
            for (auto & b : selBlocks) blockSelected |= bl[i] == b;

            ofEnableBlendMode(OF_BLENDMODE_ADD);
            //選択の明滅
            if (blockSelected)
            {
                ofColor c = ofColor::fromHex(0x2e0310);
                c.setBrightness(waving * 80);
                ofSetColor(c);
                ofDrawRectangle(hb - area.getPosition());
            }

            // ホバー中の描画
            if (hoverBlock == bl[i])
            {
                ofSetHexColor(0x032e1c);
                ofDrawRectangle(hb - area.getPosition());
                hoverBlock.reset();                
            }
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            if (bl[i]->getKeep())
            {
                ofSetColor(0, 0, 0, 70);
                ofDrawRectangle(hb - area.getPosition());
            }
            if (bl[i]->label.length() > 0)
            {
                ofSetColor(0, 150, 50, 70);
                ofDrawRectangle(hb - area.getPosition());
            }


            if (hb.inside(ofGetMouseX(), ofGetMouseY())) hoverBlock = bl[i];

            if (pr->getType() == PTYPE_FLOAT)
            {
                //終点がはみ出してなければ、to点を描画
                bool dotInside = i < kp.size() - 1 && begin < kp[i + 1];//次のブロックがビューの中にいる
                dotInside &= blockSelected;//選択されている
                if (dotInside)
                {
                    ofSetHexColor(palette::orange[0]);
                    ofRectangle dotRect;
                    float size = 3 + 15 * waving;
                    dotRect.setFromCenter(
                        ofMap(kp[i + 1], view_begin, view_end, seekLeft, seekLeft + seekWidth) - area.x,
                        ofMap(pr->getFloat(kp[i + 1], tm->getDuration()), pr->getValueMin(), pr->getValueMax(), area.height, 0),
                        size, size
                    );
                    ofDrawRectangle(dotRect);
                }
            }

            //ブロック始点の縦線
            if (begin < kp[i] && kp[i] < end)
            {
                float xPos = ofMap(kp[i], begin, end, 0, area.width);
                ofSetHexColor(blockSelected ? palette::orange[0] : 0xFFFFFF);
                ofDrawLine(xPos, 0, xPos, area.height);
                if (blockSelected)
                {
                    ofSetColor(255);
                    string kpS = "key: " + ofToString(kp[i]) + " ms";
                    ofDrawBitmapStringHighlight(kpS, xPos + 5, area.height - 10);
                }
            }
        }
    }

    ofPopMatrix();
    ofPopStyle();

    //マウスのホバー判定
    if (pr == hoverParam) hoverParam.reset();
    if (area.inside(ofGetMouseX(), ofGetMouseY())) hoverParam = pr;

    return area.height;
}

void timelineViewer::drawParam_vector(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end)
{
    int step = 5;
    float rangeMin = pr->getValueMin();
    float rangeMax = pr->getValueMax();
    int numVec = 0;
    if (pr->getType() == PTYPE_VEC2) numVec = 2;
    if (pr->getType() == PTYPE_VEC3) numVec = 3;

    for (int i = 0;i < area.width; i+=step)
    {
        float timed_a = ofMap(i, 0, area.width, begin, end);
        float timed_b = ofMap(i + step, 0, area.width, begin, end);
        
        for (int j = 0;j < numVec;j++)
        {
            float value_a = ofMap(pr->getBlockValue(j, timed_a, tm->getDuration()), rangeMin, rangeMax, area.height, 0);
            float value_b = ofMap(pr->getBlockValue(j, timed_b, tm->getDuration()), rangeMin, rangeMax, area.height, 0);

            if (j == 0) ofSetColor(255, 0, 0);
            if (j == 1) ofSetColor(0, 255, 0);
            if (j == 2) ofSetColor(0, 128, 255);
            if (j == 3) ofSetColor(255, 255, 0);
            
            ofDrawLine(i, value_a, i + step, value_b);
        }
    }
}

void timelineViewer::drawParam_json(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end)
{
    jsonParam* jpr = (jsonParam*)pr.get();
    int step = 5;
    float rangeMin = jpr->getValueMin();
    float rangeMax = jpr->getValueMax();
    //パラメータの連続値を描画
    for (int i = 0;i < area.width; i+=step)
    {
        float timed_a = ofMap(i, 0, area.width, begin, end);
        float timed_b = ofMap(i + step, 0, area.width, begin, end);
        float value_a = ofMap(jpr->getFloat(timed_a, tm->getDuration()), rangeMin, rangeMax, area.height, 0);
        float value_b = ofMap(jpr->getFloat(timed_b, tm->getDuration()), rangeMin, rangeMax, area.height, 0);

        ofSetColor(255, 100, 80);
        ofDrawLine(i, value_a, i + step, value_b);
    }
}

void timelineViewer::drawParam_float(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end)
{
    int step = 5;
    float rangeMin = pr->getValueMin();
    float rangeMax = pr->getValueMax();
    //パラメータの連続値を描画
    for (int i = 0;i < area.width; i+=step)
    {
        float timed_a = ofMap(i, 0, area.width, begin, end);
        float timed_b = ofMap(i + step, 0, area.width, begin, end);
        float value_a = ofMap(pr->getFloat(timed_a, tm->getDuration()), rangeMin, rangeMax, area.height, 0);
        float value_b = ofMap(pr->getFloat(timed_b, tm->getDuration()), rangeMin, rangeMax, area.height, 0);

        ofSetColor(0, 255, 0);
        ofDrawLine(i, value_a, i + step, value_b);
    }
}

void timelineViewer::drawParam_color(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end)
{
    int step = 5;
    vector<ofVec2f> pts;
    vector<ofFloatColor> c;
    pts.resize((area.width / step + 1) * 2);
    c.resize((area.width / step + 1) * 2);
    int cnt = 0;
    for (int i = 0;i <= area.width; i+= step)
    {
        float timed_a = ofMap(i, 0, area.width, begin, end);
        ofFloatColor va = pr->getColor(timed_a, tm->getDuration());
        
        pts[cnt * 2].set(i, 0);
        pts[cnt * 2 + 1].set(i, area.height);
        c[cnt * 2] = c[cnt * 2 + 1] = va;
        cnt++;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_FLOAT, 0, &c[0]);
    glVertexPointer(2, GL_FLOAT, 0, &pts[0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, pts.size());
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

}

void timelineViewer::drawParam_event(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end)
{
    vector<uint64_t> const & kp = pr->getKeyPoints();
    vector<ofPtr<block> > const & bl = pr->getBlocks(0);
    for (int i = 0;i < kp.size();i++)
    {
        if (view_begin <= kp[i] && kp[i] <= view_end)
        {
            float x = ofMap(kp[i], begin, end, seekLeft, seekLeft + seekWidth) - area.x + 5;
            ofDrawBitmapStringHighlight(bl[i]->eventName, x, 20);
        }
    }
}

void timelineViewer::keyPressed(ofKeyEventArgs & key){

    //再生・停止
    if (key.key == ' ')
    {
        if (tm->getIsPlay())    
        {
            if (videoSync)
            {
                bool isPaused = video->isPaused();
                pauseVideo(!isPaused);
                tm->setPause(!isPaused);
            }
            else
            {
                tm->togglePause();
            }
        }
        else
        {
            tm->play();
            playVideo();
        }
    }

    //削除
    if (!isGuiHovered && (key.key == OF_KEY_BACKSPACE || key.key == OF_KEY_DEL))
    {
        for (auto & tr : tm->getTracks())
        {
            for (auto & pm : tr->getParamsRef())
            {
                for (auto & b : selBlocks)
                {
                    pm->removeKeyPoint(b);
                }
            }
        }

        peekHeight.reset();
        hoverParam.reset();
        hoverBlock.reset();
        selBlocks.clear();
        selParentParam.clear();
    }

    if (key.key == OF_KEY_PAGE_UP)
    {
        int idx = tm->getCurrentChapterIndex();
        idx--;
        if (idx < 0) idx = tm->getChapterSize() - 1;
        tm->setChapter(idx);
        zoomOut();
    }

    if (key.key == OF_KEY_PAGE_DOWN)
    {
        int idx = tm->getCurrentChapterIndex();
        idx++;
        idx %= tm->getChapterSize();
        tm->setChapter(idx);
        zoomOut();
    }


}

void timelineViewer::keyReleased(ofKeyEventArgs & key){}
void timelineViewer::mouseMoved(ofMouseEventArgs & e){

}

void timelineViewer::mouseScrolled(ofMouseEventArgs & e)
{
    if (ofGetKeyPressed(OF_KEY_CONTROL) && hoverParam)
    {
        int sc = e.scrollY * -1;
        int middle = ofMap(e.x, seekLeft, seekLeft + seekWidth, view_begin, view_end);
        view_begin += (middle - view_begin) * 0.3 * sc;
        view_end += (middle - view_end) * 0.3 * sc;

        view_begin = MAX(0, view_begin);
        view_end = MIN(tm->getDuration(), view_end);

        if (view_end - view_begin < 5) view_end = view_begin + 5;
    }
}

void timelineViewer::mousePressed(ofMouseEventArgs & e)
{
    lastDragPoint.set(e.x, e.y);

    if (ofGetKeyPressed(' ') && hoverParam)
    {
        handToolFlag = true;
        return;
    }

    if (ofGetElapsedTimeMillis() - doubleClTimer < 300)//ダブルクリック
    {
        if (hoverParam)
        {
            uint64_t targTime = ofMap(e.x, seekLeft, seekLeft + seekWidth, view_begin, view_end);
            hoverParam->addKeyPoint(targTime);
        }
        doubleClTimer = 0;
    }
    else//シングルクリック
    {
        if (!guiHovered)
        {
            selBlocks.clear();
            selParentParam.clear();
            if (hoverBlock)
            {
                selBlocks.push_back(hoverBlock);
                selParentParam.push_back(hoverParam);
                //Shiftキー複数選択(追々実装する)
                // bool exist = false;
                // for (auto & b : selBlocks) if (b == hoverBlock) exist = true;
                // if (!exist) selBlocks.push_back();
            }
            doubleClTimer = ofGetElapsedTimeMillis();
        }
    }
}

int timelineViewer::getCurrentSnapRange()
{
    int autoSnaps[] = {100, 500, 1000, 5000, 10000, 30000, 60000, 300000};//0.5秒~5分
    int snapWidth = 0;
    for (int i = 0;i < 8;i++)
    {
        float currentWidth = seekWidth / (view_end - view_begin) * autoSnaps[snapWidth];
        if (100 < currentWidth) break;
        snapWidth++;
    }
    return autoSnaps[snapWidth];
}

void timelineViewer::mouseDragged(ofMouseEventArgs & e)
{
    ofVec2f diff = lastDragPoint - ofVec2f(e.x, e.y);
    uint64_t prevTime = ofMap(lastDragPoint.x, seekLeft, seekLeft + seekWidth, view_begin, view_end);
    uint64_t targTime = ofMap(e.x, seekLeft, seekLeft + seekWidth, view_begin, view_end);

    //targTimeをスナップする
    bool snap = doSnap;
    snaped = -1;
    vector<uint64_t> snapPoints;

    if (!guiHovered)
    {
        if (snap)
        {
            int snpW = getCurrentSnapRange();
            for (int i = int(view_begin / snpW) * snpW;i < view_end; i+=snpW) snapPoints.push_back(i);

            for (auto & tr : tm->getTracks())
                for (auto & pr : tr->getParamsRef())
                    if (pr != hoverParam)
                        for (auto & kp : pr->getKeyPoints()) snapPoints.push_back(kp);
        }

        //キーポイントの調整
        if (!handToolFlag && hoverParam)
        {
            for (auto & b : selBlocks)
            {
                snaped = hoverParam->moveKeyPoint(b, targTime - prevTime, 
                    snapPoints, 
                    MAX(1,ofMap(5, 0, seekWidth, 0, view_end - view_begin))
                );
            }
        }

        //高さの調整
        if (!handToolFlag && peekHeight)
        {
            peekHeight->view_height += diff.y * -1;
        }

        //Brush
        if (handToolFlag)
        {
            int w = view_end - view_begin;
            view_end = MIN(tm->getDuration(), view_end - (targTime - prevTime));
            view_begin = view_end - w;
            if (view_begin < 0)
            {
                view_end += view_begin * -1;
                view_begin = 0;
            }
        }

    }

    lastDragPoint.set(e.x, e.y);
}

void timelineViewer::mouseReleased(ofMouseEventArgs & e){
    handToolFlag = false;
    snaped = -1;
}

void timelineViewer::createNewTrack(string name, trackType tp)
{
    int cnt = 0;
    bool exist = true;
    string newname = "";
    while (exist)
    {
        cnt++;
        exist = false;
        newname = ofToString(cnt) + "_" + name;

        for (auto & t : tm->getTracks()) 
        {
            exist |= (t->getName() == newname);
            exist |= (t->getUniqueName() == newname);
        }
    }
    tm->addTrack(newname, tp, true);
}

void timelineViewer::drawGui()
{
    guiHovered = ImGui::IsAnyWindowHovered() | ImGui::IsAnyItemActive();
    string nextTrackNum = ofToString(tm->getTracks().size() + 1);
    
    if (ImGui::Button("Fold")) 
    {
        for (auto & t : tm->getTracks()) t->fold = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("UnFold")) for (auto & t : tm->getTracks()) t->fold = false;
    
    ImGui::Text("===Sync===");
    stringstream info;
    info << "Hash : 0x" << hex << tm->getHash() << endl;
    info << "Dest :" << tm->getSendAddr() << endl;

    ImGui::Text(info.str().c_str());
    ImGui::PushStyleColor(ImGuiCol_Button, ofFloatColor::fromHsb(tm->edited ? 0.1 : 0.4, 0.5, 0.5));
    if (ImGui::Button("Sync data")) 
    {
        tm->sendSyncJsonData();
        tm->edited = false;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::Button("Get data"))
    {
        tm->edited = false;
        ofxOscSender sender;
        ofxOscMessage m;
        m.setAddress("/json/get");
        m.addIntArg(tm->getReceiverPort());
        sender.setup(tm->getSendAddr(), tm->getSendPort());
        sender.sendMessage(m);
        cout << "Get request to " << tm->getSendAddr() << "::" << tm->getReceiverPort() << endl;
    }

    if (ImGui::Button("Save remote as main.json"))
    {
        ofxOscSender sender;
        ofxOscMessage m;
        m.setAddress("/json/save");
        m.addStringArg("main.json");
        sender.setup(tm->getSendAddr(), tm->getSendPort());
        sender.sendMessage(m);
    }
    if (ImGui::Button("Standby All"))
    {
        for (auto & tr : tm->getTracks())
        {
            ofxOscMessage mes;
            mes.setAddress("/track/standby");
            mes.addStringArg(tr->getName());
            tm->getSender().sendMessage(mes);
        }
    }

    ImGui::Text("=== Add Track ===");
    if (ImGui::Button("Float")) createNewTrack("float", TRACK_FLOAT);
    ImGui::SameLine();
    if (ImGui::Button("Color")) createNewTrack("color", TRACK_COLOR);
    ImGui::SameLine();
    if (ImGui::Button("Event")) createNewTrack("event", TRACK_EVENT);

    if (ImGui::Button("Vec2")) createNewTrack("vec2", TRACK_VEC2);
    ImGui::SameLine();
    if (ImGui::Button("Vec3")) createNewTrack("vec3", TRACK_VEC3);
    ImGui::SameLine();
    if (ImGui::Button("Json")) createNewTrack("json", TRACK_JSONSTREAM);

    if (ImGui::Button("Motor")) createNewTrack("motor", TRACK_MOTOR);
    
    ImGui::Text(" \n=== Chapter ===");
    vector<string> chapterNames = tm->getChapterNames();
    gui_chapterIndex = tm->getCurrentChapterIndex();

    if (ImGui::Button("New Chapter")) {
        tm->createChapter("", tm->getDuration());
    }

    if (combo("Chapter", &gui_chapterIndex, chapterNames))
    {
        tm->setChapter(gui_chapterIndex);
        zoomOut();
    }

    ImGui::ColorEdit3("chapterBg", &tm->getCurrentChapter()->bgColor[0], ImGuiColorEditFlags_Float);

    strcpy(gui_chapterName, tm->getCurrentChapter()->name.c_str());
    if (ImGui::InputText("ChapterName", gui_chapterName, numChapterName))
    {
        tm->getCurrentChapter()->name = string(gui_chapterName);
    }

    ImGui::Text(" \n=== Edit ===");
    ImGui::Checkbox("Snap", &doSnap);
    
    if (ImGui::Button("New")) tm->clear();
    ImGui::SameLine();
    if (ImGui::Button("Load")) 
    {
        ofFileDialogResult result = ofSystemLoadDialog("load", false, "./bin/data");
        
        if (result.bSuccess)
        {
            tm->load(result.getPath());
            zoomOut();
            strcpy(gui_oscAddrInput, tm->getSendAddr().c_str());
        }
    }
 
    if (ImGui::Button("Save"))  
    {
        if (tm->getCurrentFileName() == "")
        {
            ofFileDialogResult result = ofSystemSaveDialog(tm->getCurrentFileName(), "save");
            if (result.bSuccess)
            {
                tm->save(result.getPath());
            }
        }
        else
        {
            tm->save(tm->getCurrentFileName());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As..."))
    {
        ofFileDialogResult result = ofSystemSaveDialog(tm->getCurrentFileName(), "save");
        if (result.bSuccess)
        {
            tm->save(result.getPath());
        }
    }
    ImGui::Checkbox("AutoBackup", &autoBackup);
    ImGui::DragInt("BackupInterval[sec]", &backupInterval, 1, 10, 300);

    string ts[] = {":", ".", " "};
    int tmax[] = {99 , 59 , 99};
    ImGui::Text(" \n=== Duration ===");
    bool durEdited = false;
    guiInt_duration[0] = tm->getDuration() / 1000 / 60;
    guiInt_duration[1] = tm->getDuration() / 1000 % 60;
    guiInt_duration[2] = tm->getDuration() / 10 % 100;
    for (int i = 0;i < 3;i++)
    {
        ImGui::PushID(1000 + i);
        ImGui::PushItemWidth(30);
        durEdited |= ImGui::DragInt(ts[i].c_str(), &guiInt_duration[i], 0.1, 0, tmax[i], "%02d");
        ImGui::PopItemWidth();
        if (i < 2) ImGui::SameLine();
        ImGui::PopID();
    }
    if (durEdited)
    {
        int targDur = (guiInt_duration[0] * 60 + guiInt_duration[1]) * 1000 + guiInt_duration[2] * 10;
        tm->setDuration(targDur);
        zoomOut();
    }

    ImGui::Text(" \n=== Playing ===");
    if (ImGui::Button("Play")) 
    {
        tm->play();
        playVideo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        tm->stop();
        stopVideo();
    }
    ImGui::SameLine();

    gui_isLoop = tm->getIsLoop();
    if (ImGui::Checkbox("Loop", &gui_isLoop))
    {
        tm->setIsLoop(gui_isLoop);
    }

    ImGui::Text(" \n=== OSC ===");
    
    gui_oscSendPort = tm->getSendPort();
    gui_oscReceivePort = tm->getReceiverPort();

    ImGui::PushItemWidth(100);
    ImGui::InputText("Address", gui_oscAddrInput, numOscAddrText);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Set")) tm->setSenderAddr(string(gui_oscAddrInput));
    if (ImGui::DragInt("SendPort", &gui_oscSendPort, 1, 0, 65535)) tm->setSenderPort(gui_oscSendPort);
    if (ImGui::DragInt("ReceivePort", &gui_oscReceivePort, 1, 0, 65535)) tm->setReceiverPort(gui_oscReceivePort);
    
    ImGui::Checkbox("Send on seek", &tm->sendOnSeek);
    ImGui::SameLine();
    ImGui::Checkbox("Send always", &tm->sendAlways); 

    if (selBlocks.size() > 0)
    {
        if (currentViewBlock != selBlocks[0])
        {
            currentViewBlock = selBlocks[0];
            blockGuiViewFirst = true;
        }

        if (blockGuiViewFirst)
        {
            ImGui::SetWindowPos("BlockEditor", ImVec2(ofGetMouseX() + 10, ofGetMouseY() + 10));
            blockGuiViewFirst = false;
        }

        ImGui::Begin("BlockEditor", &blockGuiWindowOpen, ImGuiWindowFlags_NoTitleBar);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ofFloatColor::fromHsb(0.45, 0.9, 0.1, 0.75));
        // ImGui::BeginChildFrame(101, ImVec2(0, 0));

        auto & b = selBlocks[0];
        auto & p = selParentParam[0];

        drawParameterGui(b, p);

        // ImGui::EndChildFrame();
        ImGui::PopStyleColor();
        ImGui::End();
    }
    else
    {
        currentViewBlock.reset();
        blockGuiViewFirst = true;
    }

    isGuiHovered = guiHovered;
}

void timelineViewer::drawParameterGui(ofPtr<block> & b, ofPtr<param> & p)
{
    guiBool_inherit = b->getIsInherit();
    guiBool_keep = b->getKeep();
    guiFloat_to = b->getTo();
    int idx = b->getNumber();

    vector<ofPtr<block> > bs = p->pickBlocks(idx);

    // ======================================共通ヘッダー
    guiFloat_keyPoint = p->getKeyPoints()[idx];

    string title = "";
    if (p->getType() == PTYPE_COLOR) title = "COLOR:\n ";
    if (p->getType() == PTYPE_FLOAT) title = "FLOAT:\n ";
    if (p->getType() == PTYPE_EVENT) title = "Event:\n ";
    if (p->getType() == PTYPE_VEC2) title = "Vector2f:\n ";
    if (p->getType() == PTYPE_VEC3) title = "Vector3f:\n ";
    ImGui::Text(title.c_str());

    if (idx == 0) ImGui::PushStyleColor(ImGuiCol_Text, ofFloatColor::fromHsb(0.5, 0.0, 0.5));
    
    if (ImGui::Checkbox("Keep", &guiBool_keep))
    {
        for (auto & bb : bs) bb->setKeep(guiBool_keep);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Inherit", &guiBool_inherit))
    {
        for (auto & bb : bs) bb->setInherit(guiBool_inherit);
        p->refleshInherit();
    }

    if (ImGui::DragFloat("keyPoint", &guiFloat_keyPoint, 1))
    {
        p->setKeyPoint(b, guiFloat_keyPoint);
    }
    if (idx == 0) ImGui::PopStyleColor();


    //===========================================カラーパラメータ描画
    if (p->getType() == PTYPE_COLOR)
    {
        guiColor_to.set(bs[0]->getTo(), bs[1]->getTo(), bs[2]->getTo(), bs[3]->getTo());
        guiColor_from.set(bs[0]->getFrom(), bs[1]->getFrom(), bs[2]->getFrom(), bs[3]->getFrom());

        // ImGui::ColorPicker4("test", &guiColor_to[0]);
        ImGuiColorEditFlags flg = 0;
        flg |= ImGuiColorEditFlags_NoSidePreview;
        flg |= ImGuiColorEditFlags_NoSmallPreview;
        flg |= ImGuiColorEditFlags_NoTooltip;
        flg |= ImGuiColorEditFlags_NoLabel;
        ImGui::Text("===Value===");

        if (ImGui::ColorPicker4("ColorTo", &guiColor_to[0], flg))
        {
            for (int i = 0;i < 4;i++)
                bs[i]->setTo(guiColor_to[i]);
        }
        

        if (!guiBool_inherit && ImGui::ColorPicker4("ColorFrom", &guiColor_from[0], flg))
        {
            for (int i = 0;i < 4;i++)
                bs[i]->setFrom(guiColor_from[i]);
        }

    }


    //============================================Floatパラメータ描画
    if (p->getType() == PTYPE_FLOAT)
    {
        if (!guiBool_keep)
        {
            if (ImGui::DragFloat("Value", &guiFloat_to, 0.01f)) b->setTo(guiFloat_to);
        }
        
        if (!guiBool_inherit)
        {
            guiFloat_from = b->getFrom();

            if (ImGui::DragFloat("startValue", &guiFloat_from, 0.01f))
            {
                b->setFrom(guiFloat_from);
            }
        }
        
    }

    //==========================================Eventパラメータ描画
    if (p->getType() == PTYPE_EVENT)
    {
        strcpy(gui_textInput, b->eventName.substr(0, numEventText).c_str());
        if (ImGui::InputText("EventName", gui_textInput, numEventText))
        {
            b->eventName = gui_textInput;
        }
    }

    //============================================Floatパラメータ描画
    if (p->getType() == PTYPE_VEC2 || p->getType() == PTYPE_VEC3)
    {
        int numVec = (p->getType() == PTYPE_VEC2 ? 2 : 3);

        for (int i = 0;i < numVec; i++) 
            guiVec_to[i] = bs[i]->getTo();
        
        if (numVec == 2 ? ImGui::DragFloat2("To", guiVec_to, 0.01f) : ImGui::DragFloat3("To", guiVec_to, 0.01f)) 
        {
            for (int i = 0;i < numVec; i++) 
                bs[i]->setTo(guiVec_to[i]);
        }
        
        if (!guiBool_inherit)
        {
            for (int i = 0;i < numVec;i++)
                guiVec_from[i] = bs[i]->getFrom();

            if (numVec == 2 ? ImGui::DragFloat2("From", guiVec_from, 0.01f) : ImGui::DragFloat3("From", guiVec_from, 0.01f))
            {
                for (int i = 0;i < numVec;i++)
                    bs[i]->setFrom(guiVec_from[i]);
            }
        }
    }

    //===========================================共通フッター

    //ENUMのcomplementTypeと揃える
    const char* items[] = {"Linear", "Const", "Sin", "Quad", "Cubic", "MotorRamp", "Expo"};
    gui_curveSelect = int(b->getComplement());

    bool easeEnable = false;
    if(ImGui::Combo("Curve", &gui_curveSelect, items, IM_ARRAYSIZE(items)))
    {
        for (int i = 0;i < bs.size();i++)
            bs[i]->setComplement(complementType(gui_curveSelect));

    }

    easeEnable = false;
    easeEnable |= (gui_curveSelect == CMPL_SIN);
    easeEnable |= (gui_curveSelect == CMPL_QUAD);
    easeEnable |= (gui_curveSelect == CMPL_CUBIC);
    easeEnable |= (gui_curveSelect == CMPL_EXPO);

    //カーブがイーズ有効の時はイーズをラジオと同期
    if (easeEnable)
    {
        int radioTarg = bs[0]->easeOutFlag ? bs[0]->easeInFlag ? 2 : 1 : 0;
        guiRadio_easeIn = radioTarg;
        for (int i = 0;i < bs.size();i++) bs[i]->easeInFlag = (guiRadio_easeIn == 0 || guiRadio_easeIn == 2);
        for (int i = 0;i < bs.size();i++) bs[i]->easeOutFlag = (guiRadio_easeIn == 1 || guiRadio_easeIn == 2);        

        bool radioEdit = false;
        radioEdit |= ImGui::RadioButton("EaseIn", &guiRadio_easeIn, 0);ImGui::SameLine();
        radioEdit |= ImGui::RadioButton("EaseOut", &guiRadio_easeIn, 1);ImGui::SameLine();
        radioEdit |= ImGui::RadioButton("Both", &guiRadio_easeIn, 2);
        
        if (radioEdit)
        {
            for (int i = 0;i < bs.size();i++) bs[i]->easeInFlag = (guiRadio_easeIn == 0 || guiRadio_easeIn == 2);
            for (int i = 0;i < bs.size();i++) bs[i]->easeOutFlag = (guiRadio_easeIn == 1 || guiRadio_easeIn == 2);
        }

    }

    if (gui_curveSelect == CMPL_RAMP)
    {
        ImGui::DragFloat("Accel", &b->accel, 0.01, 0.01, 1000);
        ImGui::DragFloat("Decel", &b->decel, 0.001, 0.01, 1000);
        string sp = "Speed :" + ofToString(b->speed_max);
        ImGui::Text(sp.c_str());
    }

    strcpy(gui_blockLabelInput, bs[0]->label.substr(0, numBlockLabel).c_str());
    if (ImGui::InputText("Label", gui_blockLabelInput, numBlockLabel))
    {
        for (int i = 0;i < bs.size();i++) bs[i]->label = string(gui_blockLabelInput);
    }
}

void timelineViewer::removeTrack(ofPtr<trackBase> const & tr)
{
    peekHeight.reset();
    hoverTrack.reset();
    hoverParam.reset();
    hoverBlock.reset();
    selBlocks.clear();
    selParentParam.clear();

    tm->removeTrack(tr->getName());
}

void timelineViewer::seekVideo(uint64_t time)
{
    if (!videoSync || !video) return;
    video->setPosition(time / 1000.0 / video->getDuration());
}

void timelineViewer::setSyncVideo(bool enable, ofPtr<tm_videoPlayer> v)
{
    video = v;
    videoSync = enable;
    if (video && video->isLoaded())
    {
        tm->setDuration(video->getDuration() * 1000);
        zoomOut();
    }
}

void timelineViewer::playVideo()
{
    if (!videoSync || !video) return;
    video->setPosition(0);
    video->play();
}

void timelineViewer::pauseVideo(bool b)
{
    if (!videoSync || !video) return;
    video->setPaused(b);
}

void timelineViewer::stopVideo()
{
    if (!videoSync || !video) return;
    video->stop();
    video->setPosition(0.0);
}

void timelineViewer::syncFromVideo()
{
    if (!videoSync || !video) return;
    tm->setPositionByMillis(video->getDuration() * video->getPosition() * 1000);
}

#endif
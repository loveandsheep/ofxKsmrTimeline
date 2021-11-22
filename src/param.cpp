#include "param.h"

void param::setType(paramType tp)
{
    myType = tp;
    if (tp == PTYPE_FLOAT) numUsingBlockLine = 1;
    if (tp == PTYPE_BOOL) numUsingBlockLine = 1;
    if (tp == PTYPE_VEC2) numUsingBlockLine = 2;
    if (tp == PTYPE_VEC3) numUsingBlockLine = 3;
    if (tp == PTYPE_COLOR) numUsingBlockLine = 4;    
    if (tp == PTYPE_EVENT) numUsingBlockLine = 1;
    if (tp == PTYPE_JSONSTREAM) numUsingBlockLine = 1;
}

int param::getBlockIndexByTime(uint64_t millis)
{
    int index = 0;
    for (int i = 0;i < keyPoints.size();i++)
    {
        if (keyPoints[i] <= millis) index = i;
    }
    return index;
}

vector<ofPtr<block> > param::pickBlocksByTime(uint64_t millis)
{
    return pickBlocks(getBlockIndexByTime(millis));
}

//選択されたBlockと並列したBlockを列挙する（VectorやColorで参照する）
vector<ofPtr<block> > param::pickBlocks(int number)
{
    vector<ofPtr<block> > ret;
    for (int o = 0;o < numUsingBlockLine;o++)
    {
        ret.push_back(blocks[o][number]);
    }

    return ret;
}

tm_event param::getEvent(uint64_t const & time, uint64_t const & duration)
{

    tm_event ret;
    if (checkParamMatching(PTYPE_EVENT))
    {
        for (int i = 0;i < keyPoints.size();i++)
        {
            if (time > keyPoints[i])
            {
                ret.label = blocks[0][i]->eventName;
                ret.time = keyPoints[i];
            }
        }
    }
    return ret;
}

ofVec2f param::getVec2f(uint64_t const & time, uint64_t const & duration)
{
    if (checkParamMatching(PTYPE_VEC2))
    {
        return ofVec2f(
            getBlockValue(0, time, duration),
            getBlockValue(1, time, duration));
    }
    return ofVec2f(0);
}

ofVec3f param::getVec3f(uint64_t const & time, uint64_t const & duration)
{
    if (checkParamMatching(PTYPE_VEC3))
    {
        return ofVec3f(
            getBlockValue(0, time, duration),
            getBlockValue(1, time, duration),
            getBlockValue(2, time, duration));
    }
    return ofVec3f(0);
}

float param::getFloat(uint64_t const & time, uint64_t const & duration)
{
    if (checkParamMatching(PTYPE_FLOAT))
    {
        return getBlockValue(0, time, duration);
    }
    return 0.5;
}

ofFloatColor param::getColor(uint64_t const & time, uint64_t const & duration)
{
    if (checkParamMatching(PTYPE_COLOR))
    {
        return ofFloatColor(
            getBlockValue(0, time, duration),
            getBlockValue(1, time, duration),
            getBlockValue(2, time, duration),
            getBlockValue(3, time, duration));
    }
    return ofFloatColor(0, 1, 0);
}

bool param::checkParamMatching(paramType tp)
{
    if (myType != tp)
    {
        ofLogWarning("param", "Parameter Type not maching.");
        return false;
    }
    return true;
}

int param::addKeyPoint(uint64_t const & time)
{
    int index = 0;
    for (int i = 0;i < keyPoints.size();i++)
    {
        if (keyPoints[i] < time) index = i + 1;
    }

    keyPoints.insert(keyPoints.begin() + index, time);
    for (int i = 0;i < numUsingBlockLine;i++)
    {
        ofPtr<block> nb = make_shared<block>();
        nb->setFrom(0.5);
        nb->setTo(0.5);
        if (i == 3) 
        {
            nb->setFrom(1);
            nb->setTo(1);
        }
        blocks[i].insert(blocks[i].begin() + index, nb);
    }

    refleshInherit();
    return index;
}

void param::setKeyPoint(ofPtr<block> const & bl, int const & targetTime)
{
    for (int i = 0;i < keyPoints.size();i++)
    {
        for (int j = 0;j < numUsingBlockLine;j++)
        {
            if (blocks[j][i] == bl)
            {
                keyPoints[i] = targetTime;
            }
        }
    }

    refleshInherit();
}

int param::moveKeyPoint(ofPtr<block> const & bl , int const & targetTime, vector<uint64_t> const & snapPoints, int snapRange)
{
    int ret = -1;//スナップしたか
    for (int i = 0;i < keyPoints.size();i++)
    {
        for (int j = 0;j < numUsingBlockLine;j++)
        {
            if (blocks[j][i] == bl)
            {
                if (keyPoints[i] < abs(targetTime) && targetTime < 0) //0にクランプ
                    keyPoints[i] = 0;
                else
                    keyPoints[i] = keyPoints[i] + targetTime;

                for (auto &sp : snapPoints)
                {
                    if (abs(int(sp) - int(keyPoints[i])) < snapRange)
                    {
                        keyPoints[i] = sp;
                        ret = sp;
                    }
                }
            }
        }
    }

    refleshInherit();

    return ret;
}

void param::clearKeyPoints()
{
    for (int i = 0;i < numUsingBlockLine;i++)
    {
        while (blocks[i].size() > 0) 
        {
            blocks[i][0].reset();
            blocks[i].erase(blocks[i].begin());
        }
    }
    while (keyPoints.size() > 0) keyPoints.erase(keyPoints.begin());

    refleshInherit();
}

void param::removeKeyPoint(ofPtr<block> const & bl)
{
    int removeIndex = -1;
    for (int i = 0;i < keyPoints.size();i++)
    {
        for (int j = 0;j < numUsingBlockLine;j++)
        {
            if (blocks[j][i] == bl)
            {
                removeIndex = i;
            }
        }
    }

    if (removeIndex >= 0)
    {
        keyPoints.erase(keyPoints.begin() + removeIndex);
        for (int j = 0;j < numUsingBlockLine;j++)
        {
            blocks[j].erase(blocks[j].begin() + removeIndex);
        }
    }
    
    refleshInherit();
}

float param::getBlockValue(int numBlock, uint64_t const & time, uint64_t const & duration)
{
    if (keyPoints.size() == 0) return 0.0;

    numBlock = ofClamp(numBlock, 0, 3);
    int targetIndex = 0;
    //TODO: lastSeekPointを活用して効率的に探索する方法を考える
    for (int i = 0;i < keyPoints.size();i++)
    {
        if (keyPoints[i] <= time) targetIndex = i;
    }

    uint64_t endPt = duration;
    if (targetIndex < keyPoints.size() - 1) endPt = keyPoints[targetIndex + 1];

    lastSeekPoint = time;
    float value = blocks[numBlock][targetIndex]->getValue(ofMap(time, keyPoints[targetIndex], endPt, 0, 1), endPt - keyPoints[targetIndex]);

    return value;
}

void param::refleshInherit()
{
    //keyPointの時間に応じてソート
    if (keyPoints.size() == 0) return;
    for (int i = 0;i < keyPoints.size() - 1;i++) {
        for (int j = (keyPoints.size() - 1); j > i; j--) {
            if (keyPoints[j] < keyPoints[j - 1]) {
                swap(keyPoints[j], keyPoints[j - 1]);
                for (int o = 0;o < numUsingBlockLine;o++)
                {
                    swap(blocks[o][j], blocks[o][j - 1]);
                }
            }
        }
    }

    //先頭のキーポイントは0固定
    keyPoints[0] = 0;

    for (int o = 0;o < numUsingBlockLine; o++)
    {
        for (int i = 0;i < blocks[o].size();i++)
        {
            blocks[o][i]->setNumber(i);
        }
    }

    //inherit関係を再構築
    for (int i = 0;i < numUsingBlockLine;i++)
    {
        for (int j = 0;j < blocks[i].size();j++)
        {
            if (j == 0) blocks[i][j]->setInherit(false);
            else if (j > 0)
            {
                blocks[i][j]->updateInherit(blocks[i][j - 1]);
            }
        }
    }
}

string param::dumpBlocks()
{
    string ret = "";

    int cnt = 0;
    for (auto & bl : blocks[0])
    {
        ret += ofToString(cnt, 3) + ":" + ofToString(keyPoints[cnt]) + "::" + ofToString(bl->getFrom()) + " -- " + ofToString(bl->getTo()) + "\n";
        ret += string("Inherit :") + (bl->inheritPtr ? "yes\n" : "nothing\n");
        cnt++;
    }

    return ret;
}

void param::resetMaxMinRange()
{
    for (int o = 0;o < numUsingBlockLine;o++)
    {
        valueMax[o] = -100000;
        valueMin[o] =  100000;

        for (auto & b : blocks[o])
        {
            valueMax[o] = MAX(b->getMax(), valueMax[o]);
            valueMin[o] = MIN(b->getMin(), valueMin[o]);
        }
    }
}

float param::getValueMax(int index)
{
    if (index < 0)
    {
        float ret = valueMax[0];
        for (int i = 1;i < numUsingBlockLine;i++)
            ret = MAX(ret, valueMax[i]);

        return ret;
    }
    else
    {
        index = ofClamp(index, 0, 3);
        return valueMax[index];
    }
}

float param::getValueMin(int index)
{
    if (index < 0)
    {
        float ret = valueMin[0];
        for (int i = 1;i < numUsingBlockLine;i++)
            ret = MIN(ret, valueMin[i]);

        return ret;
    }
    else
    {
        index = ofClamp(index, 0, 3);
        return valueMin[index];
    }
}

ofJson param::getJsonData()
{
    ofJson j;
    j["type"] = int(myType);
    for (auto & k : keyPoints)
        j["keyPoints"].push_back(k);

    for (int o = 0;o < 4;o++)
    {
        for (auto & b : blocks[o])
            j["blocks"][o].push_back(b->getJsonData());
    }

    return j;
}
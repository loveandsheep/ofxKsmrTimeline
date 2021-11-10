#include "./param.h"

class jsonParam : public param {
public:

    float           fps = 60;
    string          sampleID = "";
    int             keyIndex = 0;
    vector<string>  keyNames;
    vector<float>   parameters;
    float vMax = 1, vMin = 0;
    string          loadedPath = "";
    ofJson          data;

    void load(string path)
    {
        data = ofLoadJson(path);
        loadedPath = path;
        keyNames.clear();

        for (ofJson::iterator it = data.begin(); it != data.end(); ++it)
        {
            keyNames.push_back(it.key());
        }

        if (keyNames.size() > 0) calcParameters(keyNames.back());
    }

    void calcParameters(string name)
    {
        sampleID = name;
        for (int i = 0;i < keyNames.size();i++) if (keyNames[i] == name) keyIndex = i;
        parameters.clear();
        if (data[sampleID].size() <= 0) return;

        vMax = vMin = data[sampleID][0].get<float>();
        for (int i = 0;i < data[sampleID].size();i++)
        {
            parameters.push_back(data[sampleID][i].get<float>());
            vMax = MAX(vMax, parameters.back());
            vMin = MIN(vMin, parameters.back());
        }
    }
 
    virtual float getFloat(uint64_t const & time, uint64_t const & duration){
        if (checkParamMatching(PTYPE_JSONSTREAM) && parameters.size() > 0)
        {
            int index = MIN(floor(time / (1000 / fps)), parameters.size() - 1);
            
            if (index < parameters.size() - 1) {
                float time_a = index * (1000 / fps);
                float time_b = (index+1) * (1000 / fps);
                return ofMap(time, time_a, time_b, parameters[index], parameters[index + 1]);
            }
            else
            {
                return parameters[index];
            }
        }
        return 0.0;
    }

    virtual float getValueMax(){return vMax;}
    virtual float getValueMin(){return vMin;}

    virtual ofJson getJsonData()
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

        j["jsonparam"]["path"] = loadedPath;
        j["jsonparam"]["id"] = sampleID;
        return j;
    }

    void parseJson(ofJson json)
    {
        if (!json["jsonparam"]["path"].empty())
        {
            load(json["jsonparam"]["path"].get<string>());
            if (!json["jsonparam"]["id"].empty())
                calcParameters(json["jsonparam"]["id"].get<string>());
        }
    }
};
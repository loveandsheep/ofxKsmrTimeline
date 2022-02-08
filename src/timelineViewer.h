#pragma once
#include "ofConstants.h"
#include "ofMain.h"

#if defined(_WIN32)
#include "timeline.h"
#include "imGuiSheepUtil.h"

typedef ofBaseVideoPlayer tm_videoPlayer;

namespace palette{
    static const int orange[]  = {
        0xFFA984,
        0xF5A480,
        0xC28165
    };

    static const int green[] = {
        0xBCF5BB,
        0x8BB7A1,
        0x658364
    };
    
    static const int gray[] = {
        0x95A6B7,
        0x768491,
        0x667080,
        0x41484F,
        0x17191C,
    };
}


class timelineViewer {
public:
    void setup(ofPtr<timeline> tmPtr);
    void update();
    void draw(ofRectangle area);
    void drawGui();

    void load(string path);

    void keyPressed(ofKeyEventArgs & key);
    void keyReleased(ofKeyEventArgs & key);

    void mouseMoved(ofMouseEventArgs & e);
    void mousePressed(ofMouseEventArgs & e);
    void mouseDragged(ofMouseEventArgs & e);
    void mouseReleased(ofMouseEventArgs & e);
    void mouseScrolled(ofMouseEventArgs & e);

    void setViewBegin(int time){view_begin = ofClamp(time, 0, tm->getDuration());}
    void setViewEnd(int time){view_end = ofClamp(time, 0, tm->getDuration());}
    void zoomOut(){view_begin = 0;view_end = tm->getDuration();}

    bool isGuiHovered = false;
    bool isEditorHovered = false;

    //同期関連==============================
    void play();
    void stop();
    void seek(uint64_t millis);
    void setPause(bool b);
    
    //動画周り
    void setSyncVideo(bool enable, ofPtr<tm_videoPlayer> v = nullptr);

protected:

    float seekLeft = 180;
    float seekWidth = 300;
    float margin = 10;
    float peek_height = 15;

    long doubleClTimer = 0;
    ofVec2f lastDragPoint;
    int view_begin = 0;
    int view_end = 1000;

    void createNewTrack(string name, trackType tp);

    float drawTrack(ofPtr<trackBase> tr, ofRectangle area, uint64_t begin, uint64_t end);
    float drawParam(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end);
    void drawParam_vector(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end);
    void drawParam_float(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end);
    void drawParam_json(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end);
    void drawParam_color(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end);
    void drawParam_event(ofPtr<param> pr, ofRectangle area, uint64_t begin, uint64_t end);
    void drawPeek(ofRectangle area);
    void drawBrush(ofRectangle area);
    void drawParameterGui(ofPtr<block> & b, ofPtr<param> & p);
    void removeTrack(ofPtr<trackBase> const & tr);

    int getCurrentSnapRange();

    bool videoSync = false;
    void seekVideo(uint64_t time);
    void playVideo();
    void pauseVideo(bool b);
    void stopVideo();
    void syncFromVideo();

    void resetSelectedItems();

    ofPtr<tm_videoPlayer> video;

    ofPtr<timeline> tm;
    ofPtr<trackBase> peekHeight;
    ofPtr<trackBase> hoverTrack;
    ofPtr<param> hoverParam;
    ofPtr<block> hoverBlock;
    vector<ofPtr<block> > selBlocks;
    vector<ofPtr<param> > selParentParam;
    ofPtr<block> lastSelBlock;
    ofPtr<param> lastSelParentParam;
    ofPtr<block> copiedBlock;
    ofPtr<param> copiedParentParam;
    string mouseLock = "free";
    bool handToolFlag = false;//スペースキー移動
    bool doSnap = false;

    static const int foldedHeight = 30;
    ofPtr<block> currentViewBlock;
    bool blockGuiWindowOpen = true;
    bool blockGuiViewFirst = false;
    bool guiHovered = false;
    bool guiBool_keep = false;
    bool guiBool_inherit = false;
    int guiRadio_easeIn = 0;
    int guiInt_duration[3] = {0};
    float guiFloat_keyPoint = 0;
    float guiFloat_from = 0;
    float guiFloat_to = 0;
    float guiVec_from[4] = {0};
    float guiVec_to[4] = {0};
    int gui_curveSelect = 0;
    static const int numEventText = 64;
    char gui_textInput[numEventText] = {0};
    static const int numTrackText = 64;
    char gui_trackInput[numTrackText] = {0};
    static const int numBlockLabel = 64;
    char gui_blockLabelInput[numBlockLabel] = {0};
    static const int numOscAddrText = 64;
    static const int numChapterName = 64;
    char gui_chapterName[numChapterName] = {0};
    char gui_oscAddrInput[numOscAddrText]= "localhost";
    int gui_oscSendPort = 12400;
    int gui_oscReceivePort = 12500;
    bool gui_oscSendOnSeek = true;
    bool gui_isLoop = false;
    float guiTrackFloat_to = 0;
    float guiTrackFloat_from = 0;
    bool gui_syncWindow = false;
    int gui_chapterIndex = 0;
    ofFloatColor guiColor_from;
    ofFloatColor guiColor_to;

    bool autoBackup = true;
    int backupInterval = 10;
    uint64_t backupTimer = 0;
    int snaped = -1;

        std::wstring utf8_to_wide_winapi(std::string const& src)
    {
        auto const dest_size = MultiByteToWideChar(CP_UTF8, 0U, src.data(), -1, nullptr, 0U);
        std::vector<wchar_t> dest(dest_size, L'\0');
        if (::MultiByteToWideChar(CP_UTF8, 0U, src.data(), -1, dest.data(), dest.size()) == 0) {
            throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
        }
        dest.resize(std::char_traits<wchar_t>::length(dest.data()));
        dest.shrink_to_fit();
        return std::wstring(dest.begin(), dest.end());
    }

    std::string wide_to_utf8_winapi(std::wstring const& src)
    {
        auto const dest_size = WideCharToMultiByte(CP_UTF8, 0U, src.data(), -1, nullptr, 0, nullptr, nullptr);
        std::vector<char> dest(dest_size, '\0');
        if (::WideCharToMultiByte(CP_UTF8, 0U, src.data(), -1, dest.data(), dest.size(), nullptr, nullptr) == 0) {
            throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
        }
        dest.resize(std::char_traits<char>::length(dest.data()));
        dest.shrink_to_fit();
        return std::string(dest.begin(), dest.end());
    }

    std::wstring multi_to_wide_winapi(std::string const& src)
    {
        auto const dest_size = MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, nullptr, 0U);
        std::vector<wchar_t> dest(dest_size, L'\0');
        if (::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, dest.data(), dest.size()) == 0) {
            throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
        }
        dest.resize(std::char_traits<wchar_t>::length(dest.data()));
        dest.shrink_to_fit();
        return std::wstring(dest.begin(), dest.end());
    }

    std::string wide_to_multi_winapi(std::wstring const& src)
    {
        auto const dest_size = WideCharToMultiByte(CP_ACP, 0U, src.data(), -1, nullptr, 0, nullptr, nullptr);
        std::vector<char> dest(dest_size, '\0');
        if (::WideCharToMultiByte(CP_ACP, 0U, src.data(), -1, dest.data(), dest.size(), nullptr, nullptr) == 0) {
            throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
        }
        dest.resize(std::char_traits<char>::length(dest.data()));
        dest.shrink_to_fit();
        return std::string(dest.begin(), dest.end());
    }

    std::wstring multi_to_wide_capi(std::string const& src)
    {
        std::size_t converted{};
        std::vector<wchar_t> dest(src.size(), L'\0');
        if (::_mbstowcs_s_l(&converted, dest.data(), dest.size(), src.data(), _TRUNCATE, ::_create_locale(LC_ALL, "jpn")) != 0) {
            throw std::system_error{ errno, std::system_category() };
        }
        dest.resize(std::char_traits<wchar_t>::length(dest.data()));
        dest.shrink_to_fit();
        return std::wstring(dest.begin(), dest.end());
    }

    string utf8_to_shiftJIS(string s)
    {
        wstring ws = utf8_to_wide_winapi(s);
        return wide_to_multi_winapi(ws);
    }

    string shiftJIS_to_utf8(string s)
    {
        wstring ws = multi_to_wide_winapi(s);
        return wide_to_utf8_winapi(ws);
    }

    bool combo(string label, int* idx, vector<string> str)
    {
        return ImGui::Combo(label.c_str(), idx,
            [](void* vec, int idx, const char** out_text) {
                std::vector<std::string>* vector = reinterpret_cast<std::vector<std::string>*>(vec);
                if (idx < 0 || idx >= vector->size())return false;
                *out_text = vector->at(idx).c_str();
                return true;
            }, reinterpret_cast<void*>(&str), str.size());
    }

};
#endif
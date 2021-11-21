#include "ofMain.h"


class timelineSyncBase{
public:
    virtual void play       (bool byMaster = false);
    virtual void setPause   (bool b, bool byMaster = false);
    virtual void stop       (bool byMaster = false);
    virtual void setPosition(uint64_t millis, bool byMaster = false);
}

// 再生
// 一時停止
// シーク（setposition）

// メモ：timelineをマスターとする同期クラス
// 動画とリモートタイムラインをここからコントロールできるようにする
// byMasterがfalseの場合はマスターのタイムラインに指示を仰ぐ
// マスターTMから指示を出すときはbyMaster true
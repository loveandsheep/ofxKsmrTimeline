#include "block.h"

float block::getLinear(float x, uint8_t easeOption)
{
    return x;
}

float block::getConstant(float x, uint8_t easeOption)
{
    return 1.0;
}

float block::getSin(float x, uint8_t easeOption)
{
    if (easeOption == EASE_INOUT) return -(cos(PI * x) - 1) / 2;
    if (easeOption == EASE_IN) return 1 - cos((x * PI) / 2);
    if (easeOption == EASE_OUT) return sin((x * PI) / 2);
    return x;
}

float block::getQuad(float x, uint8_t easeOption)
{
    return getPow(x, easeOption, 2);
}

float block::getCubic(float x, uint8_t easeOption)
{
    return getPow(x, easeOption, 3);
}

float block::getPow(float x, uint8_t easeOption, int numPow)
{
    if (easeOption == EASE_INOUT) return x < 0.5 ? pow(2, numPow - 1) * pow(x, numPow) : 1 - pow(-2 * x + 2, numPow) / 2;
    if (easeOption == EASE_IN) return pow(x, numPow);
    if (easeOption == EASE_OUT) return 1 - pow(1 - x, numPow); 

    return 0.0f;
}

float block::getExpo(float x, uint8_t easeOption)
{
    if (easeOption == EASE_INOUT)   return x == 0 ? 0 : x == 1 ? 1 : x < 0.5 ? pow(2, 20 * x - 10) / 2 : (2 - pow(2, -20 * x + 10)) / 2;
    if (easeOption == EASE_IN)      return x == 0 ? 0 : pow(2, 10 * x - 10);
    if (easeOption == EASE_OUT)     return x == 1 ? 1 : 1 - pow(2, -10 * x);
}

float block::getRampControl(float x, uint64_t const & length)
{
    // from -> toまでを時間　lengthで変化していく
    // accの一次関数y = a * x
    // decの一次関数y = -b * x + b * T
    // 最大移動時における交点＝(b * T / (a + b), a * b * T / (a + b))
    
    float T = length;
    float a = accel / 1000.0;//単位mSec
    float b = decel / 1000.0;
    float S = b * T;//減速度の一次関数切片
    float top_x = b * T / (a + b);
    float top_y = a * b * T / (a + b);

    float progress = abs(getTo() - getFrom());
    float maxProgress = T * top_y / 2;//定義された加減速で時間内に行ける最大の移動量（三角形の面積）    
    float pDiff = maxProgress - progress;

    if (maxProgress != ls_maxProgress) {
        ls_maxProgress = maxProgress;
        sq_maxProgress = sqrt(maxProgress);
    }
    if (ls_pdiff != pDiff){
        ls_pdiff = pDiff;
        sq_pdiff = sqrt(pDiff);
    }

    float speed = top_y * (1.0 - (sq_pdiff / sq_maxProgress));//最大速度
    float t2 = T * (sq_pdiff / sq_maxProgress);//定速時間

    float t1 = (T - t2) * (decel / (accel + decel));//加速時間
    float t3 = (T - t2) * (accel / (accel + decel));//減速時間

    float time = x * length;

    float ret = 0;

    speed_max = speed;
    dump_T = T;
    dump_topY = top_y;
    dump_spd = speed;
    dump_t1 = t1;
    dump_t2 = t2;
    dump_t3 = t3;
    dump_prog = progress;
    dump_maxP = maxProgress;
    dump_pDiff = pDiff;
    dumpRamp = (t1 + t1 + t2 + t3) * speed / 2;
    
    if (time < t1)//加速時・定速時・減速時のカーブを作る
    {
        // nxの積分は[1/2 * x ^ 2]0~3
        ret = (a / 2.0) * pow(time, 2.0);
    }
    else if (time < (t1 + t2))
    {
        ret = (a / 2.0) * pow(t1, 2.0);
        ret += speed * (time - t1);
    }
    else
    {
        ret = (a / 2.0) * pow(t1, 2.0);
        ret += speed * t2;// + t1 - t1
        float dec_t2 = (-b / 2.0) * pow(t1 + t2, 2.0) + S * (t1 + t2);
        float dec_tc = (-b / 2.0) * pow(time, 2.0) + S * (time);
        ret += dec_tc - dec_t2;
    }

    return ofMap(ret, 0, progress, 0, 1, true);
}
#pragma once
#include <M5Cardputer.h>
#include "persist.h"

namespace Intro {
static void show() {
    auto& d = M5Cardputer.Display;
    const auto& f = Persist::fortune();
    d.fillScreen(0x0000);
    d.drawRect(1,1,238,133,f.col);
    d.drawRect(2,2,236,131,(uint16_t)(f.col>>1)&0x7BEF);
    d.setTextSize(1); d.setTextColor(0xFEA0);
    const char* t="YOUR DAILY FORTUNE";
    d.setCursor((240-d.textWidth(t))/2,10); d.print(t);
    d.drawFastHLine(20,22,200,0x4208);
    d.setTextColor(f.col);
    d.setCursor((240-d.textWidth(f.msg))/2,32); d.print(f.msg);
    d.setTextColor(0x4208);
    char lb[40]; snprintf(lb,sizeof(lb),"Lucky game: %s",Persist::GAME_NAMES[f.luckyGame]);
    d.setCursor((240-d.textWidth(lb))/2,48); d.print(lb);
    d.drawFastHLine(20,61,200,0x4208);
    d.setTextColor(0xFFFF);
    char cr[28]; snprintf(cr,sizeof(cr),"Credits: %d",Persist::credits);
    d.setCursor((240-d.textWidth(cr))/2,72); d.print(cr);
    if(Persist::personalBest>0){
        d.setTextColor(0xFEA0);
        char pb[32]; snprintf(pb,sizeof(pb),"Personal Best: %d",Persist::personalBest);
        d.setCursor((240-d.textWidth(pb))/2,86); d.print(pb);
    }
    int achCount=0;
    for(int i=0;i<Persist::ACH_COUNT;i++) if(Persist::isUnlocked((Persist::Ach)i)) achCount++;
    d.setTextColor(0x4208);
    char ab[32]; snprintf(ab,sizeof(ab),"Achievements: %d/%d",achCount,Persist::ACH_COUNT);
    d.setCursor((240-d.textWidth(ab))/2,100); d.print(ab);
    d.setTextColor(0x39E7);
    const char* pk="Press any key to play...";
    d.setCursor((240-d.textWidth(pk))/2,118); d.print(pk);
    unsigned long start=millis();
    while(millis()-start<8000){
        M5Cardputer.update();
        if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) break;
        delay(50);
    }
}
}

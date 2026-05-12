#pragma once
#include <M5Cardputer.h>
#include <Preferences.h>

namespace Persist {
    static int      credits          = 100;
    static int      personalBest     = 0;
    static int      sessionStart     = 100;
    static int      sessionPeak      = 100;
    static int      dayCounter       = 0;
    static uint32_t achievements     = 0;
    static bool     comebackEligible = false;

    enum Ach {
        ACH_ROYAL_FLUSH=0, ACH_LUCKY_ZERO, ACH_HIGH_ROLLER,
        ACH_COMEBACK_KID, ACH_YAHTZEE, ACH_BLACKJACK, ACH_COUNT
    };
    static const char* ACH_NAME[ACH_COUNT] = {
        "ROYAL FLUSH","LUCKY ZERO","HIGH ROLLER",
        "COMEBACK KID","YAHTZEE!","NATURAL BLACKJACK"
    };
    static const char* ACH_DESC[ACH_COUNT] = {
        "Royal Flush in Video Poker",
        "Hit 0 in Roulette",
        "Reach 500 credits",
        "Recover: under 20 to 200+",
        "Five of a kind in Dice",
        "Natural Blackjack on deal"
    };

    struct Fortune { const char* msg; uint16_t col; int luckyGame; };
    static const Fortune FORTUNES[] = {
        {"Fortune favors the bold tonight.",         0xFEA0, 0},
        {"The cards run in cycles. Be patient.",     0x07FF, 1},
        {"Every roll is a fresh start.",             0xFFFF, 2},
        {"The river bends to the wise hand.",        0x07E0, 3},
        {"Trust your instincts, they know more.",    0x07E0, 4},
        {"The wheel favors the unhurried.",          0xF81F, 5},
        {"Three losses before the jackpot.",         0xFEA0, 0},
        {"Read the room before raising stakes.",     0x07FF, 1},
        {"Luck is a pattern yet to be seen.",        0xF81F, 5},
        {"Bold plays win stories, win or lose.",     0xF800, 0},
        {"Five dice, infinite outcomes. Roll!",      0xFFFF, 2},
        {"Stars align for one bold move today.",     0x07FF, 3},
    };
    static const int N_FORTUNES = 12;
    static const char* GAME_NAMES[] = {
        "SLOT MACHINE","BLACKJACK","DICE ROLL",
        "VIDEO POKER","HI / LO","ROULETTE"
    };

    static void load() {
        Preferences p; p.begin("arcadputer", true);
        credits      = p.getInt("cr",   100);
        personalBest = p.getInt("best", 0);
        dayCounter   = p.getInt("day",  0);
        achievements = p.getUInt("ach", 0);
        p.end();
        if (credits <= 0) credits = 100;
        sessionStart = credits;
        sessionPeak  = credits;
        if (personalBest < credits) personalBest = credits;
    }

    static void save() {
        Preferences p; p.begin("arcadputer", false);
        p.putInt("cr",   credits);
        p.putInt("best", personalBest);
        p.putInt("day",  dayCounter);
        p.putUInt("ach", achievements);
        p.end();
    }

    static void showPopup(Ach a) {
        auto& d = M5Cardputer.Display;
        int px=30, py=38, pw=180, ph=58;
        d.fillRoundRect(px,py,pw,ph,7,0x0821);
        d.drawRoundRect(px,py,pw,ph,7,0xFEA0);
        d.drawRoundRect(px+1,py+1,pw-2,ph-2,6,0xFEA0);
        d.setTextSize(1);
        d.setTextColor(0xFEA0);
        const char* t1="ACHIEVEMENT UNLOCKED!";
        d.setCursor(px+(pw-d.textWidth(t1))/2,py+9); d.print(t1);
        d.setTextColor(0xFFFF);
        const char* t2=ACH_NAME[a];
        d.setCursor(px+(pw-d.textWidth(t2))/2,py+23); d.print(t2);
        d.setTextColor(0x4208);
        const char* t3=ACH_DESC[a];
        d.setCursor(px+(pw-d.textWidth(t3))/2,py+37); d.print(t3);
        M5Cardputer.Speaker.tone(880,80); delay(90);
        M5Cardputer.Speaker.tone(1100,80); delay(90);
        M5Cardputer.Speaker.tone(1320,160);
        for(int i=0;i<50;i++){M5Cardputer.update();delay(50);}
    }

    static void unlock(Ach a) {
        if (achievements & (1u<<(int)a)) return;
        achievements |= (1u<<(int)a);
        save();
        showPopup(a);
    }

    static bool isUnlocked(Ach a) { return (achievements&(1u<<(int)a))!=0; }

    static void updateCredits() {
        if (credits > sessionPeak) sessionPeak = credits;
        if (credits > personalBest) { personalBest = credits; save(); }
        if (credits < 20) comebackEligible = true;
        if (credits >= 200 && comebackEligible) { comebackEligible=false; unlock(ACH_COMEBACK_KID); }
        if (credits >= 500) unlock(ACH_HIGH_ROLLER);
    }

    static void newDay() { dayCounter++; save(); }
    static const Fortune& fortune() { return FORTUNES[dayCounter % N_FORTUNES]; }
}

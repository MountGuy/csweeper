#include <Windows.h>
#include "config.h"
#include "game.h"
#include "csweeper.h"
#include "util.h"

#define GET_SCREEN_WIDTH 0
#define GET_SCREEN_HEIGHT 1

const int displayWidth = SimpleGetSystemMetrics(GET_SCREEN_WIDTH), displayHeight = SimpleGetSystemMetrics(GET_SCREEN_HEIGHT);
const int defWidth = (displayWidth - 30) / 16, defHeight = (displayHeight - 160) / 16;
const int defMine = (int) (defWidth * defHeight / 5);

Config gameConfig;

void InitializeConfigFromDefault() {
    gameConfig.difficulty = 0;
    gameConfig.mines = defMine;
    gameConfig.height = defHeight;
    height = gameConfig.height;
    gameConfig.width = defWidth;
    width = gameConfig.width;
    gameConfig.xpos = 0;
    gameConfig.ypos = 0;
    gameConfig.tick = 0;
    gameConfig.menu = 0;
    gameConfig.color = 1;
    gameConfig.times[TIME_BEGINNER] = 0;
    gameConfig.times[TIME_INTERMIDIATE] = 0;
    gameConfig.times[TIME_EXPERT] = 0;
    gameConfig.times[TIME_MAXIMUM] = 0;
    gameConfig.names[TIME_BEGINNER][0] = 0;
    gameConfig.names[TIME_INTERMIDIATE][0] = 0;
    gameConfig.names[TIME_EXPERT][0] = 0;
    gameConfig.names[TIME_MAXIMUM][0] = 0;
    
    screenHeightInPixels = GetSystemMetrics(SM_CYCAPTION) + 35;
    menuBarHeightInPixels = GetSystemMetrics(SM_CYMENU) + 1;
    windowHeightInPixels = GetSystemMetrics(SM_CYBORDER) + 1;
    windowWidthInPixels = GetSystemMetrics(SM_CXBORDER) + 14;

    HDC hDC = GetDC(GetDesktopWindow());
    int desktopColors = GetDeviceCaps(hDC, NUMCOLORS);

    ReleaseDC(GetDesktopWindow(), hDC);
}

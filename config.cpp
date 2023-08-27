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

Config GameConfig;

void InitializeConfigFromDefault() {
    GameConfig.Difficulty = 0;
    GameConfig.Mines = defMine;
    GameConfig.Height = defHeight;
    Height = GameConfig.Height;
    GameConfig.Width = defWidth;
    Width = GameConfig.Width;
    GameConfig.Xpos = 0;
    GameConfig.Ypos = 0;
    GameConfig.Tick = 0;
    GameConfig.Menu = 0;
    GameConfig.Color = 1;
    GameConfig.Times[TIME_BEGINNER] = 0;
    GameConfig.Times[TIME_INTERMIDIATE] = 0;
    GameConfig.Times[TIME_EXPERT] = 0;
    GameConfig.Times[TIME_MAXIMUM] = 0;
    GameConfig.Names[TIME_BEGINNER][0] = 0;
    GameConfig.Names[TIME_INTERMIDIATE][0] = 0;
    GameConfig.Names[TIME_EXPERT][0] = 0;
    GameConfig.Names[TIME_MAXIMUM][0] = 0;

    HDC hDC = GetDC(GetDesktopWindow());
    int desktopColors = GetDeviceCaps(hDC, NUMCOLORS);

    ReleaseDC(GetDesktopWindow(), hDC);
}

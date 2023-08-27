#pragma once
#include <Windows.h>

#define CONFIG_DIFFICULTY 0
#define CONFIG_MINES 1
#define CONFIG_HEIGHT 2
#define CONFIG_WIDTH 3
#define CONFIG_XPOS 4
#define CONFIG_YPOS 5
#define CONFIG_MENU 6
#define CONFIG_TICK 7
#define CONFIG_COLOR 8
#define CONFIG_TIME1 9
#define CONFIG_NAME1 10
#define CONFIG_TIME2 11
#define CONFIG_NAME2 12
#define CONFIG_TIME3 13
#define CONFIG_NAME3 14
#define CONFIG_TIME4 15
#define CONFIG_NAME4 16

#define TIME_BEGINNER 0
#define TIME_INTERMIDIATE 1
#define TIME_EXPERT 2
#define TIME_MAXIMUM 3

#define NAME_BEGINNER 0
#define NAME_INTERMIDIATE 1
#define NAME_EXPERT 2
#define NAME_MAXIMUM 3

typedef WCHAR NameString[32];

// Those variables here are in the same order of the strings array
// It's seems like they are represented as a struct
typedef struct _Config {
    short Difficulty;
    int Mines;

    DWORD Height;
    DWORD Width;

    // In Pixels, Screen Coordinates
    int Xpos;
    int Ypos;
    int Tick;
    int Menu;
    BOOL Color;
    int Times[4];
    NameString Names[4];
} Config;

extern Config GameConfig;

void InitializeConfigFromDefault();
void SaveConfigToRegistry();

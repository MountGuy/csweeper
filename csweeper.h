#pragma once

#include "resource.h"

extern HWND hWnd;
extern int yBottom;
extern int xRight;

extern HINSTANCE hInst;


extern int windowWidthInPixels;
extern int windowHeightInPixels;
extern int screenHeightInPixels;
extern int windowHeightIncludingMenu;
extern int menuBarHeightInPixels;
extern BOOL hasInputCaptures[2];
extern BOOL is3x3Clicks[2];
extern BOOL blink;

#define WINDOW_BORDER_MOVE_WINDOW 2
#define WINDOW_BORDER_REPAINT_WINDOW 4

#define DIFFICULTY_BEGINNER 0
#define DIFFICULTY_INTERMEDIATE 1
#define DIFFICULTY_EXPERT 2
#define DIFFICULTY_CUSTOM 3
#define TIME_TIMER_ID 1
#define BLINK_TIMER_ID 2

#define VK_Z 0x5A
#define VK_X 0x58

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializeWindowBorder(DWORD borderFlags);
void InitializeCheckedMenuItems();
void InitializeMenu(DWORD menuFlags);
void SetMenuItemState(DWORD uID, BOOL isChecked);
DWORD SimpleGetSystemMetrics(DWORD val);
__inline LRESULT CaptureMouseInput(UINT message, WPARAM wParam, LPARAM lParam);
__inline LRESULT MouseMoveHandler(UINT message, WPARAM wParam, LPARAM lParam);

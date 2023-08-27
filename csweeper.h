#pragma once

#include "resource.h"

extern HWND ghWnd;
extern int yBottom;
extern int xRight;

extern WCHAR ClassName[32];
extern HINSTANCE hInst;


extern int WindowWidthInPixels;
extern int WindowHeightInPixels;
extern int ScreenHeightInPixels;
extern int WindowHeightIncludingMenu;
extern int MenuBarHeightInPixels;
extern BOOL HasMouseCapture;
extern BOOL Is3x3Click;
extern BOOL HasMouseCapture;

#define WINDOW_BORDER_MOVE_WINDOW 2
#define WINDOW_BORDER_REPAINT_WINDOW 4

#define DIFFICULTY_BEGINNER 0
#define DIFFICULTY_INTERMEDIATE 1
#define DIFFICULTY_EXPERT 2
#define DIFFICULTY_CUSTOM 3
#define TIMER_ID 1

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializeWindowBorder(DWORD borderFlags);
void InitializeCheckedMenuItems();
void InitializeMenu(DWORD menuFlags);
void SetMenuItemState(DWORD uID, BOOL isChecked);
DWORD SimpleGetSystemMetrics(DWORD val);
__inline LRESULT CaptureMouseInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
__inline LRESULT MouseMoveHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
extern BOOL hasMouseCapture;
extern BOOL is3x3Click;

#define WINDOW_BORDER_MOVE_WINDOW 2
#define WINDOW_BORDER_REPAINT_WINDOW 4

#define DIFFICULTY_BEGINNER 0
#define DIFFICULTY_INTERMEDIATE 1
#define DIFFICULTY_EXPERT 2
#define DIFFICULTY_CUSTOM 3
#define TIMER_ID 1

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

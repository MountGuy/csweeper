#include "framework.h"
#include "csweeper.h"
#include "drawing.h"
#include "game.h"
#include "config.h"
#include "util.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];


#define GAME_MENU_INDEX 0
#define HELP_MENU_INDEX 1

#define GET_SCREEN_WIDTH 0
#define GET_SCREEN_HEIGHT 1

#define BLACK_COLOR 0
#define WHITE_COLOR 0x00FFFFFF

int windowWidthInPixels;
int windowHeightInPixels;
int screenHeightInPixels;
int windowHeightIncludingMenu;
int menuBarHeightInPixels;
int CheatPasswordIndex;

int yBottom = 0;
int xRight = 0;

HWND hWnd;
HMENU hMenu;
BOOL Minimized;

BOOL hasMouseCapture;
BOOL is3x3Click;
BOOL IsMenuOpen;


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CSWEEPER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CSWEEPER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CSWEEPER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    InitializeConfigFromDefault();

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    InitializeWindowBorder(1);

    if (!InitializeBitmapsAndBlockArray()) {
        DisplayErrorMessage(ID_OUT_OF_MEM);
        return 0;
    }

    InitializeMenu(gameConfig.menu);
    InitializeNewGame();
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CSWEEPER));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        if (hasMouseCapture) {
            ReleaseMouseCapture();
        }
        break;
    case WM_RBUTTONDOWN:
        if (!GAME_IS_ON()) {
            break;
        }

        // Left button is already clicked
        if (hasMouseCapture) {
            ReleaseBlocksClick();
            is3x3Click = TRUE;

            PostMessageW(hwnd, WM_MOUSEMOVE, wParam, lParam);
        }
        // Left button is clicked together
        else if (wParam & 1) {
            CaptureMouseInput(message, wParam, lParam);
        }
        // Right button is clicked only
        else if (!IsMenuOpen) {
            BoardPoint point = { (HIWORD(lParam) - 39) / 16, (LOWORD(lParam) + 4) / 16 };
            HandleRightClick(point);
        }

        return FALSE;
    case WM_MOUSEMOVE:
        return MouseMoveHandler(message, wParam, lParam);

    case WM_LBUTTONDOWN:
        // Handle for smile click
        // The case if the player clicked the smile button
        if (HandleLeftClick((DWORD)lParam)) {
            return 0;
        }

        if (!GAME_IS_ON()) {
            break;
        }

        // Shift or right button is clicked together
        is3x3Click = (wParam & 6) ? TRUE : FALSE;
        return CaptureMouseInput(message, wParam, lParam);

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RedrawUIOnDC(hdc);
            EndPaint(hwnd, &ps);
        }
        break;
    case WM_TIMER:
        TickSeconds();
        return FALSE;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

// Set window size
void InitializeWindowBorder(DWORD borderFlags) {
    BOOL differentCordsForMenus = FALSE;
    RECT rcGameMenu;
    RECT rcHelpMenu;

    if (hWnd == NULL) {
        return;
    }

    windowHeightIncludingMenu = screenHeightInPixels;

    if (gameConfig.menu & 1) {
        windowHeightIncludingMenu += menuBarHeightInPixels;


        if (hMenu != NULL &&
            GetMenuItemRect(hWnd, hMenu, GAME_MENU_INDEX, &rcGameMenu) &&
            GetMenuItemRect(hWnd, hMenu, HELP_MENU_INDEX, &rcHelpMenu) &&
            rcGameMenu.top != rcHelpMenu.top) {
            differentCordsForMenus = TRUE;
            // Add it twice
            windowHeightIncludingMenu += menuBarHeightInPixels;
        }
    }

    xRight = (width * 16) + 24;   // 24 is the size of pixels in the side of the window, 16 is the size of block
    yBottom = (height * 16) + 67; // 16 is the size of the pixels in a block, 67 is the size of pixels in the sides

    // Check If The Place On The Screen Overflows The End Of The Screen
    // If it is, Move The Window To A New Place
    int diffFromEnd;

    // If diffFromEnd is negative, it means the window does not overflow the screen
    // If diffFromEnd is positive, it means the window overflows the screen and needs to be moved
    diffFromEnd = (xRight + gameConfig.xpos) - SimpleGetSystemMetrics(GET_SCREEN_WIDTH);

    if (diffFromEnd > 0) {
        borderFlags |= WINDOW_BORDER_MOVE_WINDOW;
        gameConfig.xpos -= diffFromEnd;
    }

    diffFromEnd = (yBottom + gameConfig.ypos) - SimpleGetSystemMetrics(GET_SCREEN_HEIGHT);

    if (diffFromEnd > 0) {
        borderFlags |= WINDOW_BORDER_MOVE_WINDOW;
        gameConfig.ypos -= diffFromEnd;
    }

    if (Minimized) {
        return;
    }

    if (borderFlags & WINDOW_BORDER_MOVE_WINDOW) {
        MoveWindow(hWnd,
            gameConfig.xpos, gameConfig.ypos,
            windowWidthInPixels + xRight,
            windowHeightIncludingMenu + yBottom,
            TRUE);
    }

    if (differentCordsForMenus &&
        hMenu != NULL &&
        GetMenuItemRect(hWnd, hMenu, 0, &rcGameMenu) &&
        GetMenuItemRect(hWnd, hMenu, 1, &rcHelpMenu) &&
        rcGameMenu.top == rcHelpMenu.top) {
        windowHeightIncludingMenu -= menuBarHeightInPixels;

        MoveWindow(hWnd, gameConfig.xpos, gameConfig.ypos, windowWidthInPixels + xRight, windowHeightIncludingMenu + yBottom, TRUE);
    }

    if (borderFlags & WINDOW_BORDER_REPAINT_WINDOW) {
        RECT rc;
        SetRect(&rc, 0, 0, xRight, yBottom);
        // Cause a Repaint of the whole window
        InvalidateRect(hWnd, &rc, TRUE);
    }
}

void InitializeCheckedMenuItems() {
    // Set Difficulty Level Checkbox
    SetMenuItemState(ID_GAME_BEGINNER, gameConfig.difficulty == DIFFICULTY_BEGINNER);
    SetMenuItemState(ID_GAME_INTERMEDIATE, gameConfig.difficulty == DIFFICULTY_INTERMEDIATE);
    SetMenuItemState(ID_GAME_EXPERT, gameConfig.difficulty == DIFFICULTY_EXPERT);
    SetMenuItemState(ID_GAME_MAXIMUM, gameConfig.difficulty == DIFFICULTY_CUSTOM);
}

void InitializeMenu(DWORD menuFlags) {
    gameConfig.menu = menuFlags;
    InitializeCheckedMenuItems();
    //SetMenu(hWnd, (GameConfig.Menu & 1) ? NULL : hMenu);
    InitializeWindowBorder(WINDOW_BORDER_MOVE_WINDOW);
}

void SetMenuItemState(DWORD uID, BOOL isChecked) {
    CheckMenuItem(hMenu, uID, isChecked ? MF_CHECKED : MF_BYCOMMAND);
}

// Wrapper of GetSystemMetrics
DWORD SimpleGetSystemMetrics(DWORD val) {
    DWORD result;

    switch (val) {
    case GET_SCREEN_WIDTH:
        result = GetSystemMetrics(SM_CXVIRTUALSCREEN);

        if (result == 0) {
            result = GetSystemMetrics(SM_CXSCREEN);
        }
        break;
    case GET_SCREEN_HEIGHT:
        result = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        if (result == 0) {
            result = GetSystemMetrics(SM_CYSCREEN);
        }
        break;
    default:
        result = GetSystemMetrics(val);
    }

    return result;
}

// The only function that makes hasMouseCapture = True
// Called by WM_LBUTTONDOWN and WM_RBUTTONDOWN
__inline LRESULT CaptureMouseInput(UINT message, WPARAM wParam, LPARAM lParam) {
    // Shared code...
    SetCapture(hWnd);
    BoardPoint point = { -1, -1 };
    clickedPoint = point;
    hasMouseCapture = TRUE;
    DisplaySmile(SMILE_WOW);
    return MouseMoveHandler(message, wParam, lParam);
}

// Called by CaptureMouseInput and WM_MOUSEMOVE
__inline LRESULT MouseMoveHandler(UINT message, WPARAM wParam, LPARAM lParam) {
    // WM_MOUSEMOVE_Handler!
    if (!hasMouseCapture) {
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    else if (!GAME_IS_ON()) {
        ReleaseMouseCapture();
    }
    else {
        // Update Mouse Block
        BoardPoint point = { (HIWORD(lParam) - 39) / 16, (LOWORD(lParam) + 4) / 16};
        UpdateClickedPointsState(point);
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}
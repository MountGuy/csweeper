// csweeper.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "csweeper.h"
#include "drawing.h"
#include "game.h"
#include "config.h"
#include "util.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.


#define GAME_MENU_INDEX 0
#define HELP_MENU_INDEX 1

#define GET_SCREEN_WIDTH 0
#define GET_SCREEN_HEIGHT 1

#define BLACK_COLOR 0
#define WHITE_COLOR 0x00FFFFFF

int WindowWidthInPixels;
int WindowHeightInPixels;
int ScreenHeightInPixels;
int WindowHeightIncludingMenu;
int MenuBarHeightInPixels;
int CheatPasswordIndex;

int yBottom = 0;
int xRight = 0;

HWND ghWnd;
HMENU hMenu;
BOOL Minimized;
BOOL IgnoreSingleClick;

WCHAR ClassName[32];

BOOL HasMouseCapture;
BOOL Is3x3Click;
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
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   ghWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!ghWnd)
   {
      return FALSE;
   }

   ShowWindow(ghWnd, nCmdShow);
   UpdateWindow(ghWnd);

   return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CSWEEPER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    InitializeConfigFromDefault();

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    InitializeWindowBorder(1);

    if (!InitializeBitmapsAndBlockArray()) {
        DisplayErrorMessage(ID_OUT_OF_MEM);
        return 0;
    }

    InitializeMenu(GameConfig.Menu);
    InitializeNewGame();
    ShowWindow(ghWnd, nCmdShow);
    UpdateWindow(ghWnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CSWEEPER));

    MSG msg;

    // 기본 메시지 루프입니다:
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        if (HasMouseCapture) {
            ReleaseMouseCapture();
        }
        break;
    case WM_RBUTTONDOWN:
        if (IgnoreSingleClick) {
            IgnoreSingleClick = FALSE;
            return FALSE;
        }

        if ((StateFlags & STATE_GAME_IS_ON) == 0) {
            break;
        }

        if (HasMouseCapture) {
            ReleaseBlocksClick();
            Is3x3Click = TRUE;

            PostMessageW(hWnd, WM_MOUSEMOVE, wParam, lParam);
        }
        else if (wParam & 1) {
            CaptureMouseInput(ghWnd, message, wParam, lParam);
        }
        else if (!IsMenuOpen) {
            BoardPoint point = { (HIWORD(lParam) - 39) / 16, (LOWORD(lParam) + 4) / 16 };
            HandleRightClick(point);
        }

        return FALSE;
    case WM_MOUSEMOVE:
        return MouseMoveHandler(ghWnd, message, wParam, lParam);

    case WM_LBUTTONDOWN:
        if (IgnoreSingleClick) {
            IgnoreSingleClick = FALSE;
            return FALSE;
        }
        if (HandleLeftClick((DWORD)lParam)) {
            return 0;
        }

        if (!(StateFlags & STATE_GAME_IS_ON)) {
            break;
        }

        Is3x3Click = (wParam & 6) ? TRUE : FALSE;
        return CaptureMouseInput(ghWnd, message, wParam, lParam);
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            RedrawUIOnDC(hdc);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_TIMER:
        TickSeconds();
        return FALSE;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void InitializeWindowBorder(DWORD borderFlags) {
    BOOL differentCordsForMenus = FALSE;
    RECT rcGameMenu;
    RECT rcHelpMenu;

    if (ghWnd == NULL) {
        return;
    }

    WindowHeightIncludingMenu = ScreenHeightInPixels;

    if (GameConfig.Menu & 1) {
        WindowHeightIncludingMenu += MenuBarHeightInPixels;


        if (hMenu != NULL &&
            GetMenuItemRect(ghWnd, hMenu, GAME_MENU_INDEX, &rcGameMenu) &&
            GetMenuItemRect(ghWnd, hMenu, HELP_MENU_INDEX, &rcHelpMenu) &&
            rcGameMenu.top != rcHelpMenu.top) {
            differentCordsForMenus = TRUE;
            // Add it twice
            WindowHeightIncludingMenu += MenuBarHeightInPixels;
        }
    }

    xRight = (Width * 16) + 24;   // 24 is the size of pixels in the side of the window, 16 is the size of block
    yBottom = (Height * 16) + 67; // 16 is the size of the pixels in a block, 67 is the size of pixels in the sides

    // Check If The Place On The Screen Overflows The End Of The Screen
    // If it is, Move The Window To A New Place
    int diffFromEnd;

    // If diffFromEnd is negative, it means the window does not overflow the screen
    // If diffFromEnd is positive, it means the window overflows the screen and needs to be moved
    diffFromEnd = (xRight + GameConfig.Xpos) - SimpleGetSystemMetrics(GET_SCREEN_WIDTH);

    if (diffFromEnd > 0) {
        borderFlags |= WINDOW_BORDER_MOVE_WINDOW;
        GameConfig.Xpos -= diffFromEnd;
    }

    diffFromEnd = (yBottom + GameConfig.Ypos) - SimpleGetSystemMetrics(GET_SCREEN_HEIGHT);

    if (diffFromEnd > 0) {
        borderFlags |= WINDOW_BORDER_MOVE_WINDOW;
        GameConfig.Ypos -= diffFromEnd;
    }

    if (Minimized) {
        return;
    }

    if (borderFlags & WINDOW_BORDER_MOVE_WINDOW) {
        MoveWindow(ghWnd,
            GameConfig.Xpos, GameConfig.Ypos,
            WindowWidthInPixels + xRight,
            yBottom + WindowHeightIncludingMenu,
            TRUE);
    }

    if (differentCordsForMenus &&
        hMenu != NULL &&
        GetMenuItemRect(ghWnd, hMenu, 0, &rcGameMenu) &&
        GetMenuItemRect(ghWnd, hMenu, 1, &rcHelpMenu) &&
        rcGameMenu.top == rcHelpMenu.top) {
        WindowHeightIncludingMenu -= MenuBarHeightInPixels;

        MoveWindow(ghWnd, GameConfig.Xpos, GameConfig.Ypos, WindowWidthInPixels + xRight, yBottom + WindowHeightIncludingMenu, TRUE);
    }

    if (borderFlags & WINDOW_BORDER_REPAINT_WINDOW) {
        RECT rc;
        SetRect(&rc, 0, 0, xRight, yBottom);
        // Cause a Repaint of the whole window
        InvalidateRect(ghWnd, &rc, TRUE);
    }
}

void InitializeCheckedMenuItems() {
    // Set Difficulty Level Checkbox
    SetMenuItemState(ID_GAME_BEGINNER, GameConfig.Difficulty == DIFFICULTY_BEGINNER);
    SetMenuItemState(ID_GAME_INTERMEDIATE, GameConfig.Difficulty == DIFFICULTY_INTERMEDIATE);
    SetMenuItemState(ID_GAME_EXPERT, GameConfig.Difficulty == DIFFICULTY_EXPERT);
    SetMenuItemState(ID_GAME_MAXIMUM, GameConfig.Difficulty == DIFFICULTY_CUSTOM);
}

void InitializeMenu(DWORD menuFlags) {
    GameConfig.Menu = menuFlags;
    InitializeCheckedMenuItems();
    //SetMenu(ghWnd, (GameConfig.Menu & 1) ? NULL : hMenu);
    InitializeWindowBorder(WINDOW_BORDER_MOVE_WINDOW);
}

void SetMenuItemState(DWORD uID, BOOL isChecked) {
    CheckMenuItem(hMenu, uID, isChecked ? MF_CHECKED : MF_BYCOMMAND);
}

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

__inline LRESULT CaptureMouseInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Shared code...
    SetCapture(ghWnd);
    BoardPoint point = { -1, -1 };
    ClickedBlock = point;
    HasMouseCapture = TRUE;
    DisplaySmile(SMILE_WOW);
    return MouseMoveHandler(hwnd, uMsg, wParam, lParam);
}

__inline LRESULT MouseMoveHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // WM_MOUSEMOVE_Handler!
    if (!HasMouseCapture) {
        return DefWindowProcW(ghWnd, uMsg, wParam, lParam);
    }
    else if (!(StateFlags & STATE_GAME_IS_ON)) {
        ReleaseMouseCapture();
    }
    else {
        // Update Mouse Block
        BoardPoint point = { (HIWORD(lParam) - 39) / 16, (LOWORD(lParam) + 4) / 16};
        UpdateClickedBlocksState(point);
    }

    return DefWindowProcW(ghWnd, uMsg, wParam, lParam);
}
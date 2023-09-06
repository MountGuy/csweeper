#pragma once
#include <Windows.h>

typedef struct {
    int row;
    int column;
} BoardPoint;

extern int globalSmileID;
extern int leftFlags;
extern int width;
extern int height;
extern int timerSeconds;
extern DWORD stateFlags;

#define STATE_GAME_IS_ON 0b01
#define STATE_WINDOW_MINIMIZED  0b10
#define STATE_WINDOW_MINIMIZED_2 0b1000
#define STATE_GAME_FINISHED 0b10000

#define GAME_IS_ON() (stateFlags & STATE_GAME_IS_ON)

#define BLOCK_STATE_READ_EMPTY 0
#define BLOCK_STATE_NUMBER_1 1
#define BLOCK_STATE_NUMBER_2 2
#define BLOCK_STATE_NUMBER_3 3
#define BLOCK_STATE_NUMBER_4 4
#define BLOCK_STATE_NUMBER_5 5
#define BLOCK_STATE_NUMBER_6 6
#define BLOCK_STATE_NUMBER_7 7
#define BLOCK_STATE_NUMBER_8 8
#define BLOCK_STATE_BLACK_BOMB 9
#define BLOCK_STATE_EMPTY_UNCLICKED 0xA

#define BLOCK_STATE_BASE_FLAG 0xB
#define BLOCK_STATE_1P_FLAG 0xB
#define BLOCK_STATE_2P_FLAG 0xC

#define BLOCK_STATE_BASE_BOMB_WITH_X 0xD
#define BLOCK_STATE_1P_BOMB_WITH_X 0xD
#define BLOCK_STATE_2P_BOMB_WITH_X 0xE

#define BLOCK_STATE_BASE_BOMB_RED_BACKGROUND 0xF
#define BLOCK_STATE_1P_BOMB_RED_BACKGROUND 0xF
#define BLOCK_STATE_2P_BOMB_RED_BACKGROUND 0x10

#define BLOCK_STATE_BORDER_VALUE 0x11
#define BLOCK_STATE_MASK 0x1F

#define REVEALED_FLAG 0x20
#define BOMB_FLAG 0x40
#define FOCUSED_FLAG_1P 0x80
#define FOCUSED_FLAG_2P 0x100
#define BLOCK_INFO_MASK 0x1E0
#define BLOCK_FOCUSED_MASK 0x180

#define POINT_EQUAL(point1, point2) ((point1).row == (point2).row && (point1).column == (point2).column)

#define BLOCK_IS_REVEALED(block) ((block) & REVEALED_FLAG)
#define BLOCK_IS_BOMB(block) ((block) & BOMB_FLAG)

#define ACCESS_BLOCK(point) (blockArray[point.row][point.column])
#define BLOCK_STATE(block) ((block) & BLOCK_STATE_MASK)
#define BLOCK_INFO(block) ((block) & BLOCK_INFO_MASK)
#define BLOCK_IS_FOCUSED(block) ((block) & BLOCK_FOCUSED_MASK)
#define BLOCK_IS_STATE(block, state) (BLOCK_STATE(block) == state)

#define BOARD_MAX_HEIGHT 300
#define BOARD_MAX_WIDTH 1000

#define ID_1P 0
#define ID_2P 1

#define BLOCK_STATE_FLAG(playerID) (playerID + BLOCK_STATE_BASE_FLAG)
#define BLOCK_STATE_BOMB_WITH_X(playerID) (playerID + BLOCK_STATE_BASE_BOMB_WITH_X)
#define BLOCK_STATE_BOMB_RED_BACKGROUND(playerID) (playerID + BLOCK_STATE_BASE_BOMB_RED_BACKGROUND)
#define POINT_OF_PLAYER(playerID) (focusedPoints[playerID])
#define PLAYER_READING_BLOCK(block, playerID) (BLOCK_INFO(block) & focusedFlags[playerID])
#define SET_SOLVE_STATE(point, state) (solveState[point.row][point.column] = (state))

extern BYTE blockStateFlags[2];
extern BYTE blockBombWithXs[2];
extern BYTE blockBombRedBackgrounds[2];
extern BYTE focusedFlags[2];
extern const BoardPoint nullPoint;

extern BYTE blockArray[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];
extern BoardPoint focusedPoints[2];
extern BoardPoint cursorPoint;

extern int numberOfRevealedBlocks;
extern int numberOfEmptyBlocks;

void InitializeNewGame();
BOOL InitializeBitmapsAndBlockArray();
void InitializeBlockArrayBorders();

void ReleaseInputCapture(int playerID);
__inline void ReleaseBlocksInput(int playerID);
void ChangeBlockState(BoardPoint point, BYTE blockState);
void HandleBlockInput(int playerID);
void HandleNormalBlockInput(int playerID);
void Handle3x3BlockInput(int playerID);

void UpdateInputPointsState(BoardPoint point, int playerID);
void UpdateInputPointsStateNormal(BoardPoint newPoint, BoardPoint oldPoint, int playerID);
void UpdateInputPointsState3x3(BoardPoint newPoint, BoardPoint oldPoint, int playerID);

void UpdateBlockStateToFocused(BoardPoint point, int playerID);
void UpdateBlockStateToUnFocused(BoardPoint point, int playerID);

__inline BOOL IsInBoardRange(BoardPoint point);
__inline void ReplaceFirstNonBomb(BoardPoint point);
int CountNearFlags(BoardPoint point);
int CountNearBombs(BoardPoint point);
int CountNearUnRevealed(BoardPoint point);
void ExpandEmptyBlock(BoardPoint point);
void ShowBlockValue(BoardPoint point);
void RevealAllBombs(BYTE revealedBombsState, int playerID);
void FinishGame(BOOL isWon, int playerID);

BOOL HandleLeftClick(DWORD dwLocation);
void HandleRightInput(BoardPoint point, int playerID);
void TickSeconds();

BOOL Solve(BoardPoint entryPoint);

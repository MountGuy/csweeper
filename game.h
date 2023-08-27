#pragma once
#include <Windows.h>

typedef struct {
    int Row;
    int Column;
} BoardPoint;

extern int GlobalSmileId;
extern int LeftFlags;
extern int Width;
extern int Height;
extern int TimerSeconds;
extern DWORD StateFlags;


#define STATE_GAME_IS_ON 0b01
#define STATE_WINDOW_MINIMIZED  0b10
#define STATE_WINDOW_MINIMIZED_2 0b1000
#define STATE_GAME_FINISHED 0b10000

#define BLOCK_STATE_BORDER_VALUE 0x11
#define BLOCK_STATE_2P_BOMB_RED_BACKGROUND 0x10
#define BLOCK_STATE_1P_BOMB_RED_BACKGROUND 0xF
#define BLOCK_STATE_2P_BOMB_WITH_X 0xE
#define BLOCK_STATE_1P_BOMB_WITH_X 0xD
#define BLOCK_STATE_2P_FLAG 0xC
#define BLOCK_STATE_1P_FLAG 0xB
#define BLOCK_STATE_EMPTY_UNCLICKED 0xA
#define BLOCK_STATE_BLACK_BOMB 9
#define BLOCK_STATE_NUMBER_8 8
#define BLOCK_STATE_NUMBER_7 7
#define BLOCK_STATE_NUMBER_6 6
#define BLOCK_STATE_NUMBER_5 5
#define BLOCK_STATE_NUMBER_4 4
#define BLOCK_STATE_NUMBER_3 3
#define BLOCK_STATE_NUMBER_2 2
#define BLOCK_STATE_NUMBER_1 1
#define BLOCK_STATE_READ_EMPTY 0
#define BLOCK_STATE_MASK 0x1F

#define REVEALED_FLAG 0x40
#define BOMB_FLAG 0x80
#define BLOCK_INFO_MASK 0xC0

#define BLOCK_IS_REVEALED(Block) ((Block) & REVEALED_FLAG)
#define BLOCK_IS_BOMB(Block) ((Block) & BOMB_FLAG)

#define ACCESS_BLOCK(point) (StateArray[point.Row][point.Column])
#define BLOCK_IS_STATE(Block, State) ((Block & BLOCK_STATE_MASK) == State)
#define BLOCK_STATE(Block) ((Block) & BLOCK_STATE_MASK)
#define BLOCK_INFO(Block) ((Block) & BLOCK_INFO_MASK)

#define BOARD_MAX_HEGITH 3000
#define BOARD_MAX_WIDTH 3000
extern BYTE StateArray[BOARD_MAX_HEGITH][BOARD_MAX_WIDTH];
extern BoardPoint ClickedBlock;

void InitializeNewGame();
BOOL InitializeBitmapsAndBlockArray();
void InitializeBlockArrayBorders();

void ReleaseMouseCapture();
__inline void ReleaseBlocksClick();
void ChangeBlockState(BoardPoint point, BYTE blockState);
void HandleBlockClick();
void UpdateClickedBlocksState(BoardPoint point);
void HandleNormalBlockClick(BoardPoint point);
__inline void UpdateClickedBlocksStateNormal(BoardPoint newClick, BoardPoint oldClick);
void Handle3x3BlockClick(BoardPoint point);
__inline void UpdateClickedBlocksState3x3(BoardPoint newClick, BoardPoint oldClick);
void UpdateBlockStateToClicked(BoardPoint point);
void UpdateBlockStateToUnclicked(BoardPoint point);

__inline BOOL IsInBoardRange(BoardPoint point);
__inline void ReplaceFirstNonBomb(BoardPoint point, PBYTE pFunctionBlock);
int GetFlagBlocksCount(BoardPoint point);
int CountNearBombs(BoardPoint point);
void ExpandEmptyBlock(BoardPoint point);
void ShowBlockValue(BoardPoint point);
void RevealAllBombs(BYTE revealedBombsState);
void FinishGame(BOOL isWon);

BOOL HandleLeftClick(DWORD dwLocation);
void HandleRightClick(BoardPoint point);
void TickSeconds();

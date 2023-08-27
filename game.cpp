#include <Windows.h>
#include <stdio.h>
#include "game.h"
#include "csweeper.h"
#include "drawing.h"
#include "config.h"
#include "util.h"

DWORD StateFlags = STATE_GAME_FINISHED | STATE_WINDOW_MINIMIZED;

BOOL IsTimerOnAndShowed;
BOOL IsTimerOnTemp;

BYTE StateArray[BOARD_MAX_HEGITH][BOARD_MAX_WIDTH];
BoardPoint ClickedBlock = { -1, -1 };
const BoardPoint NullPoint = { -2, -2 };

int GlobalSmileId;
int LeftFlags;
int TimerSeconds;
int MinesCopy;
int Width;
int Height;
int NumberOfEmptyBlocks;
int NumberOfRevealedBlocks;

int CurrentRowColumnListIndex;
int RowsList[10000];
int ColumnsList[10000];

// New game
void InitializeNewGame() {
    DWORD borderFlags;

    if (GameConfig.Width == Width && GameConfig.Height == Height) {
        borderFlags = WINDOW_BORDER_REPAINT_WINDOW;
    }
    else {
        borderFlags = WINDOW_BORDER_MOVE_WINDOW | WINDOW_BORDER_REPAINT_WINDOW;
    }

    Width = GameConfig.Width;
    Height = GameConfig.Height;

    InitializeBlockArrayBorders();

    GlobalSmileId = 0;

    // Setup all the mines
    MinesCopy = GameConfig.Mines;

    do {
        BoardPoint randomPoint;

        // Find a location for the mine
        do {
            randomPoint.Column = GetRandom(Width) + 1;
            randomPoint.Row = GetRandom(Height) + 1;
        } while (BLOCK_IS_REVEALED(ACCESS_BLOCK(randomPoint)));

        // SET A MINE
        ACCESS_BLOCK(randomPoint) |= BOMB_FLAG;
        MinesCopy--;
    } while (MinesCopy);

    TimerSeconds = 0;
    MinesCopy = GameConfig.Mines;
    LeftFlags = MinesCopy;
    NumberOfRevealedBlocks = 0;
    NumberOfEmptyBlocks = (Height * Width) - GameConfig.Mines;
    StateFlags = STATE_GAME_IS_ON;
    AddAndDisplayLeftFlags(0); // Should have called DisplayLeftFlags()!
    InitializeWindowBorder(borderFlags);
}

// Load resource and initialize board
BOOL InitializeBitmapsAndBlockArray() {
    if (LoadBitmaps()) {
        InitializeBlockArrayBorders();
        return TRUE;
    }

    return FALSE;
}

// Fill in the board with borders
void InitializeBlockArrayBorders() {

    for (int i = 0; i < BOARD_MAX_HEGITH; i++) {
        for (int j = 0; j < BOARD_MAX_WIDTH; j++) {
            StateArray[i][j] = BLOCK_STATE_EMPTY_UNCLICKED;
        }
    }
    for (int column = Width + 1; column >= 0; column--) {
        // Fill upper border
        StateArray[0][column] = BLOCK_STATE_BORDER_VALUE;

        // Fill lower border
        StateArray[Height + 1][column] = BLOCK_STATE_BORDER_VALUE;
    }

    for (int row = Height + 1; row >= 0; row--) {
        // Fill left border
        StateArray[row][0] = BLOCK_STATE_BORDER_VALUE;

        // Fill right border
        StateArray[row][Width + 1] = BLOCK_STATE_BORDER_VALUE;
    }
}

// Called on button release
// Called on MouseMove 
void ReleaseMouseCapture() {
    HasMouseCapture = FALSE;
    ReleaseCapture();

    if (StateFlags & STATE_GAME_IS_ON) {
        // Mouse move always gets here
        HandleBlockClick();
        ReleaseBlocksClick();
    }
    else {
        HandleBlockClick();
    }
}

__inline void ReleaseBlocksClick() {
    UpdateClickedBlocksState(NullPoint);
}

// Change the block state and draw it
void ChangeBlockState(BoardPoint point, BYTE blockState) {
    PBYTE pBlock = &ACCESS_BLOCK(point);

    *pBlock = (*pBlock & 0xE0) | blockState;

    DrawBlock(point);
}

// Handles every block click release
void HandleBlockClick() {
    if (IsInBoardRange(ClickedBlock)) {
        // First Click! Initialize Timer 
        if (NumberOfRevealedBlocks == 0 && TimerSeconds == 0) {
            TimerSeconds++;
            DisplayTimerSeconds();
            IsTimerOnAndShowed = TRUE;

            if (!SetTimer(ghWnd, TIMER_ID, 1000, NULL)) {
                DisplayErrorMessage(ID_TIMER_ERROR);
            }
        }

        //In valid: skip this click
        if (!(StateFlags & STATE_GAME_IS_ON)) {
            ClickedBlock = NullPoint;
            return;
        }

        // Case of 3x3 click
        if (Is3x3Click) {
            Handle3x3BlockClick(ClickedBlock);
        }
        else {
            BYTE blockValue = ACCESS_BLOCK(ClickedBlock);

            if (!BLOCK_IS_REVEALED(blockValue) && !BLOCK_IS_STATE(blockValue, BLOCK_STATE_1P_FLAG)) {
                HandleNormalBlockClick(ClickedBlock);
            }
        }
    }

    DisplaySmile(GlobalSmileId);
}

void UpdateClickedBlocksState(BoardPoint point) {
    if (point.Column == ClickedBlock.Column && point.Row == ClickedBlock.Row) {
        return;
    }

    // Save old click point
    const BoardPoint oldClickPoint = ClickedBlock;

    // Update new click point
    ClickedBlock = point;

    if (Is3x3Click) {
        UpdateClickedBlocksState3x3(point, oldClickPoint);
    }
    else {
        UpdateClickedBlocksStateNormal(point, oldClickPoint);
    }
}

void HandleNormalBlockClick(BoardPoint point) {
    PBYTE pFunctionBlock = &ACCESS_BLOCK(point);

    // Click an empty block
    if (!BLOCK_IS_BOMB(*pFunctionBlock)) {
        ExpandEmptyBlock(point);

        if (NumberOfRevealedBlocks == NumberOfEmptyBlocks) {
            FinishGame(TRUE);
        }
    }
    // Clicked a bomb and it's the first block 
    else if (NumberOfRevealedBlocks == 0) {
        ReplaceFirstNonBomb(point, pFunctionBlock);
    }
    // Clicked A Bomb
    else {
        ChangeBlockState(point, REVEALED_FLAG | BLOCK_STATE_1P_BOMB_RED_BACKGROUND);
        FinishGame(FALSE);
    }
}

__inline void UpdateClickedBlocksStateNormal(BoardPoint newClick, BoardPoint oldClick) {
    if (IsInBoardRange(oldClick) && !(BLOCK_IS_REVEALED(ACCESS_BLOCK(oldClick)))) {
        UpdateBlockStateToUnclicked(oldClick);
        DrawBlock(oldClick);
    }

    if (IsInBoardRange(newClick)) {
        const BYTE block = ACCESS_BLOCK(newClick);

        if (!BLOCK_IS_REVEALED(block) && !BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG)) {
            UpdateBlockStateToClicked(ClickedBlock);
            DrawBlock(ClickedBlock);
        }
    }
}

// 3x3 click handler. Assume that the point is in board range
void Handle3x3BlockClick(BoardPoint point) {
    BYTE Block = ACCESS_BLOCK(point);
    BYTE BlockState = BLOCK_STATE(Block);

    if (BLOCK_IS_REVEALED(Block) && GetFlagBlocksCount(point) == BlockState) {
        BOOL lostGame = FALSE;

        for (int loopRow = (point.Row - 1); loopRow <= (point.Row + 1); loopRow++) {
            for (int loopColumn = (point.Column - 1); loopColumn <= (point.Column + 1); loopColumn++) {
                BoardPoint point = { loopRow, loopColumn };
                BYTE Block = ACCESS_BLOCK(point);

                // The user clicked a non flaged bomb
                if (BLOCK_STATE(Block) != BLOCK_STATE_1P_FLAG && BLOCK_IS_BOMB(Block)) {
                    lostGame = TRUE;
                    ChangeBlockState(point, REVEALED_FLAG | BLOCK_STATE_1P_BOMB_RED_BACKGROUND);
                }
                // The user clicked an empty block
                else if (!BLOCK_IS_BOMB(Block)) {
                    ExpandEmptyBlock(point);
                }
                // The rest case is flag
            }
        }

        if (lostGame) {
            FinishGame(FALSE);
        }
        else if (NumberOfEmptyBlocks == NumberOfRevealedBlocks) {
            FinishGame(TRUE);
        }
    }
    else {
        ReleaseBlocksClick();
        return;
    }
}

// For the mouse move with 3x3 click, redraw old blocks and draw new blocks
__inline void UpdateClickedBlocksState3x3(BoardPoint newClick, BoardPoint oldClick) {
    BOOL isNewLocationInBounds = IsInBoardRange(newClick);
    BOOL isOldLocationInBounds = IsInBoardRange(oldClick);

    // Get 3x3 bounds for the old and new clicks
    int oldTopRow = max(1, oldClick.Row - 1);
    int oldBottomRow = min(Height, oldClick.Row + 1);

    int topRow = max(1, newClick.Row - 1);
    int bottomRow = min(Height, newClick.Row + 1);

    int oldLeftColumn = max(1, oldClick.Column - 1);
    int oldRightColumn = min(Width, oldClick.Column + 1);

    int leftColumn = max(1, newClick.Column - 1);
    int rightColumn = min(Width, newClick.Column + 1);

    // Change old to unclicked
    if (isOldLocationInBounds) {
        for (int loopRow = oldTopRow; loopRow <= oldBottomRow; loopRow++) {
            for (int loopColumn = oldLeftColumn; loopColumn <= oldRightColumn; loopColumn++) {
                BoardPoint point = { loopRow, loopColumn };
                BYTE Block = ACCESS_BLOCK(point);
                if (!BLOCK_IS_REVEALED(Block) && BLOCK_STATE(Block) == BLOCK_STATE_READ_EMPTY) {
                    UpdateBlockStateToUnclicked(point);
                    DrawBlock(point);
                }
            }
        }
    }
    if (isNewLocationInBounds) {
        for (int loopRow = topRow; loopRow <= bottomRow; loopRow++) {
            for (int loopColumn = leftColumn; loopColumn <= rightColumn; loopColumn++) {
                BoardPoint point = { loopRow, loopColumn };
                BYTE Block = ACCESS_BLOCK(point);
                if (!BLOCK_IS_REVEALED(Block) && BLOCK_STATE(Block) == BLOCK_STATE_EMPTY_UNCLICKED) {
                    UpdateBlockStateToClicked(point);
                    DrawBlock(point);
                }
            }
        }
    }
}

// Set the block state to clicked
__inline void UpdateBlockStateToClicked(BoardPoint point) {
    ACCESS_BLOCK(point) = BLOCK_INFO(ACCESS_BLOCK(point)) | BLOCK_STATE_READ_EMPTY;
}

// Reset the block state to unclicked
__inline void UpdateBlockStateToUnclicked(BoardPoint point) {
    ACCESS_BLOCK(point) = BLOCK_INFO(ACCESS_BLOCK(point)) | BLOCK_STATE_EMPTY_UNCLICKED;
}

//-----------------------------------------------------------

// Check if the block is in the board range
__inline BOOL IsInBoardRange(BoardPoint point) {
    return point.Column > 0 && point.Row > 0 && point.Column <= Width && point.Row <= Height;
}

// Handler when the first click is bomb
__inline void ReplaceFirstNonBomb(BoardPoint point, PBYTE pFunctionBlock) {
    // The first block! Change a normal block to a bomb, 
    // Replace the current block into an empty block
    // Reveal the current block
    // WIERD: LOOP IS WITHOUT AN EQUAL SIGN
    for (int current_row = 1; current_row < Height; ++current_row) {
        for (int current_column = 1; current_column < Width; ++current_column) {
            PBYTE pLoopBlock = &StateArray[current_row][current_column];

            // Find the first non-bomb
            if (!BLOCK_IS_BOMB(*pLoopBlock)) {
                // Replace bomb place
                *pFunctionBlock = BLOCK_STATE_EMPTY_UNCLICKED;
                *pLoopBlock |= BOMB_FLAG;

                ExpandEmptyBlock(point);
                return;
            }
        }
    }
}

// Count the number of flags near the block
int GetFlagBlocksCount(BoardPoint point) {
    int flagsCount = 0;

    // Search in the sorrunding blocks
    for (int loopRow = (point.Row - 1); loopRow <= (point.Row + 1); ++loopRow) {
        for (int loopColumn = (point.Column - 1); loopColumn <= (point.Column + 1); ++loopColumn) {

            BYTE blockValue = StateArray[loopRow][loopColumn] & BLOCK_STATE_MASK;

            if (blockValue == BLOCK_STATE_1P_FLAG) {
                flagsCount++;
            }
        }

    }

    return flagsCount;
}

// Count the number of bombs near the block
int CountNearBombs(BoardPoint point) {
    int count = 0;

    for (int loopRow = (point.Row - 1); loopRow <= (point.Row + 1); loopRow++) {
        for (int loopColumn = (point.Column - 1); loopColumn <= (point.Column + 1); loopColumn++) {
            if (BLOCK_IS_BOMB(StateArray[loopRow][loopColumn])) {
                count++;
            }
        }
    }

    return count;
}

// BFS for empty blocks and show them
void ExpandEmptyBlock(BoardPoint point) {
    CurrentRowColumnListIndex = 1;
    ShowBlockValue(point);

    int i = 1;

    while (i != CurrentRowColumnListIndex) {
        int row = RowsList[i];
        int column = ColumnsList[i];
        BoardPoint point;

        for (int column_loop = column - 1; column_loop <= column + 1; column_loop++) {
            for (int row_loop = row - 1; row_loop <= row + 1; row_loop++) {
                if (row_loop == row && column_loop == column) {
                    continue;
                }
                point.Column = column_loop;
                point.Row = row_loop;
                ShowBlockValue(point);
            }
        }
        i++;

        if (i == 10000) {
            i = 0;
        }
    }
}

// Open the block and show it
void ShowBlockValue(BoardPoint point) {
    BYTE blockValue = ACCESS_BLOCK(point);

    if (BLOCK_IS_REVEALED(blockValue)) {
        return;
    }

    BYTE state = blockValue & BLOCK_STATE_MASK;

    if (state == BLOCK_STATE_BORDER_VALUE || state == BLOCK_STATE_1P_FLAG) {
        return;
    }

    NumberOfRevealedBlocks++;

    int nearBombsCount = CountNearBombs(point);
    ACCESS_BLOCK(point) = nearBombsCount | REVEALED_FLAG;
    DrawBlock(point);

    if (nearBombsCount == 0) {
        RowsList[CurrentRowColumnListIndex] = point.Row;
        ColumnsList[CurrentRowColumnListIndex] = point.Column;

        CurrentRowColumnListIndex++;

        if (CurrentRowColumnListIndex == 10000) {
            CurrentRowColumnListIndex = 0;
        }
    }
}

// Display all blocks when the game is over
void RevealAllBombs(BYTE revealedBombsState) {

    for (int loopRow = 1; loopRow <= Height; ++loopRow) {
        for (int loopColumn = 1; loopColumn <= Width; ++loopColumn) {
            PBYTE pBlock = &StateArray[loopRow][loopColumn];

            if (BLOCK_IS_REVEALED(*pBlock)) {
                continue;
            }

            BYTE blockState = *pBlock & BLOCK_STATE_MASK;

            if (BLOCK_IS_BOMB(*pBlock)) {
                if (blockState != BLOCK_STATE_1P_FLAG) {
                    *pBlock = (*pBlock & 0xe0) | revealedBombsState;
                }
            }
            else if (blockState == BLOCK_STATE_1P_FLAG) {
                // This is not a bomb, but flagged by the user
                *pBlock = (*pBlock & 0xeb) | BLOCK_STATE_1P_BOMB_WITH_X;
            }
        }
    }

    DisplayAllBlocks();
}

// Game finisher
void FinishGame(BOOL isWon) {
    IsTimerOnAndShowed = FALSE;
    GlobalSmileId = (isWon) ? SMILE_WINNER : SMILE_LOST;
    DisplaySmile(GlobalSmileId);

    // If the player wins, bombs are changed into borderFlags
    // If the player loses, bombs change into black bombs
    RevealAllBombs((isWon) ? BLOCK_STATE_1P_FLAG : BLOCK_STATE_BLACK_BOMB);

    if (isWon && LeftFlags != 0) {
        AddAndDisplayLeftFlags(-LeftFlags);
    }

    StateFlags = STATE_GAME_FINISHED;

    // Check if it is the best time
    if (isWon && GameConfig.Difficulty != DIFFICULTY_CUSTOM) {
        if (TimerSeconds < GameConfig.Times[GameConfig.Difficulty]) {
            GameConfig.Times[GameConfig.Difficulty] = TimerSeconds;
            //SaveWinnerNameDialogBox();
            //WinnersDialogBox();
        }
    }
}

//-----------------------------------------------------------

// Handle for smile button left click
BOOL HandleLeftClick(DWORD dwLocation) {
    MSG msg;
    RECT rect;

    // Copy point into struct
    msg.pt.y = HIWORD(dwLocation);
    msg.pt.x = LOWORD(dwLocation);

    // The rect represents the game board
    rect.left = (xRight - 24) / 2;
    rect.right = rect.left + 24;
    rect.top = 16;
    rect.bottom = 40;

    // Check if the click is in the range of the board
    if (!PtInRect(&rect, msg.pt)) {
        return FALSE;
    }
    
    GlobalSmileId = SMILE_NORMAL;
    DisplaySmile(SMILE_NORMAL);
    InitializeNewGame();
    return TRUE;
}

void HandleRightClick(BoardPoint point) {
    if (IsInBoardRange(point)) {
        BYTE block = ACCESS_BLOCK(point);

        if (!BLOCK_IS_REVEALED(block)) {
            BYTE blockState = block & BLOCK_STATE_MASK;

            switch (blockState) {
            case BLOCK_STATE_1P_FLAG:
                blockState = BLOCK_STATE_EMPTY_UNCLICKED;
                AddAndDisplayLeftFlags(1);
                break;
            default: // Assume BLOCK_STATE_EMPTY_UNCLICKED
                blockState = BLOCK_STATE_1P_FLAG;
                AddAndDisplayLeftFlags(-1);
            }

            ChangeBlockState(point, blockState);

            if (BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG) &&
                NumberOfRevealedBlocks == NumberOfEmptyBlocks) {
                FinishGame(TRUE);
            }
        }
    }
}

void TickSeconds() {
    if (IsTimerOnAndShowed && TimerSeconds < 999) {
        TimerSeconds++;
        DisplayTimerSeconds();
    }
}
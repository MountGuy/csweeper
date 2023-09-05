#include <stdio.h>
#include "solver.h"
#include "game.h"
#include "drawing.h"

BoardPoint openedStack[10000];
BoardPoint openedStackSpare[10000];
int openedStackIndex;

ConstraintPtr constPtrs[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH][100];
int constPtrNum[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];
int constPtrNumCopy[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];

void PrintConst(ConstraintPtr constPtr) {
    char str[1000];
    const char *typeString[5] = { "equal to\0", "greater than\0", "less than\0" };
    switch (constPtr->constType) {
    case EQ:
        sprintf_s(str, sizeof(str), "Constraint: equal to %d mines %d points: ", constPtr->mineNum, constPtr->pointNum);
        break;
    case GT:
        sprintf_s(str, sizeof(str), "Constraint: greater than %d mines %d points: ", constPtr->mineNum, constPtr->pointNum);
        break;
    case LT:
        sprintf_s(str, sizeof(str), "Constraint: less than %d mines %d points: ", constPtr->mineNum, constPtr->pointNum);
        break;
    }
    OutputDebugStringA(str);
    for (int i = 0; i < constPtr->pointNum; i++) {
        sprintf_s(str, sizeof(str), "(%d, %d)", constPtr->points[i].row, constPtr->points[i].column);
        OutputDebugStringA(str);
    }
    sprintf_s(str, sizeof(str), "\n");
    OutputDebugStringA(str);
}

// return 1, 0, -1 only
int CompPoint(BoardPoint point1, BoardPoint point2) {
    if (point1.row != point2.row) {
        return point1.row - point2.row > 0? 1 : -1;
    }
    else {
        if (point1.column != point2.column) {
            return point1.column - point2.column > 0? 1 : -1;
        }
        else {
            return 0;
        }
    }
}

// return 1 if equal
int EqualConst(ConstraintPtr constPtr1, ConstraintPtr constPtr2) {
    int result = 0;
    if (constPtr1->pointNum != constPtr2->pointNum ||
        constPtr1->mineNum != constPtr2->mineNum ||
        constPtr1->constType != constPtr2->constType) {
        return 0;
    }
    for (int i = 0; i < constPtr1->pointNum; i++) {
        result = CompPoint(constPtr1->points[i], constPtr2->points[i]);
        if (result != 0) return 0;
    }
    return 1;
}

void CopyConstPtrNum() {
    char str[10];
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            constPtrNumCopy[r][c] = constPtrNum[r][c];
        }
    }
}

int AddConstraint(ConstraintPtr constPtr) {
    BoardPoint point = constPtr->points[0];
    int ptrN = 0;
    for (; ptrN < constPtrNum[point.row][point.column]; ptrN++) {
        if (EqualConst(constPtr, constPtrs[point.row][point.column][ptrN])) {
            return 0;
        }
    }

    constPtrs[point.row][point.column][ptrN] = constPtr;
    constPtrNum[point.row][point.column]++;

    return 1;
}

ConstraintPtr GetEQConstraint(BoardPoint revPoint) {
    int newIdx = 0, ptrN = 0, mineNum = CountNearBombs(revPoint) - CountNearFlags(revPoint);

    if (mineNum == 0) {
        return NULL;
    }

    ConstraintPtr newConst = (ConstraintPtr) malloc(sizeof(Constraint));
    BoardPoint point;

    for (int r = (revPoint.row - 1); r <= (revPoint.row + 1); r++) {
        for (int c = (revPoint.column - 1); c <= (revPoint.column + 1); c++) {
            BoardPoint candPoint = { r, c };
            BYTE block = ACCESS_BLOCK(candPoint);            

            if (CompPoint(candPoint, revPoint) != 0 && IsInBoardRange(candPoint)) {
                if (BLOCK_IS_STATE(block, BLOCK_STATE_EMPTY_UNCLICKED) && solveState[r][c] != SOLVED) newConst->points[newIdx++] = candPoint;
            }
        }
    }

    newConst->constType = EQ;
    newConst->mineNum = mineNum;
    newConst->pointNum = newIdx;

    return newConst;
}

// Assume that GT - GT or LT - LT is not comming: filtered by the caller
int GetSubConstraint(ConstraintPtr outConst, ConstraintPtr inConst, ConstraintPtr newConst1, ConstraintPtr newConst2) {
    int outIdx = 0, inIdx = 0, newIdx = 0, commonNum = 0;
    int typeOut = outConst->constType, typeIn = inConst->constType;

    while (outIdx < outConst->pointNum && inIdx < inConst->pointNum) {
        BoardPoint pointOut = outConst->points[outIdx], pointIn = inConst->points[inIdx];

        switch (CompPoint(pointOut, pointIn)) {
        case -1:
            newConst1->points[newIdx] = pointOut;
            newConst2->points[newIdx] = pointOut;
            outIdx++;
            newIdx++;
            break;
        case 0:
            outIdx++;
            inIdx++;
            commonNum++;
            break;
        case 1:
            inIdx++;
            break;
        }
    }
    
    for (; outIdx < outConst->pointNum; outIdx++) {
        newConst1->points[newIdx] = outConst->points[outIdx];
        newConst2->points[newIdx] = outConst->points[outIdx];
        newIdx++;
    }

    if (commonNum == 0 || outIdx == commonNum) return 0;

    newConst1->pointNum = newIdx;
    newConst2->pointNum = newIdx;

    if (inConst->pointNum == commonNum) {
        if (typeOut == EQ) newConst1->constType = INV_EQ(typeIn);
        else newConst1->constType = typeOut;

        newConst1->mineNum = outConst->mineNum - inConst->mineNum;
        if ((newConst1->mineNum <= 0 || newConst1->mineNum >= newConst1->pointNum) && newConst1->constType != EQ) return 0;
        return 1;
    } else {
        int minMineCommon = 0, maxMineCommon = 0;
        switch (typeIn) {
        case GT:
            maxMineCommon = commonNum;
            minMineCommon = max(0, inConst->mineNum - (inConst->pointNum - commonNum));
            break;
        case EQ:
            maxMineCommon = min(inConst->mineNum, commonNum);
            minMineCommon = max(0, inConst->mineNum - (inConst->pointNum - commonNum));
            break;
        case LT:
            maxMineCommon = min(inConst->mineNum, commonNum);
            minMineCommon = 0;
            break;
        }

        int isValid1 = 0, isValid2 = 0;

        switch (typeOut) {
        case GT:
            newConst1->mineNum = max(0, outConst->mineNum - maxMineCommon);
            newConst1->constType = GT;
            isValid1 = (newConst1->mineNum > 0);
            return isValid1;
        case EQ:
            newConst1->mineNum = max(0, outConst->mineNum - maxMineCommon);
            newConst1->constType = GT;
            isValid1 = (newConst1->mineNum > 0);
            newConst2->mineNum = max(0, outConst->mineNum - minMineCommon);
            newConst2->constType = LT;
            isValid2 = (newConst2->mineNum < outConst->mineNum) && (newConst2->mineNum < newConst2->pointNum);

            if (!isValid1) *newConst1 = *newConst2;
            return isValid1 + isValid2;
        case LT:
            newConst1->mineNum = outConst->mineNum - minMineCommon;
            newConst1->constType = LT;
            isValid1 = (newConst1->mineNum < outConst->mineNum) && (newConst1->mineNum < newConst1->pointNum);
            return isValid1;
        }
    }
}

void ResetConsts() {
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            for (int n = 0; n < constPtrNum[r][c]; n++) {
                free(constPtrs[r][c][n]);
            }
            constPtrNum[r][c] = 0;
            solveState[r][c] = (solveState[r][c] == CONSTRAINED ? OPENED : solveState[r][c]);
        }
    }
}

int ComposeConst() {
    int result = 0;
    while (1) {
        int numValid = 0;
        CopyConstPtrNum();
        for (int r = 1; r <= height; r++) {
            for (int c = 1; c <= width; c++) {
                for (int n = 0; n < constPtrNumCopy[r][c]; n++) {
                    ConstraintPtr outConst = constPtrs[r][c][n];
                    BoardPoint pointOut = outConst->points[0];

                    for (int rr = pointOut.row - 2; rr <= pointOut.row + 2; rr++) {
                        for (int cc = pointOut.column - 2; cc <= pointOut.column + 2; cc++) {
                            BoardPoint pointIn = { rr, cc };
                            if (!IsInBoardRange(pointIn)) continue;
                            for (int nn = 0; nn < constPtrNumCopy[rr][cc]; nn++) {
                                ConstraintPtr inConst = constPtrs[rr][cc][nn];
                                Constraint newConst[2];
                                if (CompPoint(inConst->points[0], outConst->points[outConst->pointNum - 1]) > 0 ||
                                    CompPoint(outConst->points[0], inConst->points[inConst->pointNum - 1]) > 0)
                                    continue;
                                if (outConst->constType == inConst->constType && outConst->constType != EQ)
                                    continue;
                                int newConstNum = GetSubConstraint(outConst, inConst, newConst, newConst + 1);

                                for (int cn = 0; cn < newConstNum; cn++) {
                                    if (newConst[cn].mineNum < 0 || newConst[cn].mineNum == newConst[cn].pointNum && newConst[cn].constType == LT) {
                                        PrintConst(outConst);
                                        PrintConst(inConst);
                                        PrintConst(newConst + cn);
                                        OutputDebugStringA("==============\n");
                                    }
                                    int isTrivial = SolveTrivialConst(newConst + cn);

                                    if (isTrivial > 0) result += isTrivial;
                                    else {
                                        ConstraintPtr newConstPtr = (ConstraintPtr) malloc(sizeof(Constraint));
                                        *newConstPtr = newConst[cn];
                                        numValid += AddConstraint(newConstPtr);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (numValid == 0 || result > 0) return result;
    }
    return result;
}

int SolveTrivialConst(ConstraintPtr constPtr) {
    int newIdx = 0, ptrN = 0, result = 0;

    if (constPtr->mineNum == 0 && constPtr->constType != GT) {
        for (int idx = 0; idx < constPtr->pointNum; idx++) {
            BoardPoint point = constPtr->points[idx];
            if (solveState[point.row][point.column] != SOLVED) {
                SET_SOLVE_STATE(point, SOLVED);
                ExpandEmptyBlock(point);
                result++;
            }
        }
    }
    else if (constPtr->mineNum == constPtr->pointNum && constPtr->constType != LT) {
        for (int idx = 0; idx < constPtr->pointNum; idx++) {
            BoardPoint point = constPtr->points[idx];
            if (solveState[point.row][point.column] != SOLVED) {
                SET_SOLVE_STATE(point, SOLVED);
                ChangeBlockState(point, blockStateFlags[ID_1P]);
                AddAndDisplayLeftFlags(-1);
                result++;
            }
        }
    }
    return result;
}

int Heuristic1(BoardPoint point) {
    int result = 0;

    SET_SOLVE_STATE(point, SOLVED);
    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            BoardPoint tPoint = { r, c };
            BYTE block = ACCESS_BLOCK(tPoint);
            if (IsInBoardRange(tPoint) && solveState[r][c] != SOLVED && BLOCK_IS_STATE(block, BLOCK_STATE_EMPTY_UNCLICKED)) {
                if (!BLOCK_IS_BOMB(block)) {
                    ChangeBlockState(tPoint, BLOCK_STATE_1P_BOMB_RED_BACKGROUND);
                    DrawBlock(tPoint);
                    int a = 1;
                }
                ChangeBlockState(tPoint, blockStateFlags[ID_1P]);
                AddAndDisplayLeftFlags(-1);
                result++;

            }
        }
    }

    return result;
}

int Heuristic2(BoardPoint point) {
    int result = 0;

    SET_SOLVE_STATE(point, SOLVED);
    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            BoardPoint tPoint = { r, c };
            BYTE block = ACCESS_BLOCK(tPoint);
            if (IsInBoardRange(tPoint) && solveState[r][c] != SOLVED && BLOCK_IS_STATE(block, BLOCK_STATE_EMPTY_UNCLICKED)) {
                ExpandEmptyBlock(tPoint);
                result++;
            }
        }
    }

    return result;
}

int Heuristic3(BoardPoint point) {
    SET_SOLVE_STATE(point, CONSTRAINED);
    AddConstraint(GetEQConstraint(point));
    openedStack[openedStackIndex++] = point;

    return 0;
}

int Heuristic(BoardPoint point) {
    BYTE block = ACCESS_BLOCK(point);
    int numBombs = CountNearBombs(point);
    int numFlags = CountNearFlags(point);
    int numUnRevealed = CountNearUnRevealed(point);
    int result = 0;

    if (solveState[point.row][point.column] == SOLVED) {
        return 0;
    }
    if (solveState[point.row][point.column] == CLOSED) {
        openedStackSpare[openedStackIndex++] = point;
        return 0;
    }
    if (numUnRevealed == numBombs) result += Heuristic1(point);
    else if (numBombs == numFlags) result += Heuristic2(point);
    else if (solveState[point.row][point.column] != CONSTRAINED) result += Heuristic3(point);

    return result;
}

// Solve the board
BOOL Solve(BoardPoint entryPoint) {
    char str[100];
    int stackSizeCopy = 0, newOpened;

    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            constPtrNum[r][c] = 0;
        }
    }

    openedStackIndex = 0;
    ExpandEmptyBlock(entryPoint);

    while (1) {
        newOpened = 0;
        ResetConsts();

        for (int idx = 0; idx < openedStackIndex; idx++) {
            openedStackSpare[idx] = openedStack[idx];
        }
        stackSizeCopy = openedStackIndex;
        openedStackIndex = 0;

        for (int idx = 0; idx < stackSizeCopy; idx++) {
            newOpened += Heuristic(openedStackSpare[idx]);
        }

        if (newOpened == 0) {
            newOpened += ComposeConst();
        }
        if (newOpened == 0) {
            break;
        }
    }

    return FALSE;
}
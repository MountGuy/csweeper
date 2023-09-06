#include <stdio.h>
#include "solver.h"
#include "game.h"
#include "drawing.h"

BYTE solverState[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];

BoardPoint openedList[10000];
BoardPoint openedListCopy[10000];
int openedListIndex;

ConstraintPtr consts[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH][100];
int constNum[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];
int constNumCopy[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];

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
int EqualConst(ConstraintPtr pConst1, ConstraintPtr pConst2) {
    int result = 0;
    if (pConst1->pointNum != pConst2->pointNum ||
        pConst1->mineNum != pConst2->mineNum ||
        pConst1->constType != pConst2->constType) {
        return 0;
    }
    for (int i = 0; i < pConst1->pointNum; i++) {
        result = CompPoint(pConst1->points[i], pConst2->points[i]);
        if (result != 0) return 0;
    }
    return 1;
}

void CopyConstNum() {
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            constNumCopy[r][c] = constNum[r][c];
        }
    }
}

void ResetConsts() {
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            for (int n = 0; n < constNum[r][c]; n++) {
                free(consts[r][c][n]);
            }
            constNum[r][c] = 0;
        }
    }
}

// return true if the newConst is not a duplicate
BOOL AddConstraint(ConstraintPtr newConst) {
    BoardPoint point = newConst->points[0];
    int constN = 0;
    for (; constN < constNum[point.row][point.column]; constN++) {
        if (EqualConst(newConst, consts[point.row][point.column][constN])) {
            return FALSE;
        }
    }

    ConstraintPtr newConstCopy = (ConstraintPtr) malloc(sizeof(Constraint));
    *newConstCopy = *newConst;
    
    consts[point.row][point.column][constN] = newConstCopy;
    constNum[point.row][point.column]++;

    return TRUE;
}

void GetEQConstraint(BoardPoint point, ConstraintPtr newConst) {
    int newPointNum = 0, mineNum = CountNearBombs(point), flagNum = 0;

    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            BoardPoint constPoint = { r, c };
            if (CompPoint(constPoint, point) != 0 && IsInBoardRange(constPoint)) {
                if (solverState[r][c] == CLOSED) newConst->points[newPointNum++] = constPoint;
                else if (solverState[r][c] == FLAGED) flagNum++;
            }
        }
    }

    newConst->constType = EQ;
    newConst->mineNum = mineNum - flagNum;
    newConst->pointNum = newPointNum;
}

// Assume that GT - GT or LT - LT is not coming: filtered by the caller
// Return the number of valid constraints
int GetSubConstraint(ConstraintPtr outConst, ConstraintPtr inConst, ConstraintPtr newConst1, ConstraintPtr newConst2) {
    int outIdx = 0, inIdx = 0, newPointNum = 0, commonNum = 0;
    int typeOut = outConst->constType, typeIn = inConst->constType;
    int outMineNum = outConst->mineNum, inMineNum = inConst->mineNum;
    int outPointNum = outConst->pointNum, inPointNum = inConst->pointNum;

    while (outIdx < outPointNum && inIdx < inPointNum) {
        BoardPoint pointOut = outConst->points[outIdx], pointIn = inConst->points[inIdx];

        switch (CompPoint(pointOut, pointIn)) {
        case -1:
            newConst1->points[newPointNum] = pointOut;
            newConst2->points[newPointNum] = pointOut;
            outIdx++;
            newPointNum++;
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
    
    for (; outIdx < outPointNum; outIdx++) {
        newConst1->points[newPointNum] = outConst->points[outIdx];
        newConst2->points[newPointNum] = outConst->points[outIdx];
        newPointNum++;
    }

    if (commonNum == 0 || outPointNum == commonNum) return 0;

    newConst1->pointNum = newPointNum;
    newConst2->pointNum = newPointNum;

    if (inPointNum == commonNum) {
        if (typeOut == EQ) newConst1->constType = INV_EQ(typeIn);
        else newConst1->constType = typeOut;

        newConst1->mineNum = outMineNum - inMineNum;
        if ((newConst1->mineNum <= 0 || newConst1->mineNum >= newPointNum) && newConst1->constType != EQ) return 0;
        return 1;
    }
    else {
        int minMineCommon = 0, maxMineCommon = 0;
        switch (typeIn) {
        case GT:
            maxMineCommon = commonNum;
            minMineCommon = max(0, inMineNum - (inPointNum - commonNum));
            break;
        case EQ:
            maxMineCommon = min(inMineNum, commonNum);
            minMineCommon = max(0, inMineNum - (inPointNum - commonNum));
            break;
        case LT:
            maxMineCommon = min(inMineNum, commonNum);
            minMineCommon = 0;
            break;
        }

        int isValid1 = 0, isValid2 = 0;

        switch (typeOut) {
        case GT:
            newConst1->mineNum = max(0, outMineNum - maxMineCommon);
            newConst1->constType = GT;
            isValid1 = (newConst1->mineNum > 0);
            return isValid1;
        case EQ:
            newConst1->mineNum = max(0, outMineNum - maxMineCommon);
            newConst1->constType = GT;
            isValid1 = (newConst1->mineNum > 0);
            newConst2->mineNum = max(0, outMineNum - minMineCommon);
            newConst2->constType = LT;
            isValid2 = (newConst2->mineNum < outMineNum || newPointNum < outPointNum) && (newConst2->mineNum < newPointNum);

            if (!isValid1) *newConst1 = *newConst2;
            return isValid1 + isValid2;
        case LT:
            newConst1->mineNum = outMineNum - minMineCommon;
            newConst1->constType = LT;
            isValid1 = (newConst1->mineNum < outMineNum || newPointNum < outPointNum) && (newConst1->mineNum < newPointNum);
            return isValid1;
        }
    }
}

int SolveTrivialConst(ConstraintPtr constPtr) {
    int newIdx = 0, ptrN = 0, result = 0;

    if (constPtr->mineNum == 0 && constPtr->constType != GT) {
        for (int idx = 0; idx < constPtr->pointNum; idx++) {
            BoardPoint point = constPtr->points[idx];
            if (SOLVER_STATE(point) == CLOSED) {
                openedList[openedListIndex++] = point;
                SOLVER_STATE(point) = OPENED;
                ChangeBlockState(point, CountNearBombs(point) | REVEALED_FLAG);
                result++;
            }
        }
    }
    else if (constPtr->mineNum == constPtr->pointNum && constPtr->constType != LT) {
        for (int idx = 0; idx < constPtr->pointNum; idx++) {
            BoardPoint point = constPtr->points[idx];
            if (SOLVER_STATE(point) != FLAGED) {
                SOLVER_STATE(point) = FLAGED;
                ChangeBlockState(point, blockStateFlags[ID_1P]);
                AddAndDisplayLeftFlags(-1);
                result++;
            }
        }
    }
    return result;
}

int SynthConst() {
    int newOpenNum = 0;
    while (1) {
        int newConstNum = 0;
        CopyConstNum();
        for (int r = 1; r <= height; r++) {
            for (int c = 1; c <= width; c++) {
                for (int n = 0; n < constNumCopy[r][c]; n++) {
                    ConstraintPtr outConst = consts[r][c][n];
                    BoardPoint pointOut = outConst->points[0];

                    for (int rr = pointOut.row - 2; rr <= pointOut.row + 2; rr++) {
                        for (int cc = pointOut.column - 2; cc <= pointOut.column + 2; cc++) {
                            BoardPoint pointIn = { rr, cc };
                            if (!IsInBoardRange(pointIn)) continue;
                            for (int nn = 0; nn < constNumCopy[rr][cc]; nn++) {
                                ConstraintPtr inConst = consts[rr][cc][nn];
                                Constraint newConst[2];
                                if (CompPoint(inConst->points[0], outConst->points[outConst->pointNum - 1]) > 0 ||
                                    CompPoint(outConst->points[0], inConst->points[inConst->pointNum - 1]) > 0)
                                    continue;
                                if (outConst->constType == inConst->constType && outConst->constType != EQ)
                                    continue;
                                int valConstNum = GetSubConstraint(outConst, inConst, newConst, newConst + 1);

                                for (int cn = 0; cn < valConstNum; cn++) {
                                    int openFromConst = SolveTrivialConst(newConst + cn);
                                    if (openFromConst > 0) newOpenNum += openFromConst;
                                    else newConstNum += AddConstraint(newConst + cn);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (newConstNum == 0 || newOpenNum > 0) return newOpenNum;
    }
}

int ApplyTactic(BoardPoint point) {
    Constraint newConst;

    GetEQConstraint(point, &newConst);
    int openFromConst = SolveTrivialConst(&newConst);
    if (openFromConst > 0 || newConst.pointNum == 0) {
        return openFromConst;
    }
    else {
        openedList[openedListIndex++] = point;
        AddConstraint(&newConst);
        return 0;
    }
}

void InitSolver(BoardPoint entryPoint) {
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            solverState[r][c] = CLOSED;
        }
    }

    openedListIndex = 0;
    openedList[openedListIndex++] = entryPoint;
    SOLVER_STATE(entryPoint) = OPENED;
    ACCESS_BLOCK(entryPoint) = CountNearBombs(entryPoint) | REVEALED_FLAG;
    DrawBlock(entryPoint);
}

// Solve the board
BOOL Solve(BoardPoint entryPoint) {
    int listIndexCopy = 0, newOpenNum = 0, totalOpenNum = 1;
    char str[100];

    InitSolver(entryPoint);

    while (1) {
        newOpenNum = 0;
        ResetConsts();

        for (int idx = 0; idx < openedListIndex; idx++) {
            openedListCopy[idx] = openedList[idx];
        }
        listIndexCopy = openedListIndex;
        openedListIndex = 0;

        for (int idx = 0; idx < listIndexCopy; idx++) {
            newOpenNum += ApplyTactic(openedListCopy[idx]);
        }

        if (newOpenNum == 0) {
            newOpenNum = SynthConst();
            if (newOpenNum == 0) break;
        }
        totalOpenNum += newOpenNum;
    }
    ResetConsts();

    return FALSE;
}
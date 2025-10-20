#ifndef BOARD_DATA_H
#define BOARD_DATA_H

#include "const.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This section is dedicated to sorting large lists of evaluated boards and
// scores.  It represents an adaptation of the GLIBC qsort code and an explicit
// stack documented here http://www.corpit.ru/mjt/qsort.html I will thank
// Michael Tokarev for an excellent web page, and a very useful recursion
// elimination technique.

// The BoardData pseudo-class will use macros for its associated functionality.
struct boarddata {
  char TheBoardString[SQUARE_COUNT + 2 + 1];
  unsigned int TheBoardScore;
};

typedef struct boarddata BoardData;
typedef BoardData *BoardDataPtr;

#define BOARD_DATA_SET_THE_BOARD_STRING(thisboarddata, newstring)              \
  (strcpy(thisboarddata->TheBoardString, newstring))

#define BOARD_DATA_SET_THE_BOARD_SCORE(thisboarddata, newscore)                \
  (thisboarddata->TheBoardScore = (newscore))

// This statement evaluates to TRUE when the board at "First" has a higher score
// than the board at "Second".
#define COMPARE_BOARD_DATA(First, Second)                                      \
  ((*First)->TheBoardScore > (*Second)->TheBoardScore) ? TRUE : FALSE

// Swap two items pointed to by "a" and "b" using temporary buffer "T".
#define BOARD_DATA_SWAP(a, b, T) ((void)((T = *a), (*a = *b), (*b = T)))

// When a list gets this small, standard insertion sort is a faster method.
#define SORT_TRANSITION_SIZE 4

// define the size of the list being sorted.  This number represents the total
// number of boards analyzed per thread, per round.
#define LIST_SIZE (BOARDS_PER_THREAD * SINGLE_DEVIATIONS)

// A qsort stack only needs to save two values.
struct qsortstacknode {
  BoardDataPtr *Lo;
  BoardDataPtr *Hi;
};

typedef struct qsortstacknode QsortStackNode;
typedef QsortStackNode *QsortStackNodePtr;

// This is where the recursion-replacement stack is implemented.
// Using macros allows the programmer to change the value of an argument
// directly.
#define QSORT_STACK_SIZE (8 * sizeof(unsigned int))
#define QSORT_STACK_PUSH(top, low, high)                                       \
  (((top->Lo = (low)), (top->Hi = (high)), ++top))
#define QSORT_STACK_POP(low, high, top)                                        \
  ((--top, (low = top->Lo), (high = top->Hi)))
#define QSORT_STACK_NOT_EMPTY(identity, top) ((TheQsortStacks[identity]) < top)

// Each worker thread will require its own qsort() stack.
QsortStackNode *TheQsortStacks[NUMBER_OF_WORKER_THREADS];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The Global array of pointers to arrays of "BoardDataPtr"s.  All of the
// associated "BoardData" with this array will thus never move around. The Main
// thread will allocate the space required to store the actual "BoardData". The
// thread identities and attributes are also defined here.
BoardDataPtr *WorkingBoardScoreTallies[NUMBER_OF_WORKER_THREADS];
char ThreadBoardStringsToAnalyze[NUMBER_OF_WORKER_THREADS]
                                [BOARDS_PER_ROUND / NUMBER_OF_WORKER_THREADS]
                                [BOARD_STRING_SIZE];

void BoardDataExplicitStackQuickSort(BoardDataPtr *Base, unsigned int Size,
                                     unsigned int CallingThread);

#ifdef __cplusplus
}
#endif

#endif // BOARD_DATA_H

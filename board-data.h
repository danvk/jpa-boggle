#ifndef BOARD_DATA_H
#define BOARD_DATA_H

#include "const.h"

#ifdef __cplusplus
extern "C" {
#endif

// The BoardData pseudo-class will use macros for its associated functionality.
struct boarddata {
  char board[SQUARE_COUNT + 2 + 1];
  unsigned int score;
};

typedef struct boarddata BoardData;
typedef BoardData *BoardDataPtr;

// define the size of the list being sorted.  This number represents the total
// number of boards analyzed per round.
#define LIST_SIZE (BOARDS_PER_THREAD * SINGLE_DEVIATIONS)

// The global array of "BoardDataPtr"s.  All of the associated "BoardData"
// will thus never move around. The main function will allocate the space
// required to store the actual "BoardData".
BoardDataPtr *WorkingBoardScoreTally;

#ifdef __cplusplus
}
#endif

#endif // BOARD_DATA_H

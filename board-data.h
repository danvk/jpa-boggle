#ifndef BOARD_DATA_H
#define BOARD_DATA_H

#include "const.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// The BoardData pseudo-class will use macros for its associated functionality.
	struct boarddata
	{
		char board[SQUARE_COUNT + 2 + 1];
		unsigned int score;
	};

	typedef struct boarddata BoardData;
	typedef BoardData *BoardDataPtr;

// define the size of the list being sorted.  This number represents the total
// number of boards analyzed per thread, per round.
#define LIST_SIZE (BOARDS_PER_THREAD * SINGLE_DEVIATIONS)

	// The Global array of pointers to arrays of "BoardDataPtr"s.  All of the
	// associated "BoardData" with this array will thus never move around. The Main
	// thread will allocate the space required to store the actual "BoardData". The
	// thread identities and attributes are also defined here.
	BoardDataPtr *WorkingBoardScoreTallies[NUMBER_OF_WORKER_THREADS];

#ifdef __cplusplus
}
#endif

#endif // BOARD_DATA_H

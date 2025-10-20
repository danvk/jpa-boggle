#ifndef BOARD_DATA_H
#define BOARD_DATA_H

#include "const.h"

#ifdef __cplusplus
extern "C"
{
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This section is dedicated to sorting large lists of evaluated boards and
	// scores.  It represents an adaptation of the GLIBC qsort code and an explicit
	// stack documented here http://www.corpit.ru/mjt/qsort.html I will thank
	// Michael Tokarev for an excellent web page, and a very useful recursion
	// elimination technique.

	// The BoardData pseudo-class will use macros for its associated functionality.
	struct boarddata
	{
		char board[SQUARE_COUNT + 2 + 1];
		unsigned int score;
	};

	typedef struct boarddata BoardData;
	typedef BoardData *BoardDataPtr;

#define BOARD_DATA_SET_THE_BOARD_STRING(thisboarddata, newstring) \
	(strcpy(thisboarddata->board, newstring))

#define BOARD_DATA_SET_THE_BOARD_SCORE(thisboarddata, newscore) \
	(thisboarddata->score = (newscore))

// define the size of the list being sorted.  This number represents the total
// number of boards analyzed per thread, per round.
#define LIST_SIZE (BOARDS_PER_THREAD * SINGLE_DEVIATIONS)

#ifdef __cplusplus
}
#endif

#endif // BOARD_DATA_H

#ifndef INSERT_H
#define INSERT_H

#include "const.h"

// Scoreboard list constants for streamlined binary insertion sort implementation.  Note that "BOARDS_PER_ROUND" needs to be a multiple of "NUMBER_OF_WORKER_THREADS", and should be the closest multiple less than "EVALUATE_LIST_SIZE".
#define MAX_LOOP_SEARCH_DEPTH_MASTER 9
#define EVALUATE_LIST_SIZE 66
#define MAX_LOOP_SEARCH_DEPTH_EVALUATE 5
#define MASTER_LIST_SIZE 1026

Bool InsertBoardStringIntoMasterList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore);
Bool InsertBoardStringIntoEvaluateList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore);

#endif // INSERT_H

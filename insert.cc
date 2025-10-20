#include <string.h>

#include "insert.h"
#include "const.h"

Bool InsertIntoList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore, unsigned int list_size, unsigned int max_loop_depth) {
	unsigned int X;
	unsigned int Left = 0;
	unsigned int Right = list_size - 1;
	unsigned int NextElement;
	char *TempBoardStringHolder;

	// "ThisScore" does not make the cut; it is too small.
	if ( ThisScore <= TheNumbers[Right] ) return FALSE;

	// "ThisScore" belongs at the end of the list.
	Right -= 1;
	if ( ThisScore <= TheNumbers[Right] ) {
		strcpy(TheList[list_size - 1], ThisBoardString);
		TheNumbers[list_size - 1] = ThisScore;
		return TRUE;
	}

	// "ThisScore" belongs at the first position in the list.
	if ( ThisScore >=  TheNumbers[Left] ) {
		TempBoardStringHolder = TheList[(list_size - 1)];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + 1, TheList, sizeof(char*)*(list_size - 1));
		TheList[Left] = TempBoardStringHolder;
		memmove(TheNumbers + 1, TheNumbers, sizeof(unsigned int)*(list_size - 1));
		TheNumbers[Left] = ThisScore;
		return TRUE;
	}

	// Set the initial midpoint.
	NextElement = Left + ((Right - Left)>>1);

	// This loop will be unwound by compiler optimization.
	for ( X = 0; X < max_loop_depth; X++ ) {
		// "NextElement" is the new "Left".
		if ( TheNumbers[NextElement] >  ThisScore ) {
			Left = NextElement;
		}
		// "NextElement" will become the new "Right".
		else if ( TheNumbers[NextElement] <  ThisScore ) {
			Right = NextElement;
		}
		// "NextElement" holds a value equal to "ThisScore", and is the insertion point.
		else {
			// memmove() is going to employ pointer arithmatic for pointers to internal array members.
			TempBoardStringHolder = TheList[(list_size - 1)];
			strcpy(TempBoardStringHolder, ThisBoardString);
			memmove(TheList + NextElement + 1, TheList + NextElement, sizeof(char*)*(list_size - 1 - NextElement));
			TheList[NextElement] = TempBoardStringHolder;
			memmove(TheNumbers + NextElement + 1, TheNumbers + NextElement, sizeof(unsigned int)*(list_size - 1 - NextElement));
			TheNumbers[NextElement] = ThisScore;
			return TRUE;
		}
		// Advance the "NextElement";
		NextElement = Left + ((Right - Left)>>1);
	}

	// "NextElement" is now flanked by "Left" and "Right", and this is known with absolute certainty.
	// Since two cases will result in the insertion position being equal to "Right", we only need to make one comparison on the final iteration.
	if ( TheNumbers[NextElement] <  ThisScore ) {
		TempBoardStringHolder = TheList[(list_size - 1)];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + NextElement + 1, TheList + NextElement, sizeof(char*)*(list_size - 1 -NextElement));
		TheList[NextElement] = TempBoardStringHolder;
		memmove(TheNumbers + NextElement + 1, TheNumbers + NextElement, sizeof(unsigned int)*(list_size - 1 - NextElement));
		TheNumbers[NextElement] = ThisScore;
		return TRUE;
	}
	// "ThisScore" is smaller or equal to "TheNumbers[NextElement]", so the insertion position will be "Right".
	else {
		TempBoardStringHolder = TheList[(list_size - 1)];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + Right + 1, TheList + Right, sizeof(char*)*(list_size - 1 - Right));
		TheList[Right] = TempBoardStringHolder;
		memmove(TheNumbers + Right + 1, TheNumbers + Right, sizeof(unsigned int)*(list_size - 1 - Right));
		TheNumbers[Right] = ThisScore;
		return TRUE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This section covers the streamlined Binary Insertion Sort coding.

// This function inserts "ThisBoardString" into "TheList" which must have "MASTER_LIST_SIZE" elements. "TheList" will already be sorted, and a Binary Insertion Sort will be used.
// The return value is "TRUE" or "FALSE" depending on if "ThisScore" was high enough to make the cut.
Bool InsertBoardStringIntoMasterList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore){
	return InsertIntoList(TheList, TheNumbers, ThisBoardString, ThisScore, MASTER_LIST_SIZE, MAX_LOOP_SEARCH_DEPTH_MASTER);
}


// This function inserts "ThisBoardString" into "TheList" which must have "EVALUATE_LIST_SIZE" elements. "TheList" will already be sorted, and a Binary Insertion Sort will be used.
// The return value is "TRUE" or "FALSE" depending on if "ThisScore" was high enough to make the cut.
Bool InsertBoardStringIntoEvaluateList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore){
	return InsertIntoList(TheList, TheNumbers, ThisBoardString, ThisScore, EVALUATE_LIST_SIZE, MAX_LOOP_SEARCH_DEPTH_EVALUATE);
}

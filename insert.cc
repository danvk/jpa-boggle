#include <string.h>
#include <vector>
#include <algorithm>

#include "insert.h"
#include "const.h"
#include <cstdio>

Bool InsertIntoList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore, unsigned int list_size) {
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
	while (Right > Left && Right - Left > 2) {
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
	return InsertIntoList(TheList, TheNumbers, ThisBoardString, ThisScore, MASTER_LIST_SIZE);
}


// This function inserts "ThisBoardString" into "TheList" which must have "EVALUATE_LIST_SIZE" elements. "TheList" will already be sorted, and a Binary Insertion Sort will be used.
// The return value is "TRUE" or "FALSE" depending on if "ThisScore" was high enough to make the cut.
Bool InsertBoardStringIntoEvaluateList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore){
	return InsertIntoList(TheList, TheNumbers, ThisBoardString, ThisScore, EVALUATE_LIST_SIZE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector-based implementations

static void InsertIntoSortedVector(std::vector<BoardScore>& list, unsigned int max_size, unsigned int score, const char* board) {
	// If list is not full yet, always insert
	if (list.size() < max_size) {
		// Find insertion point using binary search (list is sorted in descending order)
		auto it = std::lower_bound(list.begin(), list.end(), score,
			[](const BoardScore& bs, unsigned int s) { return bs.score > s; });
		list.insert(it, BoardScore(score, board));
		return;
	}

	// List is full - check if score is high enough
	if (score <= list.back().score) {
		return; // Score too low
	}

	// Remove the last (lowest) element
	list.pop_back();

	// Find insertion point and insert
	auto it = std::lower_bound(list.begin(), list.end(), score,
		[](const BoardScore& bs, unsigned int s) { return bs.score > s; });
	list.insert(it, BoardScore(score, board));
}

void InsertBoardScoreIntoMasterList(std::vector<BoardScore>& list, unsigned int score, const char* board) {
	InsertIntoSortedVector(list, MASTER_LIST_SIZE, score, board);
}

void InsertBoardScoreIntoEvaluateList(std::vector<BoardScore>& list, unsigned int score, const char* board) {
	InsertIntoSortedVector(list, EVALUATE_LIST_SIZE, score, board);
}

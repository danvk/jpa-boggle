#include <string.h>
#include <vector>
#include <algorithm>

#include "insert.h"
#include "const.h"
#include <cstdio>

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

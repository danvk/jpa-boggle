#include "insert.h"

#include <string.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "const.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector-based implementations

static bool InsertIntoSortedVector(
    std::vector<BoardScore> &list,
    unsigned int max_size,
    unsigned int score,
    const std::string &board
) {
  // If list is not full yet, always insert
  if (list.size() < max_size) {
    // Find insertion point using binary search (list is sorted in descending
    // order)
    auto it = std::lower_bound(
        list.begin(),
        list.end(),
        score,
        [](const BoardScore &bs, unsigned int s) { return bs.score > s; }
    );
    list.insert(it, BoardScore(score, board));
    return true;
  }

  // List is full - check if score is high enough
  if (score <= list.back().score) {
    return false;  // Score too low
  }

  // Remove the last (lowest) element
  list.pop_back();

  // Find insertion point and insert
  auto it = std::lower_bound(
      list.begin(),
      list.end(),
      score,
      [](const BoardScore &bs, unsigned int s) { return bs.score > s; }
  );
  list.insert(it, BoardScore(score, board));
  return true;
}

bool InsertIntoMasterList(
    std::vector<BoardScore> &list, unsigned int score, const std::string &board
) {
  return InsertIntoSortedVector(list, MASTER_LIST_SIZE, score, board);
}

bool InsertIntoEvaluateList(
    std::vector<BoardScore> &list, unsigned int score, const std::string &board
) {
  return InsertIntoSortedVector(list, EVALUATE_LIST_SIZE, score, board);
}

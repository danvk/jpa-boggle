#include "insert.h"

#include <string.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "const.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector-based implementations

bool ContainsElement(std::vector<BoardScore> &list, const BoardScore &board_score) {
  auto score = board_score.score;
  const auto &board = board_score.board.board;

  // TODO: binary search
  for (const auto &el : list) {
    if (el.score < score) {
      return false;
    }
    if (el.score == score && el.board.board == board) {
      return true;
    }
  }
  return false;
}

bool InsertIntoSortedVector(
    std::vector<BoardScore> &list, unsigned int max_size, const BoardScore &board_score
) {
  auto score = board_score.score;
  // If list is not full yet, always insert
  if (list.size() < max_size) {
    if (ContainsElement(list, board_score)) {
      return false;
    }
    // Find insertion point using binary search (list is sorted in descending
    // order)
    auto it = std::lower_bound(
        list.begin(),
        list.end(),
        score,
        [](const BoardScore &bs, unsigned int s) { return bs.score > s; }
    );
    list.insert(it, board_score);
    return true;
  }

  // List is full - check if score is high enough
  if (score <= list.back().score) {
    return false;  // Score too low
  }

  if (ContainsElement(list, board_score)) {
    return false;
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
  list.insert(it, board_score);
  return true;
}

bool InsertIntoMasterList(std::vector<BoardScore> &list, const BoardScore &board) {
  return InsertIntoSortedVector(list, MASTER_LIST_SIZE, board);
}

bool InsertIntoEvaluateList(std::vector<BoardScore> &list, const BoardScore &board) {
  return InsertIntoSortedVector(list, EVALUATE_LIST_SIZE, board);
}

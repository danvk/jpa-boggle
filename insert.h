#ifndef INSERT_H
#define INSERT_H

#include <string>
#include <vector>

struct BoardWithCell {
  std::string board;
  int off_limit_cell;

  BoardWithCell(std::string bd, int cell) : board(bd), off_limit_cell(cell) {}
};

struct BoardScore {
  unsigned int score;
  BoardWithCell board;

  BoardScore(unsigned int s, const BoardWithCell &b) : score(s), board(b) {}
};

// Scoreboard list constants for streamlined binary insertion sort
// implementation.  Note that "BOARDS_PER_ROUND" needs to be a multiple of
// "NUMBER_OF_WORKER_THREADS", and should be the closest multiple less than
// "EVALUATE_LIST_SIZE".
#define EVALUATE_LIST_SIZE 66
#define MASTER_LIST_SIZE 1026

// TODO: change these to TopN classes
// Returns whether the item was inserted into the list.
bool InsertIntoMasterList(std::vector<BoardScore> &list, const BoardScore &board);
bool InsertIntoEvaluateList(std::vector<BoardScore> &list, const BoardScore &board);

#endif  // INSERT_H

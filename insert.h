#ifndef INSERT_H
#define INSERT_H

#include <string>
#include <vector>

#include "const.h"

struct BoardScore {
  unsigned int score;
  std::string board;

  BoardScore() : score(0), board() {}
  BoardScore(unsigned int s, const std::string &b) : score(s), board(b) {}
  BoardScore(unsigned int s, const char *b) : score(s), board(b) {}
};

// Scoreboard list constants for streamlined binary insertion sort
// implementation.  Note that "BOARDS_PER_ROUND" needs to be a multiple of
// "NUMBER_OF_WORKER_THREADS", and should be the closest multiple less than
// "EVALUATE_LIST_SIZE".
#define EVALUATE_LIST_SIZE 66
#define MASTER_LIST_SIZE 1026

void InsertIntoMasterList(
    std::vector<BoardScore> &list, unsigned int score, const char *board
);
void InsertIntoEvaluateList(
    std::vector<BoardScore> &list, unsigned int score, const char *board
);

#endif  // INSERT_H

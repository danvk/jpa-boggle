// The DeepSearch.c algorithm uses an in-depth investigation of neighbouring
// boards starting at a predefined "MASTER_SEED_BOARD". The constants that
// define the depth of the search...  NUMBER_OF_SEEDS_TO_RUN, ROUNDS,
// BOARDS_PER_ROUND

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <format>
#include <set>
#include <string>
#include <vector>

#include "boggler.h"
#include "const.h"
#include "insert.h"
#include "trie.h"

// Constants that define the high level "DeepSearch.c" algorithm.
#define MASTER_SEED_BOARD "AGRIMODAOLSTECETISMNGPART"
#define SINGLE_DEVIATIONS 312
#define NUMBER_OF_SEEDS_TO_RUN 2
#define ROUNDS 25
#define BOARDS_PER_THREAD BOARDS_PER_ROUND

// define the size of the list being sorted.  This number represents the total
// number of boards analyzed per round.
#define LIST_SIZE (BOARDS_PER_THREAD * SINGLE_DEVIATIONS)

using namespace std;

struct BoardComparator {
  bool operator()(const BoardWithCell &a, const BoardWithCell &b) const {
    return a.board < b.board;
  }
};

// Returns 1 if this adds board to the container
int AddBoard(
    set<BoardWithCell, BoardComparator> &container, const BoardWithCell &board
) {
  auto result = container.insert(board);
  return result.second ? 1 : 0;
}

void PrintBoardList(const vector<BoardScore> &list, int num_to_print = 10'000) {
  for (int i = 0; i < num_to_print && i < list.size(); i++) {
    const auto &b = list[i];
    printf(
        "#%4d -|%5d|-|%s%02d|\n",
        i + 1,
        b.score,
        b.board.board.c_str(),
        b.board.off_limit_cell
    );
  }
}

void PrintBestBoard(int round, const vector<BoardScore> &list) {
  if (!list.empty()) {
    const auto &b = list[0];
    printf(
        "\nRound|%d|, Best Board|%s%02d|, Best Score|%d|\n",
        round,
        b.board.board.c_str(),
        b.board.off_limit_cell,
        b.score
    );
  }
}

void PrintBoard(const string &board) {
  printf("-----------\n");
  printf("%s\n", board.c_str());
  printf("-----------\n");
}

void AddBoardsToMasterList(
    vector<BoardScore> &MasterResults, const vector<BoardScore> &boards
) {
  unsigned int min_score =
      MasterResults.size() == MASTER_LIST_SIZE ? MasterResults.back().score : 0;
  for (const auto &result : boards) {
    if (result.score <= min_score) {
      break;
    }
    InsertIntoMasterList(MasterResults, result);
  }
}

vector<BoardScore> RunOneSeed(
    const BoardWithCell &SeedBoard,
    Boggler<5, 5> *boggler,
    vector<BoardScore> &MasterResults,
    set<BoardWithCell, BoardComparator> &AllEvaluatedBoards
) {
  // Before checking the "AllEvaluatedBoards" Trie, test if the score is high
  // enough to make the list. The scores attached to this list needs to be
  // reset every time that we start a new seed, the important remaining list
  // is the master list.
  std::vector<BoardScore> TopEvaluationBoardList;
  TopEvaluationBoardList.reserve(EVALUATE_LIST_SIZE);

  // Vector of BoardScore objects for working board tallies
  std::vector<BoardScore> WorkingBoardScoreTally(LIST_SIZE);

  // Populate the evaluate list for the first round of boards based on the
  // best solitary deviations of the current seed board.  Add these boards to
  // the Evaluate and Master lists.  They Have not been fully evaluated yet.
  // These boards will not get evaluated in the threads, so evaluate them
  // here.  Add them to the master list if they qualify.

  for (int X = 0; X < SQUARE_COUNT; X++) {
    auto bd = SeedBoard;
    bd.off_limit_cell = X;
    char TheSeedLetter = SeedBoard.board[X];

    for (int Y = 0; Y < SIZE_OF_CHARACTER_SET; Y++) {
      // This statement indicates that less new boards are generated for each
      // evaluation board, as in one square will be off limits.  This is how
      // we arrive at the number "SOLITARY_DEVIATIONS".
      if (TheSeedLetter == CHARACTER_SET[Y]) continue;
      bd.board[X] = CHARACTER_SET[Y];

      int score = boggler->Score(bd.board.c_str());
      assert(score >= 0);
      BoardScore board_score(score, bd);

      // Try to add each board to the "MasterResultsBoardList", and the
      // "TopEvaluationBoardList".  Do this in sequence.  Only the
      // "WhatMadeTheMasterList" MinBoardTrie will be augmented.
      InsertIntoMasterList(MasterResults, board_score);
      if (AllEvaluatedBoards.find(bd) == AllEvaluatedBoards.end()) {
        InsertIntoEvaluateList(TopEvaluationBoardList, board_score);
      }
    }
  }

  // This Loop Represents the rounds cascade.
  for (int T = 0; T < ROUNDS; T++) {
    // Initiate a "MinBoardTrie" to keep track of the round returns.
    set<BoardWithCell, BoardComparator> CurrentBoardsConsideredThisRound;

    // Add the board strings from TopEvaluationBoardList to the
    // "AllEvaluatedBoards" trie.
    for (int X = 0; X < BOARDS_PER_ROUND && X < TopEvaluationBoardList.size(); X++) {
      AllEvaluatedBoards.insert(TopEvaluationBoardList[X].board);
    }

    // The boards on the evaluate list in round zero have already been added
    // to the master list.
    if (T != 0) {
      AddBoardsToMasterList(MasterResults, TopEvaluationBoardList);
    }

    // Even if nothing qualifies for the master list on this round, print out
    // the best result for the round to keep track of the progress.
    PrintBestBoard(T, TopEvaluationBoardList);
    printf("\nThe Top 10 Off The Master List After Round |%d|.\n", T);
    PrintBoardList(MasterResults, 10);

    // Process all boards directly (no threading)
    // Save the current evaluation list before processing
    std::vector<BoardScore> CurrentEvaluationList = TopEvaluationBoardList;
    // Clear the evaluation board list so we can fill it with the next round
    // boards.
    TopEvaluationBoardList.clear();

    // Fill "WorkingBoardScoreTally" with all single deviations of the boards
    // in CurrentEvaluationList
    unsigned int InsertionSlot = 0;
    for (int X = 0; X < BOARDS_PER_ROUND && X < CurrentEvaluationList.size(); X++) {
      const auto &board = CurrentEvaluationList[X].board;
      auto OffLimitSquare = board.off_limit_cell;
      for (int Y = 0; Y < SQUARE_COUNT; Y++) {
        if (Y == OffLimitSquare) continue;
        // Y will now represent the placement of the off limits Square so set
        // it as such.
        BoardWithCell temp_board(board.board, Y);
        unsigned int OffLimitLetterIndex = CHARACTER_LOCATIONS[board.board[Y] - 'A'];
        for (int Z = 0; Z < SIZE_OF_CHARACTER_SET; Z++) {
          if (Z == OffLimitLetterIndex) continue;
          temp_board.board[Y] = CHARACTER_SET[Z];
          WorkingBoardScoreTally[InsertionSlot].board = temp_board;
          InsertionSlot += 1;
        }
      }
    }

    // Evaluate all of the single deviation boards and store the scores
    for (int X = 0; X < LIST_SIZE; X++) {
      auto score = boggler->Score(WorkingBoardScoreTally[X].board.board.c_str());
      assert(score >= 0);
      WorkingBoardScoreTally[X].score = score;
    }

    // Sort the results in descending order by score using std::sort
    std::sort(
        WorkingBoardScoreTally.begin(),
        WorkingBoardScoreTally.end(),
        [](const BoardScore &a, const BoardScore &b) { return a.score > b.score; }
    );

    // Process the results - add qualifying boards to the evaluation list for
    // the next round
    for (int Y = 0; Y < LIST_SIZE; Y++) {
      // Because the list is sorted, once we find a board that doesn't make
      // this evaluation round, get the fuck out.
      auto min_score = TopEvaluationBoardList.size() == EVALUATE_LIST_SIZE
                           ? TopEvaluationBoardList.back().score
                           : 0;
      const auto &board = WorkingBoardScoreTally[Y];
      if (board.score <= min_score) {
        break;
      }
      if (AddBoard(CurrentBoardsConsideredThisRound, board.board) == 1) {
        if (AllEvaluatedBoards.find(board.board) == AllEvaluatedBoards.end()) {
          InsertIntoEvaluateList(TopEvaluationBoardList, board);
        }
      }
    }
  }

  AddBoardsToMasterList(MasterResults, TopEvaluationBoardList);

  return TopEvaluationBoardList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
  // The seed board selection process is beyond the scope of this program.
  // For now, choose seeds that are as different as possible and verify that
  // they all produce the same results.
  string SeedBoard(MASTER_SEED_BOARD);

  // The Master List - now using vector
  std::vector<BoardScore> MasterResults;
  MasterResults.reserve(MASTER_LIST_SIZE);

  // These "MinBoardTrie"s will maintain information about the search so that
  // new boards will continue to be evaluated.  This is an important construct
  // to a search algorithm.
  set<BoardWithCell, BoardComparator> AllEvaluatedBoards;
  set<BoardWithCell, BoardComparator> ChosenSeedBoards;

  // Allocate the global variables for board processing

  printf(
      "DoubleUp.c Variables - Chain Seeds |%d|, Single Deviation Rounds "
      "|%d|, Full Evaluations Per Round |%d|.\n\n",
      NUMBER_OF_SEEDS_TO_RUN,
      ROUNDS,
      BOARDS_PER_ROUND
  );

  auto trie = Trie::CreateFromFile("twl06.txt");
  if (!trie.get()) {
    fprintf(stderr, "Unable to load dictionary\n");
    return 1;
  }
  auto boggler = new Boggler<5, 5>(trie.get());

  // TODO: reduce scope of this variable
  unsigned int TemporaryBoardScore = boggler->Score(SeedBoard.c_str());
  assert(TemporaryBoardScore >= 0);

  // The very first task is to insert the original seed board into the master
  // list.
  BoardWithCell seed(SeedBoard, 0);
  InsertIntoMasterList(MasterResults, {TemporaryBoardScore, seed});

  printf(
      "This is the original seed board that will be used...  It is worth "
      "|%d| points.  Sleep for 2 seconds to look at it\n\n",
      TemporaryBoardScore
  );
  PrintBoard(SeedBoard);

  // This loop represents the chain seeds cascade.
  for (int S = 0; S < NUMBER_OF_SEEDS_TO_RUN; S++) {
    for (const auto &result : MasterResults) {
      const auto &board = result.board;
      if (ChosenSeedBoards.find(board) == ChosenSeedBoards.end()) {
        seed = board;
        TemporaryBoardScore = result.score;
        break;
      }
    }

    printf(
        "For the |%d|'th run the seed board is |%s| worth |%d| points.\n",
        S + 1,
        seed.board.c_str(),
        TemporaryBoardScore
    );
    ChosenSeedBoards.insert(seed);
    AllEvaluatedBoards.insert(seed);

    auto TopEvaluationBoardList =
        RunOneSeed(seed, boggler, MasterResults, AllEvaluatedBoards);

    // Even if nothing qualifies for the master list on this round, print out
    // the best result for the round to keep track of the progress.
    PrintBestBoard(ROUNDS, TopEvaluationBoardList);

    // The last round is now complete, so we have to get ready for the next
    // seed.
    printf("\nThe Top 10 Off The Master List After Round |%d|.\n", ROUNDS);
    PrintBoardList(MasterResults, 10);
    printf("\n");

    printf(
        "At this point, |%zu| boards have been placed on the evaluation "
        "queue, and have been singularly deviated.\n",
        AllEvaluatedBoards.size()
    );

    // Print out everything on the master results list after running each chain
    // seed.
    printf("\nThe Master List After Seed |%d|.\n", S + 1);
    PrintBoardList(MasterResults);
  }

  // Produce a list of the boards used as seeds when done, and wait for the user
  // to look at, and store the results if they want to.
  printf("The boards used as seed boards are as follows:..\n");
  printf("This Min Board Trie Contains |%zu| Boards.\n", ChosenSeedBoards.size());
  for (const auto &board : ChosenSeedBoards) {
    printf("|%s|\n", board.board.c_str());
  }

  return 0;
}

// This is a deterministic way to find the top 10 Boggle boards beyond a
// reasonable doubt.  This is one solution that works.  That is all.

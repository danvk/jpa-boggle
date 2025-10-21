// The DeepSearch.c algorithm uses an in-depth investigation of neighbouring
// boards starting at a predefined "MASTER_SEED_BOARD". The constants that
// define the depth of the search...  NUMBER_OF_SEEDS_TO_RUN, ROUNDS,
// BOARDS_PER_ROUND

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

// #include "adtdawg.h"
#include "boggler.h"
#include "trie.h"
// #include "board-evaluate.h"
#include <format>

#include "const.h"
#include "insert.h"

// The ADTDAWG for Lexicon_14, a subset of TWL06, is located in the 4 data files
// listed below.
#define FOUR_PART_DTDAWG_14_PART_ONE "Four_Part_1_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_TWO "Four_Part_2_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_THREE "Four_Part_3_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_FOUR "Four_Part_4_DTDAWG_For_Lexicon_14.dat"

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

// Custom comparator for set<string> that only compares first SQUARE_COUNT
// characters
struct BoardComparator {
  bool operator()(const string &a, const string &b) const {
    return a.compare(0, SQUARE_COUNT, b, 0, SQUARE_COUNT) < 0;
  }
};

// Returns 1 if this adds board to the container
int AddBoard(set<string, BoardComparator> &container, const char *board) {
  if (board == NULL) return 0;
  std::string s(board);
  auto result = container.insert(s);
  return result.second ? 1 : 0;
}

void PrintBoardList(const vector<BoardScore> &list, int num_to_print = 10'000) {
  for (int i = 0; i < num_to_print && i < list.size(); i++) {
    printf("#%4d -|%5d|-|%s|\n", i + 1, list[i].score, list[i].board.c_str());
  }
}

void PrintBestBoard(int round, const vector<BoardScore> &list) {
  if (!list.empty()) {
    printf(
        "\nRound|%d|, Best Board|%s|, Best Score|%d|\n",
        round,
        list[0].board.c_str(),
        list[0].score
    );
  }
}

void PrintBoard(const string &board) {
  printf("-----------\n");
  printf("%s\n", board.c_str());
  printf("-----------\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
  // for() loop counter variables.
  unsigned int X, Y, Z, T, S;

  // Variables for board processing
  unsigned int InsertionSlot;

  // The seed board selection process is beyond the scope of this program.
  // For now, choose seeds that are as different as possible and verify that
  // they all produce the same results.
  string SeedBoard(MASTER_SEED_BOARD);

  // The evaluation lists - now using vector
  std::vector<BoardScore> TopEvaluationBoardList;
  TopEvaluationBoardList.reserve(EVALUATE_LIST_SIZE);

  // The Master List - now using vector
  std::vector<BoardScore> MasterResults;
  MasterResults.reserve(MASTER_LIST_SIZE);

  // Holders used for the seed board single deviations before the deviation
  // rounds begin.
  string InitialWorkingBoard;
  string WorkingBoard;
  string TemporaryBoardString;
  string TempBoardString;
  char TheNewOffLimitSquareString[3];
  unsigned int TemporaryBoardScore;
  char TheSeedLetter;
  unsigned int OffLimitSquare;
  unsigned int OffLimitLetterIndex;

  // These "MinBoardTrie"s will maintain information about the search so that
  // new boards will continue to be evaluated.  This is an important construct
  // to a search algorithm.
  set<string, BoardComparator> CurrentBoardsConsideredThisRound;
  set<string, BoardComparator> AllEvaluatedBoards;
  set<string, BoardComparator> ChosenSeedBoards;
  set<string, BoardComparator> WhatMadeTheMasterList;

  // Allocate the global variables for board processing

  // Vector of BoardScore objects for working board tallies
  std::vector<BoardScore> WorkingBoardScoreTally(LIST_SIZE);

  printf(
      "DoubleUp.c Variables - Chain Seeds |%d|, Single Deviation Rounds "
      "|%d|, Full Evaluations Per Round |%d|.\n\n",
      NUMBER_OF_SEEDS_TO_RUN,
      ROUNDS,
      BOARDS_PER_ROUND
  );

  auto trie = Trie::CreateFromFile("enable2k.txt");
  if (!trie.get()) {
    fprintf(stderr, "Unable to load dictionary\n");
    return 1;
  }
  auto boggler = new Boggler<5, 5>(trie.get());

  // The very first task is to insert the original seed board into the master
  // list.
  WhatMadeTheMasterList.insert(SeedBoard);
  InsertIntoMasterList(MasterResults, TemporaryBoardScore, SeedBoard);

  printf(
      "This is the original seed board that will be used...  It is worth "
      "|%d| points.  Sleep for 2 seconds to look at it\n\n",
      TemporaryBoardScore
  );
  PrintBoard(InitialWorkingBoard);

  // This loop represents the chain seeds cascade.
  for (S = 0; S < NUMBER_OF_SEEDS_TO_RUN; S++) {
    // Before checking the "AllEvaluatedBoards" Trie, test if the score is high
    // enough to make the list. The scores attached to this list needs to be
    // reset every time that we start a new seed, the important remaining list
    // is the master list.
    TopEvaluationBoardList.clear();

    for (const auto &result : MasterResults) {
      const auto &board = result.board;
      if (ChosenSeedBoards.find(board) == ChosenSeedBoards.end()) {
        SeedBoard = board;
        TemporaryBoardScore = result.score;
        break;
      }
    }

    printf(
        "For the |%d|'th run the seed board is |%s| worth |%d| points.\n",
        S + 1,
        SeedBoard.c_str(),
        TemporaryBoardScore
    );
    ChosenSeedBoards.insert(SeedBoard);
    AllEvaluatedBoards.insert(SeedBoard);

    // Populate the evaluate list for the first round of boards based on the
    // best solitary deviations of the current seed board.  Add these boards to
    // the Evaluate and Master lists.  They Have not been fully evaluated yet.
    // These boards will not get evaluated in the threads, so evaluate them
    // here.  Add them to the master list if they qualify.
    TemporaryBoardString = SeedBoard + "00";
    for (X = 0; X < SQUARE_COUNT; X++) {
      if (X > 0) TemporaryBoardString[X - 1] = SeedBoard[X - 1];
      char buf[3];
      snprintf(buf, 3, "%02d", X);

      TemporaryBoardString[SQUARE_COUNT] = buf[0];
      TemporaryBoardString[SQUARE_COUNT + 1] = buf[1];
      TheSeedLetter = SeedBoard[X];

      for (Y = 0; Y < SIZE_OF_CHARACTER_SET; Y++) {
        // This statement indicates that less new boards are generated for each
        // evaluation board, as in one square will be off limits.  This is how
        // we arrive at the number "SOLITARY_DEVIATIONS".
        if (TheSeedLetter == CHARACTER_SET[Y]) continue;
        TemporaryBoardString[X] = CHARACTER_SET[Y];

        TemporaryBoardScore = boggler->Score(TemporaryBoardString.c_str());

        // Try to add each board to the "MasterResultsBoardList", and the
        // "TopEvaluationBoardList".  Do this in sequence.  Only the
        // "WhatMadeTheMasterList" MinBoardTrie will be augmented.
        if (WhatMadeTheMasterList.find(TemporaryBoardString) ==
            WhatMadeTheMasterList.end()) {
          size_t old_size = MasterResults.size();
          InsertIntoMasterList(
              MasterResults, TemporaryBoardScore, TemporaryBoardString
          );
          if (MasterResults.size() > old_size ||
              (MasterResults.size() == MASTER_LIST_SIZE &&
               TemporaryBoardScore > MasterResults.back().score)) {
            WhatMadeTheMasterList.insert(TemporaryBoardString);
            // printf("Round |%d|Pop - New On Master |%s| Score |%d|\n", 0,
            // TemporaryBoardString, TemporaryBoardScore);
          }
        }
        if (AllEvaluatedBoards.find(TemporaryBoardString) == AllEvaluatedBoards.end()) {
          InsertIntoEvaluateList(
              TopEvaluationBoardList, TemporaryBoardScore, TemporaryBoardString
          );
        }
      }
    }

    // This Loop Represents the rounds cascade.
    for (T = 0; T < ROUNDS; T++) {
      // Initiate a "MinBoardTrie" to keep track of the round returns.
      set<string, BoardComparator> CurrentBoardsConsideredThisRound;

      // Add the board strings from TopEvaluationBoardList to the
      // "AllEvaluatedBoards" trie.
      for (X = 0; X < BOARDS_PER_ROUND && X < TopEvaluationBoardList.size(); X++) {
        AllEvaluatedBoards.insert(TopEvaluationBoardList[X].board);
      }
      // The boards on the evaluate list in round zero have already been added
      // to the master list.
      if (T != 0) {
        unsigned int min_master_score =
            MasterResults.size() == MASTER_LIST_SIZE ? MasterResults.back().score : 0;
        for (const auto &result : TopEvaluationBoardList) {
          const auto &board = result.board;
          const auto &score = result.score;

          if (score > min_master_score) {
            if (WhatMadeTheMasterList.find(board) == WhatMadeTheMasterList.end()) {
              InsertIntoMasterList(MasterResults, score, board.c_str());
              WhatMadeTheMasterList.insert(board);
            }
          }
          // As soon as a board is reached that doesn't make the master list,
          // get the fuck out of here.
          else
            break;
        }
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
      InsertionSlot = 0;
      for (X = 0; X < BOARDS_PER_ROUND && X < CurrentEvaluationList.size(); X++) {
        TempBoardString = CurrentEvaluationList[X].board;
        OffLimitSquare = atoi(TempBoardString.c_str() + SQUARE_COUNT);
        for (Y = 0; Y < SQUARE_COUNT; Y++) {
          if (Y == OffLimitSquare) continue;
          // Y will now represent the placement of the off limits Square so set
          // it as such.
          char buf[3];
          snprintf(buf, 3, "%02d", Y);
          TempBoardString[SQUARE_COUNT] = buf[0];
          TempBoardString[SQUARE_COUNT + 1] = buf[1];
          OffLimitLetterIndex = CHARACTER_LOCATIONS[TempBoardString[Y] - 'A'];
          for (Z = 0; Z < SIZE_OF_CHARACTER_SET; Z++) {
            if (Z == OffLimitLetterIndex) continue;
            TempBoardString[Y] = CHARACTER_SET[Z];
            WorkingBoardScoreTally[InsertionSlot].board = TempBoardString;
            InsertionSlot += 1;
          }
          TempBoardString[Y] = CHARACTER_SET[OffLimitLetterIndex];
        }
      }

      // Evaluate all of the single deviation boards and store the scores
      for (X = 0; X < LIST_SIZE; X++) {
        auto score = boggler->Score(WorkingBoardScoreTally[X].board.c_str());
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
      for (Y = 0; Y < LIST_SIZE; Y++) {
        // Because the list is sorted, once we find a board that doesn't make
        // this evaluation round, get the fuck out.
        unsigned int min_eval_score =
            TopEvaluationBoardList.size() == EVALUATE_LIST_SIZE
                ? TopEvaluationBoardList.back().score
                : 0;
        const auto &board = WorkingBoardScoreTally[Y];
        if (board.score > min_eval_score) {
          if (AddBoard(CurrentBoardsConsideredThisRound, board.board.c_str()) == 1) {
            if (AllEvaluatedBoards.find(board.board) == AllEvaluatedBoards.end()) {
              InsertIntoEvaluateList(
                  TopEvaluationBoardList, board.score, board.board.c_str()
              );
            }
          }
        } else
          break;
      }
    }

    // Print to screen all of the new boards that qualified for the
    // "MasterResults" on the final round.
    unsigned int min_master_score_final =
        MasterResults.size() == MASTER_LIST_SIZE ? MasterResults.back().score : 0;
    for (const auto &result : TopEvaluationBoardList) {
      const auto &board = result.board;
      const auto &score = result.score;
      if (score > min_master_score_final) {
        if (WhatMadeTheMasterList.find(board) == WhatMadeTheMasterList.end()) {
          InsertIntoMasterList(MasterResults, score, board.c_str());
          WhatMadeTheMasterList.insert(board);
        }
      }
      // As soon as a board is reached that doesn't make the master list, get
      // the fuck out of here.
      else
        break;
    }
    // Even if nothing qualifies for the master list on this round, print out
    // the best result for the round to keep track of the progress.
    PrintBestBoard(T, TopEvaluationBoardList);

    // The last round is now complete, so we have to get ready for the next
    // seed.
    printf("\nThe Top 10 Off The Master List After Round |%d|.\n", T);
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
    printf("|%s|\n", board.c_str());
  }

  return 0;
}

// This is a deterministic way to find the top 10 Boggle boards beyond a
// reasonable doubt.  This is one solution that works.  That is all.

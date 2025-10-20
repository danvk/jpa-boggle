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

#include "adtdawg.h"
#include "board-evaluate.h"
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

int ReadLexicon() {
  // The ADTDAWG lexicon is stored inside of four files, and then read into
  // three arrays for speed.  This is the case because the data structure is
  // extremely small.
  FILE *PartOne = fopen(FOUR_PART_DTDAWG_14_PART_ONE, "rb");
  FILE *PartTwo = fopen(FOUR_PART_DTDAWG_14_PART_TWO, "rb");
  FILE *PartThree = fopen(FOUR_PART_DTDAWG_14_PART_THREE, "rb");
  FILE *PartFour = fopen(FOUR_PART_DTDAWG_14_PART_FOUR, "rb");
  unsigned int SizeOfPartOne;
  unsigned long int SizeOfPartTwo;
  unsigned int SizeOfPartThree;
  unsigned int SizeOfPartFour;
  unsigned char TempReadIn;

  // Read in the size of each data file.
  if (fread(&SizeOfPartOne, 4, 1, PartOne) != 1) return 0;
  if (fread(&SizeOfPartTwo, 8, 1, PartTwo) != 1) return 0;
  if (fread(&SizeOfPartThree, 4, 1, PartThree) != 1) return 0;
  PartThreeFourTransition = SizeOfPartThree + 1;
  SizeOfPartFour = SizeOfPartOne - SizeOfPartThree;

  // Print out the lexicon size values.
  printf("\n");
  printf("SizeOfPartOne |%d|\n", SizeOfPartOne);
  printf("SizeOfPartTwo |%ld|\n", SizeOfPartTwo);
  printf("SizeOfPartThree |%d|\n", SizeOfPartThree);
  printf("Transition |%d|\n", PartThreeFourTransition);
  printf("SizeOfPartFour |%d|\n", SizeOfPartFour);
  printf("\n");

  // Allocate memory for the ADTDAWG.
  PartOneArray = (unsigned int *)malloc((SizeOfPartOne + 1) * sizeof(int));
  PartTwoArray = (unsigned long int *)malloc(SizeOfPartTwo * sizeof(long int));
  PartThreeArray = (unsigned int *)malloc((SizeOfPartOne + 1) * sizeof(int));

  // Read in the data files into global arrays of basic integer types.
  // The zero position in "PartOneArray" is the NULL node.
  PartOneArray[0] = 0;
  if (fread(PartOneArray + 1, 4, SizeOfPartOne, PartOne) != SizeOfPartOne) return 0;
  if (fread(PartTwoArray, 8, SizeOfPartTwo, PartTwo) != SizeOfPartTwo) return 0;
  // The Zero position in "PartThreeArray" maps to the NULL node in
  // "PartOneArray".
  PartThreeArray[0] = 0;
  if (fread(PartThreeArray + 1, 4, SizeOfPartThree, PartThree) != SizeOfPartThree)
    return 0;
  // Part Four has been replaced by encoding the Part Four WTEOBL values as 32
  // bit integers for speed.  The size of the data structure is small enough as
  // it is.
  for (unsigned int X = (SizeOfPartThree + 1); X <= SizeOfPartOne; X++) {
    if (fread(&TempReadIn, 1, 1, PartFour) != 1) return 0;
    PartThreeArray[X] = TempReadIn;
  }

  // Close the four files.
  fclose(PartOne);
  fclose(PartTwo);
  fclose(PartThree);
  fclose(PartFour);

  // Print out the high level algorithm variables for this run of the
  // DeepSearch.
  printf("ADTDAWG Read of Lexicon_14 is Complete.\n\n");
  return 1;
}

// Returns 1 if this adds board to the container
int AddBoard(set<string, BoardComparator> &container, char *board) {
  if (board == NULL) return 0;
  std::string s(board);
  auto result = container.insert(s);
  return result.second ? 1 : 0;
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
  char SeedBoard[BOARD_STRING_SIZE] = MASTER_SEED_BOARD;

  // The evaluation lists - now using vector
  std::vector<BoardScore> TopEvaluationBoardList;
  TopEvaluationBoardList.reserve(EVALUATE_LIST_SIZE);

  // The Master List - now using vector
  std::vector<BoardScore> MasterResults;
  MasterResults.reserve(MASTER_LIST_SIZE);

  // Holders used for the seed board single deviations before the deviation
  // rounds begin.
  BoardPtr InitialWorkingBoard;
  BoardPtr WorkingBoard;
  char TemporaryBoardString[BOARD_STRING_SIZE];
  char TempBoardString[BOARD_STRING_SIZE];
  char TheNewOffLimitSquareString[3];
  unsigned int TemporaryBoardScore;
  char SquareNumberString[3];
  char TheSeedLetter;
  unsigned int TheCurrentTime = 0;
  unsigned int OffLimitSquare;
  unsigned int OffLimitLetterIndex;

  // These "MinBoardTrie"s will maintain information about the search so that
  // new boards will continue to be evaluated.  This is an important construct
  // to a search algorithm.
  set<string, BoardComparator> CurrentBoardsConsideredThisRound;
  set<string, BoardComparator> AllEvaluatedBoards;
  set<string, BoardComparator> ChosenSeedBoards;
  set<string, BoardComparator> WhatMadeTheMasterList;

  if (ReadLexicon() == 0) {
    return 0;
  }

  // Allocate the global variables for board processing

  // The global array of BoardScore pointers. All of the associated BoardScore
  // objects will thus never move around. The main function will allocate the space
  // required to store the actual BoardScore objects.
  BoardScore **WorkingBoardScoreTally;

  // Allocate the array of BoardScore pointers.
  WorkingBoardScoreTally = (BoardScore **)malloc(LIST_SIZE * sizeof(BoardScore *));

  // Allocate the actual BoardScore objects that the pointers will point to.
  for (Y = 0; Y < LIST_SIZE; Y++)
    WorkingBoardScoreTally[Y] = new BoardScore();

  // Allocate the explicit discovery stack.
  TheDiscoveryStack =
      (DiscoveryStackNode *)malloc((DISCOVERY_STACK_SIZE) * sizeof(DiscoveryStackNode));

  // Allocate the set of lexicon time stamps as unsigned integers.
  LexiconTimeStamps =
      (unsigned int *)malloc((TOTAL_WORDS_IN_LEXICON + 1) * sizeof(unsigned int));

  printf(
      "DoubleUp.c Variables - Chain Seeds |%d|, Single Deviation Rounds "
      "|%d|, Full Evaluations Per Round |%d|.\n\n",
      NUMBER_OF_SEEDS_TO_RUN,
      ROUNDS,
      BOARDS_PER_ROUND
  );

  // Populate the "InitialWorkingBoard" and "WorkingBoard" with the original
  // seed board.
  InitialWorkingBoard = (Board *)malloc(sizeof(Board));
  BoardInit(InitialWorkingBoard);
  BoardPopulate(InitialWorkingBoard, SeedBoard);

  WorkingBoard = (Board *)malloc(sizeof(Board));
  BoardInit(WorkingBoard);

  // Zero all of the time stamps for the words
  memset(LexiconTimeStamps, 0, (TOTAL_WORDS_IN_LEXICON + 1) * sizeof(unsigned int));

  // The very first task is to insert the original seed board into the master
  // list.
  TheCurrentTime += 1;
  TemporaryBoardScore = BoardSquareWordDiscover(InitialWorkingBoard, TheCurrentTime);
  WhatMadeTheMasterList.insert(SeedBoard);
  InsertIntoMasterList(MasterResults, TemporaryBoardScore, SeedBoard);

  printf(
      "This is the original seed board that will be used...  It is worth "
      "|%d| points.  Sleep for 2 seconds to look at it\n\n",
      TemporaryBoardScore
  );
  BoardOutput(InitialWorkingBoard);

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
        strcpy(SeedBoard, board.c_str());
        TemporaryBoardScore = result.score;
        break;
      }
    }

    SeedBoard[SQUARE_COUNT] = '\0';
    printf(
        "For the |%d|'th run the seed board is |%s| worth |%d| points.\n",
        S + 1,
        SeedBoard,
        TemporaryBoardScore
    );
    ChosenSeedBoards.insert(SeedBoard);
    AllEvaluatedBoards.insert(SeedBoard);

    // Populate the evaluate list for the first round of boards based on the
    // best solitary deviations of the current seed board.  Add these boards to
    // the Evaluate and Master lists.  They Have not been fully evaluated yet.
    // These boards will not get evaluated in the threads, so evaluate them
    // here.  Add them to the master list if they qualify.
    strcpy(TemporaryBoardString, SeedBoard);
    for (X = 0; X < SQUARE_COUNT; X++) {
      if (X > 0) TemporaryBoardString[X - 1] = SeedBoard[X - 1];
      ConvertSquareNumberToString(SquareNumberString, X);
      strcpy(TemporaryBoardString + SQUARE_COUNT, SquareNumberString);
      TheSeedLetter = SeedBoard[X];
      for (Y = 0; Y < SIZE_OF_CHARACTER_SET; Y++) {
        // This statement indicates that less new boards are generated for each
        // evaluation board, as in one square will be off limits.  This is how
        // we arrive at the number "SOLITARY_DEVIATIONS".
        if (TheSeedLetter == CHARACTER_SET[Y]) continue;
        TemporaryBoardString[X] = CHARACTER_SET[Y];
        BoardPopulate(InitialWorkingBoard, TemporaryBoardString);
        TheCurrentTime += 1;
        TemporaryBoardScore =
            BoardSquareWordDiscover(InitialWorkingBoard, TheCurrentTime);
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
              // printf("Round |%d|Pop - New On Master |%s| Score |%d|\n", T,
              // TopEvaluationBoardList[X].board.c_str(),
              // TopEvaluationBoardList[X].score);
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
      if (!TopEvaluationBoardList.empty()) {
        printf(
            "\nRound|%d|, Best Board|%s|, Best Score|%d|\n",
            T,
            TopEvaluationBoardList[0].board.c_str(),
            TopEvaluationBoardList[0].score
        );
      }
      printf("\nThe Top 10 Off The Master List After Round |%d|.\n", T);
      for (X = 0; X < 10 && X < MasterResults.size(); X++) {
        printf(
            "#%4d -|%5d|-|%s|\n",
            X + 1,
            MasterResults[X].score,
            MasterResults[X].board.c_str()
        );
      }

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
        strcpy(TempBoardString, CurrentEvaluationList[X].board.c_str());
        OffLimitSquare = TwoCharStringToInt(&(TempBoardString[SQUARE_COUNT]));
        for (Y = 0; Y < SQUARE_COUNT; Y++) {
          if (Y == OffLimitSquare) continue;
          // Y will now represent the placement of the off limits Square so set
          // it as such.
          ConvertSquareNumberToString(TheNewOffLimitSquareString, Y);
          TempBoardString[SQUARE_COUNT] = TheNewOffLimitSquareString[0];
          TempBoardString[SQUARE_COUNT + 1] = TheNewOffLimitSquareString[1];
          OffLimitLetterIndex = CHARACTER_LOCATIONS[TempBoardString[Y] - 'A'];
          for (Z = 0; Z < SIZE_OF_CHARACTER_SET; Z++) {
            if (Z == OffLimitLetterIndex) continue;
            TempBoardString[Y] = CHARACTER_SET[Z];
            WorkingBoardScoreTally[InsertionSlot]->board = TempBoardString;
            InsertionSlot += 1;
          }
          TempBoardString[Y] = CHARACTER_SET[OffLimitLetterIndex];
        }
      }

      // Evaluate all of the single deviation boards and store the scores
      for (X = 0; X < LIST_SIZE; X++) {
        TheCurrentTime += 1;
        // Insert the board score into the "WorkingBoardScoreTally" array.
        BoardPopulate(WorkingBoard, const_cast<char*>(WorkingBoardScoreTally[X]->board.c_str()));
        WorkingBoardScoreTally[X]->score =
            BoardSquareWordDiscover(WorkingBoard, TheCurrentTime);
      }

      // Sort the results in descending order by score using std::sort
      std::sort(
          WorkingBoardScoreTally,
          WorkingBoardScoreTally + LIST_SIZE,
          [](const BoardScore *a, const BoardScore *b) { return a->score > b->score; }
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
        if (board->score > min_eval_score) {
          if (AddBoard(CurrentBoardsConsideredThisRound, const_cast<char*>(board->board.c_str())) == 1) {
            if (AllEvaluatedBoards.find(board->board) == AllEvaluatedBoards.end()) {
              InsertIntoEvaluateList(
                  TopEvaluationBoardList, board->score, board->board.c_str()
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
          // printf("Round |%d|Pop - New On Master |%s| Score |%d|\n", T,
          // TopEvaluationBoardList[X].board.c_str(),
          // TopEvaluationBoardList[X].score);
        }
      }
      // As soon as a board is reached that doesn't make the master list, get
      // the fuck out of here.
      else
        break;
    }
    // Even if nothing qualifies for the master list on this round, print out
    // the best result for the round to keep track of the progress.
    if (!TopEvaluationBoardList.empty()) {
      printf(
          "\nRound|%d|, Best Board|%s|, Best Score|%d|\n",
          T,
          TopEvaluationBoardList[0].board.c_str(),
          TopEvaluationBoardList[0].score
      );
    }
    // The last round is now complete, so we have to get ready for the next
    // seed.
    printf("\nThe Top 10 Off The Master List After Round |%d|.\n", T);
    for (X = 0; X < 10 && X < MasterResults.size(); X++) {
      printf(
          "#%4d -|%5d|-|%s|\n",
          X + 1,
          MasterResults[X].score,
          MasterResults[X].board.c_str()
      );
    }
    printf("\n");

    printf(
        "At this point, |%zu| boards have been placed on the evaluation "
        "queue, and have been singularly deviated.\n",
        AllEvaluatedBoards.size()
    );

    // Print out everything on the master results list after running each chain
    // seed.
    printf("\nThe Master List After Seed |%d|.\n", S + 1);
    for (X = 0; X < MasterResults.size(); X++) {
      printf(
          "#%4d -|%5d|-|%s|\n",
          X + 1,
          MasterResults[X].score,
          MasterResults[X].board.c_str()
      );
    }
  }

  // Produce a list of the boards used as seeds when done, and wait for the user
  // to look at, and store the results if they want to.
  printf("The boards used as seed boards are as follows:..\n");
  printf("This Min Board Trie Contains |%zu| Boards.\n", ChosenSeedBoards.size());
  for (const auto &board : ChosenSeedBoards) {
    printf("|%s|\n", board.c_str());
  }

  free(InitialWorkingBoard);
  free(WorkingBoard);

  // Clean up WorkingBoardScoreTally
  for (Y = 0; Y < LIST_SIZE; Y++)
    delete WorkingBoardScoreTally[Y];
  free(WorkingBoardScoreTally);

  return 0;
}

// This is a deterministic way to find the top 10 Boggle boards beyond a
// reasonable doubt.  This is one solution that works.  That is all.

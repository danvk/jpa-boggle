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
#include "insert.h"
#include "trie.h"

// General "Boggle" Constants.
#define SQUARE_COUNT 25
#define NUMBER_OF_ENGLISH_LETTERS 26
#define SIZE_OF_CHARACTER_SET 14

// These constant arrays define the lexicon contained in the ADTDAWG.
char CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1] = {
    'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T', ' '
};

#define BOARDS_PER_ROUND 64

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

vector<BoardScore> GenerateSingleDeviations(
    const vector<BoardWithCell> &boards, Boggler<5, 5> *boggler
) {
  vector<BoardScore> deviations;
  deviations.reserve(boards.size() * (SQUARE_COUNT - 1) * (SIZE_OF_CHARACTER_SET - 1));

  for (const auto &board : boards) {
    auto off_limit_cell = board.off_limit_cell;
    for (int cell = 0; cell < SQUARE_COUNT; cell++) {
      if (cell == off_limit_cell) continue;
      BoardWithCell temp_board(board.board, cell);
      auto orig_char = board.board[cell];
      for (int i = 0; i < SIZE_OF_CHARACTER_SET; i++) {
        auto ch = CHARACTER_SET[i];
        if (ch == orig_char) continue;
        temp_board.board[cell] = ch;
        auto score = boggler->Score(temp_board.board.c_str());
        assert(score >= 0);
        deviations.push_back(BoardScore(score, temp_board));
      }
    }
  }

  return deviations;
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

  auto SeedVariations = GenerateSingleDeviations({{SeedBoard.board, -1}}, boggler);
  for (const auto &board_score : SeedVariations) {
    InsertIntoMasterList(MasterResults, board_score);
    if (AllEvaluatedBoards.find(board_score.board) == AllEvaluatedBoards.end()) {
      InsertIntoEvaluateList(TopEvaluationBoardList, board_score);
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

    vector<BoardWithCell> sources;
    sources.reserve(BOARDS_PER_ROUND);
    for (int X = 0; X < BOARDS_PER_ROUND && X < CurrentEvaluationList.size(); X++) {
      sources.push_back(CurrentEvaluationList[X].board);
    }
    auto WorkingBoardScoreTally = GenerateSingleDeviations(sources, boggler);

    // Sort the results in descending order by score.
    std::sort(
        WorkingBoardScoreTally.begin(),
        WorkingBoardScoreTally.end(),
        [](const BoardScore &a, const BoardScore &b) { return a.score > b.score; }
    );

    // Process the results - add qualifying boards to the evaluation list for
    // the next round
    for (const auto &board : WorkingBoardScoreTally) {
      auto min_score = TopEvaluationBoardList.size() == EVALUATE_LIST_SIZE
                           ? TopEvaluationBoardList.back().score
                           : 0;
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

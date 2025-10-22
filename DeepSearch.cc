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
#include <ranges>
#include <set>
#include <span>
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
#define NUMBER_OF_SEEDS_TO_RUN 100
#define ROUNDS 25
#define BOARDS_PER_THREAD BOARDS_PER_ROUND

// define the size of the list being sorted.  This number represents the total
// number of boards analyzed per round.
#define LIST_SIZE (BOARDS_PER_THREAD * SINGLE_DEVIATIONS)

using namespace std;

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

vector<BoardScore> BuildEvalList(
    const vector<BoardScore> &boards, const set<string> &all_evaluated_boards
) {
  vector<BoardScore> next_eval_list;
  for (const auto &board : boards) {
    if (all_evaluated_boards.find(board.board.board) == all_evaluated_boards.end()) {
      InsertIntoEvaluateList(next_eval_list, board);
    }
  }
  return next_eval_list;
}

int ScoreBoard(
    const BoardWithCell &board,
    Boggler<5, 5> *boggler,
    vector<BoardScore> &MasterResults
) {
  auto score = boggler->Score(board.board.c_str());
  assert(score >= 0);
  InsertIntoMasterList(MasterResults, {static_cast<unsigned int>(score), board});
  return score;
}

vector<BoardScore> GenerateSingleDeviations(
    const vector<BoardWithCell> &boards,
    Boggler<5, 5> *boggler,
    vector<BoardScore> &MasterResults
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
        auto score = ScoreBoard(temp_board, boggler, MasterResults);
        deviations.push_back(BoardScore(score, temp_board));
      }
    }
  }

  return deviations;
}

vector<BoardScore> RunOneSeed(
    const string &seed_board,
    Boggler<5, 5> *boggler,
    vector<BoardScore> &MasterResults,
    set<string> &AllEvaluatedBoards
) {
  auto seed_variations =
      GenerateSingleDeviations({{seed_board, -1}}, boggler, MasterResults);

  auto evaluate_list = BuildEvalList(seed_variations, AllEvaluatedBoards);

  // This Loop Represents the rounds cascade.
  for (int T = 0; T < ROUNDS; T++) {
    // Even if nothing qualifies for the master list on this round, print out
    // the best result for the round to keep track of the progress.
    PrintBestBoard(T, evaluate_list);
    printf("\nThe Top 10 Off The Master List After Round |%d|.\n", T);
    PrintBoardList(MasterResults, 10);

    // Deviate the top boards and mark them as "evaluated."
    auto to_evaluate = evaluate_list | views::take(BOARDS_PER_ROUND);

    vector<BoardWithCell> sources;
    sources.reserve(BOARDS_PER_ROUND);
    for (const auto &b : to_evaluate) {
      sources.push_back(b.board);
    }
    auto deviations = GenerateSingleDeviations(sources, boggler, MasterResults);
    for (const auto &b : to_evaluate) {
      AllEvaluatedBoards.insert(b.board.board);
    }

    evaluate_list = BuildEvalList(deviations, AllEvaluatedBoards);
  }

  return evaluate_list;
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
  set<string> AllEvaluatedBoards;
  set<string> ChosenSeedBoards;

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

  auto init_score = ScoreBoard({SeedBoard, 0}, boggler, MasterResults);

  printf(
      "This is the original seed board that will be used...  It is worth "
      "|%d| points.  Sleep for 2 seconds to look at it\n\n",
      init_score
  );
  PrintBoard(SeedBoard);

  // This loop represents the chain seeds cascade.
  for (int S = 0; S < NUMBER_OF_SEEDS_TO_RUN; S++) {
    BoardScore seed_score(-1, {"", 0});
    for (const auto &result : MasterResults) {
      if (ChosenSeedBoards.find(result.board.board) == ChosenSeedBoards.end()) {
        seed_score = result;
        break;
      }
    }
    assert(seed_score.score >= 0);
    auto seed = seed_score.board;
    ChosenSeedBoards.insert(seed.board);

    printf(
        "For the |%d|'th run the seed board is |%s| worth |%d| points.\n",
        S + 1,
        seed.board.c_str(),
        seed_score.score
    );

    auto TopEvaluationBoardList =
        RunOneSeed(seed.board, boggler, MasterResults, AllEvaluatedBoards);

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
    printf("|%s|\n", board.c_str());
  }

  return 0;
}

// This is a deterministic way to find the top 10 Boggle boards beyond a
// reasonable doubt.  This is one solution that works.  That is all.

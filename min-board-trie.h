#ifndef MIN_BOARD_TRIE_H
#define MIN_BOARD_TRIE_H

#include "const.h"

struct minboardtnode {
	struct minboardtnode* Next;
	struct minboardtnode* Child;
	char Letter;
};

typedef struct minboardtnode MinBoardTnode;
typedef MinBoardTnode* MinBoardTnodePtr;

#define MIN_BOARD_TNODE_CHILD(thisminboardtnode) ((thisminboardtnode)->Child)

struct minboardtrie {
	MinBoardTnodePtr First;
	unsigned int NumberOfBoards;
};

typedef struct minboardtrie MinBoardTrie;
typedef MinBoardTrie* MinBoardTriePtr;

unsigned int MinBoardTrieSize(MinBoardTriePtr ThisMinBoardTrie);
Bool MinBoardTrieBoardSearch(MinBoardTriePtr ThisMinBoardTrie, char * QuanderBoard);
unsigned int MinBoardTrieAddBoard(MinBoardTriePtr ThisMinBoardTrie, const char * NewBoard);
void MinBoardTrieOutput(MinBoardTriePtr ThisMinBoardTrie);
void FreeMinBoardTrie(MinBoardTriePtr ThisMinBoardTrie);
MinBoardTriePtr MinBoardTrieInit(void);

#endif //  MIN_BOARD_TRIE_H
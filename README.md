# JohnPaul Adamovsky's Boggle

Some time in late 2008 or early 2009, JohnPaul Adamovsky [claimed to have solved 5x5 BoggleMax][deep], i.e. he found the 5x5 Boggle board with the highest possible score for a given dictionary. This claim was accompanied by ~1500 lines of dense C code, some data files and several blog posts written in JPA's inimitable, brash style.

I spent a silly amount of time in 2025 [solving 4x4 BoggleMax][44]. Did JPA solve a much harder problem 15 years earlier?

In a word, "no." He didn't _prove_ anything. But I strongly suspect that his top ten list is accurate. Despite [significant effort], I have yet to find a higher scoring board than any of his.

Beyond the results, are there any interesting ideas in his code? There's at least one, but to see it we have to wade through lots of cruft. JPA is a clever man, but he's not trained as a software developer, and so his code is a mixture of good ideas and reinventing the wheel.

This repo reworks his code to separate the wheat from the chaff.

## The Claim

> The 10 Best Dense 5x5 Boggle Boards Beyond Reasonable Doubt

> The Three Demonstration DeepSearch.c's That Are Not Quite An Arcane Academic Proof...  But Are Well Beyond Good Enough

> One fact needs to be made very clear right now.  Even though DeepSearch.c considers only a fraction of the TWL06 lexicon, the global top ten list posted above is unequivocally equal to top ten analysis results for the entire TWL06 lexicon.  If DeepSearch.c was attempting to isolate the best 10,000 boards, only then, would the lexicon shrink alter the results.  The statement:  'Q' will never appear anywhere near the Boggle board top ten list for TWL06, is closer to a keen algorithm decision, than a modification of the lexicon being considered.

This is definitely false! The 23rd best board contains a B.

## The Five Parts

Adamovsky wrote [five blog posts][series], each describing part of his Boggle algorithm.

- [Streamlined Binary Insertion Sort][binary]: By restricting his list to very specific sizes (66, 130, 258, ...) JPA is able to avoid rounding in binary search. This is pretty silly. I replaced this with standard STL algorithms and saw zero performance impact.
- [The Adamovsky Direct Tracking Directed Acyclic Word Graph][adtdawg] aka ADTDAWG: This immodestly-named structure is the one interesting thing about JPA's Boggle solver so we'll come back to it.
- [GunsOfNavarone][guns]: This is JPA's Boggle solver. The name is a reference to a [1961 film]. This is kind of a mess. If its complex optimizations were a win in 2008, they most certainly are not in 2025.
- [Multi-Core Parallel Batch Processing][parallel]: I think this is just a thread pool? For me, one of the main lessons of [MapReduce] was that it's much easier to think about high-level, coarse parallelism than fine-grained parallelism.
- [DeepSearch][deep]: This is JPA's hill climbing algorithm. I'll describe exactly how it works but, at the end of the day, there's nothing interesting or novel about it.

## DeepSearch.c

> Traditional hill climbing has extremely limited scope, because the shortest distance to go up, one step at a time, is almost never the best way to the apex.
> ...
> Thus, the highest level algorithm involved in DeepSearch.c has been branded, "The Intelligent Locust Swarm" method (ILS).  Every algorithmic choice from this point forward, will concentrate on refining the swarm's raw intelligence.  At the end of this optimization exercise, the locusts will not require the burden of free will to isolate the apex.  They will always find it using the cold methods of a deterministic automaton; ruthlessly devouring the landscape in every direction.

What is JPA's "Intelligent Locust Swarm?" How does it work?

- Start with a single board. Score this and add it to the "master list." (Whenever you score a board and its score is high enough, add it to the master list.)
- Repeat 1000x (or as long as you like):
  - Take the highest-scoring board off the master list that hasn't been usd as a seed yet. This is the new seed.
  - Generate all "single deviations" of this board, i.e. replace each cell with all other possible letters. There are `5*5 * 13 = 325` of these. (JPA uses a 14 letter alphabet.) For each of these, note which cell was changed.
  - Take the top 66 of these that haven't previously been evaluated to form the "eval list."
  - Repeat 25x:
    - From the eval list, take the top 64 boards.
    - Mark these boards as "evaluated."
    - Generate single deviations of each of these, ignoring the "off-limit cell" (the one that was last deviated). There will be up to `64*24*13=19968` of these.
    - Set the next eval list to the top 66 of that haven't been previously evaluated.

Notes:

- This is broadly similar to an Evolutionary Strategy.
- JPA's concern about hill climbing going one step at a time is ameliorated by using a larger pool. (I used 1,000 or 2,000, rather than his 66.)
- JPA seeks fine-grained parallelism to speed up board scoring. If I were to add this back, I'd go for more coarse-level parallelism (at the level of `RunOneSeed`).
- I suspect his "off limits cell" optimization is a complete waste.

Summary of changes:

- Changed from C to C++.
- Replaced custom MinTrie structure with a `std::vector` (no perf diff).
- Replaced his GunsOfNavaronne Boggle scorer with mine (~30% speedup).
- Removed all threading code (more on this below).
- Renamed many variables.
- Factored out `RunOneSeed` and `GenerateSingleDeviations` functions.
- Replaced board strings with trailing numbers with a `BoardWithCell` struct.
- Replaced all macros with functions or direct struct access.
- Replaced most arrays with hard-coded lengths with `std::vector`.

To check for diffs:

    ./deepsearch > output.txt

(takes ~2 minutes to run)

## GunsOfNavarone.c

This is JPA's Boggle Solver. (The name is a reference to a [1961 film].)

Outside of his dictionary data structure, there are two notable choices he makes:

- He uses a `struct` to represent the cells of the board. Each square has a neighbor count and an array of pointers to its neighbors. As we'll see shortly, this is a performance disaster!
- To traverse the board, he uses an explicit stack rather than recursion. He says this was inspired by qsort and that the "resulting speed up is noticeable." I changed his explicit stack back to recursion and got a 7% speedup so, if this optimization was ever a win, it's not any more.

About the `Square` structure. Here's what a board looks like:

```c++
#define MAX_ROW 5
#define MAX_COL 5
#define NEIGHBOURS 8
struct Square {
  bool used;
  uint32_t letter_idx;
  uint32_t num_neighbors;
  Square *neighbors[NEIGHBOURS];
};
struct Board {
  Square Block[MAX_ROW][MAX_COL];
};
```

`letter_idx` and `used` vary from board to board, while `num_neighbors` and `neighbors` are fixed. Here's the recursive code that uses that structure in scoring a board:

```c++
Square **neighbors = square->neighbors;

// Loop through all neighbors
for (int i = 0; i < square->num_neighbors; i++) {
  auto n = neighbors[i];
  if (n->used) {
    continue;
  }
  auto letter_idx = n->letter_idx;
  if (we_should_continue) {
    // ...
    score += ScoreSquare(...);  // recursive call
  }
}
```

This is in the critical path of the Boggle solver, the hottest of the hot loops. Loops, branches and data dependencies are bad, and this one turns out to be quite consequential.

The board's adjacency graph is fixed, it does not vary from board to board. So we can eliminate this loop by hard-coding it. My Boggle solver uses a `switch` and some macros to pull this off:

```c++
void Boggler::DoDFS(unsigned int i, unsigned int len, CompactNode* t) {
  PREFIX();
  switch(i) {
    case 0: REC3(1, 5, 6); break;
    case 1: REC5(0, 2, 5, 6, 7); break;
    case 2: REC5(1, 3, 6, 7, 8); break;
    case 3: REC5(2, 4, 7, 8, 9); break;
    case 4: REC3(3, 8, 9); break;
    case 5: REC5(0, 1, 6, 10, 11); break;
    case 6: REC8(0, 1, 2, 5, 7, 10, 11, 12); break;
    case 7: REC8(1, 2, 3, 6, 8, 11, 12, 13); break;
    // ...
    case 22: REC5(16, 17, 18, 21, 23); break;
    case 23: REC5(17, 18, 19, 22, 24); break;
    case 24: REC3(18, 19, 23); break;
  }
  SUFFIX();
}
```

Cell 0 is a corner and has three neighbors (1, 5 and 6). Cell 6 is in the middle and has eight neighbors. This can all be hard-coded. This expands to much more code (~5,000 lines of assembly) but it results in a 30% speedup. This is an order of magnitude larger than any speedup we'll achieve by fiddling with the data structures.

To check for diffs:

    ./gunsofnavarone random10k.txt > guns.snapshot.txt

(takes ~0.5s to run)

## ADTDAWG

JPA's Boggle solver uses a novel data structure he invented and named after himself, the Adamovsky Direct Tracking Directed Acyclic Word Graph or ADTDAWG. This structure is genuinely clever and, so far as I can tell, completely novel. You won't understand it from reading JPA's documentation or code, so I want to try and explain it. We'll get to the ADTDAWG by way of a few simpler data structures: a Trie, a DAWG and then the ADTDAWG.

A Trie uses less memory than a list of words because it shares common prefixes. A DAWG uses less memory than a Trie by also sharing common suffixes. But this comes at a cost: it becomes more expensive to track which word you're looking at as you traverse the graph. The ADTDAWG represents a middle ground: it uses space somewhere between a Trie and a DAWG, but it's able to track words as efficiently as a Trie.

### An inefficient Trie

image of a Trie

Here's a simple Trie structure in C++:

```c++
struct Trie {
  bool is_word_;       // does this represent a complete word?
  Trie* children_[26]; // letter -> child node, or null
  uintptr_t mark_;     // used to track whether we've found a word
};
```

On a 64-bit system, this uses 220 bytes per Trie node. Most of this is in the `children_` array. We can define some methods on this struct:

```c++
struct Trie {
  bool is_word_;       // does this represent a complete word?
  Trie* children_[26]; // letter -> child node, or null
  uintptr_t mark_;     // used to track whether we've found a word

  bool StartsWord(int i) const { return children_[i]; }
  Trie* Descend(int i) const { return children_[i]; }

  bool IsWord() const { return is_word_; }

  void Mark(uintptr_t m) { mark_ = m; }
  uintptr_t Mark() const { return mark_; }
};
```

This is a completely workable Trie structure, and it's the one I used for my BoggleMax project. Here are some stats on how it performs:

- Full TWL06 wordlist: 87 MB RAM, 93,000 random boards/sec, 7800 good boards/sec.
- JPA14 TWL06 wordlist: 20.5MB RAM, 31,000 random boards/sec, 8300 good boards/sec.

On a modern computer, 870MB isn't that much. But this structure is grossly inefficient: most nodes have only zero or one children, so the vast majority of the 26 child pointers will be `null`. If we could shrink the Trie, this might improve cache coherence and thus increase speed.

### Popcount Trie

Our next Trie looks a little different:

```c++
struct Trie {
  uint32_t is_word; : 1;
  uint32_t child_mask_ : 31;
  uint16_t first_child_offset_;
  uint16_t mark_;

  bool StartsWord(int i) const { return (1 << i) & child_mask_; }

  // Requires: StartsWord(i)
  Trie* Descend(int i) const {
    auto index = std::popcount(child_mask_ & ((1 << i) - 1));
    Trie* base = (Trie*)this + children_;
    return base + index;
  }

  bool IsWord() const { return is_word_; }
  // Mark() as before
};
```

This uses only 8 bytes per node. Much less than 220! There are a few things to note here:

- We store which letters lead to children using a bit mask. We only need 26 bits to do this.
- We only out the offset from the parent node to its first child. The children are assumed to be contiguous. We can ensure this by traversing the inefficient Trie with a BFS.
- We use `std::popcount` to go from letter to child index. This is fast on all platforms, and it's even a hardware operation on x86.
- We only get 16 bits for the mark. To make this work, we have to clear the marks every 65536 Boggle boards. This is a trivial cost.

Here are the stats with this new, more space-efficient Trie:

- Full TWL06: 3.0MB RAM, 113,000 bds/sec random, 8,800 bds/sec good.
- JPA14 TWL06: 725KB RAM, 34,600 bds/sec random, 9,000 bds/sec good.

This is a 30x decrease in RAM and, depending on which types of boards you're scoring, a 10-20% speedup. Pretty good!

### A Pure DAWG

Shrinking the data structure gave us a meaningful speed boost. Can we get it even smaller? One standard way to shrink a Trie is to convert it into a DAWG by sharing suffixes in addition to prefixes. This turns the tree into a DAG (Directed Acyclic Graph):

... image ...

Our data structure doesn't need to change that much to represent this:

```c++
struct Dawg {
  uint32_t is_word; : 1;
  uint32_t child_mask_ : 31;
  int32_t child_ofsets_[];

  bool StartsWord(int i) const { return (1 << i) & child_mask_; }
  Dawg* Descend(int i) const {
    auto index = std::popcount(child_mask_ & ((1 << i) - 1));
    return this + child_offsets_[index];
  }
  bool IsWord() const { return is_word_; }
}
```

We've given up on contiguous children in favor of storing the offsets to all children. This makes the Dawg node variable-sized, but that can be handled by whatever process generates it. Because each node can represent many different words (depending on the path you took to it), it no longer makes sense to mark the node to indicate that we've found a word.

This is a problem: it means we can't calculate a board's score! We'll address this soon, but for now we can calculate the "multiboggle score," which allows the same word to be found multiple times. Since we're not doing any bookkeeping, we expect this to be faster than any DAWG-based Boggle algorithm.

- Full TWL06: 711KB RAM, 111,600 bds/sec random, 9,040 bds/sec good.
- JPA14 TWL06: 186KB RAM, 33,600 bds/sec random, 9,270 bds/sec good.

Despite using 4x less RAM, this is marginally slower than the popcount Trie on random boards and only ~3% faster on good boards. And we're not even calculating the right score!

Still, let's not let evidence get in the way of a good idea. We want that space savings! How can we adapt the DAWG to track which words we've found?

### A Counting DAWG

The trick is to store a count of the number of words under each node. Then, as we descend the DAWG, we add the number of words in the subtrees to our left to get the current word's index.

Here's the new structure:

```c++
struct CompactNode {
  uint32_t child_mask_;
  uint32_t words_under_;
  int32_t children[];  // Child indices

  // StartsWord/IsWord as before

  pair<CompactNode *, uint32_t> Descend(int i) {
    uint32_t letter_bit = 1u << i;
    if (!(child_mask_ & letter_bit)) {
      return {nullptr, 0};
    }
    uint32_t mask_before = child_mask_ & (letter_bit - 1);
    int child_index = std::popcount(mask_before);
    uint32_t word_id = 0;
    for (int idx = 0; idx < child_index; idx++) {
      auto child_offset = children[idx];
      auto child = (CompactNode *)((uint32_t *)this + child_offset);
      word_id += child->words_under_;
    }
    auto child_offset = children[child_index];
    auto child = (CompactNode *)((uint32_t *)this + child_offset);
    return {child, word_id};
  };
};
```

As we descend through the DAG, we need to keep track of a word ID, so `Descend` returns a `pair`. The Boggler needs to keep an auxiliary array of marks for each word, to avoid finding them twice. We can use 16 bits for these.

This `Descend` method is considerably more complex than the previous ones, and it has a loop. Unsurprisingly, this tanks performance:

- Full TWL06: 1.24MB RAM, 70,000 random bds/sec, 5,600 good bds/sec
- JPA TWL06: 330KB RAM, 23,000 random bds/sec, 6,150 good bds/sec

For good boards, this is roughly a 33% slowdown compared to the pure DAWG. On the plus side, though, we're getting correct scores and still using less than half the memory of the Popcount Trie.

### A Tracking DAWG



[deep]: https://pages.pathcom.com/~vadco/deep.html
[44]: https://www.danvk.org/2025/08/25/boggle-roundup.html
[significant effort]: https://github.com/danvk/hybrid-boggle/issues/183#issuecomment-3402245058
[series]: https://pages.pathcom.com/~vadco/deep.html#series
[binary]: https://pages.pathcom.com/~vadco/binary.html
[adtdawg]: https://pages.pathcom.com/~vadco/adtdawg.html
[guns]: https://pages.pathcom.com/~vadco/guns.html
[1961 film]: https://en.wikipedia.org/wiki/The_Guns_of_Navarone_(film)
[parallel]: https://pages.pathcom.com/~vadco/parallel.html

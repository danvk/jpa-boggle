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

To check for diffs:

    ./gunsofnavarone random10k.txt > guns.snapshot.txt

(takes ~0.5s to run)

[deep]: https://pages.pathcom.com/~vadco/deep.html
[44]: https://www.danvk.org/2025/08/25/boggle-roundup.html
[significant effort]: https://github.com/danvk/hybrid-boggle/issues/183#issuecomment-3402245058
[series]: https://pages.pathcom.com/~vadco/deep.html#series
[binary]: https://pages.pathcom.com/~vadco/binary.html
[adtdawg]: https://pages.pathcom.com/~vadco/adtdawg.html
[guns]: https://pages.pathcom.com/~vadco/guns.html
[1961 film]: https://en.wikipedia.org/wiki/The_Guns_of_Navarone_(film)
[parallel]: https://pages.pathcom.com/~vadco/parallel.html

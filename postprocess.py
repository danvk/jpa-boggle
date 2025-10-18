#!/usr/bin/env python

import re
import sys

def normalize(lines: list[str]):
    # First the easy stuff.
    # The "19" in "|DGRSMOEAOLSTNIDEISESRPART19|" is nondeterministic.
    # Remove times, too.
    for i, line in enumerate(lines):
        line = re.sub(r'\|([A-Z]{25})\d\d\|', r'|\1|', line)
        line = re.sub(r'\d\d-\d\d-2025 \d\d:\d\d:\d\d', '', line)
        lines[i] = line

    # In a sequence like this, the order is nondeterministic:
    #   9 -| 1854|-|AGRIMORAOLSTECENISMNGPART|
    #  10 -| 1854|-|AGRIMODAOLSTECEEISRNGPART|
    # Remove the rank prefix from each of these sections and sort by score then board (alpha)

    # Find sections that start with "#   1 " and process the ranked lists
    i = 0
    while i < len(lines):
        if lines[i].startswith('#   1 '):
            # Found start of a ranked list, collect all entries
            section_start = i
            entries = []

            # Collect all ranked entries (lines starting with "#")
            while i < len(lines) and lines[i].startswith('#'):
                line = lines[i]
                # Parse: "#   1 -| 1590|-|AGRIMODAOLSTECETISRNGPART|"
                match = re.match(r'#\s+\d+\s+-\|\s*(\d+)\|-\|([^|]+)\|', line)
                if match:
                    score = int(match.group(1))
                    board = match.group(2)
                    entries.append((score, board))
                i += 1

            # Sort by score (descending), then board (alphabetically)
            entries.sort(key=lambda x: (-x[0], x[1]))

            sys.stderr.write(f'Found section of {len(entries)} boards\n')

            # Rewrite the section with new ranks
            for rank, (score, board) in enumerate(entries, start=1):
                lines[section_start + rank - 1] = f'# {rank:3d} -| {score}|-|{board}|'
        else:
            i += 1

    return lines


for inpath in sys.argv[1:]:
    outpath = inpath.replace('.txt', '.norm.txt')
    assert inpath != outpath
    lines = [x.strip() for x in open(inpath)]
    normlines = normalize(lines)
    with open(outpath, 'w') as out:
        out.write('\n'.join(normlines))
        out.write('\n')

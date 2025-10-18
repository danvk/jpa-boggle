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

    return lines


for inpath in sys.argv[1:]:
    outpath = inpath.replace('.txt', '.norm.txt')
    assert inpath != outpath
    lines = [x.strip() for x in open(inpath)]
    normlines = normalize(lines)
    with open(outpath, 'w') as out:
        out.write('\n'.join(normlines))
        out.write('\n')

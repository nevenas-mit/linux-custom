#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Read a Zstd-compressed trace file (written by write_dump_file) and print the first N lines.
Optionally parse each line into named fields. Also computes how many distinct
first-K most-significant hex digits appear in the PMD field across the entire file.

Install dependency (once):
    pip install zstandard
"""

import argparse
import io
import sys
from collections import Counter

try:
    import zstandard as zstd
except ImportError:
    sys.stderr.write("Missing dependency: zstandard. Install with: pip install zstandard\n")
    raise

FIELDS = [
    "cycle", "ops", "va", "pa", "page_size",
    "pgd", "pud", "pmd", "pte", "entry",
    "cont_pgd", "cont_pud", "cont_pmd", "cont_pte",
]
PMD_IDX = FIELDS.index("pmd")

def parse_line_to_dict(line: str):
    parts = line.strip().split()
    if len(parts) != len(FIELDS):
        raise ValueError(f"Expected {len(FIELDS)} fields, got {len(parts)}: {line!r}")
    d = dict(zip(FIELDS, parts))
    d["cycle"] = int(d["cycle"])
    d["ops"] = int(d["ops"])
    d["page_size"] = int(d["page_size"])
    for k in ["cont_pgd", "cont_pud", "cont_pmd", "cont_pte"]:
        d[k] = int(d[k])
    for k in ["va", "pa", "pgd", "pud", "pmd", "pte", "entry"]:
        d[k] = int(d[k], 16)  # %lx format (hex without 0x)
    return d

def stream_first_lines_zstd(path: str, n: int, parse: bool):
    with open(path, "rb") as f:
        dctx = zstd.ZstdDecompressor()
        with dctx.stream_reader(f) as reader:
            text_stream = io.TextIOWrapper(io.BufferedReader(reader), encoding="utf-8", errors="strict")
            for i, line in enumerate(text_stream):
                if i >= n:
                    break
                if parse:
                    rec = parse_line_to_dict(line)
                    print(rec)
                else:
                    print(line.rstrip("\n"))

def count_pmd_prefixes(path: str, k: int = 3):
    """
    Scan the entire file and count distinct first-k hex digits (most significant) of PMD values.
    - Strips leading zeros (so 000abc... -> abc...)
    - If a value is 0 (or all zeros), its normalized string becomes '0'
    - Prefix is the first k characters of the normalized hex string
    Returns (unique_count, Counter(prefix -> freq))
    """
    counts = Counter()
    with open(path, "rb") as f:
        dctx = zstd.ZstdDecompressor()
        with dctx.stream_reader(f) as reader:
            text_stream = io.TextIOWrapper(io.BufferedReader(reader), encoding="utf-8", errors="strict")
            for line in text_stream:
                parts = line.strip().split()
                if len(parts) != len(FIELDS):
                    # Skip malformed lines quietly; you can raise if you prefer strictness
                    continue
                raw = parts[PMD_IDX].lower()
                # Accept both hex with/without 0x (though %lx usually has no 0x)
                if raw.startswith("0x"):
                    raw = raw[2:]
                # Normalize: drop leading zeros; if empty -> '0'
                norm = raw.lstrip("0")
                if norm == "":
                    norm = "0"
                prefix = norm[:k]
                counts[prefix] += 1
    return len(counts), counts

def main():
    ap = argparse.ArgumentParser(description="Print first lines from a Zstd-compressed trace file and PMD prefix stats.")
    ap.add_argument("file", help="Path to .zst (or Zstd-compressed) trace file")
    ap.add_argument("--n", type=int, default=10, help="Number of lines to print (default: 10)")
    ap.add_argument("--parse", action="store_true", help="Parse fields into a dict before printing")
    ap.add_argument("--prefix-len", type=int, default=3, help="How many most-significant hex digits to consider for PMD (default: 3)")
    ap.add_argument("--no-stats", action="store_true", help="Skip PMD prefix stats")
    args = ap.parse_args()

    # Print first N lines
    stream_first_lines_zstd(args.file, args.n, args.parse)

    # PMD prefix stats across the entire file
    if not args.no_stats:
        unique_count, counts = count_pmd_prefixes(args.file, args.prefix_len)
        print("\n=== PMD prefix stats ===")
        print(f"Distinct first-{args.prefix_len}-digit prefixes: {unique_count}")
        # Show top prefixes by frequency (descending)
        for pref, freq in counts.most_common(20):
            print(f"{pref} : {freq}")

if __name__ == "__main__":
    main()

'''
BFS
=== PMD prefix stats ===
Distinct first-2-digit prefixes: 4
48 : 64591472 -->9 2MB (98.65%)
2a : 686473 --> 5 128KB (1.05%)
36 : 173773 -->6 256KB (0.27%)
43 : 22283 --> 8 1MB (0.03%)
total = 65474001

DFS
=== PMD prefix stats ===
Distinct first-2-digit prefixes: 4
48 : 62644430 --> 98.63%
2a : 672142 --> 1.06%
36 : 171346 --> 0.27%
43 : 22130 --> 0.04%
total = 63510048

CC
=== PMD prefix stats ===
Distinct first-2-digit prefixes: 4
48 : 53481045 --> 98.42%
2a : 663020 --> 1.22%
36 : 170369 --> 0.31%
43 : 22562 --> 0.05%
total = 54336996

DC
=== PMD prefix stats ===
Distinct first-2-digit prefixes: 4
48 : 38866455 --> 99.75%
2a : 86039 --> 0.22%
36 : 12052 --> 0.03%
43 : 1196 --> 0.00%
total = 38965742

PageRank
=== PMD prefix stats ===
Distinct first-2-digit prefixes: 4
48 : 69217020 --> 99.66%
2a : 201208 --> 0.29%
36 : 32448 --> 0.05%
43 : 2285 --> 0.00%
total = 69452961


'''
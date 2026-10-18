#!/usr/bin/env python3
"""Differential-test corpus - thin entry point.

The corpus lives in corpus_pkg/: cases are grouped by THEME
(cases_alu.py, cases_loops.py, ...), bank membership and order live in
banks.py (banks are bootable-image packing units), and the generator is
gen.py. Authoring rules: corpus_pkg/registry.py. Sealed banks are
golden-bound: the generator refuses to change their images.

Usage:
  corpus.py [outdir]      generate all banks (default outdir: out/)
  corpus.py --status      bank/seal overview
  corpus.py --seal bN..   record goldens fingerprints for banks
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from corpus_pkg.gen import main

if __name__ == "__main__":
    main()

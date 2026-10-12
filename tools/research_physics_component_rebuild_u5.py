#!/usr/bin/env python3
"""Pair complete component dispatch/ordinary callers and required lowers."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as source

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    source.OWNERS=(0x1d2b10,0x1d9090)
    source.BOUNDARIES=(0x1da570,0x1d90e0,0x1da690,0x1d9330,0x1c8430)
    source.run(a.output.resolve())

if __name__=='__main__':main()

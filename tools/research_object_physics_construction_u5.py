#!/usr/bin/env python3
"""Pair complete object physical-construction wrappers and required calls."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as source

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    source.OWNERS=(0x1c7050,0x1c6f50,0x1c72e0,0xbbe70,0xc0d10)
    source.BOUNDARIES=(0x148360,0x27f190,0x1d2aa0,0x1d2b10,0x1d4f30)
    source.run(a.output.resolve())

if __name__=='__main__':main()

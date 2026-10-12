#!/usr/bin/env python3
"""Complete literal constructor-tail correspondence; no annotation authority."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as original
original.OWNERS=(0x110a20,0x116680,0x110bf0,0x110800,0x1100e0,0x10f380,0x115030,0x16f510,0x1e0aa0,0x114f70)
original.BOUNDARIES=(0x1e13c0,0x114f00,0x1cfa90,0xbb240,0x114bd0,0x1d0a60)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();original.run(a.output)

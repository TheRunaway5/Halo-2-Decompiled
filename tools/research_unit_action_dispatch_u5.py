#!/usr/bin/env python3
"""Complete raw allocation/dispatch owner correspondence in authenticated u5."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as original
original.OWNERS=(0xbd170,0xb8250,0xe7f30,0xe7ef0,0xe7ff0,0xe7e60,0x1eb150,0x1ea310)
original.BOUNDARIES=()
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();original.run(a.output)

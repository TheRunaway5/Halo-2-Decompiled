#!/usr/bin/env python3
"""Independent complete original request/resource body correspondence."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as original
# Change only the bounded source-entry inventory of this independent reader.
# The frozen original reader/source/image inputs are never modified.
original.OWNERS=(0x1cfde0,0x1deb60,0x1e13c0,0x1e1230,0x1de9b0,0x124e40,0x124dd0,0x125120,0x1248d0,0x124af0)
original.BOUNDARIES=()
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();original.run(a.output)

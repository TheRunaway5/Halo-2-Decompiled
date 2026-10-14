#!/usr/bin/env python3
"""Static complete-body TU1.1/TU1.5 weapon clear, detach and feedback mapping.

Operand/branch correspondence for these owners is retained. Callees outside
this set remain explicit; no complete TU1.5 world or framebuffer is asserted.
"""
import argparse,json
from pathlib import Path
import research_weapon_effect_u5 as correspondence

OWNERS=(0xe9a90,0x105750,0x1042e0,0x108230,0xcfed0,0xcf8c0,
        0xfb120,0xfb3f0,0xfb490,0xd2940,0x107120)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 correspondence.OWNERS=OWNERS;correspondence.BOUNDARIES=();correspondence.run(a.output)
 path=a.output/'report.json';report=json.loads(path.read_text());report.update(scope=__doc__,
   schema='source-weapon-unequip-tu15-correspondence-v1',
   correspondence_tool_sha256=report['tool_sha256'],tool_sha256=correspondence.sha(__file__))
 path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()

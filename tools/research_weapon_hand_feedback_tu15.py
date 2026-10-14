#!/usr/bin/env python3
"""Complete-body original registration, unit hand feedback and action mapping.

Static TU1.1/TU1.5 operand/branch correspondence. Runtime unit/action/world
composition and rendered parity are not established by this report.
"""
import argparse,json
from pathlib import Path
import research_weapon_effect_u5 as correspondence
OWNERS=(0x167d32,0x10a140,0xefcc0,0xd2940,0x1680b3,0xc6c50)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 correspondence.OWNERS=OWNERS;correspondence.BOUNDARIES=();correspondence.run(a.output)
 path=a.output/'report.json';report=json.loads(path.read_text());report.update(scope=__doc__,schema='source-weapon-hand-feedback-tu15-correspondence-v1',correspondence_tool_sha256=report['tool_sha256'],tool_sha256=correspondence.sha(__file__))
 path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

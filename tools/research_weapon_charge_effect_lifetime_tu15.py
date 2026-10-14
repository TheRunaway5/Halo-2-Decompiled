#!/usr/bin/env python3
"""Complete original charge-effect stop, beam cleanup and retirement correspondence.

TU1.1/TU1.5 static complete-body/call/operand evidence identifies the actual
owners required when the charging trigger retains a positive generic effect
handle. This does not supply effect allocation, pool storage, event/particle
lifetime, live service results or rendered parity.
"""
import argparse,json
from pathlib import Path
import research_weapon_effect_u5 as correspondence
OWNERS=(0x178cd0,0x178c10,0x179080)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 correspondence.OWNERS=OWNERS;correspondence.BOUNDARIES=();correspondence.run(a.output)
 path=a.output/'report.json';report=json.loads(path.read_text());report.update(scope=__doc__,schema='source-weapon-charge-effect-lifetime-tu15-correspondence-v1',correspondence_tool_sha256=report['tool_sha256'],tool_sha256=correspondence.sha(__file__))
 path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

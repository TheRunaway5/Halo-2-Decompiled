#!/usr/bin/env python3
"""Complete original weapon update/input and charge/lock trigger owner mapping.

Static complete-body correspondence between unchanged TU1.1 and TU1.5 images.
This does not execute the owners, acquire a target or establish native parity.
Callees outside the declared owner set retain separate unverified ownership.
"""
import argparse,json
from pathlib import Path
import research_weapon_effect_u5 as correspondence
OWNERS=(0x1021d0,0x1045e0,0x1066f0,0x106960,0x106c30,0x106d40,
        0x106e10,0x106e70,0x106f10,0x106fc0,0x1070c0,0x107120,
        0x107c40,0x107cc0,0x107d20,0x107d80,0x107df0,0x107e90,0x107f80)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 correspondence.OWNERS=OWNERS;correspondence.BOUNDARIES=();correspondence.run(a.output)
 path=a.output/'report.json';report=json.loads(path.read_text())
 differences=[m for owner in report['owners'] for m in owner['memory'] if m['old_hex']!=m['new_hex']]
 assert not differences,'Absolute operand bytes changed; audit these owners before proceeding'
 report.update(scope=__doc__,schema='source-weapon-trigger-tu15-correspondence-v1',
     correspondence_tool_sha256=report['tool_sha256'],tool_sha256=correspondence.sha(__file__),
     total_instruction_rows=sum(owner['rows'] for owner in report['owners']),
     absolute_operand_byte_differences=differences,
     external_callees=sorted({c['old_target'] for owner in report['owners'] for c in owner['calls'] if not c['callee_proven_here']}))
 path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

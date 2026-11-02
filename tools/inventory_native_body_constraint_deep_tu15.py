#!/usr/bin/env python3
"""Retain authenticated remaining constraint-owner listings and their call edges."""
import argparse,hashlib,json
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha
from pathlib import Path
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN;a.output.mkdir(parents=True,exist_ok=False);code=_Code(Xbe(NEW_DEFAULT),'.text');report={'original_sha256':NEW_PIN,'owners':{}}
 for owner in (0x1dd710,0x22def0,0x22e040,0x22e0d0,0x1dd7b0,0x22d0a0,0x22dd90,0x22cea0,0x122b60,0x22ede0,0x22eba0,0x22e1b0):
  body=complete_body(code,owner);text=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code));(a.output/f'{owner:x}.txt').write_text(text);report['owners'][hex(owner)]={'sites':len(body.code),'calls':[{'site':hex(pc),'target':code.at(pc).op_str}for pc in sorted(body.code)if code.at(pc).mnemonic=='call'],'listing_sha256':hashlib.sha256(text.encode()).hexdigest()}
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()

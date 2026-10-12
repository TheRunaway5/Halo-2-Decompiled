#!/usr/bin/env python3
"""Authenticate original constraint schema production before native integration.

Entry identities come from executable virtual slots/caller edges, not names.
This tool is research only; no producer is claimed native or live-integrated.
"""
import hashlib,json,sys
from pathlib import Path
import research_physics_world_step_20261010 as paired
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code

ENTRIES=(0x31bbe0,0x31bd00,0x322380,0x325bb0,0x324d00,0x325b70,0x326820)
if __name__=='__main__':
    output=Path(sys.argv[sys.argv.index('--output')+1])
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');c=_Code(x,'.text')
    evidence=[]
    for constructor,table,producer,callsite,target in (
        (0x31bb98,0x41b320,0x31bbe0,0x31bbf8,0x322380),
        (0x31bcc8,0x41b34c,0x31bd00,0x31bded,0x322380)):
        store=c.at(constructor);call=c.at(callsite)
        assert store.mnemonic=='mov' and store.operands[1].imm==table
        assert int.from_bytes(x.read(table+0x24,4),'little')==producer
        assert call.mnemonic=='call' and call.operands[0].imm==target
        evidence.append(dict(constructor_store=constructor,virtual_table=table,virtual_slot=0x24,
            producer=producer,callsite=callsite,target=target))
    call=c.at(0x32239d);assert call.mnemonic=='call' and call.operands[0].imm==0x325bb0
    paired.ENTRIES=ENTRIES;paired.main()
    p=output/'complete-report.json';r=json.loads(p.read_text())
    r.update(extension_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        virtual_slot_evidence=evidence,flat_producer_callsite=0x32239d,
        native_producers_implemented=False,live_gameplay_proof=False)
    p.write_text(json.dumps(r,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(complete_bodies=r['complete_bodies'],report_sha256=hashlib.sha256(p.read_bytes()).hexdigest())))

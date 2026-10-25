#!/usr/bin/env python3
"""Retain the authenticated visible-light callback and its admission owner."""
import csv,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    parent=_trace(code,0x2c320,starts)
    assert 0x2c32c in parent.code and code.at(0x2c32c).mnemonic=='push' and code.at(0x2c32c).operands[0].imm==0xc5370
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-client-lens'
    out=base/'light-lens-owners';n=2
    while out.exists():out=base/('light-lens-owners-'+str(n));n+=1
    out.mkdir();rows=[]
    for entry in (0x2c320,0xc5370,0x31500,0x166bb0,0x3e9b0):
        body=_trace(code,entry,starts|{0xc5370})
        lines=[];calls=[]
        for pc in sorted(body.code):
            i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
            lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
            if i.mnemonic=='call':calls.append(dict(site=pc,operand=i.op_str))
        p=out/f'{entry:08x}.asm';p.write_text('\n'.join(lines)+'\n')
        rows.append(dict(entry=entry,instructions=len(lines),calls=calls,path=str(p),bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,callback_registration_site=0x2c32c,entries=rows,instructions_or_results_replaced=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()

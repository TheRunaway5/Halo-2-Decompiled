#!/usr/bin/env python3
"""Retain authentic TU1.5 lens caller inputs and initialization boundaries.

Read-only research under a run_test.py owner. No client source world, request,
admission, graphics readiness or GPU completion is supplied.
"""
import csv,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-client-lens/original-owners'
    n=2
    while out.exists():out=out.with_name('original-owners-'+str(n));n+=1
    out.mkdir(parents=True,exist_ok=False);rows=[]
    for entry in (0x3e9b0,0xbcfb0,0x158290,0x3803b0,0x20680,0x20700,0x13660,0x1fb90,0x20e90,0x41970):
        if entry==0x3803b0:assert int.from_bytes(image.read(0x46d818,4),'little')==entry
        else:assert entry in starts
        body=_trace(code,entry,starts|{0x3803b0});lines=[];calls=[]
        for pc in sorted(body.code):
            i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
            lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
            if i.mnemonic=='call':calls.append(dict(site=pc,operand=i.op_str))
        p=out/f'{entry:08x}.asm';p.write_text('\n'.join(lines)+'\n')
        rows.append(dict(entry=entry,instructions=len(lines),calls=calls,path=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,entries=rows,source_modified=False,native_client_lens_repaired=False)
    (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(entries=[(r['entry'],r['instructions']) for r in rows])),flush=True)
if __name__=='__main__':main()

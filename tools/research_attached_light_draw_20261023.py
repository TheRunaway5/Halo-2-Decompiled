#!/usr/bin/env python3
"""Retain authenticated TU1.5 light draw branches and first-person setup."""
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
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-native-attached-light-collection/original-draw-owners'
    out.mkdir(exist_ok=True);rows=[]
    for entry in (0x31c70,0x31150,0x30d80,0x3d2b0,0xbac70,0x1692e0,0x168ded,0x31b30,0x2c470,0x31570):
        if entry not in starts:
            call=code.at(0x3d35a)
            assert entry==0x168ded and call.mnemonic=='call' and call.operands[0].imm==entry
        body=_trace(code,entry,starts|{0x168ded})
        lines=[];calls=[]
        for pc in sorted(body.code):
            i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
            lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
            if i.mnemonic=='call':calls.append(dict(site=pc,operand=i.op_str))
        p=out/f'{entry:08x}.asm';p.write_text('\n'.join(lines)+'\n')
        rows.append(dict(entry=entry,instructions=len(lines),calls=calls,path=str(p),
            bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,source_modified=False,
        native_readiness_or_admission_supplied=False,entries=rows)
    (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(entries=[(r['entry'],r['instructions']) for r in rows])),flush=True)
if __name__=='__main__':main()

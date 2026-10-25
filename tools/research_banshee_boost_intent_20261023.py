#!/usr/bin/env python3
"""Read original secondary-trigger intent before repairing the native command adapter."""
import csv,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    starts={int(row['address'],16) for row in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    body=_trace(code,0x1889d0,starts);pcs=sorted(body.code)
    found=[n for n,pc in enumerate(pcs) if code.at(pc).mnemonic=='or' and any(op.type==2 and op.imm in (0x4800,0x4000) for op in code.at(pc).operands)]
    assert len(found)==1
    selected=pcs[found[0]-12:found[0]+5];lines=[]
    for pc in selected:
        i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
        lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/boost-intent-original.json'
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,listing=lines,source_modified=False,
        native_input_or_boost_result_supplied=False)
    out.write_text(json.dumps(report,indent=2)+'\n');print('\n'.join(lines),flush=True)
if __name__=='__main__':main()

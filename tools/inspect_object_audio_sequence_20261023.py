#!/usr/bin/env python3
"""Retain bounded original CPU listings for the unresolved native loop owner."""
import csv,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');starts={int(row['address'],16) for row in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
    out=base/'original-sequence';attempt=2
    while out.exists():out=base/f'original-sequence-{attempt}';attempt+=1
    out.mkdir(exist_ok=False)
    paired=json.loads((ROOT/'analysis/multiplayer/integration/live-authored-audio-20261010/ambient-correspondence/report.json').read_text())
    assert paired['images'][2]['sha256']==NEW_PIN
    sequence=next(row['images'][1]['entry'] for row in paired['owners'] if row['base']==0x21a250)
    rows=[]
    for entry in (sequence,0x220c60,0x129f10,0x18f900,0x190460):
        assert entry in starts,hex(entry)
        body=_trace(code,entry,starts);calls=[];lines=[]
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
            if i.mnemonic=='call':calls.append(dict(site=pc,target=i.operands[0].imm if i.operands[0].type==2 else None,operand=i.op_str))
        path=out/f'{entry:08x}.asm';path.write_text('\n'.join(lines)+'\n')
        rows.append(dict(entry=entry,instructions=len(lines),calls=calls,path=str(path),bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    report=dict(original_xbe_sha256=NEW_PIN,entries=rows,source_modified=False,playback_verified=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()

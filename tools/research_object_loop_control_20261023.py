#!/usr/bin/env python3
"""Pair complete original loop-control owners before implementing native playback."""
import csv,hashlib,json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from functions import _Code,_trace
from research_mp_original_data_u5 import norm
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    images=[Xbe(ROOT/'assets/disc/default.xbe'),Xbe(NEW_DEFAULT)]
    assert hashlib.sha256(images[0].data).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    assert hashlib.sha256(images[1].data).hexdigest()==NEW_PIN
    codes=[_Code(image,'.text') for image in images]
    starts=[{int(row['address'],16) for row in csv.DictReader((ROOT/rel).open(),delimiter='\t')}
        for rel in ('analysis/retail/functions.tsv','analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')]
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/loop-control-correspondence'
    assert not out.exists();out.mkdir();owners=[]
    for entry in (0x219f80,0x21b940,0x21b0e0,0x21ce60):
        original=_trace(codes[0],entry,starts[0]);body=[codes[0].at(pc) for pc in sorted(original.code)]
        positions={insn.address:n for n,insn in enumerate(body)};normalized=[norm(insn,images[0],positions) for insn in body]
        matches=[]
        for candidate in raw_prefix_candidates(codes[0],images[0],codes[1],entry,12):
            try:
                other=_trace(codes[1],candidate,starts[1]);instructions=[codes[1].at(pc) for pc in sorted(other.code)]
                positions={insn.address:n for n,insn in enumerate(instructions)}
                if [norm(insn,images[1],positions) for insn in instructions]==normalized:matches.append((candidate,instructions))
            except Exception:pass
        assert len(matches)==1,(hex(entry),len(matches))
        selected,instructions=matches[0]
        path=out/f'{selected:08x}.asm';path.write_text(''.join(
            f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
        owners.append(dict(retail_entry=f'{entry:08x}',tu15_entry=f'{selected:08x}',instructions=len(body),
            normalized_complete_equal=True,listing_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
            pair=[dict(old=f'{a.address:08x}',new=f'{b.address:08x}') for a,b in zip(body,instructions)]))
        print(json.dumps({key:value for key,value in owners[-1].items() if key!='pair'}),flush=True)
    (out/'report.json').write_text(json.dumps(dict(scope=__doc__,original_xbe_sha256=NEW_PIN,owners=owners),indent=2)+'\n')
if __name__=='__main__':main()

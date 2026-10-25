#!/usr/bin/env python3
"""Locate literal original loop PCM callback owners before native integration."""
import bisect,csv,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    inventory=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
    starts={int(row['address'],16) for row in csv.DictReader(inventory.open(),delimiter='\t')}
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/pcm-callback-owners'
    attempt=2
    while out.exists():out=out.with_name('pcm-callback-owners-'+str(attempt));attempt+=1
    out.mkdir();tables={};owners={}
    for address in (0x455b68,0x455b88):
        words=struct.unpack('<8I',image.read(address,32));tables[f'{address:08x}']=[f'{v:08x}' for v in words]
        if address==0x455b68:words+=tuple((0x2218c0,0x222ba0,0x222e10,0x220a60,0x220a90,0x2204a0,0x2208a0,0x223670,0x2235d0,0x2235a0,0x2235f0,0x223640,0x2206d0,0x220c60,0x222710,0x222830,0x129900,0x224ec0,0x128970,0x129470,0x222690))
        for entry in words:
            if entry in owners or not entry or not code.at(entry):continue
            body=_trace(code,entry,starts|{entry});path=out/f'{entry:08x}.asm'
            path.write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n' for pc in sorted(body.code)))
            owners[entry]=dict(instructions=len(body.code),bytes=path.stat().st_size,
                sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    callers=[];ordered=sorted(starts)
    for offset in range(len(code.bytes)-4):
        if code.bytes[offset]!=0xe8:continue
        pc=code.lo+offset;target=pc+5+struct.unpack_from('<i',code.bytes,offset+1)[0]
        if target not in (0x2206d0,0x221af0):continue
        entry=ordered[bisect.bisect_right(ordered,pc)-1];body=_trace(code,entry,starts)
        if pc not in body.code:continue
        path=out/f'frame-caller-{entry:08x}.asm'
        path.write_text(''.join(f'{at:08x} {code.at(at).bytes.hex()} {code.at(at).mnemonic} {code.at(at).op_str}\n' for at in sorted(body.code)))
        callers.append(dict(callsite=f'{pc:08x}',target=f'{target:08x}',entry=f'{entry:08x}',
            listing=path.name,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,tables=tables,owners={f'{k:08x}':v for k,v in owners.items()},frame_callers=callers,
        source_modified=False,playback_verified=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()

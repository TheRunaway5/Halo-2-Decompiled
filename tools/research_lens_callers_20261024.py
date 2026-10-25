#!/usr/bin/env python3
"""Retain authenticated original lens-request callers without inventing tag admission."""
import argparse,bisect,csv,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    ap=argparse.ArgumentParser();ap.add_argument('--target',type=lambda x:int(x,0),default=0x2db80);ap.add_argument('--label',default='lens-callers');a=ap.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');starts=sorted(int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t'))
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-client-lens';out=base/a.label;n=2
    while out.exists():out=base/(a.label+'-'+str(n));n+=1
    out.mkdir();calls=[];pointers=[];owners={};unqualified=[]
    for section in image.sections:
        data=image.section_bytes(section)
        for at in range(len(data)-4) if section.flags&4 else ():
            if data[at]!=0xe8 or (section.va+at+5+struct.unpack_from('<i',data,at+1)[0])&0xffffffff!=a.target:continue
            site=section.va+at
            if not code.inside(site):continue
            instruction=code.at(site)
            if instruction.mnemonic!='call' or instruction.op_str!=hex(a.target):continue
            entry=starts[bisect.bisect_right(starts,site)-1]
            body=_trace(code,entry,set(starts))
            if site not in body.code:
                context=[];pc=entry
                while pc<site+80:
                    i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
                    context.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}');pc+=i.size
                path=out/f'unqualified-{site:08x}.asm';path.write_text('\n'.join(context)+'\n')
                unqualified.append(dict(site=site,nearest_entry=entry,context=str(path),reason='Byte candidate is outside the complete nearest canonical CFG; linear context is not execution proof'));continue
            lines=[]
            for pc in sorted(body.code):
                i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
                lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
            path=out/f'{entry:08x}.asm';path.write_text('\n'.join(lines)+'\n')
            owners[entry]=dict(entry=entry,path=str(path),bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            calls.append(dict(site=site,entry=entry,instruction_bytes=bytes(instruction.bytes).hex()))
        cursor=0
        while (at:=data.find(struct.pack('<I',a.target),cursor))>=0:
            row=dict(section=section.name,address=section.va+at);cursor=at+1
            if code.inside(section.va+at):
                entry=starts[bisect.bisect_right(starts,section.va+at)-1];body=_trace(code,entry,set(starts))
                sites=[pc for pc in body.code if any(op.type==2 and op.imm==a.target for op in code.at(pc).operands)]
                if sites:
                    lines=[]
                    for pc in sorted(body.code):
                        i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
                        lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
                    path=out/f'pointer-owner-{entry:08x}.asm';path.write_text('\n'.join(lines)+'\n')
                    row.update(entry=entry,qualified_instruction_sites=sites,owner=str(path),owner_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            pointers.append(row)
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,target=a.target,direct_calls=calls,literal_pointer_references=pointers,
        unqualified_byte_candidates=unqualified,
        owners=list(owners.values()),instructions_or_results_replaced=False,light_tag_reference_alone_proves_rendering=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()

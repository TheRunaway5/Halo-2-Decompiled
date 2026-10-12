#!/usr/bin/env python3
"""Map the actual base HUD CPU bitmap geometry into both owned title updates.

Normalized prefixes locate entries only. Complete original branches, direct
calls and literal image operands are retained independently. Writable initial
bytes do not establish the corresponding runtime initialization.
"""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path

from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN as TU_PIN
from oracle_multiplayer_hud_positions import PIN as BASE_PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, norm, full_tokens, containing

ENTRIES=(0x22aff1,0x22ae21,0x13a690,0x1396c7,0x22bcaf,0x2b460,0x1d6d0,0x22a664)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def h(value):
    return f'{value&0xffffffff:08x}'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert not args.output.exists()
    paths=[ROOT/'assets/disc/default.xbe',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',NEW_DEFAULT]
    assert [sha(p) for p in paths]==[BASE_PIN,TU_PIN,NEW_PIN]
    images=[Xbe(p) for p in paths]
    codes=[_Code(x,'.text') for x in images]
    starts=[{int(r['va'],16) for r in csv.DictReader((ROOT/'tools/reference/halo2-decompiled/config/functions.csv').open())}]
    for path in [ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']:
        starts.append({int(r['address'],16) for r in csv.DictReader(path.open(),delimiter='\t')})
    cache={}
    def body(side,address):
        key=(side,address)
        if key not in cache:
            trace=_trace(codes[side],address,starts[side])
            cache[key]=trace,[codes[side].at(p) for p in sorted(trace.code)]
        return cache[key]
    def prefix(side,address):
        out=[]
        for _ in range(16):
            ins=codes[side].at(address)
            if ins is None:break
            out.append(ins);address+=ins.size
            if ins.mnemonic.startswith('ret'):break
        return out
    pairs=[]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    for side,label in [(1,'disc_1_1'),(2,'u5_1_5')]:
        indexes={n:collections.defaultdict(list) for n in (16,12,8,4)}
        for address in starts[side]:
            instructions=prefix(side,address)
            for n,index in indexes.items():
                if len(instructions)>=n:index[tuple(norm(i,images[side]) for i in instructions[:n])].append(address)
        mapped={};evidence={};candidates={}
        for address in ENTRIES:
            instructions=prefix(0,address);found=[]
            for n,index in indexes.items():
                if len(instructions)<n:continue
                found=index.get(tuple(norm(i,images[0]) for i in instructions[:n]),[])
                if len(found)==1:
                    mapped[address]=found[0];evidence[address]={'kind':'unique_original_prefix','instructions':n};break
            candidates[address]=found
        for address in list(mapped):
            _,before=body(0,address);_,after=body(side,mapped[address])
            if full_tokens(before,images[0])!=full_tokens(after,images[side]):continue
            for left,right in zip(before,after):
                if left.mnemonic!='call' or left.operands[0].type!=X86_OP_IMM or right.operands[0].type!=X86_OP_IMM:continue
                p,q=left.operands[0].imm,right.operands[0].imm
                if not codes[0].inside(p) or not codes[side].inside(q):continue
                if p in mapped:assert mapped[p]==q,(h(address),h(p),h(mapped[p]),h(q))
                else:
                    mapped[p]=q
                    evidence[p]={'kind':'actual_direct_call_in_complete_matching_parent','base_parent':h(address),'base_site':h(left.address),'update_site':h(right.address)}
        rows=[]
        for address in dict.fromkeys((*ENTRIES,*mapped)):
            row={'base_entry':h(address),'primary':address in ENTRIES,'prefix_candidates':[h(v) for v in candidates.get(address,[])]}
            if address not in mapped:
                row['status']='unresolved_entry';rows.append(row);continue
            target=mapped[address]
            bt,before=body(0,address);at,after=body(side,target)
            same=full_tokens(before,images[0])==full_tokens(after,images[side])
            row.update(update_entry=h(target),mapping_evidence=evidence[address],base_rows=len(before),update_rows=len(after),complete_instruction_and_internal_branch_shape_equal=same)
            for tag,entry,instructions in [('base',address,before),(label,target,after)]:
                path=directory/f'{h(address)}-{tag}-{h(entry)}.asm'
                path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
                row[tag+'_listing']={'file':str(path.relative_to(args.output.parent)),'sha256':sha(path)}
            refs=[];calls=[];immediates=[];tables=[]
            if same:
                import struct
                assert len(bt.tables)==len(at.tables)
                positions=[{i.address:n for n,i in enumerate(ins)} for ins in (before,after)]
                for left,right in zip(bt.tables,at.tables):
                    p,width,count=left;q,qwidth,qcount=right
                    assert (width,count)==(qwidth,qcount)
                    b,a=images[0].read(p,width*count),images[side].read(q,width*count)
                    if width==4:
                        values=[struct.unpack('<'+'I'*count,data) for data in (b,a)]
                        targets=[[positions[s].get(v,'ZERO' if not v else 'EXTERNAL') for v in values[s]] for s in (0,1)]
                        equal=targets[0]==targets[1] and 'EXTERNAL' not in targets[0]
                    else:targets=None;equal=b==a
                    tables.append(dict(base_address=h(p),update_address=h(q),width=width,count=count,base_hex=b.hex(),update_hex=a.hex(),mapped_target_instruction_indices=targets,mapped_contents_equal=equal))
                for left,right in zip(before,after):
                    for operand,(x,y) in enumerate(zip(left.operands,right.operands)):
                        if x.type==y.type==X86_OP_MEM and not x.mem.base and not y.mem.base and containing(images[0],x.mem.disp) and containing(images[side],y.mem.disp):
                            assert x.size==y.size
                            b,a=images[0].read(x.mem.disp,x.size),images[side].read(y.mem.disp,y.size)
                            refs.append(dict(base_site=h(left.address),update_site=h(right.address),operand=operand,base_address=h(x.mem.disp),update_address=h(y.mem.disp),width=x.size,base_hex=b.hex(),update_hex=a.hex(),bytes_equal=b==a,base_section=containing(images[0],x.mem.disp).name,update_section=containing(images[side],y.mem.disp).name,indexed=bool(x.mem.index or y.mem.index)))
                        if x.type==y.type==X86_OP_IMM and x.imm!=y.imm and not (left.mnemonic.startswith('j') or left.mnemonic=='call'):
                            immediates.append(dict(base_site=h(left.address),update_site=h(right.address),operand=operand,base_value=h(x.imm),update_value=h(y.imm),both_in_images=bool(containing(images[0],x.imm) and containing(images[side],y.imm))))
                    if left.mnemonic=='call':
                        if left.operands[0].type==right.operands[0].type==X86_OP_IMM:
                            p,q=left.operands[0].imm,right.operands[0].imm
                            calls.append(dict(base_site=h(left.address),update_site=h(right.address),base_target=h(p),update_target=h(q),target_pair_present=mapped.get(p)==q))
                        else:
                            calls.append(dict(base_site=h(left.address),update_site=h(right.address),base_assembly=left.op_str,update_assembly=right.op_str,boundary='runtime indirect target'))
            row.update(paired_image_operands=refs,paired_direct_calls=calls,changed_nonbranch_immediates=immediates,switch_tables=tables,status='complete_static_body_shape_equal' if same else 'changed_or_unresolved_body')
            rows.append(row)
        pairs.append(dict(update=label,xbe_sha256=sha(paths[side]),routines=rows))
    report=dict(schema=1,tool_sha256=sha(__file__),base_xbe_sha256=BASE_PIN,updates=pairs,boundaries=['Complete original static body/branch correspondence is separate from operand data, runtime initializer ownership and indirect/platform callbacks. No title-update instructions execute here.','Only the measured actual bitmap producer CPU geometry is ported. GPU/device/shader completion, local gameplay visibility and final Xbox pixels remain separate.'])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(args.output),updates=[dict(update=p['update'],pairs=len(p['routines']),primary_unresolved=[r['base_entry'] for r in p['routines'] if r['primary'] and not r.get('complete_instruction_and_internal_branch_shape_equal')]) for p in pairs])))

if __name__=='__main__':main()

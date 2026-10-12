#!/usr/bin/env python3
"""Read-only actual-byte correspondence candidates across the owned updates.

Masked image addresses only find candidates. Body differences, image-memory
bytes, direct callees and neighboring unmatched intervals remain distinct.
No annotation names, inferred tuning, native edits or source-result stubs.
"""
import argparse,collections,csv,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_IMM
from oracle_mp_combat_postprocess_updates import OLD,ROOT,sha
from oracle_smg_firing_1_1 import Xbe,_Code,_trace,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,norm,full_tokens,containing

def load_tsv(p):return{int(r['address'],16):int(r['bytes'])for r in csv.DictReader(p.open(),delimiter='\t')}
def h(v):return f'{v:08x}'
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    assert sha(OLD)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(OLD),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    inv=load_tsv(ROOT/'analysis/map-pack-update/functions.tsv');nin=load_tsv(ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')
    md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True
    def prefix(code,p,n):
        ins=[]
        for i in range(n):
            q=code.at(p)
            if q is None:break
            ins.append(q);p+=q.size
            if q.mnemonic.startswith('ret'):break
        return ins
    # Every actual new function entry is a candidate; no correspondence labels
    # from the supplied annotation archive participate.
    prefixes={n:collections.defaultdict(list)for n in(16,12,8)}
    for p in nin:
        ins=prefix(nc,p,16)
        for n in prefixes:
            if len(ins)>=n:prefixes[n][tuple(norm(i,new)for i in ins[:n])].append(p)
    mapped={};candidates={}
    for p in inv:
        ins=prefix(oc,p,16);found=[];n=0
        for n in(16,12,8):
            if len(ins)<n:continue
            found=prefixes[n].get(tuple(norm(i,old)for i in ins[:n]),[])
            if len(found)==1:break
        candidates[p]=(n,found)
        if len(found)==1:mapped[p]=found[0]
    # Includes unit/inventory/weapon/projectile/damage and the actual map-load
    # + combat-postprocess blocks; scope is explicit, not all title-update code.
    selected=[p for p in inv if (0xc0000<=p<0x111000 or 0x11d000<=p<0x11e000 or 0x164000<=p<0x165000)and inv[p]>=32]
    rows=[];differences=[];instructions=a.output/'instructions';instructions.mkdir(exist_ok=True)
    for p in selected:
        n,found=candidates[p];row={'disc_entry':h(p),'disc_inventory_bytes':inv[p],'prefix_instructions':n,'candidate_entries':[h(q)for q in found]}
        if p not in mapped:rows.append(row);continue
        q=mapped[p];of=_trace(oc,p,set(inv));nf=_trace(nc,q,set(nin));oi=[oc.at(x)for x in sorted(of.code)];ni=[nc.at(x)for x in sorted(nf.code)]
        same=full_tokens(oi,old)==full_tokens(ni,new);row.update(later_entry=h(q),disc_instructions=len(oi),later_instructions=len(ni),normalized_instructions_and_internal_branch_equal=same)
        changed_refs=[];calls=[];immediates=[]
        if same:
            for x,y in zip(oi,ni):
                for xo,yo in zip(x.operands,y.operands):
                    if xo.type==yo.type==X86_OP_IMM and xo.imm!=yo.imm and not(x.mnemonic.startswith('j')or x.mnemonic=='call'):
                        immediates.append({'disc_site':h(x.address),'later_site':h(y.address),'disc_instruction':x.mnemonic+' '+x.op_str,'later_instruction':y.mnemonic+' '+y.op_str,'disc_immediate':h(xo.imm&0xffffffff),'later_immediate':h(yo.imm&0xffffffff),'disc_value_in_loaded_image':containing(old,xo.imm)is not None,'later_value_in_loaded_image':containing(new,yo.imm)is not None,'scope':'Image-address masking is candidate search only. These changed literal operands require separate semantic validation; no pointer/integer classification is inferred.'})
                    if xo.type==yo.type==X86_OP_MEM and not xo.mem.base and not yo.mem.base and containing(old,xo.mem.disp)and containing(new,yo.mem.disp):
                        assert xo.size==yo.size;ob=old.read(xo.mem.disp,xo.size);nb=new.read(yo.mem.disp,yo.size)
                        if ob!=nb:changed_refs.append({'disc_site':h(x.address),'later_site':h(y.address),'disc_address':h(xo.mem.disp),'later_address':h(yo.mem.disp),'size':xo.size,'disc_hex':ob.hex(),'later_hex':nb.hex(),'disc_section':containing(old,xo.mem.disp).name,'later_section':containing(new,yo.mem.disp).name})
                if x.mnemonic=='call'and x.operands[0].type==X86_OP_IMM and y.operands[0].type==X86_OP_IMM:
                    left,right=x.operands[0].imm,y.operands[0].imm
                    if mapped.get(left)!=right:calls.append({'disc_site':h(x.address),'later_site':h(y.address),'disc_target':h(left),'later_target':h(right),'disc_target_unique_prefix_mapping':h(mapped[left])if left in mapped else None})
        row['different_paired_image_memory_bytes']=changed_refs;row['different_nonbranch_immediates']=immediates;row['unresolved_or_different_paired_callees']=calls
        if not same or changed_refs or calls or immediates:
            differences.append(row)
            for label,ins in(('disc',oi),('later',ni)):(instructions/(h(p)+'-'+label+'-'+h(p if label=='disc'else q)+'.asm')).write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
        rows.append(row)
    intervals=[]
    for p in selected:
        if p in mapped:continue
        before=max((x for x in mapped if x<p),default=None);after=min((x for x in mapped if x>p),default=None)
        r={'disc_entry':h(p),'disc_bytes':inv[p],'previous_anchor':h(before)if before else None,'next_anchor':h(after)if after else None}
        if before is not None and after is not None and mapped[before]<mapped[after]:
            r.update(later_previous_anchor=h(mapped[before]),later_next_anchor=h(mapped[after]),later_unmatched_interval_entries=[h(q)for q in nin if mapped[before]<q<mapped[after]],disc_interval_entries=[h(q)for q in inv if before<q<after])
        intervals.append(r)
    report={'tool_sha256':sha(__file__),'disc_xbe_sha256':PIN,'owned_later_xbe_sha256':NEW_PIN,
            'scope':'Original unit/inventory/damage/weapon/projectile functions C0000..110FFF, map-load 11D000..11DFFF, and combat postprocess 164000..164FFF with Ghidra inventory size>=32. Entry-prefix candidates from actual owned instruction bytes; masking only locates possible correspondence. Full body/internal branches, memory-byte differences and unresolved callees retain separate evidence. No assertion that unmapped intervals correspond or that differences affect balance.',
            'all_inventory_unique_prefix_candidates':len(mapped),'selected_functions':len(rows),'routines':rows,'nonidentical_or_unresolved_routines':differences,'unmapped_neighbor_intervals':intervals,'native_gameplay_update_applied':False}
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'selected':len(rows),'mapped':sum('later_entry'in r for r in rows),'normalized_body_changed':sum(r.get('normalized_instructions_and_internal_branch_equal')is False for r in rows),'unmatched':len(intervals),'with_memory_or_call_questions':len(differences),'report':str(a.output/'report.json')}))
if __name__=='__main__':main()

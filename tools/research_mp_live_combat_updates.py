#!/usr/bin/env python3
"""Compare owned original bytes for the native live damage/SMG source slices.

Address masking locates entries only. Every complete body, branch table,
absolute operand, changed nonbranch immediate and direct-call contract is
retained separately. Writable image bytes are initial bytes, not proof of
their runtime producers. This script executes no game/platform service.
"""
import argparse, collections, csv, json, struct
from pathlib import Path
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from oracle_mp_combat_postprocess_updates import OLD, sha
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, norm, full_tokens, containing

GROUPS = {
 'damage_scalars_and_owner': (0xd71c0,0xd7290,0xd7320,0xd7380,0xd7e00,0xd8070,
  0xda2b0,0xda620,0xda710,0xdc0a0,0xdc690,0xdc710,0xd9180,0xdb160,
  0xdaa90,0xdac40,0xd90e0,0x15f520,0x1ed090),
 'lethal_attribution_and_counter': (0xcba60,0x1ed960,0x1e2f50,0x253f10,
  0x1edf70,0x1ee130,0x1ee240,0x15ce00,0x1ec930),
 'smg_constructor_and_phases': (0x101bb0,0xce510,0xe9d50,0x1040f0,0x108230,
  0x109c30,0x1021d0,0x1019f0,0x101a60,0x1099b0,0x109a60,0x109d50,
  0x105640,0x106030,0x106c30,0x106d40,0x106e70,0x106580,0x101d50,
  0x101e30,0x107120,0x107160,0x107f80,0x108010,0x108110,0x108300,0xefcc0),
 'weapon_error': (0x102090,0xd2940,0x105840,0xceab0,0xcd1a0,0x147a90),
}
SOURCES=('src/unit_damage.cpp','src/damage_amount.cpp','src/lethal_attribution.cpp',
 'src/owner_counter.cpp','src/smg.cpp','src/weapon_error.cpp','src/kernels.cpp')
def h(v):return f'{v&0xffffffff:08x}'
def inventory(p):return {int(r['address'],16):int(r['bytes']) for r in csv.DictReader(p.open(),delimiter='\t')}

def main(groups=GROUPS,seed_pairs=None,source_files=SOURCES,configuration_tool=None):
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 assert sha(OLD)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 images=(Xbe(OLD),Xbe(NEW_DEFAULT));codes=tuple(_Code(x,'.text') for x in images)
 invs=(inventory(ROOT/'analysis/map-pack-update/functions.tsv'),inventory(ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'))
 cache={}
 def trace(side,p):
  key=(side,p)
  if key not in cache:
   f=_trace(codes[side],p,set(invs[side]));cache[key]=(f,[codes[side].at(x) for x in sorted(f.code)])
  return cache[key]
 def prefix(side,p,n=16):
  r=[]
  for _ in range(n):
   i=codes[side].at(p)
   if i is None:break
   r.append(i);p+=i.size
   if i.mnemonic.startswith('ret'):break
  return r
 indexes={n:collections.defaultdict(list) for n in (16,12,8,4)}
 for p in invs[1]:
  ins=prefix(1,p)
  for n in indexes:
   if len(ins)>=n:indexes[n][tuple(norm(i,images[1]) for i in ins[:n])].append(p)
 mapped={};evidence={};candidates={}
 for p,(q,why) in (seed_pairs or {}).items():mapped[p]=q;evidence[p]=why
 roots=tuple(dict.fromkeys(p for v in groups.values() for p in v))
 for p in roots:
  ins=prefix(0,p);found=[]
  for n in indexes:
   if len(ins)<n:continue
   found=indexes[n].get(tuple(norm(i,images[0]) for i in ins[:n]),[])
   if len(found)==1:
    # A callback table is an actual owning correspondence. Ghidra can omit
    # its real entry and retain a different identical wrapper; retain that
    # candidate separately instead of replacing the owning table entry.
    if p not in mapped:mapped[p]=found[0];evidence[p]={'kind':'unique_actual_entry_prefix','instructions':n}
    break
  candidates[p]=found
 # Only exact paired direct-call sites in a body whose full instruction/branch
 # shape matches may locate a callee. This does not imply callee equivalence.
 pending=list(mapped);expanded=set()
 while pending:
  p=pending.pop(0)
  if p in expanded:continue
  expanded.add(p);q=mapped[p];of,oi=trace(0,p);nf,ni=trace(1,q)
  if full_tokens(oi,images[0])!=full_tokens(ni,images[1]):continue
  for x,y in zip(oi,ni):
   if x.mnemonic!='call' or x.operands[0].type!=X86_OP_IMM or y.operands[0].type!=X86_OP_IMM:continue
   left,right=x.operands[0].imm,y.operands[0].imm
   if not codes[0].inside(left) or not codes[1].inside(right):continue
   if left in mapped:
    assert mapped[left]==right,('contradictory_source_call_pair',h(p),h(left),h(mapped[left]),h(right))
   else:
    mapped[left]=right;evidence[left]={'kind':'actual_direct_call_in_matching_parent_body','parent_disc':h(p),'parent_later':h(q),'disc_call_site':h(x.address),'later_call_site':h(y.address)}
    # Recover the direct dependencies of primary source routines and unresolved
    # primary entries. Do not turn this into an unused whole-engine inventory.
    if left in roots:pending.append(left)
 a.output.mkdir(parents=True,exist_ok=True);d=a.output/'instructions';d.mkdir(exist_ok=True)
 rows=[]
 selected=list(dict.fromkeys((*roots,*mapped.keys())))
 for p in selected:
  row={'disc_entry':h(p),'primary_groups':[g for g,v in groups.items() if p in v],
       'correspondence_candidates':[h(x) for x in candidates.get(p,[])]}
  if p not in mapped:row['status']='unresolved_entry';rows.append(row);continue
  q=mapped[p];of,oi=trace(0,p);nf,ni=trace(1,q);same=full_tokens(oi,images[0])==full_tokens(ni,images[1])
  row.update(later_entry=h(q),mapping_evidence=evidence[p],disc_instructions=len(oi),later_instructions=len(ni),complete_instruction_shape_and_internal_branches_equal=same)
  for side,label,at,ins in ((0,'disc',p,oi),(1,'later',q,ni)):
   file=d/(h(p)+'-'+label+'-'+h(at)+'.asm');file.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
   row[label+'_listing']={'file':str(file.relative_to(a.output)),'sha256':sha(file)}
  refs=[];calls=[];imms=[];tables=[];indirect=[]
  if same:
   positions=[{i.address:n for n,i in enumerate(ins)} for ins in (oi,ni)]
   assert len(of.tables)==len(nf.tables)
   for left,right in zip(of.tables,nf.tables):
    op,width,count=left;np,nw,nn=right;assert (width,count)==(nw,nn)
    ob=images[0].read(op,width*count);nb=images[1].read(np,width*count)
    if width==4:
     vals=(struct.unpack('<'+'I'*count,ob),struct.unpack('<'+'I'*count,nb))
     target_indices=[[positions[s].get(v,'ZERO' if not v else 'EXTERNAL') for v in vals[s]] for s in (0,1)]
     equal=target_indices[0]==target_indices[1] and 'EXTERNAL' not in target_indices[0]
    else:target_indices=None;equal=ob==nb
    tables.append({'disc_address':h(op),'later_address':h(np),'width':width,'count':count,'disc_hex':ob.hex(),'later_hex':nb.hex(),'target_instruction_indices':target_indices,'mapped_contents_equal':equal})
   for x,y in zip(oi,ni):
    for k,(xo,yo) in enumerate(zip(x.operands,y.operands)):
     if xo.type==yo.type==X86_OP_MEM and not xo.mem.base and not yo.mem.base and containing(images[0],xo.mem.disp) and containing(images[1],yo.mem.disp):
      assert xo.size==yo.size;width=xo.size;ob=images[0].read(xo.mem.disp,width);nb=images[1].read(yo.mem.disp,width)
      refs.append({'disc_site':h(x.address),'later_site':h(y.address),'operand':k,'disc_address':h(xo.mem.disp),'later_address':h(yo.mem.disp),'width':width,'disc_hex':ob.hex(),'later_hex':nb.hex(),'bytes_equal':ob==nb,'disc_section':containing(images[0],xo.mem.disp).name,'later_section':containing(images[1],yo.mem.disp).name,'indexed':bool(xo.mem.index or yo.mem.index)})
     if xo.type==yo.type==X86_OP_IMM and xo.imm!=yo.imm and not (x.mnemonic.startswith('j') or x.mnemonic=='call'):
      imms.append({'disc_site':h(x.address),'later_site':h(y.address),'operand':k,'disc_value':h(xo.imm),'later_value':h(yo.imm),'both_values_in_images':bool(containing(images[0],xo.imm) and containing(images[1],yo.imm))})
    if x.mnemonic=='call':
     if x.operands[0].type==y.operands[0].type==X86_OP_IMM:
      left,right=x.operands[0].imm,y.operands[0].imm;calls.append({'disc_site':h(x.address),'later_site':h(y.address),'disc_target':h(left),'later_target':h(right),'pair_record_present':mapped.get(left)==right})
     else:indirect.append({'disc_site':h(x.address),'later_site':h(y.address),'disc_assembly':x.mnemonic+' '+x.op_str,'later_assembly':y.mnemonic+' '+y.op_str,'scope':'runtime callback target and owning state remain input/producer boundaries'})
  row.update(paired_absolute_image_operands=refs,changed_nonbranch_immediates=imms,paired_direct_calls=calls,indirect_calls=indirect,switch_tables=tables)
  row['status']='complete_static_body_shape_equal' if same else 'changed_or_unresolved_body'
  row['complete_leaf_without_calls']=same and not of.calls and not nf.calls and not calls and not indirect
  rows.append(row)
 # The full immutable random-axis table is a separate operand-sized evidence
 # item: a single indexed read operand does not establish all1026 entries.
 randomrow=next((r for r in rows if r['disc_entry']=='00147a90' and r.get('complete_instruction_shape_and_internal_branches_equal')),None)
 tables=[]
 if randomrow:
  _,left=trace(0,0x147a90);_,right=trace(1,int(randomrow['later_entry'],16))
  operands=[(x,y) for x,y in zip(left,right) if any(o.type==X86_OP_MEM and o.mem.disp==0x447e40 for o in x.operands)]
  assert len(operands)==1
  x,y=operands[0];newoperand=next(o for o in y.operands if o.type==X86_OP_MEM and not o.mem.base)
  p,q=0x447e40,newoperand.mem.disp;ob=images[0].read(p,1026*12);nb=images[1].read(q,1026*12)
  import hashlib
  tables.append({'purpose':'147A90 full1026×3 float random-direction input table','disc_site':h(x.address),'later_site':h(y.address),'disc_address':h(p),'later_address':h(q),'size':len(ob),'disc_sha256':hashlib.sha256(ob).hexdigest(),'later_sha256':hashlib.sha256(nb).hexdigest(),'bytes_equal':ob==nb})
 report={'tool_sha256':sha(__file__),'disc_xbe_sha256':PIN,'later_xbe_sha256':NEW_PIN,
  'native_source_snapshot_sha256':{p:sha(ROOT/p) for p in source_files},'primary_routine_count':len(roots),'routines':rows,'full_immutable_tables':tables,
  'configuration_tool':configuration_tool,
  'scope':'Static original source comparison for the bounded native live damage/SMG slices plus actual immediate direct callees. Complete bodies/branches/tables and literal operands are retained. Runtime global initialization, indirect callbacks, full damage/death/world owners, CPU state and service admission are not established by matching serialized image data. No game instructions or providers execute in this static audit.',
  'native_gameplay_update_applied':False}
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({'primary':len(roots),'all_pairs':len(rows),'primary_unresolved':[r['disc_entry'] for r in rows if r['primary_groups'] and not r.get('complete_instruction_shape_and_internal_branches_equal')],'body_changed':[r['disc_entry'] for r in rows if r.get('complete_instruction_shape_and_internal_branches_equal') is False],'report':str(a.output/'report.json')}))
if __name__=='__main__':main()

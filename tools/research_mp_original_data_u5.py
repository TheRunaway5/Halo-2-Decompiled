#!/usr/bin/env python3
"""Exact OriginalData table correspondence with original consumer instructions.

Read-only executable bytes. Fresh unseeded Ghidra starts provide navigation
hypotheses only; the paired complete original instruction streams are verified.
"""
import argparse,bisect,csv,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code,_trace
from capstone.x86 import X86_OP_MEM,X86_OP_IMM
from oracle_mp_title_update_display import OLD,NEW,PINS,sha,save,encoded
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/original-data-u5'
OLD_INVENTORY=ROOT/'analysis/map-pack-update/functions.tsv'
NEW_INVENTORY=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
TABLES=[dict(name='weapon_directions',old=0x447e40,bytes=1026*12,fnv64='6778693f5db53615',consumer=0x147a50),
 dict(name='fade_curve5',old=0x4745e8,bytes=1024,fnv64='e2570a57f6e2dc5e',consumer=0x17e480),
 dict(name='collision_projection',old=0x4471e4,bytes=36,fnv64='ed2df01a26bd8b1d',consumer=0x14bac0)]
def fnv64(raw):
 h=14695981039346656037
 for v in raw:h=((h^v)*1099511628211)&0xffffffffffffffff
 return f'{h:016x}'
def norm(i,image,positions):
 op=i.op_str
 if i.mnemonic.startswith('j')or i.mnemonic=='call':
  if len(i.operands)==1 and i.operands[0].type==X86_OP_IMM:
   p=i.operands[0].imm;return i.mnemonic+' '+('LOCAL_'+str(positions[p])if p in positions else'EXTERNAL')
 def replace(m):
  p=int(m.group(),16)
  return 'IMAGE_ADDRESS'if any(s.va<=p<s.va+s.vsize for s in image.sections)else m.group()
 return i.mnemonic+' '+re.sub(r'0x[0-9a-f]+',replace,op)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');a=p.parse_args()
 for path,h in PINS.items():assert sha(path)==h,path
 old,new=Xbe(OLD),Xbe(NEW);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 old_starts={int(r['address'],16)for r in csv.DictReader(OLD_INVENTORY.open(),delimiter='\t')}
 new_starts=sorted(int(r['address'],16)for r in csv.DictReader(NEW_INVENTORY.open(),delimiter='\t'))
 rows=[]
 for table in TABLES:
  raw=old.read(table['old'],table['bytes']);assert fnv64(raw)==table['fnv64']
  hits=[];pos=0
  while True:
   pos=new.data.find(raw,pos)
   if pos<0:break
   hits.append(pos);pos+=1
  assert len(hits)==1,(table['name'],hits)
  raw_offset=hits[0];sections=[s for s in new.sections if s.raw<=raw_offset and raw_offset+len(raw)<=s.raw+s.rsize]
  assert len(sections)==1;section=sections[0];new_address=section.va+raw_offset-section.raw
  assert new.read(new_address,len(raw))==raw
  # Curve5's original consumer forms the entire curve family's base+AX*400.
  old_reference=table['old']-(5*1024 if table['name']=='fade_curve5'else 0)
  new_reference=new_address-(5*1024 if table['name']=='fade_curve5'else 0)
  needle=struct.pack('<I',new_reference);references=[];pos=0
  while True:
   pos=nc.bytes.find(needle,pos)
   if pos<0:break
   references.append(nc.lo+pos);pos+=1
  assert references
  before=_trace(oc,table['consumer'],old_starts);oi=[oc.at(p)for p in sorted(before.code)];opos={i.address:n for n,i in enumerate(oi)}
  expected=[norm(i,old,opos)for i in oi];matches=[]
  for pointer_site in references:
   idx=bisect.bisect_right(new_starts,pointer_site)-1
   if idx<0:continue
   start=new_starts[idx];f=_trace(nc,start,set(new_starts));ni=[nc.at(p)for p in sorted(f.code)];npos={i.address:n for n,i in enumerate(ni)}
   if [norm(i,new,npos)for i in ni]==expected:matches.append((start,f,ni))
  unique={v[0]:v for v in matches};assert len(unique)==1,(table['name'],list(unique))
  entry,after,ni=next(iter(unique.values()));uses=[];memory=[];calls=[]
  for x,y in zip(oi,ni):
   assert x.mnemonic==y.mnemonic
   for left,right in zip(x.operands,y.operands):
    if left.type==right.type==X86_OP_IMM and left.imm==old_reference and right.imm==new_reference or \
       left.type==right.type==X86_OP_MEM and left.mem.disp==old_reference and right.mem.disp==new_reference:
     uses.append(dict(old_site=f'{x.address:08x}',u5_site=f'{y.address:08x}',old_bytes=x.bytes.hex(),u5_bytes=y.bytes.hex(),
      old_instruction=x.mnemonic+' '+x.op_str,u5_instruction=y.mnemonic+' '+y.op_str))
    if left.type==right.type==X86_OP_MEM and any(s.va<=left.mem.disp<s.va+s.vsize for s in old.sections)and any(s.va<=right.mem.disp<s.va+s.vsize for s in new.sections):
     assert left.size==right.size;ob=old.read(left.mem.disp,left.size);nb=new.read(right.mem.disp,right.size)
     memory.append(dict(old_site=f'{x.address:08x}',u5_site=f'{y.address:08x}',old_pointer=f'{left.mem.disp:08x}',u5_pointer=f'{right.mem.disp:08x}',width=left.size,old_hex=ob.hex(),u5_hex=nb.hex(),bytes_equal=ob==nb))
   if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==X86_OP_IMM:
    calls.append(dict(old_site=f'{x.address:08x}',u5_site=f'{y.address:08x}',old_target=f'{x.operands[0].imm:08x}',u5_target=f'{y.operands[0].imm:08x}'))
  assert uses
  if table['name']=='fade_curve5':
   assert any(i.mnemonic=='shl'and i.operands[-1].type==X86_OP_IMM and i.operands[-1].imm==10 for i in ni)
   assert new_reference+5*1024==new_address
  listings=[]
  for label,ins,image in(('old',oi,old),('u5',ni,new)):
   path=a.output.parent/'instructions'/f'{table["name"]}-{label}.asm'
   for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
   save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(a.output.parent)),sha256=sha(path),rows=len(ins)))
  payload=a.output.parent/'tables'/f'{table["name"]}.bin';save(payload,raw)
  rows.append(dict(name=table['name'],bytes=len(raw),old_va=f'{table["old"]:08x}',u5_va=f'{new_address:08x}',u5_raw_offset=f'{raw_offset:08x}',u5_section=section.name,
   full_table_unique_in_new_file=True,full_bytes_equal=True,sha256=hashlib.sha256(raw).hexdigest(),fnv1a64=fnv64(raw),
   table_payload=dict(file=str(payload.relative_to(a.output.parent)),sha256=sha(payload)),
   consumer=dict(old_entry=f'{table["consumer"]:08x}',u5_entry=f'{entry:08x}',full_normalized_instruction_and_branch_topology_equal=True,
    old_rows=len(oi),u5_rows=len(ni),old_reference=f'{old_reference:08x}',u5_reference=f'{new_reference:08x}',actual_original_table_use_instructions=uses,
    paired_image_memory_operands=memory,all_paired_memory_bytes_equal=all(v['bytes_equal']for v in memory),direct_call_targets=calls,listings=listings,
    scope='Static original instructions, not an executed caller or proof of external helper/live producer equivalence.'),
   fade_selector_formula='signed AX*0x400 + original_curve_family_base; selected curve5 extent1024'if table['name']=='fade_curve5'else None))
  print(json.dumps(dict(table=table['name'],u5_va=f'{new_address:08x}',u5_consumer=f'{entry:08x}',bytes=len(raw))),flush=True)
 report=dict(schema=1,tool_sha256=sha(__file__),source_pins=PINS,source_inventory_pins={str(OLD_INVENTORY):sha(OLD_INVENTORY),str(NEW_INVENTORY):sha(NEW_INVENTORY)},
  old_full_file_fnv1a64=fnv64(old.data),u5_full_file_fnv1a64=fnv64(new.data),tables=rows,
  importer_contract='Current OriginalData.load_xbe extents and FNV constraints. Source input SHA/RSA authenticity is separate; FNV is a loader identity selector, not authentication.',
  annotation_names_or_comments_used=False,production_changes=False,
  boundaries=['Actual table bytes/unique raw-backed locations and complete static representative source-consumer instruction correspondence only.',
   'No new dynamic direction/fade/projection kernel, RNG sequence, complete updated frame/balance/rendering or latest-ever behavior is claimed. No shader/matrix/render code is modified.'])
 for path,h in PINS.items():assert sha(path)==h,path
 save(a.output,encoded(report));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),full_file_fnv1a64=report['u5_full_file_fnv1a64'])))
if __name__=='__main__':main()

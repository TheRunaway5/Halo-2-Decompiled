#!/usr/bin/env python3
"""Unseeded static correspondence of original control/request/pose owners."""
import argparse,csv,hashlib,json,struct
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,sha,save,encoded,Xbe,_Code,_trace
from research_mp_original_data_u5 import norm

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5'
ENTRIES=[0x151a20,0x151980,0x151aa0,0x151c90,0xc8140,0xe7b10,0xe5680,0xc7e90,0x30bd0,0x1d10b0,
         0xde960,0xe1800,0x1e9540,0xe3b20,0x1e8f60,0x1e94e0,0x1f52f0,0xe44c0,
         0x112e10,0x1cf100,0x1d0a60,0x1cfce0,0x1cfde0,0x1ca2b0,0x1ca090,0x1c9f50,0x113830]
INVENTORIES=[ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']

def prefix(code,image,start,n=8):
 ins=[];p=start
 for _ in range(n):
  i=code.at(p)
  if i is None:return []
  ins.append(i);p+=i.size
 return [norm(i,image,{})for i in ins]

def raw_prefix_candidates(old_code,old_image,new_code,start,n=8):
 # Search actual encoded opcode bytes with only original image-address fields
 # masked. This also finds real entries omitted by heuristic Ghidra starts.
 ins=[];p=start
 for _ in range(n):
  i=old_code.at(p);ins.append(i);p+=i.size
 raw=bytearray(old_code.bytes[start-old_code.lo:p-old_code.lo]);mask=bytearray(b'\1'*len(raw))
 for i in ins:
  at=i.address-start
  for operand in i.operands:
   address=operand.mem.disp if operand.type==3 else operand.imm if operand.type==2 else 0
   if any(s.va<=address<s.va+s.vsize for s in old_image.sections):
    offset,size=(i.disp_offset,i.disp_size)if operand.type==3 else(i.imm_offset,i.imm_size)
    for k in range(offset,offset+size):mask[at+k]=0
 runs=[];k=0
 while k<len(mask):
  if not mask[k]:k+=1;continue
  end=k
  while end<len(mask)and mask[end]:end+=1
  runs.append((k,end));k=end
 begin,end=max(runs,key=lambda q:q[1]-q[0]);needle=raw[begin:end];pos=0;hits=[]
 while True:
  pos=new_code.bytes.find(needle,pos)
  if pos<0:break
  candidate=pos-begin;pos+=1
  if candidate<0 or candidate+len(raw)>len(new_code.bytes):continue
  if all(not m or a==b for a,b,m in zip(raw,new_code.bytes[candidate:candidate+len(raw)],mask)):
   hits.append(new_code.lo+candidate)
 return hits

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');out=p.parse_args().output
 for path,pin in PINS.items():assert sha(path)==pin
 images=[Xbe(OLD),Xbe(NEW)];codes=[_Code(x,'.text')for x in images]
 starts=[{int(q['address'],16)for q in csv.DictReader(path.open(),delimiter='\t')}for path in INVENTORIES]
 buckets={}
 for start in sorted(starts[1]):buckets.setdefault(tuple(prefix(codes[1],images[1],start)),[]).append(start)
 rows=[]
 for entry in ENTRIES:
  f=_trace(codes[0],entry,starts[0]);old=[codes[0].at(a)for a in sorted(f.code)];positions={i.address:n for n,i in enumerate(old)}
  tokens=[norm(i,images[0],positions)for i in old];inventory_candidates=buckets.get(tuple(prefix(codes[0],images[0],entry)),[])
  byte_candidates=raw_prefix_candidates(codes[0],images[0],codes[1],entry)if not inventory_candidates else[]
  candidates=sorted(set(inventory_candidates+byte_candidates));matches=[];failed=[]
  for start in candidates:
   try:
    nf=_trace(codes[1],start,starts[1]);new=[codes[1].at(a)for a in sorted(nf.code)];np={i.address:n for n,i in enumerate(new)}
    if [norm(i,images[1],np)for i in new]==tokens:matches.append((start,nf,new))
    else:failed.append(dict(entry=f'{start:08x}',rows=len(new)))
   except Exception as e:failed.append(dict(entry=f'{start:08x}',error=str(e)))
  row=dict(old_entry=f'{entry:08x}',old_rows=len(old),prefix_candidates=[f'{a:08x}'for a in candidates],inventory_prefix_candidates=[f'{a:08x}'for a in inventory_candidates],raw_opcode_prefix_candidates=[f'{a:08x}'for a in byte_candidates],full_topology_matches=[f'{a:08x}'for a,_,_ in matches],nonmatching_candidates=failed)
  if len(matches)==1:
   start,nf,new=matches[0];listings=[];calls=[];mem=[];tables=[];address_map={a.address:b.address for a,b in zip(old,new)}
   assert len(f.tables)==len(nf.tables)
   for (oa,stride,count),(na,new_stride,new_count)in zip(f.tables,nf.tables):
    assert stride==new_stride==4 and count==new_count
    ov=struct.unpack('<'+'I'*count,images[0].read(oa,count*4));nv=struct.unpack('<'+'I'*count,images[1].read(na,count*4))
    assert all(address_map.get(a)==b for a,b in zip(ov,nv))
    tables.append(dict(old_va=f'{oa:08x}',u5_va=f'{na:08x}',count=count,stride=4,old_targets=[f'{v:08x}'for v in ov],u5_targets=[f'{v:08x}'for v in nv],all_targets_follow_original_instruction_bijection=True))
   for label,ins,image in (('old',old,images[0]),('u5',new,images[1])):
    path=out.parent/'instructions'/f'{entry:08x}-{label}.asm'
    for i in ins:assert image.read(i.address,i.size)==i.bytes
    save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(out.parent)),sha256=sha(path),rows=len(ins)))
   for a,b in zip(old,new):
    if a.mnemonic=='call'and a.op_str.startswith('0x')and b.op_str.startswith('0x'):calls.append(dict(old_site=f'{a.address:08x}',u5_site=f'{b.address:08x}',old_target=a.op_str,u5_target=b.op_str))
    for oa,ob in zip(a.operands,b.operands):
     if oa.type==ob.type==3 and not oa.mem.base and not ob.mem.base and any(s.va<=oa.mem.disp<s.va+s.vsize for s in images[0].sections)and any(s.va<=ob.mem.disp<s.va+s.vsize for s in images[1].sections):
      assert oa.size==ob.size
      av,bv=images[0].read(oa.mem.disp,oa.size),images[1].read(ob.mem.disp,ob.size)
      mem.append(dict(old_site=f'{a.address:08x}',u5_site=f'{b.address:08x}',old_va=f'{oa.mem.disp:08x}',u5_va=f'{ob.mem.disp:08x}',size=oa.size,old_hex=av.hex(),u5_hex=bv.hex(),initial_image_bytes_equal=av==bv))
   row.update(u5_entry=f'{start:08x}',complete_instruction_and_internal_branch_topology_equal=True,listings=listings,direct_calls=calls,absolute_image_memory_operands=mem,jump_tables=tables)
  rows.append(row);print(json.dumps(dict(old_entry=row['old_entry'],matches=row['full_topology_matches'],candidates=len(candidates))),flush=True)
 proven={q['old_entry']:q['u5_entry']for q in rows if q.get('u5_entry')}
 for row in rows:
  for call in row.get('direct_calls',[]):
   old_target=f'{int(call["old_target"],16):08x}';new_target=f'{int(call["u5_target"],16):08x}'
   call['target_full_mapping_in_this_report']=old_target in proven
   if old_target in proven:assert proven[old_target]==new_target
 report=dict(schema=1,tool_sha256=sha(__file__),source_pins=PINS,source_inventory_pins={str(p):sha(p)for p in INVENTORIES},owners=rows,
  scope='Static complete normalized instructions/internal branch topology only. Actual external targets/initial operands retained; no source-created1.5 objects, fields, calls, current controls or animation outputs asserted. Unmatched owners remain open.',annotation_names_or_comments_used=False,production_changes=False)
 save(out,encoded(report));print(json.dumps(dict(output=str(out),sha256=sha(out))))
if __name__=='__main__':main()

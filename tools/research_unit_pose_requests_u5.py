#!/usr/bin/env python3
"""Complete retained pose-request/temporary-channel body and table correspondence."""
import argparse, csv, json, struct
from pathlib import Path
from research_mp_live_locomotion_owners_u5 import prefix, raw_prefix_candidates
from research_mp_original_data_u5 import norm
from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS as SOURCE_PINS, sha, save, encoded, Xbe, _Code, _trace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/unit-pose-requests/u5'
DEPENDENCIES={
 'tools/research_mp_live_locomotion_owners_u5.py':'82db82bdbbf29b269fdfca6a95cf798d41c591d98f400e5d956c01901bc1627d',
 'tools/research_mp_original_data_u5.py':'009b2d7836b63036ea80bfc7cf67b6edd6c7f4b8182fac54bd8e7ee1fc0978e7',
 'tools/oracle_mp_title_update_display.py':'5b53a4921e8e834510f972d6c11e3417f0fe6d9d9952cc3833f7d2bf9bdab311',
 'tools/oracle_mp_inventory_equipped_1_1.py':'47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655',
 'analysis/multiplayer/integration/inventory-source/equipped-pickup/report.json':'d6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b',
}
ENTRIES=(0x114bd0,0x1cf4c0,0x1cf3c0,0x1cf360,0x1cef10,0x1cfb90,0x115af0,0x1cfde0,0x1cfce0,0x1df670,0x1cee00)
INVENTORIES=(ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args()
 for path,pin in SOURCE_PINS.items():assert sha(path)==pin,path
 for path,pin in DEPENDENCIES.items():assert sha(ROOT/path)==pin,path
 images=[Xbe(OLD),Xbe(NEW)];codes=[_Code(x,'.text')for x in images]
 starts=[{int(q['address'],16)for q in csv.DictReader(path.open(),delimiter='\t')}for path in INVENTORIES]
 buckets={}
 for start in sorted(starts[1]):buckets.setdefault(tuple(prefix(codes[1],images[1],start)),[]).append(start)
 rows=[]
 for entry in ENTRIES:
  f=_trace(codes[0],entry,starts[0]);old=[codes[0].at(x)for x in sorted(f.code)];positions={i.address:n for n,i in enumerate(old)}
  expected=[norm(i,images[0],positions)for i in old]
  candidates=sorted(set(buckets.get(tuple(prefix(codes[0],images[0],entry)),[])+raw_prefix_candidates(codes[0],images[0],codes[1],entry)))
  matches=[];failures=[]
  for start in candidates:
   try:
    nf=_trace(codes[1],start,starts[1]);new=[codes[1].at(x)for x in sorted(nf.code)];np={i.address:n for n,i in enumerate(new)}
    if [norm(i,images[1],np)for i in new]==expected:matches.append((start,nf,new))
    else:failures.append(dict(entry=f'{start:08x}',instruction_rows=len(new)))
   except Exception as e:failures.append(dict(entry=f'{start:08x}',error=str(e)))
  row=dict(old_entry=f'{entry:08x}',old_rows=len(old),candidates=[f'{x:08x}'for x in candidates],full_body_matches=[f'{x:08x}'for x,_,_ in matches],unmatched_candidates=failures)
  if len(matches)==1:
   start,nf,new=matches[0];address_map={x.address:y.address for x,y in zip(old,new)}
   assert len(f.tables)==len(nf.tables)
   jump_tables=[]
   for (op,os,oc),(np,ns,nc)in zip(f.tables,nf.tables):
    assert os==ns and os in(1,4)and oc==nc
    old_raw=images[0].read(op,oc*os);new_raw=images[1].read(np,nc*ns)
    ov=struct.unpack('<'+('I'if os==4 else'B')*oc,old_raw);nv=struct.unpack('<'+('I'if ns==4 else'B')*nc,new_raw)
    if os==4:assert all(address_map[x]==y for x,y in zip(ov,nv))
    else:assert ov==nv
    jump_tables.append(dict(old_va=f'{op:08x}',u5_va=f'{np:08x}',element_bytes=os,count=oc,old_raw_hex=old_raw.hex(),u5_raw_hex=new_raw.hex(),old_targets=[f'{x:08x}'for x in ov],u5_targets=[f'{x:08x}'for x in nv]))
   listings=[]
   for label,ins,image in(('old',old,images[0]),('u5',new,images[1])):
    path=a.output_dir/'instructions'/f'{entry:08x}-{label}.asm'
    for i in ins:assert image.read(i.address,i.size)==i.bytes
    save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
    listings.append(dict(file=str(path.relative_to(a.output_dir)),rows=len(ins),sha256=sha(path)))
   calls=[];memory=[];stores=[]
   for x,y in zip(old,new):
    if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==2:
     calls.append(dict(old_site=f'{x.address:08x}',u5_site=f'{y.address:08x}',old_target=f'{x.operands[0].imm:08x}',u5_target=f'{y.operands[0].imm:08x}'))
    for ox,oy in zip(x.operands,y.operands):
     if ox.type==oy.type==3 and not ox.mem.base and not oy.mem.base and any(s.va<=ox.mem.disp<s.va+s.vsize for s in images[0].sections)and any(s.va<=oy.mem.disp<s.va+s.vsize for s in images[1].sections):
      assert ox.size==oy.size
      b0,b1=images[0].read(ox.mem.disp,ox.size),images[1].read(oy.mem.disp,oy.size)
      memory.append(dict(old_site=f'{x.address:08x}',u5_site=f'{y.address:08x}',old_va=f'{ox.mem.disp:08x}',u5_va=f'{oy.mem.disp:08x}',bytes=ox.size,old_hex=b0.hex(),u5_hex=b1.hex()))
    if x.operands and x.operands[0].type==3 and x.mnemonic.startswith('mov'):
     stores.append(dict(old_site=f'{x.address:08x}',u5_site=f'{y.address:08x}',old_instruction=x.mnemonic+' '+x.op_str,u5_instruction=y.mnemonic+' '+y.op_str))
   row.update(u5_entry=f'{start:08x}',full_normalized_instruction_internal_branch_topology_equal=True,listings=listings,direct_calls=calls,image_memory_operands=memory,memory_destination_moves=stores,jump_tables=jump_tables)
  rows.append(row);print(json.dumps(dict(old=row['old_entry'],matches=row['full_body_matches'],rows=len(old))),flush=True)
 mapping={r['old_entry']:r['u5_entry']for r in rows if 'u5_entry'in r}
 for r in rows:
  for call in r.get('direct_calls',[]):
   call['target_full_body_proven_here']=call['old_target']in mapping
   if call['old_target']in mapping:assert mapping[call['old_target']]==call['u5_target']
 action_tables=[]
 if all(f'{entry:08x}'in mapping for entry in (0xe9fe0,0xea180,0xea370,0xea270)):
  old_descriptor=images[0].read(0x46ea24,16)
  assert struct.unpack('<4I',old_descriptor)==(0xe9fe0,0xea180,0xea370,0xea270)
  new_descriptor=struct.pack('<4I',*(int(mapping[f'{x:08x}'],16)for x in(0xe9fe0,0xea180,0xea370,0xea270)))
  hits=[];pos=0
  while True:
   pos=images[1].data.find(new_descriptor,pos)
   if pos<0:break
   hits.append(pos);pos+=1
  assert len(hits)==1,hits
  sections=[s for s in images[1].sections if s.raw<=hits[0]and hits[0]+16<=s.raw+s.rsize];assert len(sections)==1
  pointer=sections[0].va+hits[0]-sections[0].raw
  action_tables.append(dict(old_descriptor_va='0046ea24',u5_descriptor_va=f'{pointer:08x}',old_raw16_hex=old_descriptor.hex(),u5_raw16_hex=new_descriptor.hex(),new_raw_file_match_unique=True,old_types=[8,18],old_type_table_va='0046ec68',
    old_type_words_hex=[images[0].read(0x46ec68+index*4,4).hex()for index in(8,18)],
    scope='New descriptor is found uniquely by all four separately proven complete handler addresses. New type table membership still requires its actual dispatch reference below; no new gameplay call executed.'))
  dispatcher=next(r for r in rows if r['old_entry']=='000e7e60')
  uses=[q for q in dispatcher.get('image_memory_operands',[])if q['old_va']=='0046ec68']
  assert len(uses)==1,uses
  table=int(uses[0]['u5_va'],16)
  assert all(struct.unpack('<I',images[1].read(table+index*4,4))[0]==pointer for index in(8,18))
  action_tables[-1].update(u5_type_table_va=f'{table:08x}',u5_type_words_hex=[images[1].read(table+index*4,4).hex()for index in(8,18)],original_dispatcher_use=uses[0],u5_types8_18_descriptor_verified=True)
 report=dict(schema=1,tool_sha256=sha(__file__),source_pins=SOURCE_PINS,dependency_pins=DEPENDENCIES,
  inventory_pins={str(path):sha(path)for path in INVENTORIES},owners=rows,source_switch_descriptors=action_tables,
  authority='Raw owned original executables. Fresh unseeded starts are navigation hypotheses; every paired complete body/internal branch/table and encoded instruction is checked against original bytes. No attached annotation seed or semantic comment is used.',
  scope='Complete retained pose-request/temporary-channel owners, static full-body/direct-call/table correspondence only. No original u5 runtime, transform/pose output, resource activation or IO success asserted; external helpers not separately mapped remain explicit.',production_edits=[],instruction_patches=[])
 for path,pin in SOURCE_PINS.items():assert sha(path)==pin,path
 for path,pin in DEPENDENCIES.items():assert sha(ROOT/path)==pin,path
 save(a.output_dir/'report.json',encoded(report));print(json.dumps(dict(output=str(a.output_dir/'report.json'),sha256=sha(a.output_dir/'report.json'),proven=len(mapping),total=len(rows))))

if __name__=='__main__':main()

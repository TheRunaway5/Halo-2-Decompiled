#!/usr/bin/env python3
"""Pair complete owned SDK capability/accounting bodies, without device success."""
import argparse,csv,json
from pathlib import Path
from research_mp_inventory_switch_u5 import prefix,raw_prefix_candidates,norm
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,_trace
from oracle_unit_animation_manager_1_1 import sha,save,encoded

ENTRIES=(0x381bee,0x380810,0x3804de,0x384c7c,0x38689a,0x3868dc,0x38683d,
         0x38ad61,0x38b257,0x3817af,0x38034c,0x3858d8,0x38b1eb,0x384d77,0x384e08,
         0x386967,0x386a38,0x384c52,0x3802a0,0x383ecc,0x383f92,
         0x38410a,0x384115,0x383c31,0x3832aa,0x383d6a,0x383dd7)

def run(out):
 for p,h in PINS.items():assert sha(p)==h,p
 images=[Xbe(OLD),Xbe(NEW)];codes=[_Code(x,'DSOUND')for x in images]
 paths=[ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']
 starts=[{int(q['address'],16)for q in csv.DictReader(p.open(),delimiter='\t')}for p in paths]
 buckets={}
 for e in sorted(starts[1]):
  if codes[1].lo<=e<codes[1].hi:buckets.setdefault(tuple(prefix(codes[1],images[1],e)),[]).append(e)
 rows=[]
 anchor_path=ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/paired/report.json'
 anchor=json.loads(anchor_path.read_text());targets={}
 for owner in anchor['owners']:
  for c in owner.get('calls',[]):targets[int(c['old_target'],16)]=int(c['new_target'],16)
 vtables={}
 for e in ENTRIES:
  f=_trace(codes[0],e,starts[0]|set(ENTRIES));ins=[codes[0].at(a)for a in sorted(f.code)];pos={i.address:n for n,i in enumerate(ins)}
  expected=[norm(i,images[0],pos)for i in ins]
  candidates=sorted(set(buckets.get(tuple(prefix(codes[0],images[0],e)),[])+raw_prefix_candidates(codes[0],images[0],codes[1],e)))
  matches=[]
  for n in candidates:
   try:
    g=_trace(codes[1],n,starts[1]);ni=[codes[1].at(a)for a in sorted(g.code)];np={i.address:k for k,i in enumerate(ni)}
    if[norm(i,images[1],np)for i in ni]==expected:matches.append((n,g,ni))
   except Exception:pass
  if e in targets:matches=[m for m in matches if m[0]==targets[e]]
  assert len(matches)==1,(hex(e),[hex(q[0])for q in matches])
  n,g,ni=matches[0];memory=[];calls=[];immediates=[]
  for old,new in zip(ins,ni):
   if old.mnemonic=='call'and old.operands[0].type==2:
    calls.append(dict(old_site=f'{old.address:08x}',u5_site=f'{new.address:08x}',old_target=f'{old.operands[0].imm:08x}',u5_target=f'{new.operands[0].imm:08x}'))
    if old.operands[0].imm in targets:assert targets[old.operands[0].imm]==new.operands[0].imm
    targets[old.operands[0].imm]=new.operands[0].imm
   for a,b in zip(old.operands,new.operands):
    if a.type==b.type==3 and a.mem.base==a.mem.index==b.mem.base==b.mem.index==0 and any(s.va<=a.mem.disp<s.va+s.vsize for s in images[0].sections)and any(s.va<=b.mem.disp<s.va+s.vsize for s in images[1].sections):
     assert a.size==b.size
     memory.append(dict(old_site=f'{old.address:08x}',u5_site=f'{new.address:08x}',old_va=f'{a.mem.disp:08x}',u5_va=f'{b.mem.disp:08x}',bytes=a.size,old_hex=images[0].read(a.mem.disp,a.size).hex(),u5_hex=images[1].read(b.mem.disp,b.size).hex()))
    elif a.type==b.type==2 and old.mnemonic not in('call','jmp'):
     if any(s.va<=a.imm<s.va+s.vsize for s in images[0].sections):
      immediates.append(dict(old_site=f'{old.address:08x}',u5_site=f'{new.address:08x}',old_va=f'{a.imm:08x}',u5_va=f'{b.imm:08x}'))
      if old.address==0x384c89:
       targets[int.from_bytes(images[0].read(a.imm+4,4),'little')]=int.from_bytes(images[1].read(b.imm+4,4),'little')
       vtables['reference_vtable']=dict(old=f'{a.imm:08x}',u5=f'{b.imm:08x}',old_raw16_hex=images[0].read(a.imm,16).hex(),u5_raw16_hex=images[1].read(b.imm,16).hex())
  for label,code,x,seq in(('old',codes[0],images[0],ins),('u5',codes[1],images[1],ni)):
   for i in seq:assert x.read(i.address,len(i.bytes))==i.bytes
   save(out/'instructions'/f'{e:08x}-{label}.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq).encode())
  rows.append(dict(old=f'{e:08x}',u5=f'{n:08x}',normalized_complete_body_and_internal_branch_equal=True,unique_full_body_match=True,rows=len(ins),calls=calls,memory=memory,image_address_immediates=immediates))
  print(json.dumps(dict(old=f'{e:08x}',u5=f'{n:08x}',rows=len(ins))),flush=True)
 mapping={r['old']:r['u5']for r in rows}
 for r in rows:
  for c in r['calls']:
   c['callee_proven_here']=c['old_target']in mapping
   if c['callee_proven_here']:assert c['u5_target']==mapping[c['old_target']]
 report=dict(schema='source-sound-capabilities-correspondence-v1',tool_sha256=sha(__file__),source_pins=PINS,anchor_report_sha256={str(anchor_path):sha(anchor_path)},inventory_sha256={str(p):sha(p)for p in paths},owners=rows,source_reference_vtable=vtables,
  scope='Complete original SDK CPU instruction/internal branch correspondence. Counter initializers and dynamic consumers are distinguished from natural device initialization. No device, saved setting, hardware register or platform capability result supplied.',production_edits=[],instruction_patches=[])
 save(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),owners=len(rows))))
 addresses={}
 for r in rows:
  for q in r['memory']:addresses[int(q['old_va'],16)]=int(q['u5_va'],16)
  for q in r['image_address_immediates']:addresses[int(q['old_va'],16)]=int(q['u5_va'],16)
 fields=dict(free_2d=0x38d130,free_3d=0x38d134,free_sge=0x38d138,pool_bytes=0x38d120,contiguous_bytes=0x38d124,fault=0x38d14c,critical_section=0x38d158,statistics_pointer=0x38d804,private_pool_pointer=0x38d74c,device_context_pointer=0x38d760)
 constants=dict(reference_vtable=0x41d724,apu_vtable=0x41d840,apu_interface_vtable=0x41d83c,sge_vtable=0x41d880,base_vtable=0x41d904,reference_add_method=0x38034c)
 addresses[0x38034c]=int(mapping['0038034c'],16)
 packet=dict(schema='source-sound-capabilities-providers-v1',paired_report_sha256=sha(out/'report.json'),
  bindings={k:dict(old=f'{v:08x}',u5=f'{addresses[v]:08x}',mutable_storage_not_image_initialization=True)for k,v in fields.items()},
  constructor_constants={k:dict(old=f'{v:08x}',u5=f'{addresses[v]:08x}',immutable_source_identity_not_host_handle=True)for k,v in constants.items()},
  reference_vtable=vtables['reference_vtable'],source_thread_IRQL_and_lock_not_defaulted=True)
 save(out/'providers.json',encoded(packet))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);run(p.parse_args().output)

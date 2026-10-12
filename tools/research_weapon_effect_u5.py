#!/usr/bin/env python3
"""Complete original weapon-effect packet/visibility owner byte/branch correspondence, no gameplay call."""
import argparse,csv,json
from pathlib import Path
from research_mp_inventory_switch_u5 import prefix,raw_prefix_candidates,norm
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,_Code,_trace
from oracle_smg_effect_owner_1_1 import sha,save,encoded
OWNERS=(0x107b50,0x106070,0x17cbd0,0x1781f0,0x177da0,0x17b1a0)
BOUNDARIES=()

def run(out):
 images=[Xbe(OLD),Xbe(NEW)];codes=[_Code(x,'.text')for x in images]
 paths=[ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']
 starts=[{int(q['address'],16)for q in csv.DictReader(p.open(),delimiter='\t')}for p in paths];buckets={}
 for e in sorted(starts[1]):buckets.setdefault(tuple(prefix(codes[1],images[1],e)),[]).append(e)
 rows=[]
 for e in OWNERS+BOUNDARIES:
  f=_trace(codes[0],e,starts[0]);ins=[codes[0].at(a)for a in sorted(f.code)];positions={i.address:n for n,i in enumerate(ins)};expected=[norm(i,images[0],positions)for i in ins]
  candidates=sorted(set(buckets.get(tuple(prefix(codes[0],images[0],e)),[])+raw_prefix_candidates(codes[0],images[0],codes[1],e)));matches=[]
  for n in candidates:
   g=_trace(codes[1],n,starts[1]);ni=[codes[1].at(a)for a in sorted(g.code)];np={i.address:k for k,i in enumerate(ni)}
   if [norm(i,images[1],np)for i in ni]==expected:matches.append((n,g,ni))
  assert len(matches)==1,(hex(e),[hex(q[0])for q in matches])
  n,g,ni=matches[0];memory=[];calls=[]
  for old,new in zip(ins,ni):
   if old.mnemonic=='call'and old.operands[0].type==2:calls.append(dict(old_site=f'{old.address:08x}',new_site=f'{new.address:08x}',old_target=f'{old.operands[0].imm:08x}',new_target=f'{new.operands[0].imm:08x}'))
   for a,b in zip(old.operands,new.operands):
    if a.type==b.type==3 and a.mem.base==a.mem.index==b.mem.base==b.mem.index==0 and a.mem.disp and b.mem.disp:
     assert a.size==b.size
     memory.append(dict(old_site=f'{old.address:08x}',new_site=f'{new.address:08x}',old_va=f'{a.mem.disp:08x}',new_va=f'{b.mem.disp:08x}',bytes=a.size,old_hex=images[0].read(a.mem.disp,a.size).hex(),new_hex=images[1].read(b.mem.disp,b.size).hex()))
  for tag,code,x,seq in(('old',codes[0],images[0],ins),('u5',codes[1],images[1],ni)):
   for i in seq:assert x.read(i.address,len(i.bytes))==i.bytes
   save(out/'instructions'/f'{e:08x}-{tag}.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq).encode())
  rows.append(dict(old=f'{e:08x}',u5=f'{n:08x}',normalized_complete_body_and_internal_branch_equal=True,unique_full_body_match=True,rows=len(ins),calls=calls,memory=memory))
  print(json.dumps(dict(old=f'{e:08x}',u5=f'{n:08x}',rows=len(ins))),flush=True)
 mapping={r['old']:r['u5']for r in rows}
 for r in rows:
  for c in r['calls']:
   c['callee_proven_here']=c['old_target']in mapping
   if c['callee_proven_here']:assert c['new_target']==mapping[c['old_target']]
 report=dict(schema='source-weapon-effect-u5-correspondence-v1',tool_sha256=sha(__file__),image_sha256={str(p):sha(p)for p in(OLD,NEW)},inventory_sha256={str(p):sha(p)for p in paths},owners=rows,
  scope='Static unchanged original complete body correspondence; call targets outside this owner set remain explicit. Only TU1.1 actual allocated player/item effect fixture execution is asserted; no TU1.5 effect execution or successful flash fabricated.',instruction_patches=[],new_gameplay_inputs=[])
 save(out/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(out/'report.json'),owners=len(rows))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();run(a.output)

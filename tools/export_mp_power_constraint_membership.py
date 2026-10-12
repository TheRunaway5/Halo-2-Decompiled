#!/usr/bin/env python3
"""Export original constraint membership and composed manager allocation prefix."""
import argparse,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_mp_power_constraint_membership_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from export_mp_power_contact_batches import SEMANTIC as BATCH_SEMANTIC

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 r=json.loads(a.report.read_text());assert r['oracle_sha256']==sha(ROOT/'tools/oracle_mp_power_constraint_membership_1_1.py')
 old_path=ROOT/'analysis/multiplayer/sandbox-source/power-contact-batches/final/measured/report.json';assert sha(old_path)=='3aac27adba706d7af4e78bf3f6d309f9bf654b49a24a93777a3fec3d397ee90a';old=json.loads(old_path.read_text())
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;semantic=set(BATCH_SEMANTIC)
 for listing in r['instruction_listings']:
  for line in(a.report.parent/listing['file']).read_text().splitlines():
   address,raw,*_=line.split();ins=next(md.disasm(bytes.fromhex(raw),int(address,16)))
   if ins.operands and ins.operands[0].type==X86_OP_MEM and ins.operands[0].access&CS_AC_WRITE:semantic.add(int(address,16))
 selected=[(r,c,None)for c in r['calls']]
 for c in old['calls']:
  if c['entry']!='0031f1b0':continue
  q=next(q for q in old['original_scheduler_call_tree']if q['entry']=='00322410'and q['caller']=='0031f232')
  q=dict(q);q['order']=next(e['order']for e in old['reads']if c['order']<e['order']<c['return_order']and 0x322410<=int(e['instruction'],16)<0x322420)
  selected.append((old,c,q))
 rows=[];audit=[]
 for source,c,boundary in selected:
  end=boundary['order']if boundary else c['return_order'];memory={}
  for p,b in full_snapshot(source['states'][c['before_state']]):add(memory,p,b)
  for q in c['packets']:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',e)for e in source['reads']if c['order']<e['order']<end]+[('W',e)for e in source['stores']if c['order']<e['order']<end],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],kind,e)
   if kind=='W':
    (writes if not e['source_stack']or int(e['instruction'],16)in semantic else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  after=dict(memory)
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']));assert e['source_stack']or logical==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  if not boundary:
   for p,b in full_snapshot(source['states'][c['after_state']]):
    for n,v in enumerate(b):
     if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
   for q in c['packets']:
    p=int(q['pointer'],16)
    for n,v in enumerate(bytes.fromhex(q['after_hex'])):
     if not(0x02000000<=p+n<0x02200000):assert after[p+n]==v,(c['id'],q['name'],n)
  name=f'call_{c["id"]}_{c["entry"]}';rows.append(' '.join(['C',name,c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax'],c['entry_esp'],c['x87_control_word'],c['mxcsr'],c['registers']['edx'],c['registers']['esi'],c['registers']['edi']]+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  if boundary:rows.append(' '.join(['B','0031f22d',boundary['entry'],boundary['registers']['ecx'],boundary['entry_esp'],'0','2']+boundary['stack16'][:2]))
  audit.append(dict(name=name,entry=c['entry'],input_bytes=len(memory),owning_writes=len(writes),excluded_abi_stores=len(ignored),retained_snapshot_correlated_bytes=matched,complete_original_call=boundary is None,required_owner=None if boundary is None else dict(instruction='0031f22d',entry=boundary['entry'],entry_esp=boundary['entry_esp'],receiver=boundary['registers']['ecx'],arguments=boundary['stack16'][:2])))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),prior_batch_report_sha256=sha(old_path),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False,callback_outputs_replayed=False)))
 print(json.dumps(dict(cases=len(audit),complete=sum(q['complete_original_call']for q in audit),required_stops=sum(not q['complete_original_call']for q in audit),owning_writes=sum(q['owning_writes']for q in audit),fixture=sha(fixture))))
if __name__=='__main__':main()

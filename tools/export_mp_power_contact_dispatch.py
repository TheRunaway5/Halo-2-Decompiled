#!/usr/bin/env python3
"""Original-only300870 empty call and retained guest packet-frame prefix."""
import argparse,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_mp_power_contact_batches_1_1 import ROOT,verify as verify_prior,sha
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-contact-dispatch'
MANIFEST=ROOT/'analysis/multiplayer/sandbox-source/power-contact-batches/final/manifest.json'
PIN='c4935c5e58f33f4692fa217ebb29b8f5c400505c825edd35ae2dabc68cd2ed22'
def verify():
 count=verify_prior();assert sha(MANIFEST)==PIN
 for p,h in json.loads(MANIFEST.read_text())['files'].items():assert sha(ROOT/p)==h,p
 return count,len(json.loads(MANIFEST.read_text())['files'])
from export_mp_power_gjk_owners import SEMANTIC as GJK_SEMANTIC
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'00300870'}
# These are all owning temporary stores from complete source listings. Saved
# registers/return arguments/SEH are excluded; actual read inputs remain literal.
SEMANTIC={0x300414,0x30041b,0x300667,0x30066b,0x3006fa,0x300709,0x300714,0x30071f,0x300726,0x300735,0x30073d,0x300773,0x30077b,0x300787,0x300795,0x3007a3,0x3007b1,0x3007bf,0x300837,0x300847,
 0x309273,0x3092ac,0x309389,0x3093b1,0x3093b6,0x3093ca,0x3093fc,0x309405,0x30940d,0x309415,0x30941f,
 0x309473,0x309488,0x309490,0x309582,0x309594,0x3095d9,0x309603,0x309645,
 0x31f1ca,0x31f1f4,0x31f20e,0x31f28b,0x31f2c9,0x31f2ce,0x31f2d4,0x31f2ec,0x31f319,0x31f321,0x31f329,0x31f332}
# The standalone constructor/destructor save this receiver only for SEH; the
# source temporary exception state/FS stores are ABI, not retained owner data.

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--phase');ap.add_argument('--call',type=int);a=ap.parse_args();verify()
 assert sha(a.report)=='3aac27adba706d7af4e78bf3f6d309f9bf654b49a24a93777a3fec3d397ee90a'
 r=json.loads(a.report.read_text());im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[];fs=set()
 for q in r['instruction_listings']:
  for line in(a.report.parent/q['file']).read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
 SEMANTIC.update(GJK_SEMANTIC)
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
 for listing in r['instruction_listings']:
  if listing['entry']not in SELECT|{'003001d0','00300260','00300400','00300100','00300650','00309210','00309450','0031f1b0','002e0ba0','0031feb0'}:continue
  for line in(a.report.parent/listing['file']).read_text().splitlines():
   address,raw,*rest=line.split();ins=next(md.disasm(bytes.fromhex(raw),int(address,16)))
   if ins.operands and ins.operands[0].type==X86_OP_MEM and ins.operands[0].access&CS_AC_WRITE and 'fs:'not in ins.op_str and int(address,16)not in {0x3001ec,0x300209,0x30027a,0x300293,0x3002c6,0x30092a,0x300942,0x300f82,0x300f98}:
    SEMANTIC.add(int(address,16))
 for c in r['calls']:
  if c['entry']not in SELECT or(a.phase and c['phase']!=a.phase)or(a.call is not None and c['id']!=a.call):continue
  boundary=None
  target='00300400'if c['registers']['ecx']=='0601ade0'else None
  if target:
   boundary=next(q for q in r['calls']if q['phase']==c['phase']and q['entry']==target and q['caller']=='00300a0c'and c['order']<q['order']<c['return_order'])
   boundary=dict(boundary,call_site='00300a07')

  end=boundary['order']if boundary else c['return_order']
  memory={}
  for p,b in full_snapshot(r['states'][c['before_state']]):add(memory,p,b)
  for q in c['packets']:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',e)for e in r['reads']if c['order']<e['order']<end]+[('W',e)for e in r['stores']if c['order']<e['order']<end],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],kind,e)
   if kind=='W':
    site=int(e['instruction'],16)
    ins=next(md.disasm(im.read(site,15),site))
    if 'fs:'in ins.op_str:fs.add(site)
    (writes if site not in fs and(not e['source_stack']or site in SEMANTIC)else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  after=dict(memory)
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']))
   assert e['source_stack']or logical==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  if not boundary:
   for p,b in full_snapshot(r['states'][c['after_state']]):
    for n,v in enumerate(b):
     if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax'],c['entry_esp'],c['x87_control_word'],c['mxcsr'],c['registers']['edx'],c['registers']['esi'],c['registers']['edi']]+c['stack16']))
  estimates=[]
  for e in writes:
   if e['instruction']=='0030bb1d' and e['size']==8 and int(e['address'],16)%16==0:estimates.append(dict(input_word=e['written_hex'][:8]))
   if e['instruction']=='0030bb26':assert estimates and 'output_word'not in estimates[-1];estimates[-1]['output_word']=e['written_hex']
  for q in estimates:
   assert 'output_word'in q;rows.append('Q '+bytes.fromhex(q['input_word'])[::-1].hex()+' '+bytes.fromhex(q['output_word'])[::-1].hex())
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  if boundary:
   frame=((int(boundary['entry_esp'],16)-4)&~15)-0x80
   rows.append(' '.join(['P',boundary['call_site'],boundary['entry'],boundary['entry_esp'],boundary['registers']['ecx'],f'{frame:08x}','00000080']))
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_abi_stack_and_fs_stores=len(ignored),retained_snapshot_correlated_bytes=matched,complete_original_call=boundary is None,required_original_owner=None if boundary is None else dict(call_id=boundary['id'],entry=boundary['entry'],instruction=boundary['call_site'],entry_esp=boundary['entry_esp'],receiver=boundary['registers']['ecx'])))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),emulator_estimate_injection_test_only=True,exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False,prefix_expectations='Literal original read/store timelines and whole real retained before/after buffers. Stop before original300400 prologue at its retained packet-frame dependency. No native expected data used.')))
 print(json.dumps(dict(cases=len(audit),complete=sum(q['complete_original_call']for q in audit),required_stops=sum(not q['complete_original_call']for q in audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()

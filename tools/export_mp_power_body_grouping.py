#!/usr/bin/env python3
"""Export measured grouping owners, with no native-derived expected data."""
import argparse,json
from pathlib import Path
from oracle_mp_power_body_grouping_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from export_mp_power_body_factory import PACKET_SITES,OUTPUT_ENTRIES
from export_mp_power_shape_wrapper import MATRIX_SITES,EXTENT_SITES,CONFIG_SITES
from oracle_smg_firing_1_1 import Xbe
SELECT={'001d9330','0020e370','00183500','00183770'}
# Source-listed owning sites, independently classified from the complete
# unchanged instruction bodies. Native implementation text is not an input.
NEW_SITES=set(int(x,16)for x in '''
143868 143876 14387b 143881 1438c4 1438cc 1438d5 1438df 1438e4 1438ea
1438f0 1438f4 1438f9 143900 143906 14390d
20e3e9 20e3f5 20e408 20e40e 20e412 20e416 20e419 20e41c
1837bf 1837c5 1837fa 183828 18353e 183578 183580 18358c 18360f 183612
1d9363 1d93af 1d93b3 1d93b7 1d93bb 1d93bf 1d93c3 1d93c7 1d93cb
1d945a 1d94a6 1d94c2 1d9525 1d9544 1d95ac
1d963d 1d9645 1d9649 1d964e 1d98d6 1d98dd 1d98e1 1da0fd 1da105 1da14a
1d9933 1d993a 1d9958 1d9961 1d996b 1d9972 1d997a 1d9985 1d9994 1d999f 1d99ac 1d9f32
1d9678 1d969e 1d96ad 1d96d3 1d96dc 1d96e2 1d96e8 1d96fd 1d9703 1d9709 1d970f
1d9724 1d972a 1d9730 1d9736 1d9741 1d974c 1d9757 1d975d 1d9768 1d9794 1d979f 1d97ad
1d9812 1d9857 1d986a 1d9871 1d9877 1d9880 1d988a
1da4ca 1da4ce 1da527 1da52c 1da50d 1da517 1da547 1da55b
183684 18368b 1836e1 1836ef 183739 183733
1db201 1eead7 1eeae4 1eeb11 1eeb7f 1eeb8b 1eeb97 1eeba3 1eebaf 1eebbb 1db228 1db843
'''.split())
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 r=json.loads(a.report.read_text());image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audits=[];fs=set();outputsites=set()
 for q in r['instruction_listings']:
  for line in(a.report.parent/q['file']).read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
   if int(q['entry'],16)in OUTPUT_ENTRIES:
    parts=line.split(' ',3);dest=parts[-1].split(',')[0]
    if '['in dest and '[esp'not in dest and '[ebp'not in dest:outputsites.add(int(parts[0],16))
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  children=[q for q in r['calls']if c['order']<=q['order']and q['return_order']<=c['return_order']]
  def child(entry,caller=None):
   q=[q for q in children if q['entry']==entry and(caller is None or q['caller']==caller)]
   return q[0]if q else None
  def packet(q,name):
   if q is None:return 0
   b=[p for p in q['packets']if p['name']==name];assert len(b)<=1
   return int(b[0]['pointer'],16)if b else 0
  rootframe=packet(c,'owning_frame')if c['entry']=='001d9330'else 0
  sf=packet(child('001d95e0'),'owning_frame');cf=packet(child('001d98b0'),'owning_frame')
  rc=child('00183770');ro=packet(rc,'region_output');po=packet(rc,'permutation_output')
  df=packet(child('001db1c0'),'owning_frame');stride=packet(child('001eead0'),'iterator_stride_scratch')
  def owned(entry):
   q=child(entry);return int(q['registers']['ecx'],16)if q else 0
  info=owned('00317890');quat=owned('001dc750')
  fc=[q for q in r['factory_calls']if q['phase']==c['phase']]
  def bounds(caller):
   if not child('001daac0'):return 0
   q=[q for q in fc if q['entry']=='00303e30'and q['caller']==caller];assert len(q)<=1
   return int(q[0]['output_pointer'],16)if q else 0
  ext=child('00184510');calc=child('00184800');config=child('002e25a0')
  scratch=[rootframe,sf,cf,ro,po,df,stride,info,quat,bounds('0031abf4'),bounds('00317d30'),
   int(ext['registers']['ecx'],16)if ext else 0,int(ext['registers']['edx'],16)if ext else 0,
   int(calc['stack16'][2],16)if calc else 0,int(config['registers']['ecx'],16)if config else 0]
  spans=[(info,0xb8),(quat,16),(scratch[9],32),(scratch[10],32),
   (scratch[11],4),(scratch[12],4),(scratch[13],0x40),(scratch[14],0x14)]
  def included(e):
   site=int(e['instruction'],16);p=int(e['address'],16)
   if not e['source_stack']:return site not in fs
   if site in NEW_SITES:return True
   produced=site in PACKET_SITES|MATRIX_SITES|EXTENT_SITES|CONFIG_SITES|outputsites or site==0x1dc7b5
   return produced and any(at and at<=p and p+e['size']<=at+n for at,n in spans)
  memory={}
  for p,b in full_snapshot(r['states'][c['before_state']]):add(memory,p,b)
  for q in c['packets']:
   if 'captured_at'not in q:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);writes=[];ignored=[];events=sorted([('R',e)for e in r['reads']if c['order']<e['order']<c['return_order']]+[('W',e)for e in r['stores']if c['order']<e['order']<c['return_order']],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],kind,e)
   if kind=='W':
    (writes if included(e)else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  # Scratch is declared input at the first original semantic owner. Excluded
  # ABI pushes/spills may precede that owner; neither result nor pose is read
  # back into expected input. This is the same measured alias rule as factory.
  touched=set()
  for e in writes:
   if not e['source_stack']:continue
   p=int(e['address'],16)
   for n,v in enumerate(bytes.fromhex(e['before_hex'])):
    if p+n not in touched:memory[p+n]=v;touched.add(p+n)
  for p in(0x41b7f0,0x41b8f0,0x45d188,0x418930):add(memory,p,image.read(p,0x60))
  add(memory,0x1da4a0,image.read(0x1da4a0,0x24));add(memory,0x1da4b4,image.read(0x1da4b4,0x20))
  after=dict(memory);alias_deltas=[]
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']))
   if logical!=bytes.fromhex(e['before_hex']):
    assert e['source_stack'],(c['id'],e)
    alias_deltas.append(dict(instruction=e['instruction'],address=e['address'],source_before_hex=e['before_hex'],earlier_source_semantic_bytes_hex=logical.hex(),scope='Temporary stack address reused after excluded ABI pushes/spills; store-before comparison excluded only for temporary scratch.'))
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  for p,b in full_snapshot(r['states'][c['after_state']]):
   for n,v in enumerate(b):
    if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
  estimates=[]
  math=[x for x in r['math']if c['order']<x['order']<c['return_order']and x['instruction']in('001dc782','001dc786')]
  for x,y in zip(math[::2],math[1::2]):
   assert x['instruction']=='001dc782'and y['instruction']=='001dc786';estimates.append((int(x['xmm']['0'][-8:],16),int(y['xmm']['1'][-8:],16)))
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry']]+[c['registers'][n]for n in('eax','ecx','esi','edi')]+[c['returned_eax'],c['x87_control_word'],c['mxcsr']]+[f'{v:08x}'for v in scratch]+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  for operand,result in estimates:rows.append(f'Q {operand:08x} {result:08x}')
  factory_args=[q['stack16'][:14]for q in children if q['entry']=='001daac0']
  for words in factory_args:rows.append('B '+' '.join(words))
  audits.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_abi_stack_and_fs_stores=len(ignored),retained_snapshot_matched_bytes=matched,scratch=scratch,rsqrtss_estimates=estimates,temporary_abi_alias_deltas=alias_deltas,complete_factory_call_words=factory_args))
 f=a.output/'fixtures.tsv';frozen_write(f,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(f),cases=audits,native_expected_data_used=False,source_data_admission_or_results_supplied=False,temporary_scratch_input='Actual bytes at first owning source store; ABI call plumbing excluded; retained backing entirely original.')))
 print(json.dumps(dict(cases=len(audits),stores=sum(x['stores']for x in audits),fixture=sha(f))))
if __name__=='__main__':main()

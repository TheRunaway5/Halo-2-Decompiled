#!/usr/bin/env python3
"""Export complete initial controller construction and biped component rebuild.

Caller scratch and platform estimates are independently observed original
inputs. Allocator, shape, motion, body and component effects run natively.
No recorded body, allocation, pose or successful lower-owner effects are used.
"""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import sha,spans
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN
SELECT={'001da6f0','001d2b10'}
def controller_scratch(c,extra):
 children=[q for q in extra if c['order']<=q['order']<c['return_order']]
 def child(e,caller=None):return next(q for q in children if q['entry']==e and(caller is None or q['caller']==caller))
 frame=child('001da6f0');ext=child('00184510');calc=child('00184800')
 return [int(frame['entry_esp'],16),int(child('00317890')['registers']['ecx'],16),int(child('001dc750','001dab35')['registers']['ecx'],16),int(child('002e4dd0','0031abf4')['stack16'][2],16),int(child('002e4dd0','00317d30')['stack16'][2],16),int(ext['registers']['ecx'],16),int(ext['registers']['edx'],16),int(calc['stack16'][2],16),int(child('002e25a0')['registers']['ecx'],16)]
def load(path):
 context=json.loads(path.read_text());assert context['source_sha256']==PIN and context['cache_sha256']==CACHE_PIN
 assert context['oracle_sha256'] in {sha(Path(__file__).with_name(n)) for n in ('oracle_mp_biped_controller_1_1.py','oracle_mp_death_membership_1_1.py','oracle_mp_death_removal_1_1.py')}
 assert context['entire_frozen_moving_death_byte_identical']and not context['instruction_patches']and not context['new_supplied_gameplay_or_physics_results']
 original=ROOT/context['source_report'];assert sha(original)==context['source_report_sha256'];j=json.loads(original.read_text())
 assert j['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_spartan_death_physical_1_1.py'))
 return context,j,original
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();ctx,j,original=load(a.report)
 events=sorted([(q['order'],'R',q)for q in j['reads']]+[(q['order'],'W',q)for q in j['stores']]);orders=[q[0]for q in events]
 fs=set()
 for listing in j['instructions']:
  p=original.parent/listing['file'];assert sha(p)==listing['sha256'];fs.update(int(line.split()[0],16)for line in p.read_text().splitlines()if 'fs:[0]'in line)
 rows=[];audit=[]
 for c in j['calls']:
  if c['entry']not in SELECT:continue
  floating=next(q for q in ctx['controller_calls']if q['entry']==c['entry']and q['entry_esp']==c['entry_esp'])
  first={};current={};writes=[]
  for _,kind,q in events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]:
   p=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
   for i,v in enumerate(raw):
    if p+i not in current:first[p+i]=current[p+i]=v
    assert current[p+i]==v
   if kind=='W':
    if not(0x2000000<=p<0x2200000)and int(q['instruction'],16)not in fs:writes.append(q)
    for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[p+i]=v
  rows.append(' '.join(['C',str(c['id']),c['entry']]+[c['registers'][k]for k in('eax','ecx','ebx','esi','edi')]+[floating['fpcw'],floating['mxcsr'],c['returned_eax']]+c['stack16']))
  rows.append('S '+' '.join(f'{v:08x}'for v in(int(c['entry_esp'],16),0,0,0,0,0,0,0,0,0,0)))
  rows.append('H '+' '.join(f'{v:08x}'for v in controller_scratch(c,ctx['controller_calls'])))
  for p,raw in spans(first):rows.append(f'M {p:08x} {raw.hex()}')
  out={}
  for q in writes:
   for i,v in enumerate(bytes.fromhex(q['written_hex'])):out[int(q['address'],16)+i]=v
  for p,raw in spans(out):rows.append(f'A {p:08x} {raw.hex()}')
  for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
  estimates=[q for q in ctx['math']if c['order']<q['order']<c['return_order']]
  for q in estimates:rows.append('V '+q['operand_word']+' '+q['result_word'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(first),output_bytes=len(out),stores=len(writes),rsqrt_estimates=len(estimates)))
 assert len(audit)==4
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_context_report_sha256=sha(a.report),source_report_sha256=sha(original),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n');print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()

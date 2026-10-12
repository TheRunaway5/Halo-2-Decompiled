#!/usr/bin/env python3
"""Export unchanged original ragdoll owners from actual constructor execution.

Inputs are first-touch original reads/store-before bytes. Expected stores come
only from original execution. Temporary ABI plumbing is excluded by the actual
owner's output spans; no native code or native results are exporter inputs.
"""
import argparse,bisect,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def spans(bank):
 if not bank:return
 addresses=sorted(bank);start=last=addresses[0];raw=bytearray([bank[start]])
 for p in addresses[1:]:
  if p!=last+1:yield start,bytes(raw);start=p;raw=bytearray()
  raw.append(bank[p]);last=p
 yield start,bytes(raw)
SELECT={'001d9f50','001da020','001db8e0','001d20f0','001d1ce0',
 '00318e80','003198c0','003192b0','00319af0','001d3b40','001db1c0','001d9330'}
GROUP_SITES={0x1da0fd,0x1da105,0x1da10d,0x1da118,0x1da14a,0x1da35d}
def grouping_scratch(c,children,root_stores):
 def child(e,caller=None):return next(q for q in children if q['entry']==e and(caller is None or q['caller']==caller))
 def stack_site(e):return next(int(q['address'],16)for q in root_stores if q['instruction']==e)
 frames=child('001db8e0');stride=child('001eeb70');ext=child('00184510');calc=child('00184800')
 return [int(c['owning_frame']['pointer'],16),stack_site('001d963d')-0x1c,
  int(child('001d9f50')['stack16'][2],16)-0x260,stack_site('001837bf'),stack_site('001837c5'),
  int(frames['registers']['eax'],16)-0x40,int(stride['registers']['ecx'],16),
  int(child('00317890')['registers']['ecx'],16),int(child('001dc750')['registers']['ecx'],16),
  int(child('002e4dd0','0031abf4')['stack16'][2],16),int(child('002e4dd0','00317d30')['stack16'][2],16),
  int(ext['registers']['ecx'],16),int(ext['registers']['edx'],16),int(calc['stack16'][2],16),
  int(child('002e25a0')['registers']['ecx'],16)]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text())
 if 'rebuild_calls'in j:
  assert j['frozen_prior_byte_identical'] and j['failure'] is None and j['capture_entry']=='000e2520'
  assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_death_physics_ticks_1_1.py'))
  j=dict(j,calls=j['rebuild_calls'],reads=j['rebuild_reads'],stores=j['rebuild_stores'],entire_frozen_moving_death_byte_identical=True)
 else:assert j['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_spartan_ragdoll_constructor_1_1.py'))
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
 assert j['entire_frozen_moving_death_byte_identical'] and not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
 events=sorted([(r['order'],'R',r)for r in j['reads']]+[(r['order'],'W',r)for r in j['stores']]);orders=[r[0]for r in events]
 fs=set()
 for entry in j['instructions']:
  p=a.report.parent/entry['file'];assert sha(p)==entry['sha256']
  for line in p.read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
 rows=[];audit=[]
 for c in j['calls']:
  if c['entry']not in SELECT:continue
  args=[int(x,16)for x in c['stack16']];regs={k:int(v,16)for k,v in c['registers'].items()}
  entry=int(c['entry'],16)
  def owned(r):
   site=int(r['instruction'],16);p=int(r['address'],16)
   if entry==0x1d9f50:return site in(0x1d9f9e,0x1d9ff8)
   if entry==0x1da020:return site in GROUP_SITES
   if entry==0x1db8e0:return any(lo<=p and p+r['size']<=lo+n for lo,n in((regs['eax'],64),(args[3],64),(args[4],4),(args[5],4)))
   if entry in(0x1d20f0,0x1d1ce0):return regs['eax']<=p and p+r['size']<=regs['eax']+0x80
   if entry in(0x318e80,0x3198c0):return regs['ecx']<=p and p+r['size']<=regs['ecx']+0x8c
   return not(0x2000000<=p<0x2200000) and site not in fs
  initial={};current={};writes=[]
  for _,kind,r in events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]:
   at=int(r['address'],16);raw=bytes.fromhex(r['raw_hex']if kind=='R'else r['before_hex'])
   for i,v in enumerate(raw):
    if at+i not in current:initial[at+i]=current[at+i]=v
    assert current[at+i]==v,(c['id'],kind,r)
   if kind=='W':
    if owned(r):writes.append(r)
    for i,v in enumerate(bytes.fromhex(r['written_hex'])):current[at+i]=v
  # Only bytes actually assigned by this owner are expected outputs. All
  # unassigned retained bytes are still input and are checked by the test.
  output={}
  for r in writes:
   at=int(r['address'],16)
   for i,v in enumerate(bytes.fromhex(r['written_hex'])):output[at+i]=v
  rows.append(' '.join(['C',str(c['id']),c['entry']]+[c['registers'][k]for k in('eax','ecx','edx')]+[c['fpcw'],c['mxcsr']]+c['stack16']))
  if entry in(0x1db1c0,0x1d9330):
   children=[q for q in j['calls']if c['order']<q['order']<c['return_order']]
   frames=next(q for q in children if q['entry']=='001db8e0')
   stride=next(q for q in children if q['entry']=='001eeb70')
   rows.append(f'D {int(frames["registers"]["eax"],16)-0x40:08x} '+stride['registers']['ecx'])
   if entry==0x1d9330:
    # Stack stores are excluded from root expected results but they remain
    # original observations for identifying the caller's calculation packets.
    root_stores=[q for q in j['stores']if c['order']<q['order']<c['return_order']]
    scratch=grouping_scratch(c,children,root_stores)
    rows.append('G '+' '.join(f'{x:08x}'for x in scratch))
    rows.append('T '+c['returned_eax'])
    for q in children:
     if q['entry']=='001daac0':rows.append('B '+' '.join(q['stack16'][:14]))

  for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
  for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
  for r in writes:rows.append('W '+r['instruction']+' '+r['address']+' '+r['before_hex']+' '+r['written_hex'])
  estimates=[r for r in j.get('math',[])if c['order']<r['order']<c['return_order']]
  for r in estimates:rows.append('Q '+r['operand_word']+' '+r['result_word'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(initial),output_bytes=len(output),stores=len(writes),rsqrt_estimates=len(estimates)))
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
 a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(exporter_sha256=sha(__file__),source_report_sha256=sha(a.report),
  fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n')
 print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()

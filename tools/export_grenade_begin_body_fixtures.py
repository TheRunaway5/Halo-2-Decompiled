#!/usr/bin/env python3
"""Export complete admitted grenade owner/callback buffers, no asset copies."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,ROOT,PIN
def number(v):return int(v,16)
def words(v):return ' '.join(f'{x:x}'for x in v)
def arguments(c):
 e=number(c['entry']);r={k:number(v)for k,v in c['entry_cpu']['registers'].items()};s=[number(v)for v in c['stack24_words']]
 sx=lambda v:struct.unpack('<h',struct.pack('<H',v&65535))[0]&0xffffffff
 if e==0x1055f0:return[r['ecx']]
 if e==0x112270:return[s[0]]
 if e==0xcf440:return[r['eax'],sx(r['ecx'])]
 if e==0xefee0:return[r['ecx'],s[0]]
 if e==0xe7ff0:return[r['ecx'],r['edi']]
 if e==0xe7ef0:return[s[0],r['ecx']]
 if e==0x115650:return[s[0],s[1],s[2],r['ecx']&255]
 if e==0x1ca8d0:return[r['eax']]
 if e==0x148160:return[s[0]]
 if e==0xef630:return[r['eax']]
 if e==0xefe30:return[r['ecx'],r['eax']]
 if e==0xc9a40:return[r['eax'],s[0]]
 if e==0x1781f0:return[s[0],r['edx'],r['ebx'],s[1],s[2],s[3]]
 if e==0xbcd80:return[r['ecx']]
 if e==0xa99f0:return[s[0]]
 raise AssertionError(hex(e))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=Path);p.add_argument('output',type=Path);a=p.parse_args();r=json.loads(a.report.read_text());composition=json.loads((a.report.parent/'composition/report.json').read_text())
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(image.data).hexdigest()==PIN
 zero=struct.unpack('<I',image.read(0x465048,4))[0]
 blocked_path=ROOT/'analysis/multiplayer/integration/grenade-action/repeat/report.json';blocked=json.loads(blocked_path.read_text())
 sites={int(line.split()[0],16)+len(bytes.fromhex(line.split()[1]))for line in(a.report.parent/'composition/instructions/000e95e0.asm').read_text().splitlines()if line.split()[2]=='call'}
 groups=[(case,composition)for case in r['body']]
 for c in blocked['calls']:
  if c['entry']!='000e95e0':continue
  services=[s for s in blocked['calls']if c['order']<s['order']<c['return_order']and number(s['caller'])in sites]
  groups.append((dict(call=c,direct_services=services,before=blocked['states'][c['before_state']],after=blocked['states'][c['after_state']]),blocked))
 lines=['# complete-original-grenade-begin-v1','# report-sha256 '+hashlib.sha256(a.report.read_bytes()).hexdigest(),'# blocked-report-sha256 '+hashlib.sha256(blocked_path.read_bytes()).hexdigest(),'ZERO '+f'{zero:x}']
 for row in composition['source_metadata']['records']:lines.append('DEFINITION '+f'{row["index"]:x}'+' '+row['raw2c_hex'])
 total_stores=0
 for case,source in groups:
  c=case['call'];unit=number(c['stack24_words'][0]);packet=c['packets'][0];reqptr=number(packet['pointer']);req=packet['raw_hex'];ctor=[s for s in source['stores']if s['order']<c['order']and 0x151515<=number(s['instruction'])<=0x15153c and reqptr<=number(s['address'])<reqptr+32][-9:]
  assert len(ctor)==9
  before=bytearray.fromhex(req)
  for st in reversed(ctor):off=number(st['address'])-reqptr;before[off:off+st['size']]=bytes.fromhex(st['before_hex'])
  lines.append('CASE '+words([unit,number(c['return_cpu']['registers']['eax'])&255]));lines+=['REQUEST '+req,'REQUEST_BEFORE '+before.hex()]
  for st in ctor:lines.append('REQUEST_STORE '+words([number(st['instruction']),number(st['address'])-reqptr,st['size'],int.from_bytes(bytes.fromhex(st['before_hex']),'little'),int.from_bytes(bytes.fromhex(st['written_hex']),'little')]))
  for name,state in [('BEFORE',case['before']),('AFTER',case['after'])]:
   for obj in state['objects']:lines.append(name+' '+obj['datum']+' '+obj['body_hex'])
  for leaf in case['direct_services']:
   args=arguments(leaf);cpu=leaf['return_cpu'];output='-'
   if number(leaf['entry'])==0x115650:output=next(p['raw_hex']for p in leaf['after_packets']if p['field']=='stack3_selected_original_channel_output4')
   lines.append('LEAF '+words([number(leaf['entry']),len(args)]+args+[number(cpu['registers']['eax']),number(cpu['xmm128'][0][-8:])])+' '+output)
   for name,field in [('LBEFORE','before_state'),('LAFTER','after_state')]:
    for obj in source['states'][leaf[field]]['objects']:lines.append(name+' '+obj['datum']+' '+obj['body_hex'])
   lines.append('END_LEAF')
  action=number(case['before']['grenade']['action_pointer']);stores=[s for s in source['stores']if c['order']<s['order']<c['return_order']and 0xe95e0<=number(s['instruction'])<=0xe99a1 and action<=number(s['address'])and number(s['address'])+s['size']<=action+0x44]
  for st in stores:lines.append('STORE '+words([number(st['instruction']),number(st['address'])-action,st['size'],int.from_bytes(bytes.fromhex(st['before_hex']),'little'),int.from_bytes(bytes.fromhex(st['written_hex']),'little')]))
  total_stores+=len(stores);lines.append('END_CASE')
 data=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==data
 else:a.output.write_bytes(data)
 print(json.dumps(dict(cases=len(groups),direct_stores=total_stores,callbacks=sum(len(c['direct_services'])for c,s in groups),sha256=hashlib.sha256(data).hexdigest())))
if __name__=='__main__':main()

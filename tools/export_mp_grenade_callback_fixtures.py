#!/usr/bin/env python3
"""Export measured original callback/state/constructor request fixtures."""
import argparse,json,struct
from pathlib import Path
from oracle_mp_grenade_callbacks_1_1 import ROOT,OUT,motion
from oracle_smg_firing_1_1 import Xbe

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,default=OUT/'measured/report.json');p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 report=json.loads(a.report.read_text());rows=[]
 original=report['original_begin'];s=report['states'][original['rows'][0]['before_state']]
 rows.append('I source_C52C0_BD270 '+s['grenade']['action44_hex'])
 selected=next(r for r in report['calls'] if r['entry']=='00115650' and r['phase'].endswith('phase_1_player'))
 rows.append('B actual_channel219 '+report['states'][selected['before_state']]['grenade']['action44_hex']+' '+report['states'][original['stages'][-1]['after_state']]['grenade']['action44_hex']+' 0 8')
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 one=image.read(0x465030,4).hex();epsilon=image.read(0x46504c,4).hex()
 def word(raw):return f'{struct.unpack("<I",bytes.fromhex(raw))[0]:08x}'
 for case in report['cases']:
  before=report['states'][case['before_state']];after=report['states'][case['after_state']]
  calls=report['calls'][slice(*case['ranges']['calls'])]
  b,a2=before['grenade'],after['grenade']
  inventory=next(o for o in before['objects']if o['datum']=='e26f0000');body=bytes.fromhex(inventory['body_hex']);remote=struct.unpack_from('<I',body,0x12c)[0]
  prepare=any(r['entry']=='000e8d60'for r in calls)
  release=any(r['entry']=='000e8f30'for r in calls)
  animation=[r for r in calls if r['entry']=='00115800']
  kind='P'if prepare else 'R'if release else 'A'if animation else 'C'
  replies=','.join(f'{r["entry_cpu"]["registers"]["edi"]}:{int(r["return_cpu"]["registers"]["eax"],16)&255}'for r in animation)or '-'
  rows.append(' '.join(['S',case['name'],case['entry'],b['action44_hex'],a2['action44_hex'],str(b['current']),','.join(map(str,b['counts'])),','.join(map(str,a2['counts'])),f'{remote:08x}',kind,replies,str(case['returned_al'])]))
  if case['invalid'] and case['invalid'].get('explicit_source_allocation_request_boundary'):
   initialized=next(r for r in calls if r['entry']=='000b8820')['after_packets'][0]['raw_hex']
   descriptor=case['invalid']['descriptor_c4_hex'];look=body[0x168:0x174].hex();point=bytes.fromhex(descriptor)[0x1c:0x28].hex()
   rows.append(' '.join(['D',case['name'],initialized,descriptor,look,point,word(one),word(epsilon)]))
 motion.frozen_write(a.output,('\n'.join(rows)+'\n').encode());print(json.dumps(dict(rows=len(rows),sha256=motion.digest(a.output))))
if __name__=='__main__':main()

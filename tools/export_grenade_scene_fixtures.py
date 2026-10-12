#!/usr/bin/env python3
"""Export literal original grenade consumer outputs; never computes expected poses."""
import argparse,json,struct,hashlib
from pathlib import Path

def words(data):return struct.unpack('<'+'I'*(len(data)//4),data)
p=argparse.ArgumentParser();p.add_argument('report',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
assert not a.output.exists();r=json.loads(a.report.read_text());a.output.mkdir(parents=True)
lines=[]
for i,c in enumerate(r['cases']):
 before=bytes.fromhex(c['before_hex']);pose=bytes.fromhex(c['pose_after_hex']);after=bytes.fromhex(c['after_hex'])
 assert not any(x['kind']=='unmapped' for x in c['boundary'])
 node=struct.unpack_from('<h',pose,0x116)[0]
 row=[i,c['image'],int(c['definition'],16),*words(before[0x64:0x88]),*words(before[0xa0:0xa4]),*words(pose[node:node+52]),*words(after[0x30:0x40]),pose[0xb1],*pose[0x200:0x20a],int(c['bounds_returned'])]
 lines.append(' '.join(f'{v:x}' for v in row))
(a.output/'fixtures.tsv').write_text('\n'.join(lines)+'\n')
(a.output/'meta.json').write_text(json.dumps(dict(report=str(a.report.resolve()),sha256=hashlib.sha256(a.report.read_bytes()).hexdigest(),rows=len(lines),columns='index,image,definition,position3,forward3,up3,scale,node13,sphere4,variant_u8,region10,bounds_returned'),sort_keys=True,indent=2)+'\n')

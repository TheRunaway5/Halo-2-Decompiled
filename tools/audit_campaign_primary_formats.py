#!/usr/bin/env python3
"""Inspect authored opening object-animation requests and their actual resources.

This checks static requests and raw codec/layout admission, not scene playback,
sampling results, effects, GPU pixels or converted MCC acceptance.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--map',type=Path,required=True)
p.add_argument('--authored',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
if a.output.exists():raise ValueError('Refusing to overwrite audit evidence')
with a.map.open('rb') as f:pin=hashlib.file_digest(f,'sha256').hexdigest()
r=json.loads(a.authored.read_text())
if r['map_sha256']!=pin:raise ValueError('Authored inventory belongs to a different map')
requests=set();unresolved=[]
def scan(node):
    call=node.get('call','')
    if call.startswith('scenery_animation_start') or call in ('custom_animation_relative','custom_animation_relative_loop'):
        args=node['arguments']
        if len(args)>2 and 'bits'in args[1] and 'bits'in args[2]:
            requests.add((int(args[1]['bits'],16),int(args[2]['bits'],16)))
        else:unresolved.append(node)
    for child in node.get('arguments',[]):scan(child)
for script in r['scripts']:scan(script['root'])
rows=[]
with a.map.open('rb') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as data:
    u=lambda at:struct.unpack_from('<I',data,at)[0]
    index=u(16);table=u(index+8)-(u(index)-(index+32))
    magic=u(table+8)-(index+u(20))
    tags={u(table+i*16+4):table+i*16 for i in range(u(index+24))}
    for graph,sid in sorted(requests):
        if graph not in tags:raise ValueError('Requested graph is absent')
        root=u(tags[graph]+8)-magic;clips=u(root+0x30)-magic
        clip=next((clips+i*0x6c for i in range(u(root+0x2c))if u(clips+i*0x6c)==sid),None)
        if clip is None:raise ValueError('Static request requires its nonlocal selection owner')
        sf,af,movement,unknown04,static,unknown08,animated=struct.unpack_from('<bbhhhii',data,clip+0x3c)
        group=u(root+0xb0)-magic+u(clip+0x24)*20;location=u(group+8)
        start=location+u(clip+0x28);flags=location>>30
        codecs=[data[start]if static and not flags else None,
            data[start+static]if animated and not flags else None]
        count=u(root+0xc);mask=((count+31)//32)*4
        supported=(data[clip+0x10]==0 and data[clip+0x13]==count and sf==af==mask*3 and
            not movement and not unknown04 and not unknown08 and not u(clip+0x64) and not flags and
            (not static or codecs[0]==1) and (not animated or codecs[1]in(3,4,5,6,7)))
        rows.append(dict(graph=f'{graph:08x}',sid=f'{sid:08x}',clip_index=(clip-clips)//0x6c,
            nodes=count,frames=struct.unpack_from('<H',data,clip+0x14)[0],codecs=codecs,
            resource_flags=flags,raw_primary_reader_admitted=supported))
report=dict(scope=__doc__,map_sha256=pin,authored_sha256=hashlib.sha256(a.authored.read_bytes()).hexdigest(),
    tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),requests=len(requests),
    admitted=sum(x['raw_primary_reader_admitted']for x in rows),unresolved=unresolved,rows=rows)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
print(report['admitted'],'/',report['requests'],'static object-animation resource formats admitted')

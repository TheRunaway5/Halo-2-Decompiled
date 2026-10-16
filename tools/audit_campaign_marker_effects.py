#!/usr/bin/env python3
"""Retain actual opening marker-effect scripts and all their authored parts.

This is a static cache inventory. It does not allocate effects, advance event
clocks/RNG, produce current poses, or establish renderer/audio acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_rendering import Cache, file_sha

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--map',type=Path,required=True)
    p.add_argument('--authored',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists():raise ValueError('Refusing to overwrite authored evidence')
    authored=json.loads(a.authored.read_text());pin=file_sha(a.map)
    if authored['map_sha256']!=pin:raise ValueError('Authored scripts belong to a different map')
    c=Cache(a.map)
    try:
        def sid(value):
            return c.string(c.u32(368)+c.u32(c.u32(364)+(value&65535)*4),256)
        def tag(value,group=None):
            if group is None:group=c.tags[value]['group']
            t,at=c.tag(value,group)
            return dict(datum=f'{value:08x}',group=t['group'],name=t['name']),at
        def floats(at,count):return list(struct.unpack_from('<'+'f'*count,c.data,at))
        _,scenario=c.tag(c.scenario,'scnr');names=c.block(scenario+0x48,0x24)
        graphs={};requests=[]
        def graph(value):
            key=f'{value:08x}'
            if key in graphs:return
            definition,at=tag(value,'effe');locations=[c.u32(x)for x in c.block(at+0xc,4)]
            row=dict(definition=definition,flags=c.u32(at),loop=struct.unpack_from('<h',c.data,at+4)[0],
                locations=[dict(sid=f'{x:08x}',name=sid(x))for x in locations],events=[])
            graphs[key]=row
            for event in c.block(at+0x14,0x38):
                out=dict(skip=floats(event+4,1)[0],delay=floats(event+8,2),duration=floats(event+16,2),parts=[],systems=[])
                row['events'].append(out)
                for part in c.block(event+0x18,0x38):
                    reference=c.u32(part+0x10)
                    if reference==0xffffffff:continue
                    definition,root=tag(reference)
                    item=dict(definition=definition,environment=c.u16(part),mode=c.u16(part+2),
                        location=struct.unpack_from('<h',c.data,part+4)[0],flags=c.u16(part+6),
                        scales=[c.u32(part+0x30),c.u32(part+0x34)])
                    if definition['group']=='ligh':
                        lens=c.u32(root+0x94)
                        item['lens']=tag(lens,'lens')[0]if lens!=0xffffffff else None
                    out['parts'].append(item)
                    if definition['group']=='effe':graph(reference)
                for system in c.block(event+0x30,0x38):
                    definition,root=tag(c.u32(system+4))
                    out['systems'].append(dict(definition=definition,location=c.u16(system+8),
                        coordinates=c.u16(system+0xc),environment=c.u16(system+0xe),
                        camera=c.u16(system+0x12),flags=c.u16(system+0x16),emitters=c.u32(system+0x30)))
                    if definition['group']=='prt3' and c.u32(root+0x74)==0x65666665 and c.u32(root+0x78)!=0xffffffff:
                        graph(c.u32(root+0x78))
        def scan(expr,script):
            if expr.get('call')=='effect_new_on_object_marker':
                args=expr['arguments']
                if len(args)!=3 or any('bits'not in x for x in args):
                    raise ValueError('Computed marker effect requires its runtime argument owner')
                effect,obj,marker=[int(x['bits'],16)for x in args];name=names[obj&65535]
                kind,placement=struct.unpack_from('<hh',c.data,name+0x20)
                if kind!=6:raise ValueError('Opening marker owner is no longer scenery')
                placed=c.block(scenario+0x50,0x5c)[placement]
                choice=c.block(scenario+0x58,0x28)[c.u16(placed)]
                definition,root=tag(c.u32(choice+4))
                _,hlmt=tag(c.u32(root+0x38),'hlmt');model,mode=tag(c.u32(hlmt+4),'mode')
                records=[]
                for group in c.block(mode+0x58,12):
                    if c.u32(group)!=marker:continue
                    for at in c.block(group+4,36):
                        region,permutation,node=struct.unpack_from('<bbB',c.data,at)
                        records.append(dict(region=region,permutation=permutation,node=node,
                            position=floats(at+4,3),rotation=floats(at+16,4),scale=floats(at+32,1)[0]))
                    break
                requests.append(dict(script=script,effect=f'{effect:08x}',object_name_index=obj&65535,
                    object_name=c.string(name,32),object_definition=definition,model=model,
                    marker_sid=f'{marker:08x}',marker_name=sid(marker),markers=records))
                graph(effect)
            for arg in expr.get('arguments',[]):scan(arg,script)
        for script in authored['scripts']:scan(script['root'],script['name'])
        report=dict(scope=__doc__,map_sha256=pin,authored_sha256=file_sha(a.authored),
            tool_sha256=file_sha(Path(__file__)),requests=requests,graphs=graphs)
        a.output.parent.mkdir(parents=True,exist_ok=True)
        a.output.write_text(json.dumps(report,indent=2)+'\n')
        print(len(requests),'authored marker requests;',len(graphs),'complete static effect inventories')
    finally:c.close()

if __name__=='__main__':main()

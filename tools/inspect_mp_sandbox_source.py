#!/usr/bin/env python3
"""Read local authored explosive/lift metadata, retaining raw offsets and bytes.

Assembly field names are navigation, not a substitute for original consumers.
This inventory changes neither assets nor runtime; no physical result is inferred.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from inspect_multiplayer import Cache, ROOT, MAPS

TOKENS=('fusion_coil','power_module','gas_container','lift','grav_colossus')
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def record(c,o,n):
    return {'file_offset':o,'original_va':f'{o+c.magic:08x}','raw_hex':c.data[o:o+n].hex()}
def ref(c,d):
    if d==0xffffffff:return None
    t=c.tags[d]
    return {k:t[k]for k in ('datum','group','offset','size','name')}
def floatfield(c,o):return {'bits':f'{c.u(o):08x}','value':c.f(o)}
def damage(c,d):
    if d==0xffffffff:return None
    q=c.tags[d]['offset']
    return {'tag':ref(c,d),'definition':record(c,q,0xc8),'fields':{f'{a:02x}':floatfield(c,q+a)for a in (0,4,8,0x18,0x1c,0x20,0x24,0x40)},'flags0c':f'{c.u(q+12):08x}','flags14':f'{c.u(q+20):08x}'}
def effect(c,d,seen=None):
    if d==0xffffffff:return None
    t=c.tags[d];o=t['offset'];r={'tag':ref(c,d),'definition':record(c,o,0x30),'events':[]}
    for ei,e in enumerate(c.block(o+0x14,0x38)):
        parts=[]
        for pi,p in enumerate(c.block(e+0x18,0x38)):
            pd=c.u(p+0x10);z={'index':pi,'record':record(c,p,0x38),'runtime_group_word':f'{c.u(p+8):08x}','type':ref(c,pd)}
            if pd!=0xffffffff and c.tags[pd]['group']=='jpt!':
                z['damage']=damage(c,pd)
            parts.append(z)
        r['events'].append({'index':ei,'record':record(c,e,0x38),'parts':parts})
    return r
def model(c,d):
    if d==0xffffffff:return None
    t=c.tags[d];o=t['offset'];r={'tag':ref(c,d),'definition':record(c,o,0xfc),'collision':ref(c,c.u(o+0xc)),'physics':ref(c,c.u(o+0x24)),'damage':[]}
    for di,p in enumerate(c.block(o+0x60,0xf8)):
        sections=[]
        for si,s in enumerate(c.block(p+0xbc,0x38)):
            responses=[]
            for ri,v in enumerate(c.block(s+12,0x50)):
                e=c.u(v+0x10);de=c.u(v+0x18);dl=c.u(v+0x34)
                responses.append({'index':ri,'record':record(c,v,0x50),'threshold08':floatfield(c,v+8),'delay2c':floatfield(c,v+0x2c),'effect':effect(c,e),'damage_effect':damage(c,de),'delay_effect':effect(c,dl)})
            sections.append({'index':si,'record':record(c,s,0x38),'responses':responses})
        r['damage'].append({'index':di,'record':record(c,p,0xf8),'body28':floatfield(c,p+0x28),'shield8c':floatfield(c,p+0x8c),'sections':sections})
    pd=c.u(o+0x24)
    if pd!=0xffffffff:
        q=c.tags[pd]['offset'];r['physics_data']={'definition':record(c,q,0x110),'phantom_types':[{'index':i,'record':record(c,v,0x68),'flags00':f'{c.u(v):08x}','fields':{f'{a:02x}':floatfield(c,v+a)for a in range(0x18,0x40,4)}}for i,v in enumerate(c.block(q+0x28,0x68))],
            'rigid_bodies':[{'index':i,'record':record(c,v,0x90),'source_motion_type1e':c.h(v+0x1e)}for i,v in enumerate(c.block(q+0x38,0x90))],
            'phantom_shapes':[{'index':i,'record':record(c,v,0x20)}for i,v in enumerate(c.block(q+0x108,0x20))],
            'shape_links_40':[{'index':i,'record':record(c,v,0xc),'source_rule_index08':c.h(v+8)}for i,v in enumerate(c.block(q+0x40,0xc))]}
    return r
def inspect(p):
    c=Cache(p);s=c.scenario;r={'path':str(p.relative_to(ROOT)),'sha256':sha(p),'magic':c.magic,'scenario':record(c,s,0x338),'objects':[]}
    placements={}
    for name,po,oo,stride in [('crate',0x330,0x328,0x4c),('machine',0xb0,0xa8,0x48)]:
        palette=[c.u(v+4)for v in c.block(s+po,0x28)]
        for i,v in enumerate(c.block(s+oo,stride)):
            pi=c.h(v)
            if pi<0:continue
            d=palette[pi]
            placements.setdefault(d,[]).append({'scenario_group':name,'index':i,'palette_index':pi,'record':record(c,v,stride),'flags04':f'{c.u(v+4):08x}','position':list(struct.unpack_from('<3f',c.data,v+8)),'rotation':list(struct.unpack_from('<3f',c.data,v+0x14)),'scale':floatfield(c,v+0x20),'type2e':c.data[v+0x2e]})
    for d,t in c.tags.items():
        if t['group']not in ('bloc','mach'):continue
        if not any(k in t['name'].lower()for k in TOKENS):continue
        o=t['offset'];r['objects'].append({'tag':ref(c,d),'definition':record(c,o,t['size']),'type00':c.h(o),'flags02':f'{struct.unpack_from("<H",c.data,o+2)[0]:04x}','flagsbc':f'{c.u(o+0xbc):08x}','placements':placements.get(d,[]),'model':model(c,c.u(o+0x38))})
    c.data.close();c.stream.close();return r
def main():
    a=argparse.ArgumentParser();a.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/sandbox-source/asset-inventory.json');args=a.parse_args()
    paths=[ROOT/'assets/disc/maps'/f'{m}.map'for m in MAPS]
    paths+=sorted((ROOT/'assets/multiplayer-map-pack/demos/dlc').rglob('*.map'))
    r={'schema_names_are_navigation_only':True,'original_source_physical_results_executed':False,'native_gameplay_update_applied':False,'maps':[inspect(p)for p in paths]}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({'maps':len(paths),'objects':sum(len(z['objects'])for z in r['maps']),'sha256':sha(args.output),'output':str(args.output)}))
if __name__=='__main__':main()

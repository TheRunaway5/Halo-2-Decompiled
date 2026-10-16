#!/usr/bin/env python3
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,'tools');from audit_rendering import Cache
p=argparse.ArgumentParser(description='Read original owned vehicle classes; no construction or dynamics claim.');p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output;out.parent.mkdir(parents=True,exist_ok=True);maps=[];failures=[];classes={}
for folder in ['assets/disc/maps','assets/map-pack/maps']:
 for path in sorted(Path(folder).glob('*.map')):
  if path.name in ('shared.map','single_player_shared.map','mainmenu.map'):continue
  c=Cache(path);vehicles=[]
  try:
   for datum,t in c.tags.items():
    if t['group']!='vehi':continue
    try:
     _,p=c.tag(datum,'vehi');kind=c.u16(p+0x1f0);assert kind<=6
     row=dict(datum=datum,name=t['name'],type=kind,control=c.u16(p+0x1f2),specific_type=c.u16(p+0x218),unit_flags=hex(c.u32(p+0xbc)),vehicle_flags=hex(c.u32(p+0x1ec)),definition_prefix_sha256=hashlib.sha256(c.data[p:p+0x300]).hexdigest())
     vehicles.append(row);classes.setdefault(str(kind),{})[t['name']]=True
    except Exception as e:failures.append(dict(map=str(path),datum=datum,name=t['name'],error=str(e)))
   maps.append(dict(map=str(path),sha256=hashlib.file_digest(path.open('rb'),'sha256').hexdigest(),vehicles=vehicles))
  finally:c.close()
report=dict(reference='Original owned Xbox map definitions; English TU1.5 is sole physics target',full_physics_parity=False,maps=maps,classes={k:sorted(v)for k,v in sorted(classes.items())},failures=failures,boundary='Read-only vehicle type/control/specific class and flag inventory. This does not prove construction, player admission, AI control, motion, solved contacts or parity for any new class.')
out.write_text(json.dumps(report,indent=2)+'\n');print('maps',len(maps),'definitions',sum(len(r['vehicles'])for r in maps),'failures',len(failures))
for k,v in report['classes'].items():print('class',k,'unique',len(v),v[:12])

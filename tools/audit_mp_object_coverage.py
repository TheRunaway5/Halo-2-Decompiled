#!/usr/bin/env python3
"""Literal owned-map object/vehicle/scenery inventory; no behavior inference.

Scenario fields agree with the current content importer. Extra creature/light
volume fields and vehicle field labels are schema navigation only until their
original consuming routines are independently established.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_multiplayer import Cache,ROOT,MAPS

PLACEMENTS=(('scenery',6,0x50,0x5c),('biped',0,0x60,0x54),
 ('vehicle',1,0x70,0x54),('equipment',3,0x80,0x38),('weapon',2,0x90,0x54),
 ('machine',7,0xa8,0x48),('control',8,0xb8,0x44),('light_fixture',9,0xc8,0x54),
 ('sound_scenery',10,0xd8,0x50),('light_volume',None,0xe8,0x6c),
 ('crate',11,0x328,0x4c),('creature',12,0x368,0x34))
EXTERNAL=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c/4d53006400000004')

def sha(path):
 with Path(path).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def label(path):return str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
def record(c,p,n):
 if not 0<=p<=p+n<=len(c.data):raise ValueError('Metadata record outside literal map')
 return dict(file_offset=f'{p:08x}',source_va=f'{p+c.magic:08x}',raw_hex=bytes(c.data[p:p+n]).hex())
def reference(c,d):
 if d==0xffffffff:return None
 if d not in c.tags:return dict(datum=f'{d:08x}',missing=True)
 t=c.tags[d];return dict(t)
def modeled(c,datum):
 t=c.tags.get(datum)
 if not t or t['offset'] is None:return dict(definition=reference(c,datum),unavailable=True)
 at=t['offset'];r=dict(definition=reference(c,datum),object_prefix=record(c,at,min(t['size'],0xbc)))
 if t['size']<0x3c:return dict(r,unavailable=True)
 d=c.u(at+0x38);r['hlmt']=reference(c,d)
 r['object_type']=c.h(at)
 r['object_flags_word']=f'{struct.unpack_from("<H",c.data,at+2)[0]:04x}'
 r['default_variant_word']=f'{c.u(at+0x30):08x}'
 h=c.tags.get(d)
 if not h or h['offset'] is None:return r
 p=h['offset'];r.update(hlmt_prefix=record(c,p,min(h['size'],0xfc)),
  render_model=reference(c,c.u(p+4)),collision_model=reference(c,c.u(p+0xc)),
  animation_graph=reference(c,c.u(p+0x14)),physics_model=reference(c,c.u(p+0x24)),
  hlmt_variants=c.u(p+0x50),damage_info_count=c.u(p+0x60))
 regions=[];fixed=[]
 for q in c.block(p+0x70,0x10):
  perms=[dict(record=record(c,v,8),flags=c.data[v+4],collision_permutation=c.data[v+5]) for v in c.block(q+8,8)]
  choices=[i for i,v in enumerate(perms)if not v['flags']&1]
  if not choices and perms:choices=[0]
  fixed.append(choices[0] if len(choices)==1 else 255)
  regions.append(dict(record=record(c,q,0x10),collision_region=c.data[q+4],permutations=perms,default_choices=choices))
 r['model_regions']=regions
 r['no_variant_selection_fixed']=not r['hlmt_variants'] and all(len(q['default_choices'])<=1 for q in regions)
 if r['no_variant_selection_fixed']:r['no_variant_selected_bytes']=fixed
 coll=c.tags.get(c.u(p+0xc))
 if coll and coll['offset'] is not None:
  q=coll['offset'];r['collision_prefix']=record(c,q,min(coll['size'],0x34))
  r['collision_banks']=[]
  for region in c.block(q+0x1c,0xc):
   rr=dict(record=record(c,region,0xc),permutations=[])
   for perm in c.block(region+4,0x14):
    pr=dict(record=record(c,perm,0x14),bsps=[])
    for bsp in c.block(perm+4,0x44):
     pr['bsps'].append(dict(record=record(c,bsp,0x44),node=c.h(bsp),
      surfaces=c.u(bsp+0x2c),vertices=c.u(bsp+0x3c)))
    rr['permutations'].append(pr)
   r['collision_banks'].append(rr)
 mode=c.tags.get(c.u(p+4))
 if mode and mode['offset'] is not None:
  q=mode['offset'];r['render_node_records']=[record(c,v,0x60)for v in c.block(q+0x48,0x60)]
 phmo=c.tags.get(c.u(p+0x24))
 if phmo and phmo['offset'] is not None:
  q=phmo['offset'];r['physics_counts_navigation']={
   'rigid_bodies':c.u(q+0x38),'phantom_types':c.u(q+0x28),'phantom_shapes':c.u(q+0x108)}
 return r
def vehicle(c,datum):
 r=modeled(c,datum);t=c.tags[datum];p=t['offset']
 if p is None:return r
 r.update(definition_sha256=hashlib.sha256(c.data[p:p+t['size']]).hexdigest(),
  prefix300=record(c,p,min(t['size'],0x300)))
 if t['size']<0x300:return dict(r,vehicle_fields_unavailable=True)
 r.update(source_object_type=c.h(p),vehicle_type_1f0=c.h(p+0x1f0),control_type_1f2=c.h(p+0x1f2),
  vehicle_flags_1ec=f'{c.u(p+0x1ec):08x}',physics_flags_2ac=f'{c.u(p+0x2ac):08x}',
  seats=[dict(index=i,record=record(c,s,0xb0),flags=f'{c.u(s):08x}',
    animation=f'{c.u(s+4):08x}',seat_marker=f'{c.u(s+8):08x}',entry_marker=f'{c.u(s+12):08x}')
   for i,s in enumerate(c.block(p+0x1c8,0xb0))],
  unit_weapons=[dict(index=i,record=record(c,w,8),weapon=reference(c,c.u(w+4)))
   for i,w in enumerate(c.block(p+0x1c0,8))],
  anti_gravity_points=[record(c,v,0x4c) for v in c.block(p+0x2e8,0x4c)],
  friction_points=[record(c,v,0x4c) for v in c.block(p+0x2f0,0x4c)])
 return r
def inspect(path):
 c=Cache(path);s=c.scenario;categories={};used_vehicles=set();used_scenery=set()
 for name,kind,off,stride in PLACEMENTS:
  rows=[];choices=c.block(s+off+8,0x28);palette=[c.u(p+4)for p in choices]
  for ordinal,p in enumerate(c.block(s+off,stride)):
   index=c.h(p);d=palette[index] if 0<=index<len(palette) else 0xffffffff
   row=dict(ordinal=ordinal,palette_index=index,definition=reference(c,d),record=record(c,p,stride),
    flags=f'{c.u(p+4):08x}',position=list(struct.unpack_from('<3f',c.data,p+8)),
    rotation=list(struct.unpack_from('<3f',c.data,p+0x14)),scale_bits=f'{c.u(p+0x20):08x}',
    name_index=c.h(p+2),origin_bsp=c.h(p+0x2c),source=struct.unpack_from('<b',c.data,p+0x2f)[0],
    bsp_policy=c.data[p+0x30])
   if kind in (0,1,2,6,11):row['variant']=f'{c.u(p+0x34):08x}'
   if kind==6:row.update(lightmapping_policy=c.h(p+0x4e),multiplayer_games=c.u(p+0x58)>>16)
   if kind==6:
    flag=c.u(p+4);games=row['multiplayer_games']
    game=bool(games&2)if games not in(0,0x7f)else not(flag&64)
    row['automatic_slayer_bsp0_filter']=bool(game and not(flag&1) and d!=0xffffffff and
      c.tags[d]['group']=='scen' and row['origin_bsp']in(-1,0))
   rows.append(row)
   if d!=0xffffffff and kind==1:used_vehicles.add(d)
   if d!=0xffffffff and kind==6:used_scenery.add(d)
  categories[name]=dict(object_type=kind,offset=f'{off:x}',stride=stride,
   imported_by_current_content=name not in ('light_volume','creature'),palette=[reference(c,d)for d in palette],rows=rows)
 net=[];vehicle_rows=[]
 for ordinal,p in enumerate(c.block(s+0x120,0x90)):
  d=c.u(p+0x5c);collection=c.collection(d);items=[]
  if d!=0xffffffff and c.tags[d]['group'] in ('vehc','itmc'):
   q=c.tags[d]['offset']
   for at in c.block(q,16):
    selected=c.u(at+8);items.append(dict(record=record(c,at,16),definition=reference(c,selected),variant=f'{c.u(at+12):08x}'))
    if selected!=0xffffffff and c.tags[selected]['group']=='vehi':used_vehicles.add(selected)
  row=dict(ordinal=ordinal,classification=c.data[p+0x14],record=record(c,p,0x90),collection=collection,items=items)
  net.append(row)
  if collection and (collection['group']=='vehc' or any(v['definition'] and v['definition'].get('group')=='vehi' for v in items)):
   vehicle_rows.append(ordinal)
 vehicles={f'{d:08x}':vehicle(c,d)for d,t in sorted(c.tags.items()) if t['group']=='vehi'}
 scenery={f'{d:08x}':modeled(c,d)for d in sorted(used_scenery)}
 collidable=[k for k,v in scenery.items() if v.get('collision_model') and not v['collision_model'].get('missing')]
 collisions=sum(1 for row in categories['scenery']['rows'] if row['definition'] and row['definition']['datum'] in collidable)
 conservative=[]
 for row in categories['scenery']['rows']:
  if not row['automatic_slayer_bsp0_filter']:continue
  v=scenery[row['definition']['datum']]
  if(v.get('collision_model')and not v.get('animation_graph')and not v.get('physics_model')and
     not v['damage_info_count']and v['no_variant_selection_fixed']):conservative.append(row['ordinal'])
 r=dict(path=label(path),sha256=sha(path),scenario_va=f'{s+c.magic:08x}',categories=categories,
  netgame_rows=net,netgame_vehicle_rows=vehicle_rows,used_vehicle_datums=[f'{d:08x}'for d in sorted(used_vehicles)],
  resident_vehicles=vehicles,placed_scenery_definitions=scenery,
  conservative_fixed_scenery_data_candidates=conservative,
  summary=dict(placements={k:len(v['rows'])for k,v in categories.items()},netgame_rows=len(net),
   netgame_vehicle_rows=len(vehicle_rows),resident_vehicles=len(vehicles),used_vehicle_definitions=len(used_vehicles),
   resolved_vehicle_placements=sum(bool(v['definition']) and not v['definition'].get('missing',False) for v in categories['vehicle']['rows']),
   conservative_fixed_scenery_data_candidates=len(conservative),
   scenery_collision_definitions=len(collidable),scenery_collision_placements=collisions))
 c.data.close();c.stream.close();return r
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 paths=[ROOT/'assets/disc/maps'/f'{m}.map'for m in MAPS]
 paths+=sorted((ROOT/'assets/multiplayer-map-pack/demos/dlc').rglob('*.map'))
 paths+=[EXTERNAL/'derelict.map',EXTERNAL/'highplains2.map']
 if len(paths)!=23:raise ValueError(f'Expected23 distinct map inputs, found{len(paths)}')
 maps=[inspect(p)for p in paths]
 report=dict(schema='literal-multiplayer-object-coverage-v1',source_behavior_executed=False,
  schema_labels_navigation_only=True,
  conservative_filter_scope='Data-only candidate restriction: current automatic-Slayer/BSP0 predicate, actual scen group, coll present, jmad/phmo/damage absent, no hlmt variants and at most one source fallback choice per region. Does not produce object flags, physics, broadphase membership or current WORLD poses.',
  tool_sha256=sha(Path(__file__)),reader_sha256=sha(ROOT/'tools/inspect_multiplayer.py'),maps=maps)
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
 for m in maps:print(Path(m['path']).stem,json.dumps(m['summary'],sort_keys=True))
 print('sha256',sha(args.output))
if __name__=='__main__':main()

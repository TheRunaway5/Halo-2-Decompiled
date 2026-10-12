#!/usr/bin/env python3
"""Authenticate the next complete body/world contact listener dispatchers."""
import argparse,hashlib,json,collections
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
import research_mp_sandbox_update_functions as base
PARENTS={
 'power-child-agents':'b8d9560f13f4f92c307f59ac57c0f3a591e82f09f0110f3d7210e461f2c23237',
 'power-bsp-children':'5e3a1512945857217222f121e61135c2a4a9f731ac38d7f96fc84869060d67f1',
 'power-narrowphase-owners':'23bad35ef780f31b88319ca03bc776cfc4da9d7e8ce8e21d4abb5183231b81f6',
 'power-gjk-owners':'3c19e91c07ff2956ce2225fe9afdf564be52c1a730eb929ca9f1c8ae9527b4df',
 'power-contact-batches':'176bff542fd35afd174b3fffad0e62f225fadf286843024b6a148833f89cd8cd',
 'power-broadphase':'27e0ca004227a19010adb23f1fa8f455cbf54787d328a05ab9ccf9cf3a08f553',
 'power-current-broadphase':'7fc356215532e834f83e1dbf61f48ff1f437f1247f08204a35c9c00d1f7597ef',
 'power-constraint-membership':'17d60ba06ef7797236d815447acad924efa47dc2b36485c8a17eba1ebd4bcd9d',
 'power-pair-manager':'5ca584c05b98ce02f201fec83098d57268ffc3f8bd136457f99a4b72b63294b4',
 'power-agent-scheduler':'0dbfe235952b97f50c0a2492a95083fac137a614bf309f8b06cd09b0321e6470',
 'power-world-insertion':'81fc63bb3eacd865d376e130d441f58e650b6ed55451692a60c8479f8f75ad79'}
# Actual virtual bounds and allocator/TLS calls reached before the first packet.
# Constructor 2E2530 authenticates the virtual slot, rather than assuming the
# dynamic pointer has the same value in the two images.
SELECT={'power-broadphase':{0x148a10,0x148a70,0x22e950,0x22eb20,0x1d1c00,0x2e2380,0x303e30,0x2e2530,0x2df360},
 'power-current-broadphase':{0x2e4670}}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 a.output.mkdir(parents=True,exist_ok=False);d=a.output/'instructions';d.mkdir()
 mapping={};parents={}
 for family,pin in PARENTS.items():
  p=ROOT/'analysis/multiplayer/sandbox-source'/family/'final/later-correspondence'/('report.json' if family=='power-constraint-membership' else 'complete-report.json')
  assert sha(p)==pin;r=json.loads(p.read_text());parents[family]=pin
  assert r['disc_1_1_sha256']==PIN and r['later_owned_xbe_sha256']==base.NEW_PIN
  for row in r['routines']:
   x,y=int(row['disc_1_1_entry'],16),int(row['later_entry'],16)
   if family in SELECT and x not in SELECT[family]:continue
   assert x not in mapping or mapping[x]==y;mapping[x]=y
 complete_path=ROOT/'analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011/correspondence-verified/complete-report.json'
 assert sha(complete_path)=='27291554f1e41b5b58aa4948274a28848943261aff95c5b57998373173d9283b'
 complete=json.loads(complete_path.read_text());parents['complete_constraint_and_motion_graph']=sha(complete_path)
 assert complete['disc_1_1_sha256']==PIN and complete['later_owned_xbe_sha256']==base.NEW_PIN
 for row in complete['routines']:
  x,y=int(row['disc_1_1_entry'],16),int(row['later_entry'],16)
  assert x not in mapping or mapping[x]==y;mapping[x]=y
 oldp=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assert sha(oldp)==PIN and sha(base.NEW_DEFAULT)==base.NEW_PIN
 old,new=Xbe(oldp),Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 # Bind these additional owners from actual paired direct callers. Their full
 # bodies are independently authenticated below, not inferred from a prefix.
 additional={0x322410,0x3223b0,0x31e8b0,0x2e1070,0x3126f0,0x27f6c0,0x27f570,0x1d3700,0x31c2e0,0x31ead0,0x27de20,0x183910,0x183960,0x31b8d0,0x27e160,0x184600,0xbb0c0,0x1f1de0,0x1d43c0,0x1d44c0,0x1c8f10,0x1d3df0,0x1d2ca0,0x1d2df0,0x1d32a0,0x1d40b0,0x2298b0,0x1dc1a0}
 callback_path=ROOT/'analysis/multiplayer/sandbox-source/physics-contact-listener-runtime-20261011/input-bindings-v1/report.json'
 assert sha(callback_path)=='e9d5eae38852a2fd394d4cb9977d19fbdbd4c66314288ff6774e0dc2024c8c38'
 callback=json.loads(callback_path.read_text())
 assert callback['tool_sha256']==sha(ROOT/'tools/research_physics_contact_listener_bindings_20261011.py')
 row=next(x for x in callback['routines'] if x['old_entry']==0x2e1070)
 mapping[0x2e1070]=row['later_entry'];parents['actual_listener_callback_binding']=sha(callback_path)
 binding_path=ROOT/'analysis/multiplayer/sandbox-source/physics-world-runtime-20261011/input-bindings-v2/report.json'
 assert sha(binding_path)=='4c749d1afca123e53243eda75532b60064dc84fc786475b017b7a90cc41e7347'
 binding=json.loads(binding_path.read_text());assert binding['tool_sha256']==sha(ROOT/'tools/research_physics_world_runtime_bindings_20261011.py')
 for slot in binding['verified_virtual_slots']:
  p,q=slot['old_entry'],slot['later_entry'];assert p not in mapping or mapping[p]==q;mapping[p]=q
 parents['actual_world_virtual_slots']=sha(binding_path)
 extra_path=ROOT/'analysis/multiplayer/sandbox-source/physics-world-runtime-20261011/input-bindings-extra-v3/report.json'
 assert sha(extra_path)=='e72bd4ba8d5f545054b92b2574f963cbafb706bc81874762dd971e8cabe3c732'
 extra=json.loads(extra_path.read_text());assert extra['tool_sha256']==sha(ROOT/'tools/research_physics_world_runtime_extra_bindings_20261011.py')
 for slot in extra['verified_virtual_slots']:
  p,q=slot['old_entry'],slot['later_entry'];assert p not in mapping or mapping[p]==q;mapping[p]=q
 parents['actual_dispatcher_and_event_listener_slots']=sha(extra_path)
 game_path=ROOT/'analysis/multiplayer/sandbox-source/physics-world-runtime-20261011/input-bindings-extra-v4/report.json'
 assert sha(game_path)=='e2311bd9cb96bb734646ff47bd919900d0602a3cc43e066ceb97b0dac1329277'
 game=json.loads(game_path.read_text());assert game['tool_sha256']==sha(ROOT/'tools/research_physics_world_runtime_extra_bindings_20261011.py')
 old_slot=0x461bbc;later_slot=game['address_map'][f'{old_slot:08x}']
 callback=int.from_bytes(old.read(old_slot,4),'little');target=int.from_bytes(new.read(later_slot,4),'little');assert callback==0x27dca0
 assert callback not in mapping or mapping[callback]==target;mapping[callback]=target
 parents['actual_game_contact_callback_slot0']=dict(input_binding_sha256=sha(game_path),old_slot=old_slot,later_slot=later_slot,old_entry=callback,later_entry=target)
 # Complete1EE380 stores this selected shape table in the retained factory.
 #184600 reaches its actual slot14 type query; bind through that table.
 shape_table=0x45d188;later_table=game['address_map'][f'{shape_table:08x}']
 x,y=int.from_bytes(old.read(shape_table+0x14,4),'little'),int.from_bytes(new.read(later_table+0x14,4),'little');assert x==0x9d8e0
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_factory_shape_type_slot14']=dict(input_binding_sha256=sha(game_path),old_slot=shape_table+0x14,later_slot=later_table+0x14,old_entry=x,later_entry=y)
 # Complete31F6C0 installs the actual manager table. Follow its packet slots.
 for offset,expected in ((0x18,0x31f4b0),(0x14,0x31f4d0)):
  table=0x41b6b4;later=game['address_map'][f'{table:08x}']
  x,y=int.from_bytes(old.read(table+offset,4),'little'),int.from_bytes(new.read(later+offset,4),'little');assert x==expected
  assert x not in mapping or mapping[x]==y;mapping[x]=y
  parents[f'actual_manager_packet_slot{offset:x}']=dict(input_binding_sha256=sha(game_path),constructor=0x31f6c0,old_slot=table+offset,later_slot=later+offset,old_entry=x,later_entry=y)
 while not additional<=mapping.keys():
  before=len(mapping)
  for entry,target in list(mapping.items()):
   oi=[oc.at(p) for p in sorted(_trace(oc,entry,set(mapping)|additional).code)]
   ni=[nc.at(p) for p in sorted(_trace(nc,target,set(mapping.values())).code)]
   assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(entry),hex(target))
   for x,y in zip(oi,ni):
    if x.mnemonic=='call' and x.operands[0].type==y.operands[0].type==2 and x.operands[0].imm in additional:
     p,q=x.operands[0].imm,y.operands[0].imm;assert p not in mapping or mapping[p]==q;mapping[p]=q
  assert len(mapping)>before,'Required original direct-caller anchor absent'

 rows=[];addresses={};sites={};mnemonics=collections.Counter();outside=collections.defaultdict(list);indirect=[]
 for entry,target in sorted(mapping.items()):
  of,nf=_trace(oc,entry,set(mapping)),_trace(nc,target,set(mapping.values()))
  oi=[oc.at(p) for p in sorted(of.code)];ni=[nc.at(p) for p in sorted(nf.code)]
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(entry),hex(target))
  for x,y in zip(oi,ni):
   key=f'{x.address:08x}';assert key not in sites or sites[key]==y.address;sites[key]=y.address;mnemonics[x.mnemonic]+=1
   for xo,yo in zip(x.operands,y.operands):
    pairs=[]
    if xo.type==yo.type==3:pairs.append((xo.mem.disp,yo.mem.disp))
    elif xo.type==yo.type==2:pairs.append((xo.imm,yo.imm))
    for p,q in pairs:
     if base.containing(old,p) and base.containing(new,q):
      key=f'{p:08x}';assert key not in addresses or addresses[key]==q;addresses[key]=q
   if x.mnemonic=='call':
    if x.operands[0].type==2:
     p,q=x.operands[0].imm,y.operands[0].imm
     if p in mapping:assert mapping[p]==q
     else:outside[f'{p:08x}'].append(dict(site=f'{x.address:08x}',later_target=f'{q:08x}',owner=f'{entry:08x}'))
    else:indirect.append(dict(site=f'{x.address:08x}',owner=f'{entry:08x}',operand=x.op_str))
  row=dict(disc_1_1_entry=f'{entry:08x}',later_entry=f'{target:08x}',instructions=len(oi),complete_paired_body=True,tables=[])
  assert len(of.tables)==len(nf.tables)
  for (p,width,count),(q,w2,n2) in zip(of.tables,nf.tables):
   assert (width,count)==(w2,n2)
   ob,nb=old.read(p,width*count),new.read(q,width*count)
   if width==4:
    for k in range(count):
     x,y=int.from_bytes(ob[k*4:k*4+4],'little'),int.from_bytes(nb[k*4:k*4+4],'little')
     assert (x==y==0) or sites.get(f'{x:08x}')==y,(hex(entry),hex(x),hex(y))
   else:assert ob==nb
   addresses[f'{p:08x}']=q;row['tables'].append(dict(disc=p,later=q,width=width,count=count))
  for label,ins in (('disc',oi),('later',ni)):
   p=d/f'{entry:08x}-{label}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
   row[label+'_listing']=dict(file=p.name,sha256=sha(p))
  rows.append(row)
 r=dict(schema=1,extension_tool_sha256=sha(Path(__file__)),parents=parents,disc_1_1_sha256=PIN,
  later_owned_xbe_sha256=base.NEW_PIN,complete_bodies=len(mapping),routines=rows,
  entries={f'{p:08x}':q for p,q in mapping.items()},instruction_map=sites,address_map=addresses,
  mnemonics=dict(mnemonics),outside_direct_calls=dict(outside),indirect_calls=indirect,
  production_original_code_execution=False,live_gameplay_proof=False)
 p=a.output/'complete-report.json';p.write_text(json.dumps(r,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(complete_bodies=len(mapping),instructions=sum(mnemonics.values()),mnemonics=dict(mnemonics),outside_calls=list(outside),report_sha256=sha(p))))
if __name__=='__main__':main()

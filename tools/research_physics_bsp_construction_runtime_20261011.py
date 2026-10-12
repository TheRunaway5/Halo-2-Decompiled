#!/usr/bin/env python3
"""Authenticate the BSP collision construction continuation from paired direct callers."""
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
 additional={0x322410,0x3223b0,0x31e8b0,0x2e1070,0x3126f0,0x27f6c0,0x27f570,0x1d3700,0x31c2e0,0x31ead0,0x27de20,0x183910,0x183960,0x31b8d0,0x27e160,0x184600,0xbb0c0,0x1f1de0,0x1d43c0,0x1d44c0,0x1c8f10,0x1d3df0,0x1d2ca0,0x1d2df0,0x1d32a0,0x1d40b0,0x2298b0,0x1dc1a0,0x208fb0,0x209040,0x16d2c0,0x1d8e50,0x1d8ef0,0x2297d0,0x22c660,0x2285a0,0x2295f0,0x2287a0,0x229490,0x1218c0,0x22a4e0,0x22a2a0,0x1d42d0,0x142d00,0x2e0b60,0x2e09e0,0x31b950,0x27e5a0,0x187550,0x315c00,0x313310,0x313150,0x314c70,0x316180,0x313290,0x313950,0x27f9f0,0x31ebb0,0x315d50,0x312f20,0x313870,0x1c66a0,0x317bf0,0x308f70,0x301400,0x2efac0,0x2e00f0,0x31e940,0x31c350,0x322220,0x31f640,0x1d3fc0,0x1d2c70,0x1d2cf0,0x1d4160,0x31e840,0x31bad0,0x321f80,0x313240,0xdd970,0x2dd7f0,0x1d29a0,0x1d4260,0x3138e0,0x31f4f0,0x3126e0}
 additional|={0x326b74,0x2dac5e,0x2d7969,0x329353,0x32937a,0x3293a6,0x2df390,0x2dfa20,0x22e430,0x2df640,0x148b00,0x2dd5b0,0x2dd9b0,0x2dda40,0x2ddab0,0x2ddb20,0x2de250,0x2dd590,0x22e4e0,0x148190,0x1482f0,0x16d220,0x1c6270,0x1c6370,0x1c73a0,0x27e770,0x2e1340,0x2e5ed0,0x2e5f10,0x2e6170,0x3126d0,0x312700,0x312a30,0x312fa0,0x313060,0x316710,0x316e50,0x317720,0x318900,0x318a90,0x32684b}
 additional|={0x2db3c6,0x326baf,0x326970,0x1ecf60,0x1ecfe0,0x1f1600,0x1f2710,0x1c7480,0x2e5ce0,0x306f40,0x317890,0x31c1a0,0x31c110,0x318100,0x312410,0x320cc0,0x317ea0,0x3121a0,0x3121c0,0x317d10,0x318af0,0x31f970,0x31fd40,0x317df0,0x317e80,0x312a90,0x318a20,0x317a30,0x2eb4c0,0x2e8260,0x3292f1,0x32933b,0x329233,0x379030,0x31f620}
 startup_path=ROOT/'analysis/multiplayer/sandbox-source/physics-world-bootstrap-runtime-20261011/static-bindings-v4/report.json'
 assert sha(startup_path)=='9acb16f6ec0196380e6ff2a678e9ff0f233fad7185a31dd8bf0b57e37307b7ae'
 startup=json.loads(startup_path.read_text())
 assert startup['tool_sha256']==sha(ROOT/'tools/research_physics_world_bootstrap_bindings_20261011.py')
 assert startup['disc_sha256']==PIN and startup['later_sha256']==base.NEW_PIN and startup['all_eight_registrations_bound']
 for key,q in startup['entries'].items():
  p=int(key,16);assert p not in mapping or mapping[p]==q;mapping[p]=q
 parents['actual_static_registration_and_factory_bindings']=sha(startup_path)
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
 # Complete27DAE0 also installs game callback table461BBC. Slot4 is the
 # retained31C371 removal dispatch; slot8 is the same class event consumer.
 for offset,expected in ((4,0x27e2d0),(8,0x27e3a0)):
  x,y=int.from_bytes(old.read(old_slot+offset,4),'little'),int.from_bytes(new.read(later_slot+offset,4),'little');assert x==expected
  assert x not in mapping or mapping[x]==y;mapping[x]=y
  parents[f'actual_game_contact_callback_slot{offset:x}']=dict(input_binding_sha256=sha(game_path),constructor=0x27dae0,old_slot=old_slot+offset,later_slot=later_slot+offset,old_entry=x,later_entry=y)

 # The retained call31C301 has actual EDX461B9C, installed by the
 # complete paired BSP listener constructor1C5FF0; it is not the game table.
 bsp_table=0x461b9c;later_bsp_table=game['address_map'][f'{bsp_table:08x}']
 x,y=int.from_bytes(old.read(bsp_table,4),'little'),int.from_bytes(new.read(later_bsp_table,4),'little');assert x==0x27e610
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_bsp_contact_callback_slot0']=dict(input_binding_sha256=sha(game_path),constructor=0x1c5ff0,old_slot=bsp_table,later_slot=later_bsp_table,old_entry=x,later_entry=y)
 # The complete1C66A0 world setup installs the actual deactivation listener
 # at1C6C0B. The retained call31EBCF uses table461C14+8.
 setup=next(row for row in game['routines'] if row['old_entry']==0x1c66a0)
 assert setup['complete_body_equal'] and setup['instructions']==411
 table=0x461c14;later=game['address_map'][f'{table:08x}']
 x,y=int.from_bytes(old.read(table+8,4),'little'),int.from_bytes(new.read(later+8,4),'little');assert x==0x27f030
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_world_deactivation_listener_slot8']=dict(input_binding_sha256=sha(game_path),constructor=0x1c66a0,old_slot=table+8,later_slot=later+8,old_entry=x,later_entry=y)
 # Complete300170 installs41959C at3001BA. The actual contact-manager
 # deactivation path calls slot14 and then its own slot0 destructor.
 table=0x41959c;later=game['address_map'][f'{table:08x}']
 for offset,expected in ((0x14,0x2ef640),(0,0x2f2070)):
  x,y=int.from_bytes(old.read(table+offset,4),'little'),int.from_bytes(new.read(later+offset,4),'little');assert x==expected
  assert x not in mapping or mapping[x]==y;mapping[x]=y
  parents[f'actual_contact_manager_retirement_slot{offset:x}']=dict(input_binding_sha256=sha(game_path),constructor=0x300170,old_slot=table+offset,later_slot=later+offset,old_entry=x,later_entry=y)
 # Complete2E13B0 installs4188D0 at2E1407. Retire the actual selected
 # child agent through slot14 and its own deleting destructor slot0.
 table=0x4188d0;later=game['address_map'][f'{table:08x}']
 for offset,expected in ((0x14,0x2f4870),(0,0x2f3700)):
  x,y=int.from_bytes(old.read(table+offset,4),'little'),int.from_bytes(new.read(later+offset,4),'little');assert x==expected
  assert x not in mapping or mapping[x]==y;mapping[x]=y
  parents[f'actual_child_agent_retirement_slot{offset:x}']=dict(input_binding_sha256=sha(game_path),constructor=0x2e13b0,old_slot=table+offset,later_slot=later+offset,old_entry=x,later_entry=y)
 # Complete31D730 installs41B610 at31D751. Constraint removal322303
 # calls this actual island accounting slotC.
 table=0x41b610;later=game['address_map'][f'{table:08x}']
 x,y=int.from_bytes(old.read(table+0xc,4),'little'),int.from_bytes(new.read(later+0xc,4),'little');assert x==0x31c570
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_island_constraint_removal_slotc']=dict(input_binding_sha256=sha(game_path),constructor=0x31d730,old_slot=table+0xc,later_slot=later+0xc,old_entry=x,later_entry=y)
 # Actual312F91 removes this constraint from the same island through slot4.
 x,y=int.from_bytes(old.read(table+4,4),'little'),int.from_bytes(new.read(later+4,4),'little');assert x==0x31c620
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_island_constraint_release_slot4']=dict(input_binding_sha256=sha(game_path),constructor=0x31d730,old_slot=table+4,later_slot=later+4,old_entry=x,later_entry=y)

 # Complete1D5210 installs45CAD0 at1D5222. Body deactivation317CEC
 # actually deletes this selected body property object through slot0.
 table=0x45cad0;later=game['address_map'][f'{table:08x}']
 x,y=int.from_bytes(old.read(table,4),'little'),int.from_bytes(new.read(later,4),'little');assert x==0x1d53c0
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_body_property_destructor_slot0']=dict(input_binding_sha256=sha(game_path),constructor=0x1d5210,old_slot=table,later_slot=later,old_entry=x,later_entry=y)
 # Complete27DAE0 installs461BC8 at27DAEA. Actual31EC42 dispatches
 # the body deactivation listener through its slot4.
 table=0x461bc8;later=game['address_map'][f'{table:08x}']
 x,y=int.from_bytes(old.read(table+4,4),'little'),int.from_bytes(new.read(later+4,4),'little');assert x==0x27db80
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_game_body_deactivation_slot4']=dict(input_binding_sha256=sha(game_path),constructor=0x27dae0,old_slot=table+4,later_slot=later+4,old_entry=x,later_entry=y)
 # Complete1EE380 stores this selected shape table in the retained factory.
 #184600 reaches its actual slot14 type query; bind through that table.
 shape_table=0x45d188;later_table=game['address_map'][f'{shape_table:08x}']
 x,y=int.from_bytes(old.read(shape_table+0x14,4),'little'),int.from_bytes(new.read(later_table+0x14,4),'little');assert x==0x9d8e0
 assert x not in mapping or mapping[x]==y;mapping[x]=y
 parents['actual_factory_shape_type_slot14']=dict(input_binding_sha256=sha(game_path),old_slot=shape_table+0x14,later_slot=later_table+0x14,old_entry=x,later_entry=y)
 # Complete31F6C0 installs the actual manager table. Follow its packet slots.
 for offset,expected in ((0x18,0x31f4b0),(0x14,0x31f4d0),(0x8,0x31f400),(0x10,0x31f6b0),(0,0x31f740),(0x1c,0x31f540)):
  table=0x41b6b4;later=game['address_map'][f'{table:08x}']
  x,y=int.from_bytes(old.read(table+offset,4),'little'),int.from_bytes(new.read(later+offset,4),'little');assert x==expected
  assert x not in mapping or mapping[x]==y;mapping[x]=y
  parents[f'actual_manager_packet_slot{offset:x}']=dict(input_binding_sha256=sha(game_path),constructor=0x31f6c0,old_slot=table+offset,later_slot=later+offset,old_entry=x,later_entry=y)
 # The actual first initialization callback is the image-held function
 # pointer at the paired1C5FF5 call operand. Read both owned image words,
 # then authenticate the entire pointed-to body below.
 oi=[oc.at(p) for p in sorted(_trace(oc,0x1c5ff0,set(mapping)|additional).code)]
 ni=[nc.at(p) for p in sorted(_trace(nc,mapping[0x1c5ff0],set(mapping.values())).code)]
 assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
 matches=[(x,y,xo,yo) for x,y in zip(oi,ni) for xo,yo in zip(x.operands,y.operands)
  if x.mnemonic=='call' and xo.type==yo.type==3 and not xo.mem.base and xo.mem.disp==0x469290]
 assert len(matches)==1
 x,y,xo,yo=matches[0]
 left,right=int.from_bytes(old.read(xo.mem.disp,4),'little'),int.from_bytes(new.read(yo.mem.disp,4),'little')
 assert left==0x2ded30
 assert left not in mapping or mapping[left]==right
 mapping[left]=right
 parents['actual_world_initialization_callback']=dict(old_site=x.address,later_site=y.address,
  old_operand=xo.mem.disp,later_operand=yo.mem.disp,old_entry=left,later_entry=right)
 def bind_direct(required):
  while not required<=mapping.keys():
   before=len(mapping)
   for entry,target in list(mapping.items()):
    oi=[oc.at(p) for p in sorted(_trace(oc,entry,set(mapping)|additional).code)]
    ni=[nc.at(p) for p in sorted(_trace(nc,target,set(mapping.values())).code)]
    assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(entry),hex(target))
    for x,y in zip(oi,ni):
     if x.mnemonic in ('call','jmp') and x.operands[0].type==y.operands[0].type==2 and x.operands[0].imm in additional:
      p,q=x.operands[0].imm,y.operands[0].imm;assert p not in mapping or mapping[p]==q;mapping[p]=q
   assert len(mapping)>before,('Required original direct-caller/tail-jump anchor absent',sorted(additional-mapping.keys()))
 bind_direct({0x148190})
 # Actual148A5F allocation dispatch uses the45E77C table stored by
 # the complete148190 initializer at1481DB. Bind its actual slot10.
 oi=[oc.at(p) for p in sorted(_trace(oc,0x148190,set(mapping)|additional).code)]
 ni=[nc.at(p) for p in sorted(_trace(nc,mapping[0x148190],set(mapping.values())).code)]
 assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
 matches=[(x,y) for x,y in zip(oi,ni) if x.mnemonic=='mov' and len(x.operands)==2
  and x.operands[0].type==3 and x.operands[1].type==2 and x.operands[1].imm==0x45e77c]
 assert len(matches)==1
 x,y=matches[0];table=0x45e77c;later_table=y.operands[1].imm
 for offset,expected in ((0x10,0x22e400),(8,0x22e1d0)):
  left,right=int.from_bytes(old.read(table+offset,4),'little'),int.from_bytes(new.read(later_table+offset,4),'little')
  assert left==expected
  assert left not in mapping or mapping[left]==right
  mapping[left]=right
  parents[f'actual_initialization_allocator_slot{offset:x}']=dict(constructor=0x148190,
   old_store=x.address,later_store=y.address,old_slot=table+offset,later_slot=later_table+offset,
   old_entry=left,later_entry=right)
 # Actual2DDB5F reads slot4 from the object whose table41836C is
 # installed at2DED86 by this complete initialization callback.
 oi=[oc.at(p) for p in sorted(_trace(oc,0x2ded30,set(mapping)|additional).code)]
 ni=[nc.at(p) for p in sorted(_trace(nc,mapping[0x2ded30],set(mapping.values())).code)]
 assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
 matches=[(x,y) for x,y in zip(oi,ni) if x.mnemonic=='mov' and len(x.operands)==2
  and x.operands[0].type==3 and x.operands[1].type==2 and x.operands[1].imm==0x41836c]
 assert len(matches)==1
 x,y=matches[0];table=0x41836c;later_table=y.operands[1].imm
 left,right=int.from_bytes(old.read(table+4,4),'little'),int.from_bytes(new.read(later_table+4,4),'little')
 assert left==0x2ddca0
 assert left not in mapping or mapping[left]==right
 mapping[left]=right
 parents['actual_bootstrap_property_factory_slot4']=dict(constructor=0x2ded30,
  old_store=x.address,later_store=y.address,old_slot=table+4,later_slot=later_table+4,
  old_entry=left,later_entry=right)
 bind_direct(additional)
 # The actual array construction call329315 consumes the initializer argument
 # pushed by complete2E8260 at2E82AD. Bind that function pointer from both
 # authenticated constructor bodies; do not infer it from the runtime target.
 oi=[oc.at(p) for p in sorted(_trace(oc,0x2e8260,set(mapping)).code)]
 ni=[nc.at(p) for p in sorted(_trace(nc,mapping[0x2e8260],set(mapping.values())).code)]
 assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
 matches=[(x,y) for x,y in zip(oi,ni) if x.mnemonic=='push' and x.operands[0].type==2 and x.operands[0].imm==0x2e7d80]
 assert len(matches)==1
 x,y=matches[0];left,right=x.operands[0].imm,y.operands[0].imm
 assert left not in mapping or mapping[left]==right;mapping[left]=right
 parents['actual_bsp_array_element_initializer']=dict(constructor=0x2e8260,
  old_push=x.address,later_push=y.address,old_entry=left,later_entry=right,
  array_constructor=0x3292f1,actual_indirect_consumer=0x329315)

 rows=[];addresses={};sites={};mnemonics=collections.Counter();outside=collections.defaultdict(list);outside_tails=collections.defaultdict(list);indirect=[]
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
   elif x.mnemonic=='jmp' and x.operands[0].type==2:
    p,q=x.operands[0].imm,y.operands[0].imm
    if p in mapping:assert mapping[p]==q
    elif p not in of.code:outside_tails[f'{p:08x}'].append(dict(site=f'{x.address:08x}',later_target=f'{q:08x}',owner=f'{entry:08x}'))
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
  mnemonics=dict(mnemonics),outside_direct_calls=dict(outside),outside_tail_jumps=dict(outside_tails),indirect_calls=indirect,
  production_original_code_execution=False,live_gameplay_proof=False)
 p=a.output/'complete-report.json';p.write_text(json.dumps(r,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(complete_bodies=len(mapping),instructions=sum(mnemonics.values()),mnemonics=dict(mnemonics),outside_calls=list(outside),report_sha256=sha(p))))
if __name__=='__main__':main()

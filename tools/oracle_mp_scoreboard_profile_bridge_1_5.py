#!/usr/bin/env python3
"""Execute the original default full-profile -> local storage -> wire profile.

Raw output storage, local user index0 and profile handle NONE are declared API
inputs. Complete original local-profile initialization is a separate owning case.
No player pool, local player/view/controller binding, readiness, selected frontend
name, live online profile or game/graphics/clock success is supplied. The later
player-copy routine is compared completely and attempted with its actual missing
pool. Actual Lockout palette is consumed only after the original cache-loader
tail, with the previously declared imported tag-table/header/frame boundary.
"""
import argparse,csv,json,struct
from pathlib import Path
from unicorn.x86_const import *
from oracle_mp_full_scoreboard_1_5 import Trace,ROOT,NEW,NEW_PIN,PIN,CACHE_PIN,HudCache,literal,sha,F,Xbe
from oracle_smg_firing_1_1 import _Code
from research_mp_lobby_later_update import complete_tokens,references,raw_operands

OUT=ROOT/'analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge'
D,W,I,A=F+0x70000,F+0x71000,F+0x72000,F+0x73000
PINS={
 'tools/oracle_mp_full_scoreboard_1_5.py':'fd65899fb9ced274fd7d2442ab151223ab066f85d87cb6ea960abd18fbbddfb7',
 'tools/oracle_mp_scoreboard_providers_1_5.py':'492c77bed874eb8def07371a509df4c767cded35a629e2d2f476ca05e653033e',
 'docs/MP_SCOREBOARD_PROVIDERS_RESEARCH.md':'724df4a91405e98e35abfa50b86bdbe87aa36c5ccbe967e06216b828c8310a1e',
 'analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/report.json':'c40ae4b2cd9fc556d054a58c6ddeb5e29f468fe4fef9d31fc7d95bbb68741086',
 'tools/oracle_mp_scoreboard_draw_1_5.py':'eadeb3bea10f6946f9e26c182f4154bc858783057a6be010f0fbbf6e2164726d',
 'docs/MP_FULL_SCOREBOARD_PAINTER_RESEARCH.md':'44f0096ab038dc51aee98a7a0589ee381b2d5b2138016a47f1414a1ddf811f7d',
 'include/halo2/full_scoreboard_painter.hpp':'f0740929d70897eaf114603dd1ec429677ae389e1bd723d860db665e3b49f5ef',
 'src/ui/hud/full_scoreboard_painter.cpp':'2811191b545d03e6fbc098390596382934af20051a32efb8a7199851c41008ce',
 'analysis/multiplayer/title-update-1.5/scoreboard-painter/measured/report.json':'bd98d8946b863080e8fd6ee9cb1d03dbe3934a0a01b9b48c4051cccb3a3ec443'}
PAIRS=((0x1a40b0,0x1aa360),(0x14d600,0x14e9e0),(0x14d3d0,0x14e7b0),
 (0x14dff0,0x14f3d0),(0x1508b0,0x151c90),(0x150950,0x151d30))
STATIC=(0x21c290,0x21c1d0,0x21d170,0x21e430,0x14b617,0x14e9e0,0x14e7b0,0x14f3d0,0x151c90,0x151d30)
RECORDS=(('full_profile',D,0x1e0),('wire_profile',W,0x90),('identity_output',I,12),('palette_output',A,48),
 ('local_profiles',0x55d770,0x3218),('input_members',0x4da128,0x340),('controller_preferences',0x52d4a8,112),
 ('profile_event_globals',0x56afc0,0x144),('platform_mac',0x4dc42c,8),('online_gates',0x4d61c8,0x14),
 ('profile_pool_pointer',0x52d4a4,4),('language_selector',0x56b120,4),('player_pool_pointer',0x4f54f4,4),
 ('player_globals_pointer',0x4f54f0,4),('game_pointer',0x4f3218,4),('engine_pointer',0x4f63b8,4))

class ProfileTrace(Trace):
 def hook(self,g,a,n,user):
  if a in self.starts:
   sp=g.reg_read(UC_X86_REG_ESP)
   self.calls.append(dict(phase=self.phase,entry=f'{a:08x}',registers=self.reg(),stack64_hex=bytes(g.mem_read(sp,64)).hex()))
  super().hook(g,a,n,user)
 def records(self):return {name:dict(address=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())for name,p,n in RECORDS}
 def stage(self,name,entry,args=(),regs=(),tail=False):
  before=self.records();r=self.run(name,entry,args,regs,tail=tail);r['before_records']=before;r['after_records']=self.records();return r

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in PINS.items():assert sha(ROOT/p)==h,p
 assert sha(NEW)==NEW_PIN;assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
 c=HudCache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
 cases=[];all_entries=set(STATIC);all_visited=set();all_stores=set();t=None
 for initialize in (False,True):
  t=ProfileTrace(c);t.watch=[(p,n)for _,p,n in RECORDS]
  t.g.mem_write(D,b'\xcc'*0x1e0);t.g.mem_write(W,b'\xcc'*0x90);t.g.mem_write(I,b'\xcc'*12)
  case=dict(name='owning_local_init'if initialize else'original_cold_image',initial_records=t.records(),declared_inputs=dict(local_user=0,profile_handle='ffffffff',raw_output_storage=[f'{D:08x}',f'{W:08x}',f'{I:08x}']),stages=[])
  if initialize:
   r=t.stage('complete_local_profile_initializer',0x192380);assert r['completion']=='complete_original_return';case['stages'].append(r)
  r=t.stage('complete_default_full_profile_constructor',0x1aa360,regs=((UC_X86_REG_EDX,D),));assert r['completion']=='complete_original_return';case['stages'].append(r)
  original_full=bytes(t.g.mem_read(D,0x1e0));assert original_full[0x118:0x11c]==bytes((10,0,0,10))
  r=t.stage('complete_default_profile_local_storage_binding',0x192931,(0,D,0xffffffff));assert r['completion']=='complete_original_return';case['stages'].append(r)
  assert bytes(t.g.mem_read(0x55d788,0x1e0))==original_full and t.u(0x55d770)==0x10 and t.u(0x55d774)==0
  r=t.stage('complete_source_local_profile_to_wire',0x192baa,(0,W),((UC_X86_REG_EAX,I),));assert r['completion']=='complete_original_return';case['stages'].append(r)
  assert bytes(t.g.mem_read(W+0x40,16))==original_full[0x118:0x128]
  assert bytes(t.g.mem_read(I,12))==bytes(t.g.mem_read(0x55d778,12))
  # The source player-copy entry is attempted. Its missing source pool is not supplied.
  r=t.stage('actual_player_copy_first_pool_dependency',0x14e9e0,(0x80000000,),((UC_X86_REG_EAX,W),));assert r['completion']=='stopped_original_instruction'and r['stop']['address']=='00000044';case['player_copy_stop']=r
  # Original cache-loader tail and FFA color provider use actual produced appearance.
  index=c.u(16);t.w(0x556d90,c.magic+index)
  r=t.stage('original_cache_loader_tail',0x125434,regs=((UC_X86_REG_EBP,c.u(index+12)),),tail=0x130);assert r['completion']=='complete_original_return';r['caller_frame_kind']='original1252E0_tail_saved0x130_frame';case['palette_cache_owner']=r
  r=t.stage('ordinary_FFA_source_wire_appearance_palette',0x830c0,(-1,0),((UC_X86_REG_EDI,W+0x40),(UC_X86_REG_ESI,A)));assert r['completion']=='complete_original_return';case['palette_provider']=r
  case['full_profile1e0_hex']=original_full.hex();case['wire_profile90_hex']=bytes(t.g.mem_read(W,0x90)).hex();case['wire_appearance16_hex']=bytes(t.g.mem_read(W+0x40,16)).hex();case['first_RGB12_hex']=bytes(t.g.mem_read(A,12)).hex();case['full_profile_name64_hex']=original_full[8:0x48].hex();case['wire_name64_hex']=bytes(t.g.mem_read(W,64)).hex();case['services']=t.services
  assert not t.services
  all_entries|=t.entries;all_visited|=t.visited;all_stores|={int(r['instruction'],16)for r in t.stores};cases.append(case)
 # Default frontend name owner remains behind its actual allocator dependency.
 q=ProfileTrace(c);q.watch=[(p,n)for _,p,n in RECORDS]
 pool_stop=q.stage('actual_frontend_profile_pool_constructor_missing_allocator',0x21c290,regs=((UC_X86_REG_ECX,0),));assert pool_stop['completion']=='stopped_original_instruction'
 all_entries|=q.entries;all_visited|=q.visited;all_stores|={int(r['instruction'],16)for r in q.stores}
 listingdir=a.output.parent/'instructions';listingdir.mkdir(exist_ok=True);manifest={};listings=[]
 def save(image,code,e,starts,side):
  _,ins=literal(code,e,starts);p=listingdir/f'{side}-{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));manifest[p.name]=sha(p);listings.append(dict(side=side,entry=f'{e:08x}',file=p.name,instructions=len(ins),sha256=sha(p),raw_operands=raw_operands(ins,image)));return ins
 for e in sorted(all_entries):save(t.image,t.code,e,t.starts,'u5')
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');oc=_Code(old,'.text');os={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')};pairs=[]
 for oe,ne in PAIRS:
  oi=save(old,oc,oe,os,'tu11');_,ni=literal(t.code,ne,t.starts);assert complete_tokens(oi,old)==complete_tokens(ni,t.image),(hex(oe),hex(ne))
  pairs.append(dict(old_entry=f'{oe:08x}',u5_entry=f'{ne:08x}',complete_instructions=len(oi),complete_instructions_internal_branches_equal=True,relocated_operands=references(oi,ni,old,t.image),call_targets=[dict(old_site=f'{x.address:08x}',new_site=f'{y.address:08x}',old_operand=x.op_str,new_operand=y.op_str)for x,y in zip(oi,ni)if x.mnemonic=='call']))
 for label,addresses in (('executed-original',all_visited),('observed-original-store-sites',all_stores)):
  p=listingdir/(label+'.asm');p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in (t.code.at(e)for e in sorted(addresses))));manifest[p.name]=sha(p)
 mp=a.output.parent/'instruction-manifest.json';mp.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
 report=dict(schema='original-default-scoreboard-profile-bridge-v1',oracle_sha256=sha(__file__),source_xbe_sha256=NEW_PIN,comparison_xbe_sha256=PIN,lockout_sha256=CACHE_PIN,preserved_pins=PINS,boundaries=__doc__,cases=cases,frontend_profile_pool_stop=pool_stop,complete_source_pairs=pairs,instruction_listings=listings,instruction_manifest_sha256=sha(mp),executed_instruction_count=len(all_visited),observed_store_site_count=len(all_stores),instruction_patches=[],supplied_gameplay_graphics_readiness_clock_returns=[])
 a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 for p,h in PINS.items():assert sha(ROOT/p)==h,p
 print(json.dumps(dict(cases=len(cases),complete_pairs=len(pairs),appearance=cases[1]['wire_appearance16_hex'],RGB=cases[1]['first_RGB12_hex'],instructions=len(all_visited),services=0)))
if __name__=='__main__':main()

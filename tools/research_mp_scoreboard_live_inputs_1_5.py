#!/usr/bin/env python3
"""Narrow original scoreboard option metadata and viewport input contract.

Original metadata routines write their actual type/offset/multiplier outputs.
The original Slayer default constructor consumes unchanged actual cache tags;
the inherited XGetLanguage provider is explicit, with no loaded-locale result.
Original rasterizer prefix12560..12591 creates the margin rectangles before its
GPU work. Grid roots receive declared native participant count/display mode and
raw output storage; complete render/view/world scheduling is not claimed.
"""
import argparse,csv,json,struct
from pathlib import Path
from unicorn.x86_const import *
from oracle_mp_full_scoreboard_1_5 import Trace,ROOT,NEW,NEW_PIN,HudCache,literal,sha,F,S,STOP,Xbe
from oracle_smg_firing_1_1 import _Code
from research_mp_lobby_later_update import complete_tokens,references,raw_operands
OUT=ROOT/'analysis/multiplayer/title-update-1.5/scoreboard-live-inputs'
V,M,GRID,SPAN,POS,OUTER,SAFE=[F+p for p in(0x70000,0x71000,0x72000,0x72100,0x72200,0x72300,0x72400)]
PAIRS=((0x2374f0,0x23dee0),(0x19d220,0x1a6d40),(0x2f600,0x2f5e0),(0x2f640,0x2f620),(0x2f6b0,0x2f690),(0x223080,0x229a90),(0x16a30,0x16a10))
STATIC=(0x13a8b0,0x23dee0,0x12560,0x169d0,0x16a10,0x229a90,0x2b9f0,0x163360,0x15bb50,0x160570,0x161530)
PINS={
 'tools/oracle_mp_scoreboard_profile_bridge_1_5.py':'05cfda0f9da6b3662a7eee736b74a45c6eec5c8b0965e2344c9d43a82929d5b2',
 'docs/MP_SCOREBOARD_PROFILE_BRIDGE_RESEARCH.md':'6600a4373ff4b273d2d83a182cb21dd616a69526b543298f95d991dd9e76f899',
 'analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/report.json':'ed51be6ed8cdd89bdb4c81902c996c34ce47f3aa32b98232bc8a9e90142660c8'}
class InputTrace(Trace):
 def hook(self,g,a,n,user):
  if a in self.starts:
   sp=g.reg_read(UC_X86_REG_ESP);self.calls.append(dict(phase=self.phase,entry=f'{a:08x}',registers=self.reg(),stack64_hex=bytes(g.mem_read(sp,64)).hex()))
  super().hook(g,a,n,user)
 def fragment(self,name,e,end):
  self.phase=name;self.stopped=None;self.block=None;self.w(S,STOP);self.g.reg_write(UC_X86_REG_ESP,S)
  before=self.reg();begin=len(self.calls);wb=len(self.stores);sb=len(self.services)
  self.g.emu_start(e,end,count=1000000);assert self.g.reg_read(UC_X86_REG_EIP)==end
  return dict(name=name,entry=f'{e:08x}',stop_exclusive=f'{end:08x}',caller_frame_kind='declared_API_frame_preserved_until_original_exclusive_stop',before_registers=before,after_registers=self.reg(),completion='complete_original_fragment',calls=self.calls[begin:],stores=self.stores[wb:],services=self.services[sb:])
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in PINS.items():assert sha(ROOT/p)==h,p
 assert sha(NEW)==NEW_PIN
 t=InputTrace(HudCache(ROOT/'assets/disc/maps/lockout.map'));t.watch=[(V,0x130),(M,12),(0x4922aa,16),(GRID,0x500)]
 _,reader=literal(t.code,0x23dee0,t.starts);table=next(i for i in reader if i.mnemonic=='mov'and i.op_str.startswith('eax, dword ptr [eax*4')).operands[1].mem.disp
 rows=[]
 for parameter in(0,2,16):
  target=t.u(table+parameter*4);before=bytes(t.g.mem_read(M,12)).hex();r=t.run('original_parameter_metadata_'+str(parameter),target,(M,M+4,M+8));assert r['completion']=='complete_original_return'
  r.update(parameter=parameter,table_address=f'{table+4*parameter:08x}',table_record4_hex=t.image.read(table+4*parameter,4).hex(),output_before12_hex=before,output_after12_hex=bytes(t.g.mem_read(M,12)).hex(),type=t.u(M),variant_offset=t.u(M+4),multiplier_bits=f'{t.u(M+8):08x}');rows.append(r)
 assert [(r['parameter'],r['type'],r['variant_offset'],r['multiplier_bits'])for r in rows]==[(0,5,0x4c,'3f800000'),(2,5,0x50,'3f800000'),(16,5,0x7c,'3f800000')]
 variant=t.run('whole_original_default_Slayer_variant',0x1a6d40,(V,0));assert variant['completion']=='complete_original_return';variant['output130_hex']=bytes(t.g.mem_read(V,0x130)).hex();variant['source_fields']={f'{p:02x}':f'{t.u(V+p):08x}'for p in(0x44,0x48,0x4c,0x50,0x7c)}
 getters=[]
 for parameter in(0,2,16):
  r=t.run('original_default_parameter_read_'+str(parameter),0x23dee0,regs=((UC_X86_REG_EAX,parameter),(UC_X86_REG_EDI,V)));assert r['completion']=='complete_original_return';r['parameter']=parameter;r['returned_signed_int']=struct.unpack('<i',struct.pack('<I',t.g.reg_read(UC_X86_REG_EAX)))[0];getters.append(r)
 assert [r['returned_signed_int']for r in getters]==[0,25,0]and t.u(V+0x44)==2
 before=bytes(t.g.mem_read(0x4922aa,16)).hex();prefix=t.fragment('owning_rasterizer_margin_prefix',0x12560,0x12591);prefix.update(rectangles_before16_hex=before,rectangles_after16_hex=bytes(t.g.mem_read(0x4922aa,16)).hex(),outer=list(struct.unpack('<4h',t.g.mem_read(0x4922aa,8))),safe=list(struct.unpack('<4h',t.g.mem_read(0x4922b2,8))))
 assert prefix['outer']==[0,0,480,640]and prefix['safe']==[36,48,444,592]
 cells=[]
 for players in range(1,5):
  for widescreen in (False,True):
   mode=0 if players==1 else 2 if widescreen else 1
   r=t.run('grid_'+str(players)+'_'+str(widescreen),0x2f5e0,regs=((UC_X86_REG_EDX,mode),(UC_X86_REG_ESI,players),(UC_X86_REG_EDI,GRID)));assert r['completion']=='complete_original_return'
   group=dict(participants=players,widescreen=widescreen,declared_grid_mode=mode,grid_call=r,views=[])
   for view in range(players):
    q=t.run('source_grid_span_position',0x2f620,(players,mode,POS),((UC_X86_REG_ECX,GRID),(UC_X86_REG_EDX,SPAN),(UC_X86_REG_EAX,view)));assert q['completion']=='complete_original_return'
    z=t.run('source_safe_outer_rectangles',0x2f690,(GRID,SPAN,OUTER),((UC_X86_REG_ESI,POS),(UC_X86_REG_EBX,SAFE)));assert z['completion']=='complete_original_return'
    group['views'].append(dict(view=view,position_call=q,rectangle_call=z,grid4_hex=bytes(t.g.mem_read(GRID,4)).hex(),span4_hex=bytes(t.g.mem_read(SPAN,4)).hex(),position4_hex=bytes(t.g.mem_read(POS,4)).hex(),outer8_hex=bytes(t.g.mem_read(OUTER,8)).hex(),safe8_hex=bytes(t.g.mem_read(SAFE,8)).hex(),outer=list(struct.unpack('<4h',t.g.mem_read(OUTER,8))),safe=list(struct.unpack('<4h',t.g.mem_read(SAFE,8)))))
   cells.append(group)
 listingdir=a.output.parent/'instructions';listingdir.mkdir(exist_ok=True);manifest={};listings=[]
 def save(image,code,e,starts,side):
  _,ins=literal(code,e,starts);p=listingdir/f'{side}-{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));manifest[p.name]=sha(p);listings.append(dict(side=side,entry=f'{e:08x}',file=p.name,instructions=len(ins),sha256=sha(p),raw_operands=raw_operands(ins,image)));return ins
 for e in sorted(t.entries|set(STATIC)):save(t.image,t.code,e,t.starts,'u5')
 old=Xbe(ROOT/'assets/disc/default.xbe');oc=_Code(old,'.text');os={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/retail/functions.tsv').open(),delimiter='\t')};pairs=[]
 for oe,ne in PAIRS:
  oi=save(old,oc,oe,os,'base');_,ni=literal(t.code,ne,t.starts);equal=complete_tokens(oi,old)==complete_tokens(ni,t.image);assert equal,(hex(oe),hex(ne))
  pairs.append(dict(base_entry=f'{oe:08x}',later_entry=f'{ne:08x}',complete_instructions=len(oi),complete_instructions_internal_branches_equal=equal,relocated_operands=references(oi,ni,old,t.image),call_targets=[dict(base_site=f'{x.address:08x}',later_site=f'{y.address:08x}',base_operand=x.op_str,later_operand=y.op_str)for x,y in zip(oi,ni)if x.mnemonic=='call']))
 p=listingdir/'executed-original.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in(t.code.at(e)for e in sorted(t.visited))));manifest[p.name]=sha(p)
 mp=a.output.parent/'instruction-manifest.json';mp.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
 report=dict(schema='scoreboard-live-inputs-original-v1',oracle_sha256=sha(__file__),source_xbe_sha256=NEW_PIN,preserved_pins=PINS,boundaries=__doc__,parameter_metadata=rows,source_variant=variant,actual_parameter_reads=getters,source_margin_prefix=prefix,source_grid_cells=cells,complete_source_pairs=pairs,instruction_listings=listings,instruction_manifest_sha256=sha(mp),services=t.services,instruction_patches=[],supplied_game_graphics_readiness_clock_results=[])
 a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 for p,h in PINS.items():assert sha(ROOT/p)==h,p
 print(json.dumps(dict(metadata=len(rows),reads=[r['returned_signed_int']for r in getters],cells=sum(len(c['views'])for c in cells),pairs=len(pairs),services=len(t.services))))
if __name__=='__main__':main()

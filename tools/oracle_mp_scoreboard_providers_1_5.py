#!/usr/bin/env python3
"""Execute unchanged original ordinary-scoreboard name/palette provider kernels.

UTF16 names and output storage are declared API inputs. Literal original name
mapping tables and actual Lockout tag bytes are retained. The original cache
loader tail owns the scenario/matg stores, from literal cache header datums and
an explicit imported tag-table boundary; full preceding disk loading is not
claimed. Full profile->wire/player binding and dynamic glyph callbacks remain
explicit when unavailable. No game, graphics, readiness or clock result is
supplied.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import *
from oracle_mp_full_scoreboard_1_5 import Trace,ROOT,NEW,NEW_PIN,PIN,CACHE_PIN,HudCache,literal,sha,F,Xbe
from oracle_smg_firing_1_1 import _Code
from research_mp_lobby_later_update import complete_tokens,references
import csv
OUT=ROOT/'analysis/multiplayer/title-update-1.5/scoreboard-providers'
N,D,A=F+0x70000,F+0x72000,F+0x74000
PINS={
 'tools/oracle_mp_full_scoreboard_1_5.py':'fd65899fb9ced274fd7d2442ab151223ab066f85d87cb6ea960abd18fbbddfb7',
 'docs/MP_FULL_SCOREBOARD_RESEARCH.md':'2c6fce3ca2b88ec6eae7f7ce41c81748dcd01c277f220b39cbb1a4a89df5be0c',
 'analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json':'a0eecfb5dd9752c4d1185e64215aa569cf63b27b81c786664df2b81015c8fdf2',
 'include/halo2/full_scoreboard.hpp':'e43a6eb94dea67f989b85062ec8437f1d29270f95d66feb5bcff601561f21393',
 'src/full_scoreboard.cpp':'89b2ed16d722e2ec448a05f402393bc621227e77d785446812b8a8e7427e5e98',
 'tests/full_scoreboard_tests.cpp':'50a3688b393f0d31ef9e80290e35b10ef04b76438786d05b86b9b66f18e85e4a'}
class ProviderTrace(Trace):
 def hook(self,g,a,n,user):
  if a in {0x233cf1,0x830c0,0x1aa2b0,0x1aa360,0x125434,0x141750,0x141640,0x141f50,0x141bc0,0x1f050,0x1fa30,0x233faa}:
   sp=g.reg_read(UC_X86_REG_ESP);self.calls.append(dict(phase=self.phase,entry=f'{a:08x}',registers=self.reg(),stack64_hex=bytes(g.mem_read(sp,64)).hex()))
  super().hook(g,a,n,user)
def wide(t,p):
 b=bytearray()
 for i in range(2048):
  ch=t.u(p+i*2,2)
  if not ch:return bytes(b)
  b+=struct.pack('<H',ch)
 raise RuntimeError('unterminated original UTF16')
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'draft/report.json');args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in PINS.items():assert sha(ROOT/p)==h,p
 assert sha(NEW)==NEW_PIN
 c=HudCache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN;t=ProviderTrace(c)
 tokens=[]
 for i in range(18):
  at=0x455e48+i*8;p=t.u(at);s=wide(t,p)
  tokens.append(dict(index=i,address=f'{at:08x}',record8_hex=bytes(t.g.mem_read(at,8)).hex(),string_pointer=f'{p:08x}',text_utf16_hex=s.hex(),glyph=f'{t.u(at+4,2):04x}'))
 glyphs=[]
 for i in range(131):
  at=0x47c91c+i*12
  glyphs.append(dict(index=i,address=f'{at:08x}',record12_hex=bytes(t.g.mem_read(at,12)).hex(),glyph=f'{t.u(at):04x}',handler=f'{t.u(at+4):08x}',unused=f'{t.u(at+8):08x}'))
 names=[]
 fixtures=[('empty',''),('ordinary','Ada'),('long_ordinary','PlayerABCDEFGHIJKLMNOPQRSTUVWXYZ'),('literal_unicode','Å\u03a9\u4e2d'),('unlisted_private','\uf8ff'),('literal_bar','A|B'),('source_capacity255',':)'+('X'*253))]
 fixtures += [('literal_token_'+str(r['index']),bytes.fromhex(r['text_utf16_hex']).decode('utf-16le'))for r in tokens]
 fixtures += [('embedded_token_'+str(r['index']),'A'+bytes.fromhex(r['text_utf16_hex']).decode('utf-16le')+'Z')for r in tokens]
 for name,s in fixtures:
  raw=s.encode('utf-16le',errors='surrogatepass');t.g.mem_write(N,b'\xcc'*2048);t.g.mem_write(N,raw+b'\0\0');t.watch=[(N,2048)]
  r=t.run(name,0x233cf1,(N,256));r.update(input_utf16_hex=raw.hex(),capacity=256,output_utf16_hex=wide(t,N).hex(),raw_output2048_hex=bytes(t.g.mem_read(N,2048)).hex());names.append(r)
 # Private glyph callback provenance is kept as a stop, not fabricated output.
 first=next(r for r in glyphs if int(r['handler'],16))
 t.g.mem_write(N,struct.pack('<HH',int(first['glyph'],16),0));t.watch=[(N,2048)]
 cb=t.run('first_private_callback_dependency',0x233cf1,(N,256),block=int(first['handler'],16));cb['table_record']=first;cb['stop']['kind']='bounded_stop_before_dynamic_glyph_callback';cb['caller_frame_kind']='declared_API_call'
 t.g.mem_write(N,struct.pack('<HH',int(first['glyph'],16),0));cb_attempt=t.run('actual_private_callback_first_dependency',0x233cf1,(N,256));cb_attempt['table_record']=first
 if cb_attempt['completion']=='complete_original_return':cb_attempt['output_utf16_hex']=wide(t,N).hex()
 # Execute the original complete profile default constructor into raw storage.
 t.g.mem_write(D,b'\xcc'*0x1e0);t.watch=[(D,0x1e0)]
 profile=t.run('original_full_profile_defaults',0x1aa360,regs=((UC_X86_REG_EDX,D),));profile['output1e0_hex']=bytes(t.g.mem_read(D,0x1e0)).hex();profile['appearance4_hex']=bytes(t.g.mem_read(D+0x118,4)).hex()
 t.watch=[(A,12)];cold=t.run('palette_without_loaded_scenario',0x1aa2b0,regs=((UC_X86_REG_EAX,A),(UC_X86_REG_ECX,10)));cold.update(scenario_pointer_before=f'{t.u(0x4ecc20):08x}',signed_index=10,output12_hex=bytes(t.g.mem_read(A,12)).hex())
 # Original cache-loader tail resolves scenario and matg from the actual header.
 index=c.u(16);scenario=c.u(index+12);matg=c.u(index+16)
 t.w(0x556d90,c.magic+index);t.watch=[(0x4ecc1c,8),(0x474c20,4)]
 owner=t.run('original_cache_loader_scenario_matg_store_tail',0x125434,regs=((UC_X86_REG_EBP,scenario),),tail=0x130)
 owner['caller_frame_kind']='original1252E0_tail_saved0x130_frame';owner.update(literal_index32_hex=bytes(c.data[index:index+32]).hex(),literal_index_offset=index,scenario_datum=f'{scenario:08x}',matg_datum=f'{matg:08x}',before_boundary='existing Trace import of literal tag-table records and cache backing; EBP actual scenario tag from header+0C and original0x130-byte saved frame; no preceding I/O success return supplied',scenario_pointer=f'{t.u(0x4ecc20):08x}',matg_pointer=f'{t.u(0x4ecc1c):08x}')
 assert t.u(0x4ecc20)==c.magic+c.scenario and t.u(0x4ecc1c)==t.matg
 palette=c.block(c.tags[matg]['offset']+0x160,12);records=[bytes(c.data[p:p+12]).hex()for p in palette]
 pal=[]
 for i in [-128,-1,*range(len(palette)),len(palette),127]:
  t.watch=[(A,12)];r=t.run('palette_index_'+str(i),0x1aa2b0,regs=((UC_X86_REG_EAX,A),(UC_X86_REG_ECX,i&0xffffffff)));r.update(signed_index=i,output12_hex=bytes(t.g.mem_read(A,12)).hex());pal.append(r)
 # Full ordinary FFA provider invokes the four signed lookups in original order.
 t.g.mem_write(N,bytes(t.g.mem_read(D+0x118,4)));t.watch=[(A,48)]
 color=t.run('ordinary_FFA_default_profile_appearance_API',0x830c0,(-1,0),((UC_X86_REG_EDI,N),(UC_X86_REG_ESI,A)));color.update(profile4_hex=bytes(t.g.mem_read(N,4)).hex(),output48_hex=bytes(t.g.mem_read(A,48)).hex(),firstRGB12_hex=bytes(t.g.mem_read(A,12)).hex(),boundary='appearance is actual full profile constructor+118; full profile to player+84 bridge remains unexecuted, supplied four-byte provider input')
 # Source text-color normalization, not a menu-specific inferred flag mapping.
 colors=[]
 for label,v in [('white',(1.,1.,1.,1.)),('palette_scoreboard',(1.,*(struct.unpack('<3f',bytes.fromhex(color['firstRGB12_hex']))))),('bright',(1.,.8,.9,1.))]:
  t.g.mem_write(N,struct.pack('<4f',*v));t.watch=[(0x4f3c70,0x38)];r=t.run('full_text_color_'+label,0x141750,regs=((UC_X86_REG_EAX,N),));r.update(input16_hex=struct.pack('<4f',*v).hex(),text_state38_hex=bytes(t.g.mem_read(0x4f3c70,0x38)).hex());colors.append(r)
 listingdir=args.output.parent/'instructions';listingdir.mkdir(exist_ok=True);manifest={}
 for e in sorted(t.entries|{0x233cf1,0x830c0,0x1aa2b0,0x1aa360,0x1252e0,0x141750,0x141640,0x141bc0,0x1f050,0x1fa30,0x1f470,0x141f50,0x1421e0,0x163ea0,0x162a00}):
  f,ins=literal(t.code,e,t.starts);p=listingdir/f'{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));manifest[p.name]=sha(p)
 (args.output.parent/'instruction-manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
 pairs=[];table_sources={}
 images=[('base',Xbe(ROOT/'assets/disc/default.xbe'),[(0x13ec70,0x141750),(0x13eb60,0x141640),(0x13f0e0,0x141bc0),(0x13f470,0x141f50),(0x13f700,0x1421e0),(0x1f490,0x1f470),(0x1a0750,0x1aa360)]),('TU11',Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),[(0x22f40e,0x233cf1),(0x7ff50,0x830c0),(0x1a4000,0x1aa2b0),(0x1a40b0,0x1aa360)])]
 for side,image,mapping in images:
  code=_Code(image,'.text')
  if side=='TU11':
   ts=[];gs=[]
   for i in range(18):
    a=0x451548+8*i;ptr=int.from_bytes(image.read(a,4),'little');b=bytearray()
    for k in range(255):
     w=image.read(ptr+k*2,2)
     if w==b'\0\0':break
     b+=w
    glyph=int.from_bytes(image.read(a+4,2),'little');assert bytes(b).hex()==tokens[i]['text_utf16_hex'] and f'{glyph:04x}'==tokens[i]['glyph']
    ts.append(dict(record8_hex=image.read(a,8).hex(),text_utf16_hex=bytes(b).hex()))
   for i in range(131):
    a=0x4776ec+12*i;b=image.read(a,12);assert int.from_bytes(b[:4],'little')==int(glyphs[i]['glyph'],16)
    gs.append(b.hex())
   table_sources[side]=dict(tokens_address='00451548',glyphs_address='004776ec',tokens=ts,glyph_records12_hex=gs)
  for old,new in mapping:
   _,oi=literal(code,old,set());_,ni=literal(t.code,new,t.starts)
   equal=complete_tokens(oi,image)==complete_tokens(ni,t.image);assert equal,(side,hex(old),hex(new))
   p=listingdir/f'{side}-{old:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in oi));manifest[p.name]=sha(p)
   pairs.append(dict(side=side,source_image_sha256=hashlib.sha256(image.data).hexdigest(),old_entry=f'{old:08x}',u5_entry=f'{new:08x}',instructions=len(oi),complete_shape_branches_equal=equal,relocated_data_operands=references(oi,ni,image,t.image)))
 (args.output.parent/'instruction-manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
 out=dict(schema='scoreboard-providers-original-v1',oracle_sha256=sha(__file__),source_xbe_sha256=NEW_PIN,lockout_sha256=CACHE_PIN,preserved_pins=PINS,boundaries=__doc__,tokens=tokens,private_glyphs=glyphs,name_cases=names,private_callback_stop=cb,private_callback_actual_attempt=cb_attempt,palette_cold_case=cold,source_table_mapping=table_sources,complete_source_pairs=pairs,renderer_literal_state={f'{a:08x}':t.image.read(a,n).hex()for a,n in((0x474c2c,4),(0x474c34,4),(0x44aad8,16),(0x44aaf8,16),(0x46a438,4),(0x46a2ec,4))},profile_constructor=profile,cache_pointer_owner=owner,palette=dict(count=len(records),records12_hex=records,default_pointer=f'{t.u(0x474c70):08x}',default12_hex=bytes(t.g.mem_read(t.u(0x474c70),12)).hex()),palette_cases=pal,ordinary_color=color,text_color_cases=colors,instruction_manifest=manifest,instruction_patches=[],supplied_gameplay_graphics_readiness_clock_returns=[])
 args.output.write_text(json.dumps(out,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(name_cases=len(names),palette_cases=len(pal),profile=profile['appearance4_hex'],scenario=owner['scenario_pointer'],RGB=color['firstRGB12_hex'],completion=owner['completion'])))
 for p,h in PINS.items():assert sha(ROOT/p)==h,p
if __name__=='__main__':main()

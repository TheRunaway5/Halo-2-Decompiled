#!/usr/bin/env python3
"""Bounded original full-scoreboard layout, held fade and renderer admission.

The pool/options/clock/control records are declared API stimuli. No join, local
binding, readiness, contact, graphic result or gameplay score producer is supplied
as a recovered owner. Raw cache metadata stays unchanged and its serialized
locale-loaded field stays false. Only XGetLanguage is a declared host-language
provider. All game, sorting, string, arithmetic and widget instructions execute.
"""
import argparse,collections,csv,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN,CACHE_PIN
from inspect_multiplayer_hud import HudCache
from research_mp_lobby_later_update import NEW,NEW_PIN,complete_tokens,references,raw_operands,table_pair,sha
OUT=ROOT/'analysis/multiplayer/title-update-1.5/full-scoreboard'
F,S,STOP=0x1000000,0x21fd000,0x3000000
G,M,P,R,B,U,O,C,L=[F+v for v in(0x40000,0x42000,0x44000,0x46000,0x4a000,0x4c000,0x50000,0x54000,0x56000)]
PAIRS=((0x161790,0x162b70),(0x161a90,0x162e70),(0x161f80,0x163360),(0x162ac0,0x163ea0),
 (0x164c40,0x166020),(0x164cf0,0x1660d0),(0x164da0,0x166180),(0x164e50,0x166230),
 (0x15d160,0x15e540),(0x15aec0,0x15c2a0),(0x15d7c0,0x15eba0),(0x241500,0x2486f0),
 (0x241d30,0x248f20),(0x241880,0x248a70),(0x1642f0,0x1656d0),(0x15c930,0x15dd10),
 (0x163200,0x1645e0),(0x164f00,0x1662e0),(0x164fd0,0x1663b0),(0x161620,0x162a00),
 (0x13f5a0,0x140910),(0x13f6f0,0x140a60),(0x164c10,0x165ff0),(0x164be0,0x165fc0),
 (0x163280,0x164660),(0x37eb40,0x3825d0))
FROZEN={
 'include/halo2/multiplayer_kbm.hpp':'0f00df7572bf32e17db48cdb7dba430f89a5c46b804b9b1a43e2eb5b2e952028',
 'src/input/multiplayer_kbm.cpp':'35940b37504d6efd7ba717cba4ab07f7a98e75a1b6f0e03035f405ef193fc3b9',
 'tests/multiplayer_kbm_tests.cpp':'7c6d1abc6dad1461c144737b435b9e635cace2558364eda8f6016d4df288c88f',
 'analysis/multiplayer/integration/spawn-source/kbm-native-adapter/report.json':'a485d5ffd77ab5fcc7943b4cea37566354c16bc5cc526986d0382dcb4acb24c5'}
REGS=(('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),('ebp',UC_X86_REG_EBP),('esp',UC_X86_REG_ESP),('eflags',UC_X86_REG_EFLAGS),('xmm0',UC_X86_REG_XMM0),('xmm1',UC_X86_REG_XMM1),('xmm2',UC_X86_REG_XMM2),('xmm3',UC_X86_REG_XMM3))
def literal(code,e,starts):
 f=_trace(code,e,starts|{e});ins=[code.at(a)for a in sorted(f.code)];return f,ins
class Trace:
 def __init__(self,cache):
  self.image=Xbe(NEW);self.code=_Code(self.image,'.text');self.g=Uc(UC_ARCH_X86,UC_MODE_32);self.g.mem_map(0x10000,0x800000)
  for section in self.image.sections:self.g.mem_write(section.va,self.image.section_bytes(section))
  self.g.mem_map(F,0x100000);self.g.mem_map(0x2000000,0x200000);self.g.mem_map(STOP,0x1000)
  page=cache.magic&~4095;self.g.mem_map(page,((len(cache.data)+cache.magic-page+4095)//4096)*4096);self.g.mem_write(cache.magic,bytes(cache.data))
  tags=F+0x20000
  for datum,tag in cache.tags.items():
   if tag['offset'] is not None:self.w(tags+(datum&65535)*16+8,cache.magic+tag['offset'])
  matg=next(t for t in cache.tags.values()if t['group']=='matg');self.matg=cache.magic+matg['offset'];self.w(0x4ecc1c,self.matg);self.w(0x4f0414,tags)
  self.starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}|{e for _,e in PAIRS}
  self.visited=set();self.entries=set();self.calls=[];self.stores=[];self.services=[];self.recent=collections.deque(maxlen=24);self.phase='initial';self.block=None;self.stopped=None;self.watch=[];self.entry_counts=collections.Counter()
  self.g.hook_add(UC_HOOK_CODE,self.hook);self.g.hook_add(UC_HOOK_MEM_INVALID,self.invalid);self.g.hook_add(UC_HOOK_MEM_WRITE,self.write)
  self.g.reg_write(UC_X86_REG_FPCW,0x27f);self.g.reg_write(UC_X86_REG_MXCSR,0x1f80)
 def w(self,a,v):self.g.mem_write(a,struct.pack('<I',v&0xffffffff))
 def h(self,a,v):self.g.mem_write(a,struct.pack('<H',v&65535))
 def f(self,a,v):self.g.mem_write(a,struct.pack('<f',v))
 def u(self,a,n=4):return int.from_bytes(self.g.mem_read(a,n),'little')
 def reg(self):return{n:f'{self.g.reg_read(r):032x}'if n.startswith('xmm')else f'{self.g.reg_read(r):08x}'for n,r in REGS}
 def hook(self,g,a,n,user):
  self.recent.append(a);self.visited.add(a)
  if a in self.starts:
   self.entries.add(a);self.entry_counts[a]+=1
   if a in {e for _,e in PAIRS}|{0x1a9d90,0x1a9f60,0x1a9ee0,0x15bb50,0x69e80,0x37cde8,0x2325ca,0x33a4a6}:
    sp=g.reg_read(UC_X86_REG_ESP);self.calls.append(dict(phase=self.phase,entry=f'{a:08x}',registers=self.reg(),stack64_hex=bytes(g.mem_read(sp,64)).hex()))
  if a==0x33a4a6:
   # Explicit platform locale API. No game field or locale-loaded byte is written.
   sp=g.reg_read(UC_X86_REG_ESP);self.services.append(dict(phase=self.phase,entry=f'{a:08x}',boundary='host XGetLanguage provider',raw_language_code=1,returned_eax='00000001',return_address=f'{self.u(sp):08x}',stack64_hex=bytes(g.mem_read(sp,64)).hex()))
   g.reg_write(UC_X86_REG_EAX,1);g.reg_write(UC_X86_REG_EIP,self.u(sp));g.reg_write(UC_X86_REG_ESP,sp+4)
  if a==self.block:
   sp=g.reg_read(UC_X86_REG_ESP);self.stopped=dict(phase=self.phase,kind='stop_before_original_graphics_request',entry=f'{a:08x}',registers=self.reg(),stack64_hex=bytes(g.mem_read(sp,64)).hex());g.emu_stop()
 def invalid(self,g,access,a,n,v,user):
  self.stopped=dict(phase=self.phase,kind='unavailable_original_dependency',eip=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',access=access,size=n,value=v,recent=[f'{p:08x}'for p in self.recent]);return False
 def write(self,g,access,a,n,v,user):
  if any(p<=a<p+size for p,size in self.watch):
   self.stores.append(dict(phase=self.phase,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',size=n,before_hex=bytes(g.mem_read(a,n)).hex(),value_hex=f'{v&((1<<(8*n))-1):0{2*n}x}'))
 def run(self,name,e,args=(),regs=(),tail=False,block=None):
  self.phase=name;self.stopped=None;self.block=block;self.w(S,STOP)
  for i,v in enumerate(args):self.w(S+4+i*4,v)
  self.g.reg_write(UC_X86_REG_ESP,S-(24 if tail is True else tail) if tail else S)
  for k,v in regs:self.g.reg_write(k,v)
  before=self.reg();begin=len(self.calls);writebegin=len(self.stores);servicebegin=len(self.services)
  try:self.g.emu_start(e,STOP,count=1000000)
  except UcError:assert self.stopped is not None
  end=self.g.reg_read(UC_X86_REG_EIP);assert end==STOP or self.stopped is not None,(name,hex(end))
  return dict(name=name,entry=f'{e:08x}',arguments=[f'{v:08x}'for v in args],caller_frame_kind=('original15E540tail_saved_register_frame'if tail is True else'original15C2A0tail_saved_register_frame')if tail else'declared_API_call',before_registers=before,after_registers=self.reg(),completion='complete_original_return'if end==STOP else'stopped_original_instruction',defined_al=self.g.reg_read(UC_X86_REG_EAX)&255,fpcw=f'{self.g.reg_read(UC_X86_REG_FPCW):04x}',mxcsr=f'{self.g.reg_read(UC_X86_REG_MXCSR):08x}',calls=self.calls[begin:],stores=self.stores[writebegin:],services=self.services[servicebegin:],stop=self.stopped)
 def fixture(self):
  for a,v in((0x4f3218,G),(0x4f63b8,M),(0x4f54f4,P),(0x4f54f0,U),(0x51df54,C)):self.w(a,v)
  self.w(P+0x20,16);self.w(P+0x24,0x21c);self.w(P+0x38,4);self.w(P+0x44,R);self.w(P+0x48,B);self.w(B,15)
  self.h(M+0x6c,1);self.w(M+0xc04,1);self.w(M+0xc14,2);self.g.mem_write(G+0xc,b'\x04');self.w(G+0x180,2)
  self.h(C+2,30);self.f(C+4,1/30);self.w(C+8,101)
  self.g.mem_write(0x4c65f0,struct.pack('<4h',0,0,480,640));self.g.mem_write(0x4c65f8,struct.pack('<4h',24,32,456,608))
  rows=[('Ada',8),('Grace',8),('Edsger',3),('Barbara',-1)]
  for i,(name,score)in enumerate(rows):
   p=R+i*0x21c;self.h(p,0x8000+i);self.h(p+2,1);self.w(p+0x24,0xffffffff);self.h(p+0x28,0 if i==0 else -1);self.w(p+0x2c,0xe2700000+i);self.w(p+0x1b0,-1);self.g.mem_write(p+0xc0,bytes([i]));self.g.mem_write(p+0x44,(name+'\0').encode('utf-16le'));self.h(M+0x304+i*0x1c+4,score)
  self.w(U+0xc,0x80000000);self.g.mem_write(O,b'\xcc'*0x293c)
  return dict(player_datums=[f'{0x80000000+i:08x}'for i in range(4)],names_scores=rows,scope='explicit pool/stat/options/control/clock API stimuli, not source allocation/join/score/local readiness',raw_records=self.records(),rectangle_outer=[0,0,480,640],rectangle_inner=[24,32,456,608])
 def records(self):return{name:bytes(self.g.mem_read(p,n)).hex()for name,p,n in(('game',G,0x1200),('engine',M,0xc18),('pool',P,0x4c),('players',R,4*0x21c),('bitmap',B,4),('local_users',U,0x20),('clock',C,12),('scoreboard_globals',0x4f63c0,24),('processed_control',0x4f2aac,0x48))}
 def widget(self,p,n,valid):
  raw=bytes(self.g.mem_read(p,valid+1));text=raw[0x38:0x38+2*n].decode('utf-16le').split('\0')[0]
  return dict(address=f'{p:08x}',valid=raw[valid],rectangle=list(struct.unpack_from('<4h',raw)),text_rectangle=list(struct.unpack_from('<4h',raw,0xa)),right_justified=raw[8],text=text,text_utf16_hex=raw[0x38:0x38+2*n].hex(),background_bits=[f'{v:08x}'for v in struct.unpack_from('<4I',raw,0x14)],text_color_bits=[f'{v:08x}'for v in struct.unpack_from('<4I',raw,0x24)],raw_hex=raw.hex())
 def output(self,p):
  n=self.u(p+0x2938);assert n<=16
  return dict(raw293c_hex=bytes(self.g.mem_read(p,0x293c)).hex(),columns=list(self.g.mem_read(p,3)),team_count=self.u(p+0xcb4),player_count=n,title=self.widget(p+4,80,0xd8),headers={name:self.widget(p+at,20,0x60)for name,at in(('place',0xe0),('name',0x144),('wins',0x1a8),('score',0x20c))},players=[dict(index=i,full_datum=f'{self.u(p+0xcb8+i*0x1c8+0x7c):08x}',current_player=self.u(p+0xcb8+i*0x1c8+0x1c4,1),widgets={name:self.widget(p+0xcb8+i*0x1c8+at,n,v)for name,at,n,v in(('place',0,6,0x44),('icon',0x48,6,0x44),('name',0x90,20,0x60),('wins',0xf4,6,0x44),('score',0x13c,6,0x44),('connection',0x184,2,0x3c))})for i in range(n)])
def original_metadata(c):
 matg=next(t for t in c.tags.values()if t['group']=='matg');m=c.tags[c.u(matg['offset']+0x16c)];un=c.block(m['offset'],0x20)[0];runtime=c.block(m['offset']+8,0x568)[0];tag=c.tags[c.u(un+0x1c)];start,count=struct.unpack_from('<HH',c.data,tag['offset']+0x10)
 total,size,indices,strings=struct.unpack_from('<4I',c.data,matg['offset']+0x190);wanted={0x07000105,0x070000fd,0x08000102};labels=[]
 for i in range(start,start+count):
  sid,o=struct.unpack_from('<II',c.data,indices+i*8)
  if sid in wanted:labels.append(dict(string_id=f'{sid:08x}',sid_name=c.sid(sid),index=i,index_file_offset=indices+i*8,index8_hex=bytes(c.data[indices+i*8:indices+i*8+8]).hex(),utf8_file_offset=strings+o,text=c.text(strings+o)))
 return dict(matg=matg,mulg=m,universal_offset=un,universal20_hex=bytes(c.data[un:un+0x20]).hex(),runtime_offset=runtime,runtime568_hex=bytes(c.data[runtime:runtime+0x568]).hex(),global_messages_tag=tag,english_slice=[start,count],english_disk_record28_hex=bytes(c.data[matg['offset']+0x188:matg['offset']+0x1a4]).hex(),english_file_storage=dict(count=total,size=size,indices=indices,strings=strings),labels=labels,font_table= (ROOT/'assets/disc/maps/fonts/font_table.txt').read_text(),font_table_sha256=sha(ROOT/'assets/disc/maps/fonts/font_table.txt'))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in FROZEN.items():assert sha(ROOT/p)==h,p
 assert sha(NEW)==NEW_PIN;assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN;assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 c=HudCache(ROOT/'assets/disc/maps/lockout.map');t=Trace(c);metadata=original_metadata(c);inputs=t.fixture();t.watch=[(O,0x293c),(0x4f63c0,24),(R,4*0x21c),(0x56dfa0,44),(0x48c750,4)]
 ctor=t.run('original_engine_table_CRT_constructor',0x3825d0)
 layout=t.run('full_roster_builder',0x163360,(0x80000000,O,0x3f800000));assert layout['completion']=='complete_original_return';layout['output']=t.output(O)
 assert [(r['widgets']['name']['text'],r['widgets']['place']['text'],r['widgets']['score']['text'])for r in layout['output']['players']]==[('Grace','1','8'),('Ada','1','8'),('Edsger','3','3'),('Barbara','4','-1')]
 # Independent source locale search with literal disk index/string backing.
 # This API record avoids inventing the loader's +18 readiness byte.
 store=metadata['english_file_storage'];t.w(L,c.magic+store['indices']);t.w(L+4,c.magic+store['strings']);t.w(L+8,store['count']);labels=[]
 for r in metadata['labels']:
  result=t.run('original_literal_locale_index_lookup_'+r['sid_name'],0x1a9ee0,regs=((UC_X86_REG_EAX,metadata['english_slice'][0]),(UC_X86_REG_ECX,metadata['english_slice'][1]),(UC_X86_REG_EDX,L),(UC_X86_REG_EDI,int(r['string_id'],16))))
  ptr=t.g.reg_read(UC_X86_REG_EAX);b=bytearray();q=ptr
  while t.u(q,1):b.append(t.u(q,1));q+=1
  result['lookup_record12_hex']=bytes(t.g.mem_read(L,12)).hex();result['string_id']=r['string_id'];result['returned_utf8_hex']=bytes(b).hex();result['returned_text']=b.decode();assert result['returned_text']==r['text'];labels.append(result)
 # Explicit processed request delivered at the scoreboard tail of15E540.
 # ESI,EBX and24-byte saved frame are owning caller inputs, not full phase proof.
 t.w(R+0x24,0);t.w(0x4f2a9c,1);t.watch.append((0x4f2aac,0x48));sequence=[]
 for i,held in enumerate([True]*10+[False]*9):
  t.g.mem_write(0x4f2aac+0x1d,bytes([1 if held else 0]));before=t.records();step=t.run('held_scoreboard_tail_'+str(i),0x15e6bc,(0x80000000,),((UC_X86_REG_ESI,R),(UC_X86_REG_EBX,0xffffffff)),tail=True);assert step['completion']=='complete_original_return';step['held_input']=held;step['before_records']=before;step['after_records']=t.records();step['counter']=t.u(0x4f63c0,1);predicate=t.run('HUD_suppression_predicate_'+str(i),0x1645e0,regs=((UC_X86_REG_ECX,0x80000000),));step['HUD_predicate_call']=predicate;sequence.append(step)
 assert [r['counter']for r in sequence]==[1,2,3,4,5,6,7,8,8,8,7,6,5,4,3,2,1,0,0]
 # Full view controller owns fade exponent and calls the renderer. Stop at the
 # first source emblem graphics request; no callback gets fabricated success.
 t.g.mem_write(0x4f63c0,b'\x08');t.w(0x4c66f8,0);controller_stop=t.run('full_current_view_display_dependency_stop',0x15c2a0,block=0x2325ca);render=t.run('current_view_display_tail_to_first_graphics_request',0x15c448,regs=((UC_X86_REG_EBX,0x80000000),(UC_X86_REG_EDI,0),(UC_X86_REG_EBP,M),(UC_X86_REG_XMM1,0)),tail=44,block=0x2325ca);assert render['completion']=='stopped_original_instruction'and render['stop']['entry']=='002325ca';renderer=next(r for r in render['calls']if r['entry']=='00163ea0');sp=int(renderer['registers']['esp'],16);render['renderer_roster_output']=t.output(sp-0x293c);render['renderer_alpha_bits']=renderer['stack64_hex'][8:16]
 # Complete original pairs, image operands and owned call-target constraints.
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');oc=_Code(old,'.text');os={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')};ns=t.starts;pair_rows=[];listings=[];mapping=dict(PAIRS);listingdir=a.output.parent/'instructions';listingdir.mkdir(exist_ok=True)
 def save(image,code,entry,starts,side):
  f,ins=literal(code,entry,starts);p=listingdir/f'{side}-{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));row=dict(side=side,entry=f'{entry:08x}',file=p.name,sha256=sha(p),instructions=len(ins),raw_operands=raw_operands(ins,image));listings.append(row);return f,ins
 for oe,ne in PAIRS:
  of,oi=save(old,oc,oe,os,'tu11');nf,ni=save(t.image,t.code,ne,ns,'u5');assert complete_tokens(oi,old)==complete_tokens(ni,t.image),(hex(oe),hex(ne));tables=table_pair(of,nf,oi,ni,old,t.image);assert tables is not None and all(r['topology_equal']for r in tables);constraints=[]
  for x,y in zip(oi,ni):
   if x.mnemonic=='call'and x.op_str.startswith('0x')and int(x.op_str,16)in mapping:
    target=mapping[int(x.op_str,16)];assert int(y.op_str,16)==target;constraints.append(dict(old_site=f'{x.address:08x}',later_site=f'{y.address:08x}',old_target=x.op_str,later_target=y.op_str))
  pair_rows.append(dict(old_entry=f'{oe:08x}',later_entry=f'{ne:08x}',complete_instructions=len(oi),complete_shape_internal_branch_topology_equal=True,switch_tables=tables,relocated_data_operands=references(oi,ni,old,t.image),mapped_direct_call_constraints=constraints))
 executed_entries=t.entries|{0x1a9ee0,0x37cde8,0x2325ca,0x1fa30,0x36570,0x36870,0x141750,0x1f050,0x33a4a6,0x15bb50,0x248ec0,0x248e40,0x248c90,0x830c0,0x83050,0x1aa2b0,0x160570,0x161530}
 for e in sorted(executed_entries-{n for _,n in PAIRS}):save(t.image,t.code,e,ns,'u5')
 manifest={r['file']:r['sha256']for r in listings};mp=a.output.parent/'instruction-manifest.json';mp.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
 report=dict(schema='bounded-original-full-scoreboard-v1',oracle_sha256=sha(__file__),source_xbe_sha256=NEW_PIN,comparison_xbe_sha256=PIN,lockout_sha256=CACHE_PIN,dependency_sha256={str(Path(m).relative_to(ROOT)):sha(m)for m in(__file__,ROOT/'tools/research_mp_lobby_later_update.py',ROOT/'tools/oracle_smg_firing_1_1.py',ROOT/'tools/inspect_multiplayer_hud.py',ROOT/'tools/reference/halo2-decompiled/tools/functions.py',ROOT/'tools/reference/halo2-decompiled/tools/xbe.py')},preserved_KBM_pins=FROZEN,boundaries=__doc__,tag_metadata=metadata,explicit_layout_inputs=inputs,original_constructor=ctor,full_layout=layout,literal_locale_lookup=labels,held_counter_sequence=sequence,full_view_controller_dependency_stop=controller_stop,first_graphics_stop=render,source_data_words={f'{p:08x}':dict(width=n,bytes_hex=t.image.read(p,n).hex())for p,n in((0x46a130,4),(0x46a120,4),(0x46a11c,4),(0x46a0cc,4),(0x46a0d0,4),(0x46a1d8,4),(0x456918,4),(0x46a22c,4),(0x46a240,4),(0x46a81c,4),(0x445b20,4),(0x46a378,8),(0x466164,10),(0x466170,16),(0x466180,8),(0x466188,4))},complete_source_pairs=pair_rows,instruction_listings=listings,instruction_manifest_sha256=sha(mp),observed_instruction_addresses=sorted(f'{p:08x}'for p in t.visited),platform_service_instruction_not_executed='0033a4a6',graphics_request_instruction_not_executed='002325ca',executed_entry_counts={f'{p:08x}':n for p,n in sorted(t.entry_counts.items())},instruction_patches=[],supplied_graphic_world_or_gameplay_returns=[],summary=dict(complete_source_pairs=len(pair_rows),positive_full_layouts=1,locale_lookups=len(labels),held_counter_calls=len(sequence),graphics_request_returns_supplied=0))
 a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 for p,h in FROZEN.items():assert sha(ROOT/p)==h,p
 print(json.dumps(report['summary']))
if __name__=='__main__':main()

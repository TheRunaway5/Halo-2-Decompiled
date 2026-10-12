#!/usr/bin/env python3
"""Execute unchanged whole submit from explicit retained source ABI inputs.

No system availability, controller, device, listener, RNG, or callback result is
supplied. Whole prior constructors are retained, not rerun/claimed complete.
"""
import argparse,json,struct,hashlib
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from oracle_sound_startup_native import AUDIO_ARENA,AUDIO_CAPACITY
from inspect_multiplayer import Cache
PRIOR=ROOT/'analysis/multiplayer/integration/sound-startup-native/measured/report.json'
PRIOR_PIN='b2a1175e7dd524fc7d9587333e8055abd8cb4d30f6417e0ff293e9d1e4444287'
RAW=ROOT/'analysis/multiplayer/map-pack/smg-audio-sample-boundary/source-fixture.json'
RAW_PIN='222245ded354b337a2e624beef2dbaf2609aa1721cecd383d0ef70b1756495d6'
CACHE=ROOT/'assets/disc/maps/lockout.map';CACHE_PIN='14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
PAIR=ROOT/'analysis/multiplayer/integration/sound-submit-native/paired-owners'
TAGS,STACK,STOP,REQUEST=0x1080000,0x200d000,0x3000000,0xa400000

def raw_call(q):
 if isinstance(q,dict):
  if q.get('entry')=='0021f220'and'play_state_before_hex'in q:return q
  for v in q.values():
   r=raw_call(v)
   if r:return r
 elif isinstance(q,list):
  for v in q:
   r=raw_call(v)
   if r:return r

def map_pairs(pair):
 pcs={};globals_map={};entries={}
 for q in pair['owners']:
  entries[int(q['old'],16)]=int(q['u5'],16)
  for a,b in zip(*[(PAIR/'instructions'/f'{q["old"]}-{tag}.asm').read_text().splitlines()for tag in('old','u5')]):pcs[int(a.split()[0],16)]=int(b.split()[0],16)
  for r in q['memory']:globals_map[int(r['old_va'],16)]=int(r['new_va'],16)
 return pcs,globals_map,entries

def run_image(tag,path,pair,out):
 assert sha(PRIOR)==PRIOR_PIN and sha(RAW)==RAW_PIN and sha(CACHE)==CACHE_PIN and sha(path)==PINS[str(path)]
 original=raw_call(json.loads(RAW.read_text()));raw=bytes.fromhex(original['play_state_before_hex']);assert len(raw)==160
 prior=json.loads(PRIOR.read_text())['stopped_original_caller'];x=Xbe(path);code=_Code(x,'.text');c=Cache(CACHE)
 pcs,global_map,entries=map_pairs(pair);reverse={b:a for a,b in pcs.items()}
 resident=[t for t in c.tags.values()if t['offset']is not None];base=(min(t['offset']for t in resident)+c.magic)&~4095;end=(max(t['offset']+t['size']for t in resident)+c.magic+4095)&~4095
 index=c.u(16);table=c.u(index+8)-(c.u(index)-(index+32));table_raw=bytes(c.data[table:table+c.u(index+24)*16])
 matg=next(t for t in c.tags.values()if t['group']=='matg');ugh=c.tags[c.u(c.block(matg['offset']+0xc0,0x24)[0]+0x20)];scenario=next(t for t in c.tags.values()if t['group']=='scnr')
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 for at,size in((TAGS,0x100000),(0x2000000,0x10000),(STOP,0x1000),(REQUEST,0x1000),(AUDIO_ARENA,AUDIO_CAPACITY),(base,end-base)):g.mem_map(at,size)
 g.mem_write(base,bytes(c.data[base-c.magic:min(len(c.data),end-c.magic)]));g.mem_write(TAGS,table_raw)
 def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
 def w(a,v,n=4):g.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 state=dict(system=int(prior['globals']['system'],16),tag_table=TAGS,current_matg=matg['offset']+c.magic,current_ugh=ugh['offset']+c.magic,current_scenario=scenario['offset']+c.magic,effect_pool=int(prior['globals']['effects'],16),controller_pool=int(prior['globals']['controllers'],16),controller_root=int(prior['globals']['controller_root'],16))
 bindings={0x4ed950:state['system'],0x4eb114:TAGS,0x4e791c:state['current_matg'],0x4e7920:state['current_scenario'],0x526464:state['current_ugh'],0x526470:state['effect_pool'],0x526468:state['controller_pool'],0x52646c:state['controller_root']}
 for a,v in bindings.items():w(a if tag=='old'else global_map[a],v)
 initial=bytes.fromhex(prior['arena_hex']);g.mem_write(AUDIO_ARENA,initial)
 sounds=[t for t in c.tags.values()if t['group']=='snd!'and t['offset']is not None]
 cases=[];executed=set();current=None
 def hook(_g,a,n,_u):
  if code.at(a):executed.add(a)
 def store(_g,k,a,n,v,_u):
  if AUDIO_ARENA<=a<AUDIO_ARENA+AUDIO_CAPACITY or REQUEST<=a<REQUEST+160:
   pc=g.reg_read(UC_X86_REG_EIP);current['stores'].append(dict(instruction=f'{pc if tag=="old"else reverse.get(pc,pc):08x}',actual_instruction=f'{pc:08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(n*8))-1):0{n*2}x}'))
 def invalid(_g,k,a,n,v,_u):current['invalid'].append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,access=k));return False
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
 specs=[(t,0,None,None)for t in sounds]
 reload_tag=next(t for t in sounds if t['datum']=='e5ad0438')
 specs +=[(reload_tag,value,0x100,None)for value in(0,0x40000000,0x80000000,0xc0000000,0xffffffff)]
 specs +=[(reload_tag,0x40000000,0x100,modifier)for modifier in('clear_callback_flag','existing_controller_flag','both_flags')]
 for sound,interface,flags,modifier in specs:
  g.mem_write(AUDIO_ARENA,initial);g.mem_write(REQUEST,raw)
  controls=[]
  if flags is not None:
   for at,size,val in((REQUEST,4,u(REQUEST)|flags),(REQUEST+0x98,4,interface)):
    controls.append(dict(address=f'{at:08x}',bytes=size,before=f'{u(at,size):0{size*2}x}',after=f'{val:0{size*2}x}'));w(at,val,size)
  if modifier:
   val=u(REQUEST)
   if modifier in('clear_callback_flag','both_flags'):val &=~0x20
   if modifier in('existing_controller_flag','both_flags'):val |=0x10
   controls.append(dict(address=f'{REQUEST:08x}',bytes=4,before=f'{u(REQUEST):08x}',after=f'{val:08x}'));w(REQUEST,val)
  before=bytes(g.mem_read(REQUEST,160)).hex();current=dict(sound=sound,modifier=modifier,controls=controls,request_before_hex=before,stores=[],invalid=[])
  g.mem_write(STACK,struct.pack('<II',STOP,int(sound['datum'],16)));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EAX,REQUEST);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
  try:g.emu_start(0x21f220 if tag=='old'else entries[0x21f220],STOP,count=1000000)
  except UcError:assert current['invalid']
  outcome='invalid_storage'if current['invalid']else'returned';assert current['invalid']or g.reg_read(UC_X86_REG_EIP)==STOP
  current.update(outcome=outcome,return_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',request_after_hex=bytes(g.mem_read(REQUEST,160)).hex(),arena_sha256=hashlib.sha256(bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY))).hexdigest(),source_return_supplied=False)
  assert u(state['system']+0x78,1)==0
  if outcome=='returned':assert g.reg_read(UC_X86_REG_EAX)==0xffffffff
  cases.append(current)
 listing=out/'instructions'/f'{tag}-executed.asm';rows=[code.at(a)for a in sorted(executed)]
 for i in rows:assert i.bytes==x.read(i.address,len(i.bytes))
 save(listing,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows).encode())
 metadata=dict(table_address=f'{TAGS:08x}',table_hex=table_raw.hex(),literal_table_file_offset=table,current_matg=matg,current_ugh=ugh,current_scenario=scenario,custom_playback_records=[dict(address=f'{a+c.magic:08x}',raw_hex=bytes(c.data[a:a+0x34]).hex())for a in c.block(ugh['offset']+0x30,0x34)],current_bank_provider='Explicit literal current-map/tag-bank ABI composition; complete map activation is not claimed')
 c.data.close();c.stream.close()
 return dict(image=tag,image_sha256=sha(path),state={k:f'{v:08x}'for k,v in state.items()},arena_initial_hex=initial.hex(),metadata=metadata,raw_request_origin=dict(source_fixture_sha256=RAW_PIN,original_entry=original['entry'],phase=original['phase'],original_stack_arguments=original['stack'],raw_request_hex=raw.hex(),supplied_to_new_call_as_explicit_raw_ABI_composition=True,unowned_bytes_retained_not_initialized=True),cases=cases,listing=dict(file=str(listing.relative_to(out)),sha256=sha(listing),rows=len(rows)))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();pair=json.loads((PAIR/'report.json').read_text());images=[run_image(t,p,pair,a.output)for t,p in(('old',OLD),('u5',NEW))]
 r=dict(schema='original-raw-sound-submit-cold-owner-v1',oracle_sha256=sha(__file__),prior_startup_report_sha256=PRIOR_PIN,prior_raw_request_fixture_sha256=RAW_PIN,cache_sha256=CACHE_PIN,paired_report_sha256=sha(PAIR/'report.json'),images=images,instruction_patches=[],platform_results=[],native_audio_connected=False,limits=['Whole21F220 executes unchanged with genuine prior source-startup constructor buffers at unavailable2205C0, not completed driver initialization.','Literal current map/tag table/ugh/scenario inputs are explicit ABI composition; no full88-callback map activation or later local listener/session is claimed.','Whole160byte previous original18BA10 output is explicit caller backing, including unowned stack bytes and obsolete source pointers. No semantic zero-padding or current-world producer is asserted.','Ordinary sound datum variations and flagged interface variations are declared ABI stimuli, not new animation events.','No availability, effect/controller/voice identity, admission/factory/permutation/RNG/listener or native device callback result is supplied. u5 consumes the same declared1.1 startup-layout/request inputs, not wholeu5startup.'])
 save(a.output/'report.json',encoded(r));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=sum(len(q['cases'])for q in images),returns=sum(c['outcome']=='returned'for q in images for c in q['cases']),stops=sum(c['outcome']=='invalid_storage'for q in images for c in q['cases']),stores=sum(len(c['stores'])for q in images for c in q['cases']))))
if __name__=='__main__':main()

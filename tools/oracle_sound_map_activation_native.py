#!/usr/bin/env python3
"""Unchanged map-activation CPU prefix with literal owned metadata.

Restores the entire previously source-created startup arena. No boot, driver,
plugin, capability, effect, ready flag or platform result is manufactured.
The original clock thunk reads real/replayed host boot milliseconds through
an explicitly routed raw data import. Invalid source memory access is retained.
"""
import argparse,json,struct,time
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,PINS,Xbe,_Code,sha,save,encoded
from inspect_multiplayer import Cache
from oracle_sound_startup_native import GLOBALS,AUDIO_ARENA,AUDIO_CAPACITY

PRIOR=ROOT/'analysis/multiplayer/integration/sound-startup-native'
PRIOR_PIN='b2a1175e7dd524fc7d9587333e8055abd8cb4d30f6417e0ff293e9d1e4444287'
CACHE=ROOT/'assets/disc/maps/lockout.map'
CACHE_PIN='14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
TAGS,STACK,STOP=0x1080000,0x200d000,0x3000000
STATE={'current_matg_address':0x4e791c,'sound_system_address':0x4ed950,'current_ugh_address':0x526464,
 'driver_address':0x526474,'environment_address':0x518480,'options_address':0x4edf18,'queue_budget':0x4865e8}

def run(name,change,out,replay):
 assert sha(OLD)==PINS[str(OLD)] and sha(CACHE)==CACHE_PIN
 assert sha(PRIOR/'measured/report.json')==PRIOR_PIN
 prior=json.loads((PRIOR/'measured/report.json').read_text())['stopped_original_caller']
 x=Xbe(OLD);code=_Code(x,'.text');c=Cache(CACHE)
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 for p,n in((TAGS,0x100000),(0x2000000,0x10000),(STOP,0x1000),(AUDIO_ARENA,AUDIO_CAPACITY),(0x80000000,0x1000)):g.mem_map(p,n)
 resident=[t for t in c.tags.values()if t['offset'] is not None]
 base=(min(t['offset']for t in resident)+c.magic)&~4095
 end=(max(t['offset']+t['size']for t in resident)+c.magic+4095)&~4095
 g.mem_map(base,end-base);start=base-c.magic
 g.mem_write(base,bytes(c.data[start:min(len(c.data),start+end-base)]))
 def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
 def w(a,v):g.mem_write(a,struct.pack('<I',v))
 tagmap=[]
 for datum,t in c.tags.items():
  if t['offset'] is not None:
   address=t['offset']+c.magic;w(TAGS+(datum&0xffff)*16+8,address);tagmap.append(dict(index=f'{datum&0xffff:04x}',address=f'{address:08x}',datum=t['datum'],group=t['group'],name=t['name']))
 w(0x4eb114,TAGS)
 g.mem_write(AUDIO_ARENA,bytes.fromhex(prior['arena_hex']))
 for key,va in GLOBALS.items():w(va,int(prior['globals'][key],16))
 matg=next(t for t in c.tags.values()if t['group']=='matg');w(0x4e791c,matg['offset']+c.magic)
 block=u(u(0x4e791c)+0xc4)
 controls=[]
 if change=='sncl_none':
  before=u(block+0x14);w(block+0x14,0xffffffff);controls.append(dict(address=f'{block+0x14:08x}',before=f'{before:08x}',after='ffffffff',meaning='Explicit branch control: sound-class datum NONE'))
 if change=='zero_globals_count':
  address=u(0x4e791c)+0xc0;before=u(address);w(address,0);controls.append(dict(address=f'{address:08x}',before=f'{before:08x}',after='00000000',meaning='Explicit branch control: current matg sound-global count zero'))
 sncl_datum=c.u(c.block(matg['offset']+0xc0,0x24)[0]+0x14);sncl=c.tags[sncl_datum]
 regions=[]
 for t in(matg,sncl):
  address=t['offset']+c.magic;raw=bytes(g.mem_read(address,t['size']))
  regions.append(dict(address=f'{address:08x}',bytes=len(raw),raw_hex=raw.hex(),mutable=False,tag=t))
 regions.append(dict(address=f'{AUDIO_ARENA:08x}',bytes=AUDIO_CAPACITY,raw_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex(),mutable=True,source='Entire previously produced original126B20 arena at its actual2205C0 stop'))
 def state():
  values={k:f'{u(v):08x}'for k,v in STATE.items()}
  values['runtime_environment_parameters_address']='00477548'
  return values
 before=state();g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
 stores=[];executed=[];clocks=[];boundary=None
 def clock():
  if replay is None:
   ns=time.clock_gettime_ns(time.CLOCK_BOOTTIME);row=dict(provider='actual clock_gettime_ns(CLOCK_BOOTTIME)',observed_ns=ns,tick_ms_u32=(ns//1000000)&0xffffffff)
  else:
   assert len(clocks)<len(replay);row=replay[len(clocks)]
  assert u(0x417cd4)==0x8000009c
  w(0x8000009c,row['tick_ms_u32']);clocks.append(row)
 def hook(_g,a,n,_u):
  executed.append(a)
  if a==0x337a40:clock()
 def write(_g,k,a,n,v,_u):
  if AUDIO_ARENA<=a<AUDIO_ARENA+AUDIO_CAPACITY or a in(0x526464,0x4865e8):
   stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(n*8))-1):0{n*2}x}'))
 def invalid(_g,k,a,n,v,_u):
  nonlocal boundary
  boundary=dict(access=k,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,value=f'{v&((1<<(n*8))-1):0{n*2}x}',registers={q:f'{g.reg_read(r):08x}'for q,r in(('eax',UC_X86_REG_EAX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI))},original_platform_result_supplied=False)
  return False
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
 try:g.emu_start(0x126de0,STOP,count=100000)
 except UcError:assert boundary
 else:raise AssertionError('Unexpected complete map activation with no source driver owner')
 expected={'authored':'002217e8','sncl_none':'00192cac','zero_globals_count':'00126df9'}[change]
 assert boundary['instruction']==expected,boundary
 assert u(u(0x4ed950)+0x78,1)==0 and u(0x526474)==0
 if replay is not None:assert len(clocks)==len(replay)
 rows=[code.at(a)for a in sorted(set(executed))]
 for q in rows:assert q.bytes==x.read(q.address,len(q.bytes))==bytes(g.mem_read(q.address,len(q.bytes)))
 listing=out/'instructions'/f'{name}.asm';save(listing,''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in rows).encode())
 output=dict(state=state(),arena_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex())
 c.data.close();c.stream.close()
 return dict(name=name,entry='00126de0',before=before,after=output,regions=regions,tag_table=tagmap,controls=controls,clock_calls=clocks,stores=stores,boundary=boundary,
   listing=dict(file=str(listing.relative_to(out)),sha256=sha(listing),rows=len(rows)),
   platform_callbacks_executed=[],whole_owner_returned=False)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--replay',type=Path);a=p.parse_args()
 replay=json.loads(a.replay.read_text())if a.replay else None
 cases=[run(name,change,a.output,None if replay is None else replay['cases'][i]['clock_calls'])for i,(name,change)in enumerate((('literal_lockout','authored'),('declared_sound_class_none','sncl_none'),('declared_sound_global_count_zero','zero_globals_count')))]
 report=dict(schema='original-sound-map-activation-prefix-v1',oracle_sha256=sha(__file__),image_sha256=sha(OLD),cache_sha256=sha(CACHE),prior_startup_report_sha256=PRIOR_PIN,
   cases=cases,source_instruction_patches=[],platform_success_returns=[],
   bounds=['The entire prior source-created original126B20 arena/globals at actual2205C0 are restored verbatim, without executing or admitting that device constructor.',
     'Current matg and low16 tag table are explicit literal owned Lockout metadata bindings. They are not the full original88-callback active-map producer. No inherited weapon/unit substitutions occupy matg/sncl table slots.',
     'Original337A40 executes unchanged and reads genuine/replayed host boot milliseconds at its data-import ordinal sentinel8000009C. This is declared emulator import routing, not a retail Xbox tick/address or clock-granularity claim.',
     'Only sound-class NONE and zero sound-global count cases modify authored inputs, at the exact retained addresses. No driver/environment/plugin/listener/admission or callback return is supplied.',
     'Original whole126DE0 remains stopped. Its earlier+7A store is not completed map activation; source+78 and driver remain zero. Separate class-fade reset and cache initialization are not added to this callback.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=len(cases),stores=sum(len(c['stores'])for c in cases),stops=[c['boundary']['instruction']for c in cases])))
if __name__=='__main__':main()

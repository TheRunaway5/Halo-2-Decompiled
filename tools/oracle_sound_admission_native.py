#!/usr/bin/env python3
"""Unchanged admission/query owners on retained original startup/map bytes.

No availability, listener, local player, seed, voice or platform success is set.
The chance continuation is explicitly a raw instruction slice, not admission.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from oracle_sound_create_native import StartupTrace,PRIOR,AUDIO_ARENA,AUDIO_CAPACITY,REQUEST,TAGS
from oracle_smg_firing_1_1 import STACK,STOP
BASE=ROOT/'analysis/multiplayer/integration/sound-admission-native'
PAIR=BASE/'paired-v2'
CREATE=ROOT/'analysis/multiplayer/integration/sound-create-native/measured/report.json'
CREATE_PIN='351249cda02ced140a75d6414c0087f55d8a3ff8fd0bc3248df8d84e7411fa2b'
# Read exact pin below through the already immutable manifest, never infer it.
def maps(pair):
 pcs={};va={};entries={}
 for row in pair['owners']:
  entries[int(row['old'],16)]=int(row['u5'],16)
  a=(PAIR/'instructions'/f'{row["old"]}-old.asm').read_text().splitlines();b=(PAIR/'instructions'/f'{row["old"]}-u5.asm').read_text().splitlines()
  pcs.update({int(x.split()[0],16):int(y.split()[0],16)for x,y in zip(a,b)})
  va.update({int(m['old_va'],16):int(m['new_va'],16)for m in row['memory']})
 return pcs,va,entries

def prepare(out):
 prior=json.loads((PRIOR/'report.json').read_text());ctx=json.loads((PRIOR/'raw-host-context.json').read_text());t=StartupTrace(ctx,prior['clock_calls']);t.execute();executed=set();stores=[];active=False
 def code(g,a,n,u):
  if active and t.code.at(a):executed.add(a)
 def write(g,k,a,n,v,u):
  if active and not(0x2000000<=a<0x2200000):stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,before=f'{t.u(a,n):0{n*2}x}',after=f'{v&((1<<(n*8))-1):0{n*2}x}'))
 t.g.hook_add(UC_HOOK_CODE,code);t.g.hook_add(UC_HOOK_MEM_WRITE,write)
 for e in(0x223960,0x2239b0):t.call(f'actual_{e:08x}',e)
 active=True;t.call('actual_complete14CC00_local_table_constructor',0x14cc00);active=False
 first=t.u(0x4f01f0);assert all(t.u(first+12+4*i)==0xffffffff for i in range(4))
 base=t.u(0x4ed650);used=t.u(0x4ed654)
 regions=[dict(address=f'{AUDIO_ARENA:08x}',raw_hex=bytes(t.g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex()),dict(address=f'{base:08x}',raw_hex=bytes(t.g.mem_read(base,used)).hex())]
 seq=[t.code.at(a)for a in sorted(executed)]
 for z in seq:assert z.bytes==t.image.read(z.address,len(z.bytes))
 save(out/'instructions/actual14cc00-preparation.asm',''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in seq).encode())
 r=dict(prior_report=dict(path=str(PRIOR/'report.json'),sha256=sha(PRIOR/'report.json')),context=dict(path=str(PRIOR/'raw-host-context.json'),sha256=sha(PRIOR/'raw-host-context.json')),regions=regions,first_person_globals=f'{first:08x}',first_person_130_hex=bytes(t.g.mem_read(first,0x130)).hex(),stores=stores,source_state=t.state(),constructor_returned=True,reset14CCD0_executed=False,local_binding_supplied=False)
 save(out/'preparation.json',encoded(r));return r

def run(m,pair,prep,out):
 tag=m['image'];path=OLD if tag=='old'else NEW;assert sha(path)==PINS[str(path)];x=Xbe(path);code=_Code(x,'.text');pcs,vas,entries=maps(pair);reverse={b:a for a,b in pcs.items()};va=lambda a:a if tag=='old'else vas[a];entry=lambda a:a if tag=='old'else entries[a]
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 regions=[dict(q)for q in m['regions']]
 for new in prep['regions']:
  regions=[q for q in regions if q['address']!=new['address']];regions.append(new)
 state={k:m['state'][k]for k in('system','tag_table','current_matg','current_ugh','class_fades','local_rng')}
 state.update(alternate_codec_flag=f'{va(0x4865ec):08x}',first_person_globals=prep['first_person_globals'],current_scene=f'{int.from_bytes(x.read(va(0x4e7918),4),"little"):08x}')
 regions.append(dict(address=state['alternate_codec_flag'],raw_hex=x.read(va(0x4865ec),4).hex()))
 ranges=[]
 for q in regions:
  at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']);ranges.append((at&~4095,(at+len(raw)+4095)&~4095))
 merged=[]
 for lo,hi in sorted(ranges):
  if merged and lo<=merged[-1][1]:merged[-1]=(merged[-1][0],max(hi,merged[-1][1]))
  else:merged.append((lo,hi))
 for lo,hi in merged:
  if not(0x10000<=lo and hi<=0x810000):g.mem_map(lo,hi-lo)
 for q in regions:g.mem_write(int(q['address'],16),bytes.fromhex(q['raw_hex']))
 for at,size in((0x2000000,0x200000),(STOP,4096)):g.mem_map(at,size)
 def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
 def w(a,v):g.mem_write(a,struct.pack('<I',v))
 for old,name in((0x4ed950,'system'),(0x4eb114,'tag_table'),(0x4e791c,'current_matg'),(0x526464,'current_ugh'),(0x5098f0,'class_fades'),(0x4ee9d8,'local_rng'),(0x4f01f0,'first_person_globals')):w(va(old),int(state[name],16))
 observed=[];executed=set();boundary=None;cases=[];track=False
 watched=[(int(q['address'],16),int(q['address'],16)+len(bytes.fromhex(q['raw_hex'])))for q in regions]
 def hook(_g,a,n,_u):
  if code.at(a):executed.add(a)
 def store(_g,k,a,n,v,_u):
  if track and any(b<=a and a+n<=e for b,e in watched):
   pc=g.reg_read(UC_X86_REG_EIP);observed.append(dict(instruction=f'{pc if tag=="old"else reverse.get(pc,pc):08x}',actual_instruction=f'{pc:08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(8*n))-1):0{n*2}x}'))
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,store)
 def invoke(kind,e,sound,raw,arg=0,reason=True):
  nonlocal observed,track
  for q in regions:g.mem_write(int(q['address'],16),bytes.fromhex(q['raw_hex']))
  g.mem_write(REQUEST,raw);observed=[];track=True
  for reg in(UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):g.reg_write(reg,0)
  g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_XMM0,arg);g.reg_write(UC_X86_REG_EAX,sound)
  location=REQUEST+12;args=[];start=entry(e)if e!=0x1283f4 else(e if tag=='old'else pcs[e]);ends=[STOP];return_kind='eax'
  if e==0x128380:g.reg_write(UC_X86_REG_EAX,REQUEST);args=[sound,REQUEST+0x88,REQUEST+0x8c if reason else 0];return_kind='al'
  elif e==0x128320:return_kind='al'
  elif e in(0x126710,0x126760):g.reg_write(UC_X86_REG_ECX,location);return_kind='xmm0'
  elif e==0x129570:g.reg_write(UC_X86_REG_ECX,location);g.reg_write(UC_X86_REG_EDX,arg);return_kind='xmm0'
  elif e==0x129450:args=[location,arg,location+0x60]
  elif e==0x12c370:g.reg_write(UC_X86_REG_EDI,location);args=[arg];return_kind='xmm0'
  elif e==0x126930:g.reg_write(UC_X86_REG_ESI,u(TAGS+(sound&0xffff)*16+8));args=[u(location+8),u(location+4)]
  elif e==0x128760:g.reg_write(UC_X86_REG_ECX,arg)
  elif e==0x1283f4:
   start=0x1283f4 if tag=='old'else pcs[0x1283f4];ends=[0x12846a if tag=='old'else pcs[0x12846a],0x128524 if tag=='old'else pcs[0x128524]]
   g.reg_write(UC_X86_REG_EDI,location);g.reg_write(UC_X86_REG_ESI,u(TAGS+(sound&0xffff)*16+8));return_kind='slice'
  g.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));g.reg_write(UC_X86_REG_ESP,STACK)
  # Hooks stop the explicit slice at its actual branch boundaries, never
  # return a synthetic callee result or alter the original instructions.
  stop_hook=None
  if return_kind=='slice':
   def stop_at(_g,a,n,_u):
    if a in ends:_g.emu_stop()
   stop_hook=g.hook_add(UC_HOOK_CODE,stop_at)
  g.emu_start(start,STOP,count=1000000)
  if stop_hook:g.hook_del(stop_hook)
  assert g.reg_read(UC_X86_REG_EIP)in ends
  result=(g.reg_read(UC_X86_REG_EAX)&(255 if return_kind=='al'else 0xffffffff))if return_kind in('eax','al')else(g.reg_read(UC_X86_REG_XMM0)&0xffffffff if return_kind=='xmm0'else int(g.reg_read(UC_X86_REG_EIP)==ends[0]))
  track=False
  cases.append(dict(kind=kind,entry=f'{e:08x}',sound=f'{sound:08x}',argument=f'{arg:08x}',reason_output=reason,raw_request_hex=raw.hex(),result=f'{result:08x}',stores=observed))
 for original in m['cases']:
  sound=int(original['sound'],16);raw=bytes.fromhex(original['raw_request_hex']);name=original['scenario']
  for reason in(True,False):invoke(name+'_cold_admission',0x128380,sound,raw,reason=reason)
  for e in(0x128320,0x126710,0x126760,0x129450,0x126930,0x1283f4):invoke(name+'_actual_query',e,sound,raw,0x3f800000 if e==0x129450 else 0)
  for arg in(0,0x3f800000,0x41200000,0x447a0000):invoke(name+'_distance_control',0x12c370,sound,raw,arg)
  for arg in(0,0x3f800000,0x3f000000,0xbf800000,0x7f800000,0x7fc12345):invoke(name+'_linear_db_control',0x21b900,sound,raw,arg)
  for listener in range(4):invoke(name+'_actual_distance',0x129570,sound,raw,listener)
  definition=u(TAGS+(sound&0xffff)*16+8);class_index=u(definition+2,1);invoke(name+'_actual_class_fade',0x128760,sound,raw,class_index)
  # Direct location ABI controls alter only caller-owned request bytes. Source
  # listener positions/active flags and local-player slots remain unchanged.
  for typ in(1,2,8,15):
   changed=bytearray(raw);changed[15]=(changed[15]&0xf0)|typ
   invoke(name+'_location_type_control',0x129570,sound,bytes(changed),0)
   if typ==1:invoke(name+'_positional_no_listener',0x129450,sound,bytes(changed),0x3f800000)
 seq=[code.at(a)for a in sorted(executed)]
 for i in seq:assert i.bytes==x.read(i.address,len(i.bytes))
 listing=out/'instructions'/f'{tag}-{Path(m["map"]["path"]).stem}.asm';save(listing,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq).encode())
 return dict(image=tag,map=m['map'],state=state,regions=regions,cases=cases,availability_hex=bytes(g.mem_read(int(state['system'],16)+0x78,3)).hex(),local_slots=[f'{u(int(state["first_person_globals"],16)+12+n*4):08x}'for n in range(4)],listing=dict(path=str(listing.relative_to(out)),sha256=sha(listing),rows=len(seq)))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--prepared',type=Path);a=p.parse_args()
 manifest=json.loads((CREATE.parent.parent/'final-manifest.json').read_text());assert sha(CREATE)==CREATE_PIN and any(str(q).find(CREATE_PIN)!=-1 for q in manifest.values())
 prep=json.loads(a.prepared.read_text())if a.prepared else prepare(a.output);save(a.output/'preparation.json',encoded(prep))
 pair=json.loads((PAIR/'report.json').read_text());r=json.loads(CREATE.read_text());banks=[]
 for m in r['maps']:
  q=run(m,pair,prep,a.output);banks.append(q);print(json.dumps(dict(image=q['image'],map=Path(q['map']['path']).name,cases=len(q['cases']))),flush=True)
 report=dict(schema='original-sound-admission-native-v1',oracle_sha256=sha(__file__),paired_report_sha256=sha(PAIR/'report.json'),create_fixture=dict(path=str(CREATE),sha256=sha(CREATE)),preparation=prep,banks=banks,supplied_availability_listener_seed_voice_or_platform_results=[],instruction_patches=[],limits=['Whole128380 runs only on actual unavailable constructor/map state; no successful full admission is established.','1283F4 continuation is an explicit raw instruction-slice ABI with the actual retained RNG, not a bypass admitted into submit.','Direct numeric/location queries use declared caller inputs; they do not produce active listener/world ownership.','Actual14CC00 source local-player storage constructor returns with fourNONE slots.14CCD0 reset and local membership are not claimed.','Old genuine startup/map-prefix bytes are explicitly retained raw ABI inputs in both authenticated images; this is not wholeu5startup or live playback.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=sum(len(q['cases'])for q in banks))))
if __name__=='__main__':main()

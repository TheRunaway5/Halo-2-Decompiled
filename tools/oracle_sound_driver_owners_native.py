#!/usr/bin/env python3
"""Execute real empty flush/queue owners and declared raw-state stop controls."""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID,UcError
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from oracle_sound_startup_native import AUDIO_ARENA,AUDIO_CAPACITY
BASE=ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/measured/report.json'
BASE_PIN='12751ad5e29ef686a654884baa716d6b9446cf489ccf5194d91d7d3eb5845f24'
CACHE=ROOT/'analysis/multiplayer/integration/sound-cache-startup-native/measured/report.json'
CACHE_PIN='dcec5a5f112f44cf262af78e119b6163a643d1c1f2b09fffc694e003bb58d436'
PAIR=ROOT/'analysis/multiplayer/integration/sound-driver-owners-native/paired'
STACK,STOP=0x200d000,0x3000000
OPERATIONS={'flush':0x2213a0,'shutdown':0x2b4470,'release':0x2b4bf0,'complete':0x2b48a0,'static':0x222b80,'cleanup_witness':0x220bf0}

def mapping(pair):
 pcs={};memory={};calls={};entries={}
 for q in pair['owners']:
  entries[int(q['old'],16)]=int(q['u5'],16)
  files=[(PAIR/'instructions'/f'{q["old"]}-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
  for a,b in zip(*files):pcs[int(a.split()[0],16)]=int(b.split()[0],16)
  for r in q['memory']:memory[int(r['old_va'],16)]=int(r['new_va'],16)
  for r in q['calls']:calls[int(r['old_target'],16)]=int(r['new_target'],16)
 return pcs,memory,calls,entries

def image_cases(tag,path,pair,packet,out):
 assert sha(path)==PINS[str(path)]and sha(BASE)==BASE_PIN and sha(CACHE)==CACHE_PIN
 parent=json.loads(BASE.read_text())['cases'][0 if tag=='old'else 4]
 cache=json.loads(CACHE.read_text())['cases'][0]['stages'][-1]['state']
 pool=int(cache['globals']['entry_pool'],16);block=next(q for q in cache['regions']if int(q['address'],16)==pool)
 driver=int(parent['after']['driver_address'],16);queue=int(packet['source_static_queue_identity'][tag],16)
 x=Xbe(path);code=_Code(x,'.text');pcs,globals_map,call_map,entries=mapping(pair)
 reverse_pcs={b:a for a,b in pcs.items()};regions=[dict(address=f'{AUDIO_ARENA:08x}',raw_hex=parent['output_arena_hex'],owner='Complete prior unchanged driver allocation prefix'),dict(address=f'{pool:08x}',raw_hex=block['raw_hex'],owner='Complete prior actual21A6E0 entry-pool constructor output, declared1.1 ABI input'),dict(address=f'{queue:08x}',raw_hex=x.read(queue,16).hex(),owner='Literal original image static queue bytes; later dynamic initialization is not claimed')]
 # These are original raw-state stimuli, not valid native voices/packets or
 # a substitute for positive platform admission. Actual handles stay absent.
 specs=[('flush_empty','flush',0,[]),('flush_negative_one','flush',0,[(driver+4,2,0xffff)]),('flush_negative_minimum','flush',0,[(driver+4,2,0x8000)]),('flush_count_one_unvoiced','flush',0,[(driver+4,2,1)]),('queue_empty','shutdown',0,[]),('queue_invalid_identity','shutdown',0,[(queue+8,4,1)]),('cleanup_empty_witness','cleanup_witness',0,[])]
 specs += [(f'release_flags_{flag:02x}','release',0,[(driver+0xe,1,flag)])for flag in range(256)]
 specs += [(f'complete_flags_{flag:02x}','complete',0,[(driver+0xe,1,flag)])for flag in range(256)]
 # The exact authored static count is policy+0C. This request-builder root
 # is independent of the still-unavailable normal driver caller.
 count=int.from_bytes(bytes.fromhex(parent['policy16_hex'])[12:14],'little',signed=True);assert count==51
 specs += [(f'static_index_{index}','static',index,[])for index in range(count)]
 cases=[];executed=set()
 for name,op,arg,changes in specs:
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
  for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
  g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);g.mem_map(pool,0x3000);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
  for region in regions:g.mem_write(int(region['address'],16),bytes.fromhex(region['raw_hex']))
  def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
  def w(a,v,n=4):g.mem_write(a,v.to_bytes(n,'little'))
  w(0x526474 if tag=='old'else globals_map[0x526474],driver)
  w(0x5098dc if tag=='old'else globals_map[0x5098dc],pool)
  controls=[]
  for at,n,v in changes:controls.append(dict(address=f'{at:08x}',bytes=n,before=f'{u(at,n):0{n*2}x}',after=f'{v:0{n*2}x}'));w(at,v,n)
  stores=[];bad=[];local=set();platform=[]
  def hook(_g,a,n,user):
   if code.at(a):local.add(a)
  def write(_g,kind,a,n,v,user):
   if 0x2000000<=a<0x2010000:return
   pc=g.reg_read(UC_X86_REG_EIP);canonical_pc=pc if tag=='old'else reverse_pcs.get(pc,pc)
   stores.append(dict(instruction=f'{canonical_pc:08x}',actual_instruction=f'{pc:08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(n*8))-1):0{n*2}x}'))
  def invalid(_g,kind,a,n,v,user):bad.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',access=kind,address=f'{a:08x}',bytes=n));return False
  g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
  g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
  entry=OPERATIONS[op]if tag=='old'else entries[OPERATIONS[op]]
  if op=='shutdown':g.reg_write(UC_X86_REG_EDI,queue)
  if op=='release':g.reg_write(UC_X86_REG_ECX,driver+0xc)
  if op=='complete':g.reg_write(UC_X86_REG_ESI,driver+0xc);g.reg_write(UC_X86_REG_EDX,0)
  if op=='static':g.reg_write(UC_X86_REG_EAX,arg)
  boundary=(0x383afe if tag=='old'else call_map[0x383afe])if op=='static'else STOP
  try:g.emu_start(entry,boundary,count=1000000)
  except UcError:assert bad,(name,'unexpected source stop')
  if op=='static':
   assert g.reg_read(UC_X86_REG_EIP)==boundary
   sp=g.reg_read(UC_X86_REG_ESP);args=[u(sp+4+4*i)for i in range(4)]
   descriptor=bytes(g.mem_read(args[1],24)).hex()
   platform=dict(source_entry=f'{boundary:08x}',device_identity=f'{args[0]:08x}',output_address=f'{args[2]:08x}',outer_identity=f'{args[3]:08x}',descriptor24_hex=descriptor,original_return_supplied=False)
   assert args[0]==0 and args[2]==driver+arg*0x3c+0x1a4c and args[3]==0
   assert descriptor=='180000001020000000000000000000000000000000000000'
   outcome='platform_stop'
  else:outcome='invalid_storage'if bad else'returned';assert bad or g.reg_read(UC_X86_REG_EIP)==STOP
  outputs=[]
  for region in regions:
   at=int(region['address'],16);raw=bytes(g.mem_read(at,len(bytes.fromhex(region['raw_hex']))));outputs.append(dict(address=region['address'],sha256=hashlib.sha256(raw).hexdigest()))
  executed.update(local)
  cases.append(dict(name=f'{tag}_{name}',operation=op,index=arg,controls=controls,stores=stores,outcome=outcome,invalid=bad,platform_request=platform,output_regions=outputs,source_return_supplied=False))
 for address in executed:
  i=code.at(address);assert i.bytes==x.read(address,len(i.bytes))
 path_out=out/'instructions'/f'{tag}-executed.asm';save(path_out,''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in(code.at(a)for a in sorted(executed))).encode())
 return dict(image=tag,image_sha256=sha(path),driver_address=f'{driver:08x}',cache_address=f'{pool:08x}',queue_address=f'{queue:08x}',initial_regions=regions,cases=cases,listing=dict(file=str(path_out.relative_to(out)),sha256=sha(path_out),rows=len(executed)))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();pair=json.loads((PAIR/'report.json').read_text());packet=json.loads((PAIR/'providers.json').read_text())
 images=[image_cases(tag,path,pair,packet,a.output)for tag,path in(('old',OLD),('u5',NEW))]
 report=dict(schema='original-sound-driver-owners-stops-v1',oracle_sha256=sha(__file__),driver_prefix_report_sha256=BASE_PIN,cache_prefix_report_sha256=CACHE_PIN,paired_report_sha256=sha(PAIR/'report.json'),images=images,instruction_patches=[],source_platform_returns=[],limits=['Naturally empty flush/static queue shutdown and whole cleanup witness execute unchanged on retained prior driver-prefix storage. The static queue bytes are literal source-image input, not a proved later constructor.','Declared count/byte/invalid-identity controls establish exact masks, writes and hard stops; they do not create source voices, packets, cache pin ownership or audio admission.','The actual prior1.1 cache entry pool and driver arena are explicit retained raw ABI inputs to both images; ordinary fullu5 initialization is not claimed.','Static buffer indices0..50 come from exact authored policy+0C, but this independent request-builder root does not bypass earlier driver admission; it stops before the real create-buffer operation returns.','No section-load, DSP, device, voice, platform method or IO result is supplied.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=sum(len(q['cases'])for q in images),stores=sum(len(c['stores'])for q in images for c in q['cases']))))
if __name__=='__main__':main()

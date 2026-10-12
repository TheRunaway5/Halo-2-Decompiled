#!/usr/bin/env python3
"""Execute unchanged stream-format/build owners up to real DirectSoundCreateStream."""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from oracle_sound_startup_native import AUDIO_ARENA,AUDIO_CAPACITY
PRIOR=ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/measured/report.json'
PRIOR_PIN='12751ad5e29ef686a654884baa716d6b9446cf489ccf5194d91d7d3eb5845f24'
PAIR=ROOT/'analysis/multiplayer/integration/sound-driver-stream-native/paired'
ABI=ROOT/'analysis/multiplayer/integration/sound-driver-stream-native/method-abi'
PAIR_PIN='781933decd99123c63996cdea1900a527247db159750f9dd74711dbe8d254646'
ABI_PIN='5d892b7f3c223c6274cbf70ae43c4a3b496c8e2bdd3c8d27117c3992bbe4be9b'
STACK,STOP=0x200d000,0x3000000
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PRIOR)==PRIOR_PIN and sha(PAIR/'report.json')==PAIR_PIN and sha(ABI/'report.json')==ABI_PIN
 pair=json.loads((PAIR/'report.json').read_text());abi=json.loads((ABI/'report.json').read_text());images=[]
 pcs={};entries={}
 for owner in pair['owners']:
  entries[int(owner['old'],16)]=int(owner['u5'],16)
  left=(PAIR/'instructions'/f'{owner["old"]}-old.asm').read_text().splitlines();right=(PAIR/'instructions'/f'{owner["old"]}-u5.asm').read_text().splitlines()
  for x,y in zip(left,right,strict=True):pcs[int(y.split()[0],16)]=int(x.split()[0],16)
 for i in range(3):
  left=(ABI/abi['images'][0]['owners'][i]['listing']).read_text().splitlines();right=(ABI/abi['images'][1]['owners'][i]['listing']).read_text().splitlines()
  for x,y in zip(left,right,strict=True):pcs[int(y.split()[0],16)]=int(x.split()[0],16)
 for side,(tag,path)in enumerate((('old',OLD),('u5',NEW))):
  assert sha(path)==PINS[str(path)];xbe=Xbe(path);code=_Code(xbe,'.text')
  parent=json.loads(PRIOR.read_text())['cases'][side*4];driver=int(parent['after']['driver_address'],16)
  pcm,adpcm=abi['images'][side]['owners'][:2];queue=int(pcm['queue'],16)
  # This callback literal is authenticated at the matching whole call-site.
  caller=(PAIR/'instructions'/f'002215f0-{tag}.asm').read_text().splitlines()
  at=next(i for i,row in enumerate((PAIR/'instructions/002215f0-old.asm').read_text().splitlines())if row.startswith('00221661 '))
  completion=int(caller[at].split()[-1],16)
  def operand(old_address):
   word=next(q for q in next(q for q in pair['owners']if q['old']=='00220520')['memory']if int(q['old_va'],16)==old_address)
   address=int(word['old_va'if side==0 else'new_va'],16)
   raw=xbe.read(address,4);assert raw.hex()==word['old_hex'if side==0 else'new_hex'];return raw.hex()
  constants=dict(one_bits=operand(0x465030),stereo_bits=operand(0x46507c),
     pcm_queue=pcm['queue'],adpcm_queue=adpcm['queue'],pcm_initialize_method=pcm['entry'],adpcm_initialize_method=adpcm['entry'],completion_callback=f'{completion:08x}')
  regions=[dict(address=f'{AUDIO_ARENA:08x}',raw_hex=parent['output_arena_hex'],owner='Actual prior complete driver allocation prefix; no device admitted'),
    dict(address=f'{queue:08x}',raw_hex=xbe.read(queue,64).hex(),owner='Declared literal original mutable queue ABI input; later initialization not claimed'),
    *[dict(address=q['vtable'],raw_hex=xbe.read(int(q['vtable'],16),4).hex(),owner='Actual immutable first queue method pointer')for q in(pcm,adpcm)],
    dict(address=f'{STACK-0x2c:08x}',raw_hex='00'*44,owner='Declared raw caller stack backing; not ordinary uninitialized host stack'),
    dict(address=f'{STACK+4:08x}',raw_hex='00'*4,owner='Explicit original caller index argument slot'),
    dict(address=f'{STACK+0x1000:08x}',raw_hex='a5'*20,owner='Declared format ABI preservation control'),
    dict(address=f'{STACK+0x1020:08x}',raw_hex='44332211',owner='Declared format output-slot control')]
  specs=[(f'format_{category}','format',category,0)for category in(0,1,2,3,0xffffffff)]
  policy=bytes.fromhex(parent['policy16_hex']);index=0
  for category in range(3):
   for local in range(int.from_bytes(policy[2*category:2*category+2],'little',signed=True)):
    specs.append((f'stream_{index}_{category}','stream',category,index));index+=1
  cases=[];executed=set()
  for name,op,category,index in specs:
   g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
   for section in xbe.sections:g.mem_write(section.va,xbe.section_bytes(section))
   g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
   for region in regions:g.mem_write(int(region['address'],16),bytes.fromhex(region['raw_hex']))
   def u(address,size=4):return int.from_bytes(g.mem_read(address,size),'little')
   # Authenticate the driver's actual absolute global operand from whole2B4970.
   driver_global=int(next(q for q in next(q for q in pair['owners']if q['old']=='002b4970')['memory']if q['old_va']=='00526474')['old_va'if side==0 else'new_va'],16)
   g.mem_write(driver_global,struct.pack('<I',driver))
   g.mem_write(STACK,struct.pack('<II',STOP,index));controls=[]
   if index:controls.append(dict(address=f'{STACK+4:08x}',bytes=4,before='00000000',after=f'{index:08x}'))
   stores=[];local=set()
   def instruction(_g,address,size,user):local.add(address)
   def write(_g,kind,address,size,value,user):
    if not any(int(q['address'],16)<=address and address+size<=int(q['address'],16)+len(bytes.fromhex(q['raw_hex']))for q in regions):return
    pc=g.reg_read(UC_X86_REG_EIP);canonical=pc if side==0 else pcs[pc]
    stores.append(dict(instruction=f'{canonical:08x}',actual_instruction=f'{pc:08x}',address=f'{address:08x}',bytes=size,before=f'{u(address,size):0{size*2}x}',after=f'{value&((1<<(8*size))-1):0{size*2}x}'))
   g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,write)
   call_stack=STACK if op=='stream'else 0x2008000
   g.mem_write(call_stack,struct.pack('<II',STOP,index));g.reg_write(UC_X86_REG_ESP,call_stack)
   g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
   if op=='format':
    entry=0x220520 if side==0 else entries[0x220520];boundary=STOP
    wave,descriptor,queue_slot=STACK+0x1000,0,STACK+0x1020
    g.reg_write(UC_X86_REG_EAX,category);g.reg_write(UC_X86_REG_ESI,wave);g.reg_write(UC_X86_REG_EDI,queue_slot)
   else:
    entry=0x2215f0 if side==0 else entries[0x2215f0]
    boundary=int(next(q for q in next(q for q in pair['owners']if q['old']=='002b4970')['calls']if q['old_target']=='00383b22')['old_target'if side==0 else'new_target'],16)
    wave,descriptor,queue_slot=STACK-0x2c,STACK-0x18,STACK+4;g.reg_write(UC_X86_REG_EBX,category)
   g.emu_start(entry,boundary,count=100000);assert g.reg_read(UC_X86_REG_EIP)==boundary
   request=None
   if op=='stream':
    sp=g.reg_read(UC_X86_REG_ESP);args=[u(sp+4+i*4)for i in range(4)]
    request=dict(device_identity=f'{args[0]:08x}',descriptor_address=f'{args[1]:08x}',output_address=f'{args[2]:08x}',outer_identity=f'{args[3]:08x}',descriptor24_hex=bytes(g.mem_read(args[1],24)).hex(),format20_hex=bytes(g.mem_read(u(args[1]+8),20)).hex())
    assert args==[0,descriptor,driver+index*0x34+0x30,0]
   output=[dict(address=q['address'],sha256=hashlib.sha256(g.mem_read(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))).hexdigest())for q in regions]
   executed.update(local);cases.append(dict(name=f'{tag}_{name}',operation=op,category=category,index=index,format=f'{wave:08x}',descriptor=f'{descriptor:08x}',queue_slot=f'{queue_slot:08x}',controls=controls,stores=stores,request=request,output_regions=output,outcome='returned'if op=='format'else'platform_stop',platform_return_supplied=False))
  listing=a.output/'instructions'/f'{tag}-executed.asm';rows=[code.at(pc)for pc in sorted(executed)]
  assert all(q and q.bytes==xbe.read(q.address,len(q.bytes))for q in rows)
  save(listing,''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in rows).encode())
  images.append(dict(image=tag,image_path=str(path),image_sha256=sha(path),driver=f'{driver:08x}',constants=constants,initial_regions=regions,cases=cases,listing_sha256=sha(listing),listing_rows=len(rows)))
 report=dict(schema='original-sound-driver-stream-prefix-v1',oracle_sha256=sha(__file__),prior_report_sha256=PRIOR_PIN,paired_report_sha256=PAIR_PIN,method_abi_sha256=ABI_PIN,images=images,platform_returns=[],instruction_patches=[],
  limits=['Explicit raw-ABI format controls and standalone stream builder on actual prior driver allocation bytes; original driver admission not supplied.',
          'All82 indices/categories are taken from the actual source startup policy; no stream creation result, pause, downstream flags, voice or playback is supplied.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=sum(len(q['cases'])for q in images),stores=sum(len(c['stores'])for q in images for c in q['cases']))))
if __name__=='__main__':main()

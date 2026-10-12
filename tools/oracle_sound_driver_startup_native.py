#!/usr/bin/env python3
"""Unchanged 1.1/u5 driver allocation prefixes; no device return supplied.

Both images receive the same explicitly retained prior1.1 raw arena boundary.
U5 execution is a paired ABI experiment, not a recovered u5 title startup.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from oracle_sound_startup_native import AUDIO_ARENA,AUDIO_CAPACITY
PRIOR=ROOT/'analysis/multiplayer/integration/sound-startup-native/measured/report.json'
PRIOR_PIN='b2a1175e7dd524fc7d9587333e8055abd8cb4d30f6417e0ff293e9d1e4444287'
PAIR=ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/paired'
FIELDS=('arena_base','arena_used','arena_crc','driver_address','environment_address','runtime_environment_parameters_address','hardware_queue_address','hardware_queue_flag','queue_4865dc','queue_budget','queue_4865ec')
OLD_FIELDS={'arena_base':0x517ff0,'arena_used':0x517ff4,'arena_crc':0x517ff8,'driver_address':0x526474,'environment_address':0x518480,'hardware_queue_address':0x51842c,'hardware_queue_flag':0x518430,'queue_4865dc':0x4865dc,'queue_budget':0x4865e8,'queue_4865ec':0x4865ec}
STACK,STOP=0x200d000,0x3000000
def mappings(pair):
 pcs={};memory={};calls={}
 for q in pair['owners']:
  files=[(PAIR/'instructions'/f'{q["old"]}-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
  for a,b in zip(*files):pcs[int(a.split()[0],16)]=int(b.split()[0],16)
  for r in q['memory']:
   a,b=int(r['old_va'],16),int(r['new_va'],16);assert a not in memory or memory[a]==b;memory[a]=b
  for r in q['calls']:calls[int(r['old_target'],16)]=int(r['new_target'],16)
 # CRC destination is a literal PUSH pointer, not a decoded memory operand.
 lines=[(PAIR/'instructions'/f'002205c0-{tag}.asm').read_text().splitlines()for tag in('old','u5')]
 at=next(i for i,q in enumerate(lines[0])if q.startswith('002205ed '))
 assert int(lines[0][at].split()[-1],16)==0x517ff8
 memory[0x517ff8]=int(lines[1][at].split()[-1],16)
 return pcs,memory,calls
def run(tag,image,delta,pair,packet,out):
 assert sha(image)==PINS[str(image)]and sha(PRIOR)==PRIOR_PIN
 pcs,globals_map,call_map=mappings(pair);reverse_pcs={b:a for a,b in pcs.items()};reverse_globals={b:a for a,b in globals_map.items()}
 va=lambda a:a if tag=='old'else globals_map[a]
 entry=0x2205c0 if tag=='old'else 0x224ec0;device=0x383dd7 if tag=='old'else call_map[0x383dd7]
 x=Xbe(image);code=_Code(x,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
 prefix=json.loads(PRIOR.read_text())['stopped_original_caller'];g.mem_write(AUDIO_ARENA,bytes.fromhex(prefix['arena_hex']))
 def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
 def w(a,v):g.mem_write(a,struct.pack('<I',v))
 controls=[]
 for key in('arena_base','arena_used','arena_crc'):w(va(OLD_FIELDS[key]),int(prefix['globals'][key],16))
 original_used=u(va(0x517ff4));w(va(0x517ff4),original_used+delta)
 if delta:controls.append(dict(field='arena_used',before=f'{original_used:08x}',after=f'{original_used+delta:08x}',scope='Declared raw cursor alignment control; not ordinary title allocation history'))
 policy_address=0x451220 if tag=='old'else 0x455b20;policy=x.read(policy_address,16)
 env=int(packet['runtime_environment_packet']['old'if tag=='old'else'u5'],16)
 def state():
  values={k:f'{u(va(a),1 if k=="hardware_queue_flag"else 4):08x}'for k,a in OLD_FIELDS.items()};values['runtime_environment_parameters_address']=f'{env:08x}';return values
 before=state();initial=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY));stores=[];executed=set();aux=[]
 g.mem_write(STACK,struct.pack('<II',STOP,policy_address));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
 def ins(_g,a,n,user):executed.add(a)
 def store(_g,k,a,n,v,user):
  pc=g.reg_read(UC_X86_REG_EIP);row=dict(instruction=f'{pc:08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(8*n))-1):0{n*2}x}')
  if AUDIO_ARENA<=a<AUDIO_ARENA+AUDIO_CAPACITY or a in[va(OLD_FIELDS[k])for k in('arena_used','arena_crc','driver_address')]:
   canonical=row.copy();canonical['instruction']=f'{pc if tag=="old"else reverse_pcs[pc]:08x}';canonical['address']=f'{a if tag=="old"else reverse_globals.get(a,a):08x}';stores.append(dict(actual=row,canonical=canonical))
  elif not(0x2000000<=a<0x2010000):aux.append(row)
 g.hook_add(UC_HOOK_CODE,ins);g.hook_add(UC_HOOK_MEM_WRITE,store);g.emu_start(entry,device,count=1000000);assert g.reg_read(UC_X86_REG_EIP)==device
 sp=g.reg_read(UC_X86_REG_ESP);request=[u(sp+4+4*i)for i in range(3)];assert request==[0,u(va(0x526474))+0x2ab0,0]
 rows=[code.at(a)for a in sorted(executed)]
 for q in rows:assert q and q.bytes==x.read(q.address,len(q.bytes))==bytes(g.mem_read(q.address,len(q.bytes)))
 name=f'{tag}_cursor_plus_{delta}';listing=out/'instructions'/f'{name}.asm';save(listing,''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in rows).encode())
 return dict(name=name,image=tag,image_sha256=sha(image),entry=f'{entry:08x}',before=before,initial_arena_hex=initial.hex(),after=state(),output_arena_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex(),policy_address=f'{policy_address:08x}',policy16_hex=policy.hex(),controls=controls,stores=stores,auxiliary_crc_cache_stores=aux,
  native_platform_boundary=dict(source_entry=f'{device:08x}',actual_stack_arguments=[f'{v:08x}'for v in request],source_platform_return_supplied=False,source_device_initialized_byte=u(u(va(0x526474)),1),source_device_identity=u(u(va(0x526474))+0x2ab0)),
  listing=dict(file=str(listing.relative_to(out)),sha256=sha(listing),rows=len(rows)),whole_constructor_returned=False)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();pair=json.loads((PAIR/'report.json').read_text());packet=json.loads((PAIR/'providers.json').read_text())
 cases=[run(tag,image,n,pair,packet,a.output)for tag,image in(('old',OLD),('u5',NEW))for n in range(4)]
 for n in range(4):
  left,right=cases[n],cases[n+4];assert left['output_arena_hex']==right['output_arena_hex']and left['before']['arena_crc']==right['before']['arena_crc']and[left['stores'][i]['canonical']for i in range(len(left['stores']))]==[right['stores'][i]['canonical']for i in range(len(right['stores']))]
 report=dict(schema='original-sound-driver-startup-prefix-v1',oracle_sha256=sha(__file__),prior_report_sha256=PRIOR_PIN,paired_report_sha256=sha(PAIR/'report.json'),cases=cases,source_instruction_patches=[],source_platform_returns=[],
  bounds=['Entire prior1.1 source-created raw startup arena and base/cursor/CRC are declared inputs. Three cursor changes explicitly cover the other alignment residues.','U5 executes its own unchanged complete prefix on that same declared ABI/raw arena boundary; this is not a separately recovered u5 title startup.','The original CRC helper executes its cold source-image cache initialization. Auxiliary CRC cache stores are independently retained; native polynomial computation represents scalar output, not guest cache memory.','Original call reaches actual DirectSoundCreate with nullGUID/nullouter and retained output address. No device creation, caps, saved setting, routing, DSP, stream, plugin, ready or audio-admission return is supplied.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=len(cases),stores=sum(len(q['stores'])for q in cases))))
if __name__=='__main__':main()

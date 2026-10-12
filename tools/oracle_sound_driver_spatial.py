#!/usr/bin/env python3
"""Original spatial driver CPU boundaries and complete filter helper results.

One full retained-state/SDK-payload case is the actual initialized game capture.
Additional declared ABI cases execute unchanged original CPU instructions and
stop before their first SDK operation; no callee result is supplied. Filter
helpers execute completely, including original CPU32909C conversion. Neither
declared probes nor native ABI observers constitute SDK/DSP playback.
"""
import argparse,atexit,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def fbits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--host-cpu',action='store_true');p.add_argument('--unicorn-diagnostic',dest='host_cpu',action='store_false');p.set_defaults(host_cpu=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 image_path=ROOT/'assets/disc/default.xbe';assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';x=Xbe(image_path)
 capture=json.loads((a.capture/'report.json').read_text());assert capture['image_sha256']==sha(image_path)
 assert not any(capture[k]for k in('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
 before=(a.capture/'driver-before.bin').read_bytes();after=(a.capture/'driver-after.bin').read_bytes()
 assert hashlib.sha256(before).hexdigest()==capture['before_sha256']and hashlib.sha256(after).hexdigest()==capture['after_sha256']
 controls=bytes.fromhex(capture['filter_controls_hex']);assert len(controls)==8
 pair_paths=[ROOT/'analysis/audio-spatial-20261011/paired-v1',ROOT/'analysis/audio-spatial-20261011/driver-helper-paired-v1'];pcs={};operands={}
 for path in pair_paths:
  report=json.loads((path/'report.json').read_text())
  for row in report['owners']:
   left=(path/'instructions'/f'{row["base"]}-base.asm').read_text().splitlines();right=(path/'instructions'/f'{row["base"]}-old.asm').read_text().splitlines();assert len(left)==len(right)==row['rows']
   pcs.update({int(l.split()[0],16):int(r.split()[0],16)for l,r in zip(left,right)})
   for q in row['memory']:operands[int(q['base_va'],16)]=q
 addresses=(0x45dc1c,0x45dc70,0x45dbd8,0x45dbc0,0x45dc20,0x45dde8,0x45e008,0x45dcfc,0x45dde4,0x45dde0,0x45dddc,0x45ddd8,0x45e004,0x45e000,0x45dffc)
 constants=[]
 for address in addresses:
  raw=x.read(address,4);assert raw.hex()==operands[address]['base_hex']==operands[address]['old_hex'];constants.append(int.from_bytes(raw,'little'))
 provider_path=ROOT/'analysis/multiplayer/integration/sound-platform-voice-contract/paired/providers.json'
 driver_constants=json.loads(provider_path.read_text())['constants'];P=[int.from_bytes(bytes.fromhex(q['raw4_hex']),'little')for q in driver_constants]
 assert len(P)==16 and P[14]==int.from_bytes(x.read(0x45dbdc,4),'little')and P[9]==constants[3]
 lines=['DRIVER_SOUND_SPATIAL_ORIGINAL_V1','K\t'+'\t'.join(f'{v:08x}'for v in constants),'P\t'+'\t'.join(f'{v:08x}'for v in P)]
 fp=capture['registers'];driver=int(capture['driver_address'],16);index=capture['voice_index'];offset=0x1a18+index*60
 operations={'SetPosition':0,'SetI3DL2Source':1,'SetFilter':2}
 def emit(label,prefix,initial,final,request,settings,ops,stores):
  lines.append(f'C\t{label}\t{int(prefix)}\t{driver:08x}\t{index&0xffffffff:08x}\t{settings.hex()}\t{request.hex()}')
  lines.append(f'F\t{fp["fctrl"]:04x}\t{fp["mxcsr"]:08x}');lines.append('B\t'+initial.hex());lines.append('A\t'+final.hex())
  for operation,buffer,apply,words in ops:lines.append(f'O\t{operation}\t{buffer:08x}\t{apply:08x}'+'\t'+words if apply is not None else f'O\t{operation}\t{buffer:08x}\t-\t{words}')
  for pc,address,n,old,new in stores:lines.append(f'S\t{pc:08x}\t{address:08x}\t{n}\t{old:08x}\t{new:08x}')
  lines.append('E')
 ops=[]
 for q in capture['sdk_calls']:
  op=operations[q['method']];words=[int(v,16)for v in q['words']]
  ops.append((op,words[0],words[-1]if op in(0,1)else None,struct.pack('<3I',*words[1:4]).hex()if op==0 else q['payload_hex']))
 assert [q[0]for q in ops]==[0,1,2]
 emit('actual_game',False,before,after,bytes.fromhex(capture['request_hex']),controls,ops,[])
 # Explicit ABI scratch, not initialized game/device state.
 buffer_address,stack,stop=0x1001000,0x100e000,0x3000000
 def cpu(initial,settings):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
  for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
  g.mem_map(0x1000000,0x10000);g.mem_map(stop,4096)
  g.mem_write(driver,initial);g.mem_write(0x51ebe4,struct.pack('<I',driver));g.mem_write(0x470084,settings)
  g.reg_write(R.UC_X86_REG_FPCW,fp['fctrl']);g.reg_write(R.UC_X86_REG_MXCSR,fp['mxcsr'])
  return g
 declared=[]
 def case(label,edits=(),parameter_edits=(),setting_edits=()):declared.append((label,edits,parameter_edits,setting_edits))
 case('control')
 case('unchanged_active',[(offset+4,1,2)],[(i+1,int.from_bytes(before[offset+8+4*i:offset+12+4*i],'little'))for i in range(3)])
 values=(0,0x80000000,0x00000001,0x00800000,0x3a83126f,0x3d4ccccd,0x3f000000,0x3f800000,0xbf800000,0x40000000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345)
 # Keep position unchanged for the smoothing/environment branches. A forced
 # position case stops at its SetPosition boundary before any callee executes.
 held=[(i+1,int.from_bytes(before[offset+8+4*i:offset+12+4*i],'little'))for i in range(3)]
 for flag in range(4):case(f'voice_flag_{flag}',[(offset+4,1,flag)],held)
 for v in values:
  for field in (4,5,6,7,8):case(f'parameter_{field}_{v:08x}',[(offset+4,1,2)],held+[(field,v)])
  for field in (0x2abc,0x2ac0,0x2ac4):case(f'volume_{field:04x}_{v:08x}',[(offset+4,1,2)],held+[(4,fbits(.5))],[(field,4,v)])
  for field in (0x2c,0x30):case(f'retained_{field:02x}_{v:08x}',[(offset+4,1,2),(offset+field,4,v)],held+[(4,fbits(.25)),(5,fbits(.75)),(7,fbits(.125)),(8,fbits(.25))])
 for field in range(3):
  for v in (fbits(.049),fbits(.05),fbits(.051),0x7fc12345):
   old=struct.unpack('<f',before[offset+8+4*field:offset+12+4*field])[0]
   delta=struct.unpack('<f',struct.pack('<I',v))[0];value=fbits(old+delta)
   case(f'position_{field}_{v:08x}',[(offset+4,1,2)],held+[(field+1,value)])
 for field,rate_field in ((4,7),(5,8)):
  for value in values:
   case(f'smoothing_{field}_{value:08x}',[(offset+4,1,2),(offset+0x2c,4,fbits(.9)),(offset+0x30,4,fbits(.9))],held+[(4,fbits(.1)),(5,fbits(.1)),(7,fbits(.125)),(8,fbits(.25)),(rate_field,value)])
 all_instructions=set();declared_reports=[];host=None
 if a.host_cpu:
  from original_audio_cpu import OriginalAudioCpu
  from functions import _Code
  import subprocess
  layout=a.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
  subprocess.run(['cc',str(layout),'-o',str(a.output/'user-layout')],check=True);subprocess.run([str(a.output/'user-layout')],check=True)
  host=OriginalAudioCpu(x,a.output/'original32.elf',fp['fctrl'],fp['mxcsr']);atexit.register(host.close);host_code=_Code(x,'.text')
 def measure(initial,request):
  if host:
   host.write(driver,bytes(initial));host.write(0x51ebe4,struct.pack('<I',driver));host.write(0x470084,controls)
   host.write(buffer_address,bytes(request));host.write(stack,struct.pack('<Ii',stop,index))
   regs,raw_stores,executed=host.run_traced(0x220e00,stack,host_code,driver,len(initial),rdi=buffer_address)
   assert all(p in pcs for p in executed);all_instructions.update(executed)
   stores=[(pcs[pc],address,n,old,new)for pc,address,n,old,new in raw_stores];observed=[];boundary=regs.rip
   if boundary!=stop:
    op={0x37c668:0,0x37c6e5:1,0x37b68b:2}[boundary];args=struct.unpack('<'+'I'*({0:5,1:3,2:2}[op]),host.read(regs.rsp+4,{0:20,1:12,2:8}[op]))
    words=struct.pack('<3I',*args[1:4]).hex()if op==0 else host.read(args[1],36 if op==1 else 24).hex()
    observed.append((op,args[0],args[-1]if op!=2 else None,words))
   return host.read(driver,len(initial)),observed,stores,boundary
  g=cpu(bytes(initial),controls);g.mem_write(buffer_address,bytes(request));g.mem_write(stack,struct.pack('<Ii',stop,index));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EDI,buffer_address)
  stores=[];observed=[]
  def code(g,address,size,user):
   assert bytes(g.mem_read(address,size))==x.read(address,size)
   all_instructions.add(address)
   if address in (0x37c668,0x37c6e5,0x37b68b):
    sp=g.reg_read(R.UC_X86_REG_ESP)
    op={0x37c668:0,0x37c6e5:1,0x37b68b:2}[address]
    args=struct.unpack('<'+'I'*({0:5,1:3,2:2}[op]),g.mem_read(sp+4,{0:20,1:12,2:8}[op]))
    words=struct.pack('<3I',*args[1:4]).hex()if op==0 else bytes(g.mem_read(args[1],36 if op==1 else 24)).hex()
    observed.append((op,args[0],args[-1]if op!=2 else None,words));g.emu_stop()
   else:assert address in pcs,hex(address)
  def store(g,kind,address,size,value,user):
   if driver<=address<driver+len(initial):stores.append((pcs[g.reg_read(R.UC_X86_REG_EIP)],address,size,int.from_bytes(g.mem_read(address,size),'little'),value&((1<<(8*size))-1)))
  g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,store);g.emu_start(0x220e00,stop,count=100000)
  boundary=g.reg_read(R.UC_X86_REG_EIP);assert boundary in (stop,0x37c668,0x37c6e5)
  return bytes(g.mem_read(driver,len(initial))),observed,stores,boundary
 for label,edits,param_edits,volume_edits in declared:
  initial=bytearray(before);request=bytearray.fromhex(capture['request_hex'])
  for off,n,value in (*edits,*volume_edits):initial[off:off+n]=value.to_bytes(n,'little')
  for i,value in param_edits:request[4*i:4*i+4]=value.to_bytes(4,'little')
  final,observed,stores,boundary=measure(initial,request)
  emit(label,True,bytes(initial),final,bytes(request),controls,observed,stores)
  declared_reports.append({'label':label,'voice_edits':edits,'parameter_edits':param_edits,'volume_edits':volume_edits,'first_sdk_operation':observed[0][0]if observed else None,'stores':len(stores)})
 # Complete pure filter helpers and the original CPU integer conversion.
 scalar=set(values)
 for i in range(-2000,2001):scalar.add(fbits(i/8))
 for i in range(0,8501,17):scalar.add(fbits(float(i)))
 helper_rows=0
 for entry,kind in ((0x21e270,'frequency'),(0x21e2d0,'gain')):
  g=cpu(before,controls)if not host else None
  def code(g,address,size,user):
   assert address in pcs and bytes(g.mem_read(address,size))==x.read(address,size);all_instructions.add(address)
  if g:g.hook_add(UC_HOOK_CODE,code)
  for value in sorted(scalar):
   if host:
    host.write(stack,struct.pack('<II',stop,value));regs=host.run(entry,stack);assert regs.rip==stop;result=regs.rax&0xffffffff
   else:
    g.mem_write(stack,struct.pack('<II',stop,value));g.reg_write(R.UC_X86_REG_ESP,stack)
    g.emu_start(entry,stop,count=10000);assert g.reg_read(R.UC_X86_REG_EIP)==stop;result=g.reg_read(R.UC_X86_REG_EAX)
   lines.append(f'Q\t{kind}\t{value:08x}\t{result:08x}');helper_rows+=1
 fixture=a.output/'fixtures.tsv';fixture.write_text('\n'.join(lines)+'\n')
 report={'boundary':__doc__,'execution_backend':'host32_hardware'if host else'unicorn','tool_sha256':sha(__file__),'image_sha256':sha(image_path),'actual_capture_sha256':sha(a.capture/'report.json'),'fixture_sha256':sha(fixture),'parameter_provider_sha256':sha(provider_path),'declared_cases':declared_reports,'actual_game_cases':1,'complete_filter_helper_cases':helper_rows,'unchanged_original_instructions':len(all_instructions),'supplied_sdk_returns':[],'instruction_patches':[],'expected_values_from_native':False}
 if host:
  report['host_runner_sha256']=sha(ROOT/'tools/original_audio_cpu.py');report['original_elf_sha256']=sha(a.output/'original32.elf');host.close()
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(f'Original driver:1 actual full call,{len(declared)} declared first-SDK boundaries,{helper_rows} complete filter helper cases')
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Original scalar parameter bodies/slice, without driver or admission success.

The declared buffers are mathematical ABI inputs only. A binding for the
221760 read-only query exposes its input integer; it is not an allocated
voice, valid device or sound-system readiness flag. The221B90 pitch slice
stops before its first update/admission/service branch, with no patches.
"""
import argparse,json,struct
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *

BUFFER,STACK,STOP=0x1001000,0x200d000,0x3000000
PAIR=ROOT/'analysis/multiplayer/integration/sound-platform-voice-contract/paired/report.json'
PAIR_PIN='29bf3e2273704f6d2f3bf18ece740389febeda3b987f16e55c3fa9a29d2e9e4d'
def word(v):return struct.pack('<I',v&0xffffffff)
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def low(g,r):return g.reg_read(r)&0xffffffff
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PAIR)==PAIR_PIN;pair=json.loads(PAIR.read_text());owners={int(q['old'],16):q for q in pair['owners']}
 reports=[]
 for name,path,key in(('original1_1',OLD,'old'),('authenticated_u5',NEW,'u5')):
  assert sha(path)==PINS[str(path)];x=Xbe(path);code=_Code(x,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32)
  g.mem_map(0x10000,0x800000)
  for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
  for address in(0x1000000,0x2000000,STOP):g.mem_map(address,0x10000)
  executed=set();active=[];observed_gain_normalization=[];gain_normalized_site=None
  def hook(_g,address,size,user):
   executed.add(address);active.append(address)
   if address==gain_normalized_site:observed_gain_normalization.append(low(g,UC_X86_REG_XMM0))
  g.hook_add(UC_HOOK_CODE,hook)
  fpcw,mxcsr=0x027f,0x1f80
  def run(entry,args=(),regs=(),end=STOP):
   active.clear();g.mem_write(STACK,b''.join(word(v)for v in(STOP,*args)))
   g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,fpcw);g.reg_write(UC_X86_REG_MXCSR,mxcsr)
   for r,v in regs:g.reg_write(r,v)
   g.emu_start(entry,end,count=10000);assert g.reg_read(UC_X86_REG_EIP)==end
   for addr in active:
    z=code.at(addr);assert z.bytes==x.read(addr,len(z.bytes))==bytes(g.mem_read(addr,len(z.bytes)))
  normalize=int(owners[0x12c760][key],16);ratio=int(owners[0x221760][key],16)
  pitch=int(owners[0x221b90][key],16)
  # Complete body correspondence pins the same original instruction positions.
  seq=(PAIR.parent/f'instructions/00221b90-{key}.asm').read_text().splitlines()
  old=(PAIR.parent/'instructions/00221b90-old.asm').read_text().splitlines()
  start=int(seq[next(i for i,l in enumerate(old)if l.startswith('0022225a '))].split()[0],16)
  end=int(seq[next(i for i,l in enumerate(old)if l.startswith('002222b4 '))].split()[0],16)
  gain_start=int(seq[next(i for i,l in enumerate(old)if l.startswith('00221cf1 '))].split()[0],16)
  gain_end=int(seq[next(i for i,l in enumerate(old)if l.startswith('00221d14 '))].split()[0],16)
  gain_normalized_site=int(seq[next(i for i,l in enumerate(old)if l.startswith('00221d00 '))].split()[0],16)
  global_slot=int(next(q for q in owners[0x221760]['memory']if q['old_va']=='00526474')['old_va'if key=='old'else'new_va'],16)
  # Explicit query input storage: no source constructor/device/session flag.
  g.mem_write(global_slot,word(BUFFER));normal=[];pitches=[];ratios=[];gains=[]
  values=[bits(v)for v in(-100.,-64.,-63.999996,-32.,-1.,-0.,0.,1.,64.,100.)]+[0x7f800000,0xff800000,0x7fc00000,0x7fa00001,0x00000001,0x80000001]
  pitch_values=[bits(v)for v in(0.,-0.,1.,-1.,100.,-100.,1200.,-1200.,9600.,-9600.)]+values[10:]
  integers=[-4597,-501,-2396,0,1,-1,-32767,8191,-32768,8192,1200,-1200]
  contexts=((0x027f,0x1f80),(0x067f,0x1f80),(0x0a7f,0x1f80),(0x0e7f,0x1f80),
            (0x037f,0x1f80),(0x027f,0x3f80),(0x027f,0x5f80),(0x027f,0x7f80))
  for fpcw,mxcsr in contexts:
   context=dict(source_fpcw=f'{fpcw:04x}',source_mxcsr=f'{mxcsr:08x}')
   state=0x91837cab
   for i in range(256):
    state=(state*1664525+1013904223)&0xffffffff;value=values[i]if i<len(values)else state
    run(normalize,(bits(-64.),bits(0.),value),((UC_X86_REG_ECX,1),))
    normal.append(dict(**context,value_bits=f'{value:08x}',lower_bits='c2800000',upper_bits='00000000',cl=1,result_bits=f'{low(g,UC_X86_REG_XMM0):08x}'))
    before=len(observed_gain_normalization)
    run(gain_start,regs=((UC_X86_REG_EBP,value),),end=gain_end)
    assert len(observed_gain_normalization)==before+1
    gains.append(dict(**context,value_bits=f'{value:08x}',normalized_bits=f'{observed_gain_normalization[-1]:08x}',scaled_driver_volume_float_bits=f'{low(g,UC_X86_REG_XMM0):08x}',driver_volume_bits=f'{low(g,UC_X86_REG_ECX):08x}'))
   # Exact zero/tiny/reversed/NaN range and bool controls, not a world producer.
   ranges=((0,0),(bits(1),bits(1)),(0,bits(1e-8)),(bits(1e-8),0),
           (bits(2),bits(1)),(0x7fc00000,0),(0,0x7f800000))
   for lower,upper in ranges:
    for value in(0,bits(-1),bits(1),bits(2),0x7fc00000):
     for cl in(0,1,2,255):
      run(normalize,(lower,upper,value),((UC_X86_REG_ECX,cl),))
      normal.append(dict(**context,value_bits=f'{value:08x}',lower_bits=f'{lower:08x}',upper_bits=f'{upper:08x}',cl=cl,result_bits=f'{low(g,UC_X86_REG_XMM0):08x}'))
   state=0x2375acef
   for i in range(512):
    state=(state*1664525+1013904223)&0xffffffff;value=pitch_values[i]if i<len(pitch_values)else state
    rate=(i%5)-1;g.mem_write(BUFFER+4,word(rate));g.mem_write(BUFFER+8,word(value))
    run(start,regs=((UC_X86_REG_EDI,BUFFER),),end=end)
    pitches.append(dict(**context,rate_index=rate,source_pitch_bits=f'{value:08x}',clamped_float_bits=f'{low(g,UC_X86_REG_XMM0):08x}',driver_pitch_bits=f'{low(g,UC_X86_REG_ESI):08x}'))
   state=0x82c7ab3d
   for i in range(256):
    state=(state*1664525+1013904223)&0xffffffff;value=integers[i]if i<len(integers)else int(state%40959)-32767
    rate=(i%5)-1;g.mem_write(BUFFER+0xc+8,word(value))
    run(ratio,(rate&0xffffffff,),((UC_X86_REG_EAX,0),))
    ratios.append(dict(**context,rate_index=rate,driver_pitch=value,result_bits=f'{low(g,UC_X86_REG_XMM0):08x}'))
  ins=[code.at(addr)for addr in sorted(executed)];f=a.output/'instructions'/f'{name}-executed.asm'
  save(f,''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins).encode())
  reports.append(dict(image=name,source_sha256=sha(path),normalize_entry=f'{normalize:08x}',ratio_entry=f'{ratio:08x}',pitch_owner=f'{pitch:08x}',pitch_slice_start=f'{start:08x}',pitch_slice_end_exclusive=f'{end:08x}',gain_slice_start=f'{gain_start:08x}',gain_slice_end_exclusive=f'{gain_end:08x}',normalize=normal,pitch_slices=pitches,gain_slices=gains,relative_rates=ratios,executed_listing=dict(path=str(f.relative_to(a.output)),sha256=sha(f),rows=len(ins))))
 assert [{k:q[k]for k in('normalize','pitch_slices','gain_slices','relative_rates')}for q in reports][0]==[{k:q[k]for k in('normalize','pitch_slices','gain_slices','relative_rates')}for q in reports][1]
 report=dict(schema='original-sound-platform-scalar-parameters-v1',tool_sha256=sha(__file__),paired_report_sha256=PAIR_PIN,images=reports,
  boundaries=['Declared mathematical scalar inputs and raw ABI buffers, not owning gameplay/driver/voice state.','221760 global query binding only exposes the explicit integer query input; no successful device or sound initialized/admitted flags are supplied.','221B90 executes only original scalar gain/pitch slices, stopping before the first store/service consumer; complete owner body is static paired evidence, not successful execution.','Eight explicitly varied scalar harness FPCW/MXCSR pairs; these are not assigned ordinary game clock/thread contexts or physical Xbox audio/resampler parity.'],new_successful_platform_returns=[],instruction_patches=[],native_audio_connected=False)
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),images=2,normalize_cases_per_image=len(normal),pitch_slices_per_image=len(pitches),gain_slices_per_image=len(gains),relative_rate_cases_per_image=len(ratios))))

if __name__=='__main__':main()

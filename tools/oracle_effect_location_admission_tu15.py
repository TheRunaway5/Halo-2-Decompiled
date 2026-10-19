#!/usr/bin/env python3
"""Replay every node word through unchanged English Xbox TU1.5 17DCE0.

The declared pool/cursor/stack isolate the original WORLD/FP iterator. No
instruction or callee is replaced; only cursor/stack writes are permitted.
This is location admission evidence, not geometry, sound or a full match.
Run through tools/run_test.py and compare original-world-admission.bin to the
checked-in fixture. Both the fixture and report are small generated outputs.
"""
from pathlib import Path
import argparse,hashlib,json,os,struct,sys,time
root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
report=args.output.resolve();assert report.is_relative_to(scratch.resolve()), 'Generated outputs must remain under HALO2_TEST_SCRATCH'
report.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
image=args.xbe
x=Xbe(image);pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
assert hashlib.sha256(x.data).hexdigest()==pin
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
u.mem_map(0x2100000,0x10000);u.mem_map(0x3000000,4096)
header,records,cursor,effect,stack,stop=0x2100000,0x2101000,0x2102000,0x2103000,0x210f000,0x3000000
word=lambda a,v:u.mem_write(a,struct.pack('<I',v&0xffffffff))
half=lambda a,v:u.mem_write(a,struct.pack('<H',v&65535))
read=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
word(0x4f7208,header);word(header+0x44,records)
for push,callsite in [(0x17d2c4,0x17d2d0),(0x17d6dc,0x17d6e4)]:
 assert bytes(u.mem_read(push,2))==b'\x6a\x00'
 raw=bytes(u.mem_read(callsite,5));assert raw[0]==0xe8 and callsite+5+struct.unpack('<i',raw[1:])[0]==0x17dce0
active=False;writes=0
def observe(vm,access,address,size,value,data):
 global writes
 if active:
  assert cursor<=address and address+size<=cursor+4 or stack-256<=address and address+size<=stack+16,hex(address)
  writes+=1
u.hook_add(UC_HOOK_MEM_WRITE,observe)
def call(mode,initial):
 global active
 word(cursor,initial);u.mem_write(stack,struct.pack('<IIII',stop,effect,cursor,mode&0xffffffff))
 for reg in [R.UC_X86_REG_EAX,R.UC_X86_REG_EBX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI,R.UC_X86_REG_EBP]:u.reg_write(reg,0)
 u.reg_write(R.UC_X86_REG_ESP,stack);u.reg_write(R.UC_X86_REG_EFLAGS,0x202)
 active=True
 try:u.emu_start(0x17dce0,stop,count=20000)
 finally:active=False
 assert u.reg_read(R.UC_X86_REG_EIP)==stop and u.reg_read(R.UC_X86_REG_ESP)==stack+16
 return u.reg_read(R.UC_X86_REG_EAX),read(cursor)
word(records+4,0xffffffff)
started=time.monotonic();modes={}
for mode in [0,1,3]:
 results=bytearray()
 for node in range(65536):
  half(records+2,node);address,following=call(mode,0x10000)
  assert address in [0,records] and following==0xffffffff
  results.append(int(address==records))
 path=scratch/f'mode-{mode}.bin';path.write_bytes(results)
 modes[str(mode)]={'accepted':sum(results),'sha256':hashlib.sha256(results).hexdigest()}
 if mode==0:(report/'original-world-admission.bin').write_bytes(results)
cases=[]
for nodes in [[0,0x8025],[0x8025,0],[0,1],[0xffff,0x8000],[0x8000,0xffff],[0x8000,0x8025]]:
 for n,node in enumerate(nodes):
  half(records+n*60+2,node);word(records+n*60+4,((n+2)<<16)|(n+1) if n+1<len(nodes) else 0xffffffff)
 for mode in [0,1,3]:
  admitted=[];following=0x10000
  while following!=0xffffffff:
   address,following=call(mode,following)
   if address:admitted.append((address-records)//60)
  cases.append({'nodes':nodes,'mode':mode,'selected':admitted})
receipt={'image':str(image),'xbe_sha256':pin,'entry':'0x17DCE0','caller_world_mode':'0x17D2C4 and 0x17D6DC push mode0 inside original17D210','scope':'Whole unchanged original location iterator; declared standalone pool/cursor/stack only, no source writes or hooked callees; not geometry, sound backend or complete sound dispatcher','node_domain':65536,'modes':modes,'chain_cases':cases,'observed_scratch_writes':writes,'seconds':time.monotonic()-started,'scratch':str(scratch)}
(report/'original.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'modes':modes,'chains':cases[:3],'seconds':receipt['seconds']}))

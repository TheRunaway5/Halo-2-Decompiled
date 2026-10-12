#!/usr/bin/env python3
"""Unchanged183EC0/1348E0 and40E90 up to its41C80 service entry.

Actual retained main/secondary query contexts, queue headers, command storage
and persistent material visibility are inputs. Declared data controls exercise
active-prefix clearing and byte preservation. No instruction/callee is replaced.
This checks reset owners; nonempty object commands, collectors and GPU pixels
are not supplied by this fixture.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff
PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(b):return list(struct.unpack('<'+'I'*(len(b)//4),b))
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path)
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)==PIN;x=Xbe(ROOT/'assets/disc/default.xbe')
 assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 g=s.guest();g.mem_map(0x70000000,0x100000);stack,stop=0x700fe000,0x700fff00
 def read(at,n):
  regions=list(g.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
  return bytes(g.mem_read(at,n))
 def val(at):return u(read(at,4))
 def write(at,b):read(at,len(b));g.mem_write(at,b)
 executed={}
 def unchanged(vm,pc,size,_):
  raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Instruction differs{pc:08x}'
  executed[pc]=raw
 g.hook_add(UC_HOOK_CODE,unchanged)
 def call(entry,end=stop):
  write(stack,pack([stop]));g.reg_write(R.UC_X86_REG_ESP,stack)
  g.emu_start(entry,end,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==end
 def bank(b):return [len(b)//4,*words(b)]
 fixture=[0x46505342,1,0];cases=[];rng=random.Random(0x1348e0183ec0)
 def record(label,kind,before,after,**extra):
  fixture.extend([len(before),len(after),*before,*after]);cases.append(dict(label=label,kind=kind,checked_words=len(after),**extra))
 visibility=val(0x4ed280)
 for variant in range(4):
  guarded=bytes(rng.randrange(256)for _ in range(0x4224));write(visibility-16,guarded)
  call(0x183ec0);after=read(visibility-16,len(guarded))
  assert after[:16]==guarded[:16] and after[16+0x4201:]==guarded[16+0x4201:]
  record('persistent material activation, guarded tail',0,[0,*bank(guarded)],bank(after),variant=variant)
 for context in [0x547f88,0x54aa68]:
  original=read(context,0x2ae0);headers=[u(original[off:off+4])for off in [12,16,20,24]]
  assert len(set(headers))==4
  for used in [0,1,31,510,511,512]:
   write(context,original)
   for offset,n in [(0x1c,512),(0x21c,64),(0x25c,2048)]:write(context+offset,bytes(rng.randrange(256)for _ in range(n)))
   write(context+0xa5c,struct.pack('<HH',used,rng.randrange(65536)))
   for h in headers:write(h+4,struct.pack('<H',rng.randrange(65536)))
   before=[1,*bank(read(context,0x2ae0)),*[v for h in headers for v in bank(read(h,24))]]
   g.reg_write(R.UC_X86_REG_EDX,context);call(0x1348e0)
   after=[*bank(read(context,0x2ae0)),*[v for h in headers for v in bank(read(h,24))]]
   record('whole current query reset',1,before,after,context=f'{context:08x}',previous_mask_words=used)
 headers=[0x4c1a48,0x4c1a60,0x4c1a78]
 initial=[read(h,24)for h in headers]
 special=0x4c62f8+val(0x509440)*0x408
 def command_state():
  result=[]
  for h in headers:
   hh=words(read(h,24));result.extend(hh);result.append(hh[0]);result.extend(words(read(hh[5],hh[0]*24)))
  result.extend(words(read(0x4b6280,8)));result.append(struct.unpack('<H',read(special+0x404,2))[0]);result.extend(words(read(special,1024)))
  return result
 for mode in [0,1,2,NONE]:
  for selector in [0,1,2,3,256,NONE]:
   for h,raw in zip(headers,initial):
    hh=words(raw);hh[1:5]=[rng.getrandbits(32)for _ in range(4)];write(h,pack(hh))
    write(hh[5],pack([rng.getrandbits(32)for _ in range(hh[0]*6)]))
   write(0x4b6280,pack([rng.getrandbits(32)for _ in range(2)]));write(special,pack([rng.getrandbits(32)for _ in range(256)]));write(special+0x404,struct.pack('<H',rng.randrange(65536)))
   context=0x54aa68;write(0x4c0b80,pack([context]));write(context+0x2a78,pack([mode]))
   before=[2,mode,selector,*command_state()]
   g.reg_write(R.UC_X86_REG_EAX,selector);call(0x40e90,0x41c80)
   after=[g.reg_read(R.UC_X86_REG_ESI),*command_state()]
   record('40E90 reset before object queue service',2,before,after,mode=mode,selector=selector)
 fixture[2]=len(cases);path=a.output/'fixture.bin';path.write_bytes(pack(fixture))
 report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),intercepted_instructions=0,cases=cases,
  executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),
  checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  r=subprocess.run([str(a.native.resolve()),str(path.resolve())],text=True,capture_output=True)
  report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=r.returncode,native_stdout=r.stdout,native_stderr=r.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2));return report.get('native_returncode',0)
if __name__=='__main__':raise SystemExit(main())

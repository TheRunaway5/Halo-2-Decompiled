#!/usr/bin/env python3
"""Execute literal texture pool constructors and physical-stage arithmetic.

Constructor suffixes explicitly admit already allocated storage; they are not
full12C0D0 kernel allocation replays. Complete startup prefixes stop before the
actual captured kernel instruction. No success, return, protection, callback,
allocation or readiness value is supplied at a service boundary.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,re,struct,subprocess,sys
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,BUFFER,STACK,STOP=0x20000000,0x20001000,0x200fe000,0x200ff000
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);a=p.parse_args()
 s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};dirty=set();seen={};phase=[''];calls=[];boundary=[];stop=[0]
 def page(address):
  at=address&~4095
  if at not in pages:
   raw=s.read(at,4096);u.mem_map(at,4096);u.mem_write(at,raw);pages[at]=raw
 def read(address,size):
  for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:page(at)
  return bytes(u.mem_read(address,size))
 def put(address,raw):
  read(address,len(raw));u.mem_write(address,raw)
  for at in range(address&~4095,((address+len(raw)-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:dirty.add(at)
 def event(pc):
  return dict(entry=f'{pc:08X}',registers={n:u.reg_read(r)for n,r in[
   ('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),
   ('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI)]},
   stack=words(read(u.reg_read(UC_X86_REG_ESP)+4,32)))
 def missing(g,access,address,size,value,data):
  try:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(at)
   return True
  except ValueError as error:
   boundary.append(dict(kind='uncaptured memory',pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',
     address=f'{address:08X}',reason=str(error)));g.emu_stop();return False
 def code(g,pc,size,data):
  try:original=x.read(pc,size)
  except (ValueError,KeyError):
   boundary.append(dict(kind='actual kernel entry; not executed',**event(pc)));g.emu_stop();return
  assert read(pc,size)==original,(hex(pc),'changed original instruction');seen[pc]=size
  if pc in[0x12c0d0,0x16b5f0,0x16b650,0x16b7a0,0x13d170,0x12b400,0x12c1e0,
    0x11ca20,0x3312cc,0x2d156a,0x2d15da,0x13d8b0,0x12d200]:calls.append(event(pc))
  if pc==stop[0]:g.emu_stop()
 def writing(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(at)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
 ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',s.report['registers_before']).groups())
 read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,val in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:u.reg_write(reg,val)
 def reset():
  for at in dirty:u.mem_write(at,pages[at])
  dirty.clear();calls.clear();boundary.clear();stop[0]=0
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
  u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.mem_write(STACK,packed([STOP]+[0]*15))
 def run(entry):
  try:u.emu_start(entry,STOP,count=2000000)
  except UcError:
   if not boundary:raise
  assert u.reg_read(UC_X86_REG_EIP)in[STOP,stop[0],0x2d15da]or boundary,hex(u.reg_read(UC_X86_REG_EIP))
 allocator=uint(s.read(0x468758,4));pool_allocator=uint(s.read(0x46875c,4))
 actual_addresses=[uint(s.read(p,4))for p in[0x4e6454,0x4e6458,0x4e6464]]
 initial_banks={f'{p:08X}':s.read(p,n).hex()for p,n in[(0x4e6420,52),(0x468758,8),(0x479860,8),
  (actual_addresses[0],0x4c),(actual_addresses[1],0x4c),(actual_addresses[2],0xbc)]}
 cases=[];rows=[];constructor_records=[];sizes=[0x508c,0x6a8,0x30fc]
 for kind in range(3):
  for index in range(16):
   reset();address=actual_addresses[kind]if index==0 else BUFFER+index*7
   seed=(index*19+kind*73)&255;size=sizes[kind]
   initial=bytes((seed+i*37+(i>>3))&255 for i in range(size));put(address,initial)
   u.reg_write(UC_X86_REG_EAX,address);u.reg_write(UC_X86_REG_EDI,allocator)
   if kind==0:entry=0x12c0e4;stop[0]=0x12c119
   elif kind==1:entry=0x12c11d;stop[0]=0x12c152
   else:
    entry=0x12c156;u.mem_write(STACK,packed([0,0,0,STOP]))
   run(entry);assert not boundary
   actual=words(read(address,size));row=[kind,address,allocator,pool_allocator,seed,0,0]
   cases.append(row);rows.append(actual)
   constructor_records.append(dict(kind=kind,address=f'{address:08X}',seed=seed,
    label='retained storage address, explicit new allocation admission'if index==0 else'controlled already allocated storage/alignment',
    entry=f'{entry:08X}',stop=f'{u.reg_read(UC_X86_REG_EIP):08X}',
    before_sha256=sha(initial),after_sha256=sha(read(address,size)),
    header_hex=read(address,min(size,0xbc if kind==2 else 0x4c)).hex(),source_calls=calls.copy()))
 stage=uint(s.read(0x4e6420,4));lower=uint(s.read(0x4e642c+stage*4,4));upper=uint(s.read(0x4e6440+stage*4,4))
 parent_upper=uint(s.read(0x4e6440+(stage-1)*4,4))
 stage_inputs=[(lower,upper),(lower,parent_upper)]
 for lo in[0,1,0xfff,0x1000,0x61000,0x66c000,0x7ffffff0,0x80000000,0xfffff000,0xffffffff]:
  for delta in[0,1,4095,4096,4097,0xffffffff,0xfffff001,0xfffff000,0x7fffffff,0x80000000]:stage_inputs.append((lo,(lo+delta)&0xffffffff))
 rng=random.Random(0x12c1e0)
 stage_inputs.extend((rng.getrandbits(32),rng.getrandbits(32))for _ in range(4096))
 counts=collections.Counter()
 for lo,hi in stage_inputs:
  reset();put(0x4e6420,packed([0]));put(0x4e642c,packed([lo]));put(0x4e6440,packed([hi]))
  # Branch-specific stop: protection entry versus unprotected resize entry.
  stop[0]=0x12c253
  def protection_hook(g,pc,size,data):
   if pc==0x2d15da:g.emu_stop()
  hook=u.hook_add(UC_HOOK_CODE,protection_hook);run(0x12c1e0);u.hook_del(hook)
  pc=u.reg_read(UC_X86_REG_EIP);assert not boundary and pc in[0x12c253,0x2d15da]
  out=[uint(read(0x4e6440,4)),u.reg_read(UC_X86_REG_EBX),u.reg_read(UC_X86_REG_ECX),
    u.reg_read(UC_X86_REG_ESI),int(pc==0x2d15da)]
  cases.append([3,lo,hi,0,0,0,0]);rows.append(out);counts[f'{pc:08X}']+=1
 # Full actual retained allocator/physical bootstrap callers, stopped at kernel.
 diagnostics=[]
 for entry,label,restored_upper in[(0x12c0d0,'complete current allocation caller',None),
   (0x12b400,'complete fixed physical bootstrap caller',None),
   (0x12c1e0,'current stage already reserved; repeated activation is diagnostic only',None),
   (0x12c1e0,'explicit restored parent-stage upper input; not observed original activation',parent_upper)]:
  reset()
  if restored_upper is not None:put(0x4e6440+stage*4,packed([restored_upper]))
  run(entry);assert boundary
  diagnostics.append(dict(entry=f'{entry:08X}',label=label,restored_upper=restored_upper,
   completed=False,source_calls=calls.copy(),
   boundary=boundary.copy(),stage_bank_hex=read(0x4e6420,52).hex()))
 reset()
 for p,h in initial_banks.items():assert read(int(p,16),len(bytes.fromhex(h))).hex()==h
 for at,initial in pages.items():assert bytes(u.mem_read(at,4096))==initial,hex(at)
 encoded='\n'.join(' '.join(f'{v:08X}'for v in row)for row in cases)+'\n'
 completed=subprocess.run([str(a.binary.resolve()),'--probe'],input=encoded,text=True,capture_output=True,check=True)
 native=[list(int(n,16)for n in line.split())for line in completed.stdout.splitlines()]
 assert len(native)==len(rows)
 for index,(actual,expected)in enumerate(zip(native,rows)):assert actual==expected,(index,cases[index],actual[:50],expected[:50])
 fixture=ROOT/'tests/fixtures/retail_source_bitmap_initialization.tsv'
 fixture.write_text('# Explicit already-allocated constructor storage / actual current-stage reservation inputs.\n'+
  '\n'.join(' '.join(f'{v:08X}'for v in row)+' | '+' '.join(f'{v:08X}'for v in out)for row,out in zip(cases,rows))+'\n')
 report=dict(boundary=__doc__,source_xbe_sha256=PIN,snapshot_ram_sha256=sha(s.ram),
  snapshot_manifest_sha256=sha((s.directory/'export.json').read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),
  helper_sha256=sha((ROOT/'src/rendering/source_bitmap_initialization.cpp').read_bytes()),fixture_sha256=sha(fixture.read_bytes()),
  native_binary_sha256=sha(a.binary.read_bytes()),cases=len(cases),checked_words=sum(len(r)for r in rows),
  constructor_cases=len(constructor_records),reservation_cases=len(stage_inputs),reservation_stops=dict(counts),
  retained_stage=dict(index=stage,lower=f'{lower:08X}',upper=f'{upper:08X}',
   expected=rows[48],parent_upper=f'{parent_upper:08X}',parent_upper_control=rows[49],
   literal_manager_capacity=uint(s.read(actual_addresses[2]+0x30,4)),literal_base=uint(s.read(0x4e6460,4))),
  fpcw='023F',mxcsr='1F80',restored_pages_checked=len(pages),
  initial_banks=initial_banks,constructor_records=constructor_records,
  actual_startup_boundaries=diagnostics,unique_instructions=len(seen),
  instruction_sha256=sha(b''.join(packed([pc])+x.read(pc,n)for pc,n in sorted(seen.items()))),
  allocation_admission='Constructor suffixes start after explicit successful storage allocation, with EAX=allocated address and EDI=allocator interface. These are controlled inputs, not fabricated original kernel returns or complete12C0D0 replays.',
  physical_admission='The supplied current-stage bounds produce the exact signed page-count/upper/base arithmetic. Stop before XPhysicalProtect and manager capacity/eviction/pool activation; no protection result is supplied.',
  services_replaced=[],source_instruction_patches=[])
 output=ROOT/'analysis/multiplayer-shading/source-bitmap-initialization-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
 provenance={k:report[k]for k in['source_xbe_sha256','snapshot_ram_sha256','snapshot_manifest_sha256','tool_sha256',
  'helper_sha256','fixture_sha256','cases','checked_words','constructor_cases','reservation_cases','fpcw','mxcsr',
  'allocation_admission','physical_admission','services_replaced','source_instruction_patches']}
 fixture.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
 print(json.dumps({k:report[k]for k in['cases','checked_words','constructor_cases','reservation_cases','reservation_stops','retained_stage','unique_instructions']},indent=2))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Independent u5 original classifier execution on frozen 1.1 supplied packets.

Locate from unique original bytes, verify full topology/table, then execute.
Old packets/CPU/physical byte are input fixtures, not a source-created u5 unit.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import *
from export_unit_locomotion_fixtures import verify_authority
from oracle_mp_unit_locomotion_1_1 import ROOT,OUT as OLD_OUT,FROZEN,digest,encoded,frozen_write
from oracle_mp_title_update_display import NEW,OLD,PINS
from xbe import Xbe
from functions import _Code,_trace
from oracle_mp_item_motion_boundaries_1_1 import MotionTrace
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/unit-locomotion-u5'
INPUT_PINS={**FROZEN,
 'tools/oracle_mp_unit_locomotion_1_1.py':'ec102f6366669797c040c804978d1d35fa343afb811abfeee906a6e5cfb7a614',
 'analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/report.json':'70b1fc431af6636358fadc942677f2351ed8382f893f58f15729ecfabfe8e6f0',
 'analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/manifest.json':'49c825e5fa1f62f9580057bc311acabc38c2a66f1d181225c507b0fc667dcdc6',
 'analysis/multiplayer/integration/inventory-source/title-update-display/manifest.json':'c7de416be29e3ffc9dc5a56c028bbd7d2ce09bd76f33f4190a275b503f1e60ae'}
REGS={name:reg for name,reg in(('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),('ebp',UC_X86_REG_EBP))}
class Trace:
 raw=MotionTrace.raw;cpu=MotionTrace.cpu
 def __init__(self,g):self.g=g
 def registers(self):return {k:f'{self.g.reg_read(v):08x}'for k,v in REGS.items()}
 def restore(self,cpu):
  for n,v in enumerate(cpu['fp80']):self.g.reg_write(UC_X86_REG_FP0+n,tuple(v))
  for n,v in enumerate(cpu['xmm128']):self.g.reg_write(UC_X86_REG_XMM0+n,int(v,16))
  for name,r in(('fpcw',UC_X86_REG_FPCW),('fpsw',UC_X86_REG_FPSW),('fptag',UC_X86_REG_FPTAG),('mxcsr',UC_X86_REG_MXCSR),('eflags',UC_X86_REG_EFLAGS),('esp',UC_X86_REG_ESP)):
   self.g.reg_write(r,int(cpu[name],16))
  for name,v in cpu['registers'].items():self.g.reg_write(REGS[name],int(v,16))
def unique(code,pattern):
 hits=[];at=0
 while True:
  at=code.bytes.find(pattern,at)
  if at<0:break
  hits.append(code.lo+at);at+=1
 assert len(hits)==1,hits
 return hits[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');a=p.parse_args()
 for name,h in INPUT_PINS.items():assert digest(ROOT/name)==h,name
 for path,h in PINS.items():assert digest(path)==h,path
 source=verify_authority();old=Xbe(OLD);new=Xbe(NEW);oc=_Code(old,'.text');nc=_Code(new,'.text')
 old_leaf,old_wrapper=0x1f52f0,0x1e94e0
 leaf=unique(nc,old.read(old_leaf,0x82));wrapper=unique(nc,old.read(old_wrapper,0x16))
 routines={leaf:_trace(nc,leaf,{leaf,wrapper}),wrapper:_trace(nc,wrapper,{leaf,wrapper})}
 oldr={old_leaf:_trace(oc,old_leaf,{old_leaf,old_wrapper}),old_wrapper:_trace(oc,old_wrapper,{old_leaf,old_wrapper})}
 mapping={};topology=[]
 for before,after in((old_leaf,leaf),(old_wrapper,wrapper)):
  oi=[oc.at(p)for p in sorted(oldr[before].code)];ni=[nc.at(p)for p in sorted(routines[after].code)]
  assert len(oi)==len(ni)
  mapping.update({x.address:y.address for x,y in zip(oi,ni)})
  for x,y in zip(oi,ni):
   assert x.mnemonic==y.mnemonic
   # Literal bytes must match except original external branches/table pointer.
   if x.mnemonic not in('jmp','call'):assert x.bytes==y.bytes
   if x.mnemonic.startswith('j')and x.mnemonic!='jmp':
    assert x.operands[0].imm-before==y.operands[0].imm-after
  topology.append(dict(old_entry=f'{before:08x}',u5_entry=f'{after:08x}',rows=len(oi),complete_instruction_shape_internal_branches_equal=True,
   all_bytes_equal=all(x.bytes==y.bytes for x,y in zip(oi,ni))))
 assert nc.at(wrapper+0x21).operands[0].imm==leaf
 oldtable,newtable=old_wrapper+0x40,wrapper+0x40
 ot=struct.unpack('<6I',old.read(oldtable,24));nt=struct.unpack('<6I',new.read(newtable,24))
 assert all(mapping[o]==n for o,n in zip(ot,nt))
 g=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in new.sections:g.mem_write(s.va,new.section_bytes(s))
 g.mem_map(0x2000000,0x200000);g.mem_map(0x3000000,0x10000)
 physical=int(source['source_owner']['physical_pointer'],16);g.mem_map(physical&~0xfff,0x1000)
 t=Trace(g);current=None;order=0;executed=set();rows=[]
 sites=set().union(*(r.code for r in routines.values()))
 def code_hook(_g,p,n,u):
  nonlocal order
  assert p in sites,('Unbounded original execution',hex(p))
  i=nc.at(p);assert i.bytes==new.read(p,len(i.bytes))==bytes(g.mem_read(p,len(i.bytes)))
  executed.add(p);current['points'].append(dict(order=order,instruction=f'{p:08x}',bytes=i.bytes.hex(),cpu=t.cpu()));order+=1
 def read_hook(_g,k,p,n,v,u):
  nonlocal order
  current['reads'].append(dict(order=order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,raw_hex=t.raw(p,n)));order+=1
 def write_hook(_g,k,p,n,v,u):
  nonlocal order
  assert int(current['result_pointer'],16)<=p and p+n<=int(current['result_pointer'],16)+0x70
  current['stores'].append(dict(order=order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,
   before_hex=t.raw(p,n),written_hex=(v&((1<<(8*n))-1)).to_bytes(n,'little').hex()));order+=1
 g.hook_add(unicorn.UC_HOOK_CODE,code_hook);g.hook_add(unicorn.UC_HOOK_MEM_READ,read_hook);g.hook_add(unicorn.UC_HOOK_MEM_WRITE,write_hook)
 for c in source['cases']:
  rp,op=int(c['request_pointer'],16),int(c['result_pointer'],16)
  for ptr,key in((rp,'request_before_hex'),(op,'result_before_hex'),(physical,'physical_before_hex')):g.mem_write(ptr,bytes.fromhex(c[key]))
  cpu=c['points'][0]['cpu'];stack=int(cpu['esp'],16);g.mem_write(stack,struct.pack('<I',0x3000000));t.restore(cpu);assert t.cpu()==cpu
  entry=leaf if c['route']=='leaf'else wrapper
  current=dict(global_id=c['id'],route=c['route'],vector=c['vector'],u5_entry=f'{entry:08x}',
   request_pointer=c['request_pointer'],result_pointer=c['result_pointer'],physical_pointer=c['physical_pointer'],
   request_before_hex=c['request_before_hex'],result_before_hex=c['result_before_hex'],physical_before_hex=c['physical_before_hex'],
   reads=[],stores=[],points=[]);order=0
  g.emu_start(entry,0x3000000,count=1000);assert g.reg_read(UC_X86_REG_EIP)==0x3000000
  current.update(request_after_hex=t.raw(rp,0x154),result_after_hex=t.raw(op,0x70),physical_after_hex=t.raw(physical,16),return_cpu=t.cpu())
  assert current['request_after_hex']==c['request_after_hex']and current['result_after_hex']==c['result_after_hex']and current['physical_after_hex']==c['physical_after_hex']
  assert current['return_cpu']==c['return_cpu'],c['id']
  assert len(current['stores'])==len(c['stores'])
  for before,after in zip(c['stores'],current['stores']):
   assert after==dict(before,instruction=f'{mapping[int(before["instruction"],16)]:08x}')
  assert len(current['points'])==len(c['points'])
  for before,after in zip(c['points'],current['points']):
   assert before['cpu']==after['cpu']and mapping[int(before['instruction'],16)]==int(after['instruction'],16)
  current['full_output_and_cpu_points_equal_old_original']=True;rows.append(current);current=None
 listings=[]
 for e,r in routines.items():
  ins=[nc.at(p)for p in sorted(r.code)];path=a.output.parent/'instructions'/f'{e:08x}.asm'
  frozen_write(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(a.output.parent)),rows=len(ins),sha256=digest(path)))
 report=dict(schema=1,oracle_sha256=digest(__file__),old_input_pins=INPUT_PINS,original_xbe_pins=PINS,
  old_input_report_sha256=digest(OLD_OUT/'report.json'),mapping=topology,
  instruction_mapping=[dict(old=f'{o:08x}',u5=f'{n:08x}')for o,n in sorted(mapping.items())],
  dispatch_tables=dict(old_pointer=f'{oldtable:08x}',old_raw18_hex=old.read(oldtable,24).hex(),u5_pointer=f'{newtable:08x}',u5_raw18_hex=new.read(newtable,24).hex(),mapped_targets_equal=True),
  cases=rows,listings=listings,executed_addresses=[f'{p:08x}'for p in sorted(executed)],services=[],instruction_patches=[],production_edits=[],
  supplied_inputs=['Every request154/result70/physical16 packet and complete root-entry CPU state comes explicitly from the frozen1.1 original measured suite. It is input configuration only.',
   'Explicit source address-space/stack backing; new raw executable sections and actual new wrapper dispatch table remain unchanged.'],
  boundaries=['No actual u5 unit, controller/device admission, physical state producer, world/pose/clock/frame, live movement or latest-ever claim.',
   'Only leaf and wrapper state1/flag20 paths execute. Other physical-state branches remain source-listed/unexecuted with different actual u5 targets.',
   'The native1.1 API still labels store sites with1.1 addresses. This report maps and records separate u5 sites; it does not silently reinterpret native trace addresses as u5.',
   'Stored outputs and original instruction CPU snapshots match in this emulator with declared masked FP state. Physical Xbox FP exception/hardware parity is not claimed.'])
 for name,h in INPUT_PINS.items():assert digest(ROOT/name)==h,name
 for path,h in PINS.items():assert digest(path)==h,path
 frozen_write(a.output,encoded(report));print(json.dumps(dict(output=str(a.output),sha256=digest(a.output),cases=len(rows),old_leaf=f'{old_leaf:08x}',u5_leaf=f'{leaf:08x}',old_wrapper=f'{old_wrapper:08x}',u5_wrapper=f'{wrapper:08x}')))
if __name__=='__main__':main()

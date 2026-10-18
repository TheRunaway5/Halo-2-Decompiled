#!/usr/bin/env python3
"""Prepared bounded original13A7E0 micro-oracle with declared external callbacks.

No engine readiness is provided. Original code is unchanged49-byte/15-PC body;
fixture callback machine code is separately labelled, not an original owner.
Executing this script requires a separate coordinator original-CPU grant.
"""
from pathlib import Path
import argparse,hashlib,json,struct,sys
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
ENTRY=0x13a7e0;END=0x13a811;GAME=0x2000000;STACK=0x4000000;SP=STACK+0x800
CALLBACK=0x3001000;CANARY=0x3100000;MISSING=0x300e000;RETURN=0x300f000
CASES=(('empty-active',(),(),0xa5,0x93),('empty-inactive',(),(),0,0),
       ('sparse-edges',(67,31,0),(),1,1),('all68',tuple(range(68)),(),0xff,0xff),
       ('duplicate-target',(67,31,0),(),0x80,1),
       ('missing-first',(67,31,0),(67,),1,0x5a),
       ('missing-middle',(67,31,0),(31,),1,0x5a))
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def dump(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
def word(value):return struct.pack('<I',value)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--helpers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--execute-original-after-grant',action='store_true');a=p.parse_args()
 if not a.execute_original_after_grant:raise RuntimeError('Prepared only: original CPU execution grant required')
 if not a.xbe.is_file():return 77
 assert sha(a.xbe)==PIN,'Expected authenticated English TU1.5 XBE'
 assert not a.output.exists();a.output.mkdir(parents=True)
 sys.path.insert(0,str(a.helpers));from xbe import Xbe
 from functions import _Code,_trace
 from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 import unicorn.x86_const as r
 image=Xbe(a.xbe);code=_Code(image,'.text');body=_trace(code,ENTRY,{ENTRY})
 pcs=set(body.code);assert len(pcs)==15 and max(pcs)==0x13a810
 body_bytes=image.read(ENTRY,END-ENTRY);assert all(body_bytes[pc-ENTRY:pc-ENTRY+code.at(pc).size]==bytes(code.at(pc).bytes)for pc in pcs)
 report=dict(status='RUNNING_ORIGINAL_MICRO_ORACLE',suite='campaign-cleanup',xbe_sha256=PIN,generator_sha256=sha(__file__),entry=ENTRY,original_body_hex=body_bytes.hex(),static_original_PCs=sorted(pcs),instruction_patches=[],engine_results_supplied=[],declared_external_callback_scope='Fixture-only x86 MOV stores/register writes and RET; no game-bank writes or success returns. Absent callback stops before execution.',cases=[])
 dump(a.output/'report.json',report)
 try:
  for name,active,missing,active_byte,loaded_byte in CASES:
   uc=Uc(UC_ARCH_X86,UC_MODE_32)
   for at,size in [(0x13a000,0x1000),(0x4f3000,0x1000),(0x44a000,0x2000),(GAME,0x2000),(STACK,0x1000),(0x3000000,0x10000),(CANARY,0x1000)]:uc.mem_map(at,size)
   uc.mem_write(ENTRY,body_bytes)
   banks={0x4f3218:word(GAME),GAME+1:bytes([loaded_byte]),GAME+0x1120:bytes([active_byte]),STACK:bytes([0xcd])*0x1000,CANARY:bytes([0xee])*68}
   stack=bytearray(banks[STACK]);struct.pack_into('<I',stack,SP-STACK,RETURN);banks[STACK]=bytes(stack)
   fixtures={};targets={};stubs={};stub_PCs=set()
   for index in range(68):
    at=0x44aedc+index*0x24;target=0
    if index in active:
     fixture=7 if name=='duplicate-target'else index;target=CALLBACK+fixture*0x40
     if index in missing:target=MISSING+index*0x10
     else:
      stub=b'\xc6\x05'+word(CANARY+fixture)+bytes([fixture+1])+b'\xb8'+word(0xa0000000|fixture)+b'\xb9'+word(0xc0000000|fixture)+b'\xba'+word(0xd0000000|fixture)+b'\xc3'
      stubs[target]=stub;fixtures[target]=dict(id=fixture,code_hex=stub.hex(),entry=target,canary_address=CANARY+fixture,canary_value=fixture+1)
      stub_PCs|={target+o for o in(0,7,12,17,22)}
     targets[target]=dict(row=index,missing=index in missing)
    banks[at]=word(target)
   for at,data in banks.items():uc.mem_write(at,data)
   for at,data in stubs.items():uc.mem_write(at,data)
   regs=[r.UC_X86_REG_EAX,r.UC_X86_REG_ECX,r.UC_X86_REG_EDX,r.UC_X86_REG_EBX,r.UC_X86_REG_ESP,r.UC_X86_REG_EBP,r.UC_X86_REG_ESI,r.UC_X86_REG_EDI]
   gp=[0x10203040,0x20304050,0x30405060,0x40506070,SP,0x50607080,0x66778899,0x708090a0]
   xmm=[[0x3f000000+n*16+k for k in range(4)]for n in range(8)]
   for reg,value in zip(regs,gp):uc.reg_write(reg,value)
   for n,value in enumerate(xmm):uc.reg_write(getattr(r,'UC_X86_REG_XMM'+str(n)),sum(v<<(32*k)for k,v in enumerate(value)))
   uc.reg_write(r.UC_X86_REG_EFLAGS,0x202);uc.reg_write(r.UC_X86_REG_FPCW,0x27f);uc.reg_write(r.UC_X86_REG_FPSW,0);uc.reg_write(r.UC_X86_REG_FPTAG,0xffff);uc.reg_write(r.UC_X86_REG_MXCSR,0x1f80)
   fp_before=dict(cw=uc.reg_read(r.UC_X86_REG_FPCW),sw=uc.reg_read(r.UC_X86_REG_FPSW),tag=uc.reg_read(r.UC_X86_REG_FPTAG),mxcsr=uc.reg_read(r.UC_X86_REG_MXCSR))
   trace=[];stores=[];fixture_stores=[];calls=[];stopped=[];last=[None]
   def snapshot():
    return dict(gp=[uc.reg_read(v)for v in regs],xmm=[[int(uc.reg_read(getattr(r,'UC_X86_REG_XMM'+str(n))))>>(32*k)&0xffffffff for k in range(4)]for n in range(8)],eflags=uc.reg_read(r.UC_X86_REG_EFLAGS),stack_return=struct.unpack('<I',uc.mem_read(uc.reg_read(r.UC_X86_REG_ESP),4))[0])
   def locate(at,size):
    matches=[(base,data)for base,data in banks.items()if base<=at and at+size<=base+len(data)]
    assert len(matches)==1,('Access outside declared fixture owner',hex(at),size);return matches[0]
   def hook_code(cpu,pc,size,_):
    last[0]=pc;trace.append(pc);assert len(trace)<=8192
    if pc in targets:
     row=dict(target=pc,abi=snapshot());calls.append(row)
     if targets[pc]['missing']:stopped.append(row);cpu.emu_stop();return
    assert pc in pcs or pc in stub_PCs,('Execution outside original or declared callback',hex(pc))
   def hook_read(cpu,access,at,size,value,_):locate(at,size)
   def hook_write(cpu,access,at,size,value,_):
    locate(at,size);row=dict(pc=last[0],address=at,before=bytes(cpu.mem_read(at,size)).hex(),written=int(value).to_bytes(size,'little').hex())
    (stores if last[0]in pcs else fixture_stores).append(row)
   uc.hook_add(UC_HOOK_CODE,hook_code);uc.hook_add(UC_HOOK_MEM_READ,hook_read);uc.hook_add(UC_HOOK_MEM_WRITE,hook_write)
   uc.emu_start(ENTRY,RETURN,count=8192)
   final_pc=uc.reg_read(r.UC_X86_REG_EIP);assert len(stopped)==int(bool(missing))
   assert (final_pc==RETURN)if not missing else(final_pc==stopped[0]['target'])
   fp_after=dict(cw=uc.reg_read(r.UC_X86_REG_FPCW),sw=uc.reg_read(r.UC_X86_REG_FPSW),tag=uc.reg_read(r.UC_X86_REG_FPTAG),mxcsr=uc.reg_read(r.UC_X86_REG_MXCSR));assert fp_before==fp_after
   final=snapshot()if missing else dict(gp=[uc.reg_read(v)for v in regs],xmm=[[int(uc.reg_read(getattr(r,'UC_X86_REG_XMM'+str(n))))>>(32*k)&0xffffffff for k in range(4)]for n in range(8)],eflags=uc.reg_read(r.UC_X86_REG_EFLAGS))
   row=dict(name=name,entry=ENTRY,before={f'{k:08x}':v.hex()for k,v in sorted(banks.items())},after={f'{k:08x}':bytes(uc.mem_read(k,len(v))).hex()for k,v in sorted(banks.items())},gp=gp,xmm=xmm,eflags=0x202,fp_before=fp_before,fp_after=fp_after,declared_callbacks=list(fixtures.values()),calls=calls,stopped=stopped,final=final,final_pc=final_pc,stores=stores,fixture_stores=fixture_stores,trace=trace,original_instruction_count=sum(pc in pcs for pc in trace),fixture_instruction_count=sum(pc in stub_PCs for pc in trace))
   dump(a.output/(name+'.json'),row);report['cases'].append(dict(name=name,file=name+'.json'));dump(a.output/'report.json',report)
  assert len(report['cases'])==7
  report.update(status='PASS_UNCHANGED_ORIGINAL_CLEANUP_WITH_DECLARED_FIXTURE_CALLBACKS',engine_readiness_proven=False,full_activation_proven=False)
  dump(a.output/'report.json',report)
 except BaseException as error:
  report.update(status='FAIL_RETAINED',failure=repr(error));dump(a.output/'report.json',report);raise
 return 0
if __name__=='__main__':sys.exit(main())

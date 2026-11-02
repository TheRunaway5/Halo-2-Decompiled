#!/usr/bin/env python3
"""Execute unchanged FC510 for every flag byte, aliases and actual read faults.

Physical Linux i386 PTRACE_SINGLESTEP, with no patched original instruction,
supplied return or game service. This is a bounded CPU-owner reference.
Run through tools/run_test.py.
"""
import argparse,ctypes,gzip,hashlib,json,os,random,signal,struct
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_marker_matrix_tu15 import State,BASE,END,BAD
from oracle_weapon_effect_marker_parent_tu15 import POOL,HEADERS,OBJECT
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,EXTENDED
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import _Code,_trace
ENTRY=0xfc510

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 _,image,pin=next(r for r in images() if r[0]=='TU1.5');code=_Code(image,'.text');sites=_trace(code,ENTRY,{ENTRY}).code;configs=[('byte',v) for v in range(256)]
 configs += [(k,5) for k in ['fault-pool','fault-header','fault-header-cross-page','fault-flag-high','fault-flag-low','fault-return','fault-return-cross-page','alias-return','alias-after-return','alias-pool','alias-header']]
 configs += [('active-mmx',v) for v in [0,1,2,3]];rng=random.Random(ENTRY);rows=[];instructions={};steps=0
 with OriginalAudioCpu(image,scratch/'marker-mode-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
  host.trace(6,host.debug_offset+7*8,0);machine=State(host)
  for n,(kind,value) in enumerate(configs):
   gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK+(n%4)*4;index=[0,3,32768,65535][n%4];unit=((n*43&65535)<<16)|index;gp[1]=unit;body=OBJECT;memory=[];blobs=[];regions=[[BASE,0x150000],[STACK-4096,8192],[END-4096,4096]]
   for at,size in regions:host.write(at,bytes(size))
   def word(at,v,width=4):
    if at>=BASE and at+width>END:return
    memory.append([at,v&((1<<(8*width))-1),width])
   word(0x4ecbd0,POOL);word(POOL+0x44,HEADERS)
   if kind=='fault-pool':word(0x4ecbd0,BAD-0x44)
   elif kind=='fault-header':word(POOL+0x44,BAD-index*12-8)
   elif kind=='fault-header-cross-page':word(POOL+0x44,END-2-index*12-8)
   elif kind=='fault-flag-high':body=END-0x12c
   elif kind=='fault-flag-low':body=BASE-0x12d
   elif kind=='fault-return':gp[4]=END
   elif kind=='fault-return-cross-page':gp[4]=END-2
   elif kind=='alias-return':body=gp[4]-0x12c;value=STOP&255
   elif kind=='alias-after-return':body=gp[4]+4-0x12c
   elif kind=='alias-pool':body=POOL+0x44-0x12c;value=HEADERS&255
   elif kind=='alias-header':body=HEADERS+index*12+8-0x12c;value=body&255
   if kind not in ['fault-header','fault-header-cross-page']:word(HEADERS+index*12+8,body)
   if not kind.startswith('fault-flag'):word(body+0x12c,value,1)
   initial_stack=rng.randbytes(1024).hex();blobs.append([STACK-512,initial_stack]);word(gp[4],STOP)
   for at,raw in blobs:host.write(at,bytes.fromhex(raw))
   for at,v,width in memory:host.write(at,v.to_bytes(width,'little'))
   fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4],stack=[EXTENDED[(n+i)%len(EXTENDED)] for i in range((0,1,3,8)[n%4])],xmm=[rng.randbytes(16).hex() for _ in range(8)])
   if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
   host.prepare(ENTRY,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags;trace=[];reads=[];fault=[];defined=NONE&~DEBUG_FLAGS
   for _ in range(40):
    pc=regs.rip
    if pc==STOP:break
    assert pc in sites,(kind,hex(pc));ins=code.at(pc);assert host.read(pc,len(ins.bytes))==bytes(ins.bytes);instructions[f'{pc:08x}']=bytes(ins.bytes).hex();trace.append(pc);operands=[]
    def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
    def operand(at,width):
     at&=NONE
     if BASE<=at and at+width<=END:assert any(lo<=at and at+width<=lo+size for lo,size in regions),(kind,hex(pc),hex(at),width)
     reads.append([pc,at,width]);operands.append((at,width))
    if ins.mnemonic.startswith('ret'):operand(regs.rsp&NONE,4)
    for op in ins.operands:
     if op.type==3 and ins.mnemonic!='lea':operand(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp,op.size)
    host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
    if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
     info=ctypes.create_string_buffer(128);host.trace(0x4202,0,ctypes.addressof(info));actual=struct.unpack_from('<Q',info.raw,16)[0];at,width=next((at,width) for at,width in operands if at<=actual<at+width);fault=[dict(instruction=pc,address=actual,operand_address=at,size=width,access='read')];break
    assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
    if ins.mnemonic in ['and','xor','test']:defined=(NONE&~DEBUG_FLAGS)&~0x10
    elif ins.mnemonic=='shr':defined=(NONE&~DEBUG_FLAGS)&~0x10
   else:raise ValueError('Original marker mode instruction bound')
   if kind.startswith('fault-'):assert fault,kind
   rows.append(dict(entry=ENTRY,kind=kind,n=n,flag_byte=value,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,regions=regions,initial_stack=initial_stack,reads=reads,stores=[],calls=[],fault=fault,trace=trace,expected=machine.snapshot(regs),flags_mask=defined))
 assert {int(pc,16) for pc in instructions}==set(sites)
 packed=gzip.compress((json.dumps(dict(schema='H2_TU15_WEAPON_MARKER_MODE_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=[],read_modify_write_sites=[],rows=rows),separators=(',',':'))+'\n').encode(),mtime=0);assert len(packed)<=1024*1024,len(packed);a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_bytes(packed)
 report=dict(passed=True,cases=len(rows),exhaustive_byte_values=256,original_sha256=pin,original_instruction_sites=len(instructions),hardware_instructions=steps,read_stops=sum(bool(r['fault']) for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),active_mmx_cases=4,instruction_patches=[],supplied_game_services=[],instruction_map=instructions,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_bytes=len(packed),fixture_sha256=hashlib.sha256(packed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',physical_xbox_proof=False,full_user_goal_complete=False)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))
if __name__=='__main__':main()

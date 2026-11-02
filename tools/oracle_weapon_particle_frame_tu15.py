#!/usr/bin/env python3
"""Physical unchanged177830 with explicit lower-owner ABI controls.

Only1778D0,17DE10 and177020 are declared test boundaries. Their arbitrary
GP/flags/FP and bank-write results are component controls, not evidence for
those callees or game-service/admission results in production. All outer
instructions, stores, caller states and faults execute unchanged on i386.
"""
import argparse,ctypes,gzip,hashlib,json,os,random,signal,struct,tempfile
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE,pack
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,CSRS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images
ENTRY=0x177830
POOL,ROWS,BITS,REPLACEMENT=0x1000000,0x1004000,0x1002000,0x1020000
READONLY,BAD=0x4100000,0x4000000
SERVICES={0x1778d0:4,0x17de10:0,0x177020:4}
EXTENTS=(0,1,2,3,31,32,33,63,64,65,97,129)


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);_,image,pin=next(x for x in images() if x[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;rng=random.Random(ENTRY);rows=[];instructions={};steps=0
 recipes=[('base',n) for n in range(96)]+[('raw-limit',n) for n in (NONE,0x80000000,0x7fffffff)]+[('raw-stride',n) for n in range(4)]+[('alias-pool',n) for n in (0x24,0x38,0x44,0x48)]
 recipes += [(name,n) for n,name in enumerate(('fault-pool','fault-bitmap','fault-rows','fault-row-parent','readonly-first','readonly-second','readonly-third','readonly-fourth','readonly-pool-save','readonly-call-argument','readonly-call-return'))]
 with tempfile.TemporaryDirectory(prefix='particle-frame-original-',dir=scratch) as temporary,OriginalAudioCpu(image,Path(temporary)/'original.elf',0x27f,0x1f80,extra_declared_regions=[(READONLY-4096,4096),(READONLY+4096,4096)],read_only_declared_regions=[(READONLY,4096)],source_bank_bytes=0x1100000) as host:
  host.trace(6,host.debug_offset+7*8,0)
  for case,(kind,n) in enumerate(recipes):
   stack={'readonly-first':READONLY+128,'readonly-second':READONLY+4116,'readonly-third':READONLY+4120,'readonly-fourth':READONLY+4124,'readonly-pool-save':READONLY+16,'readonly-call-argument':READONLY+4128,'readonly-call-return':READONLY+4132}.get(kind,STACK)
   extent=EXTENTS[n%12] if kind=='base' else n if kind=='raw-limit' else 3
   mode=(n//12)%4
   occupied=[] if not extent or extent>=0x80000000 else [0] if mode==0 else [extent-1] if mode==1 else sorted(set((0,min(31,extent-1),extent-1))) if mode==2 else []
   if kind!='base':occupied=[0] if extent and extent<0x80000000 else []
   if kind=='raw-stride':occupied=[0,1,2]
   pool=stack-n if kind=='alias-pool' else POOL;records=ROWS;bitmap=BITS;stride=(0,0x54,0x55,0x80000000)[n] if kind=='raw-stride' else 0x54
   if kind=='fault-rows':records=BAD
   elif kind=='fault-row-parent':records=0x2100000-0x4c
   elif kind=='fault-bitmap':bitmap=BAD
   regions=[(0x1000000,0x1100000),(STOP,4096)]
   for at,size in regions:host.write(at,bytes(size))
   host.write(stack-512,rng.randbytes(1024));memory=[]
   def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
   word(0x51df74,BAD if kind=='fault-pool' else pool)
   word(pool+0x24,stride);word(pool+0x38,extent);word(pool+0x44,records);word(pool+0x48,bitmap)
   word(REPLACEMENT+0x24,0x54);word(REPLACEMENT+0x38,0);word(REPLACEMENT+0x44,ROWS);word(REPLACEMENT+0x48,BITS)
   masks=[0]*5
   for index in occupied:
    if index<160:masks[index//32]|=1<<(index%32)
   if bitmap!=BAD:
    for index,value in enumerate(masks):word(bitmap+index*4,value)
   for position,index in enumerate(occupied):
    at=(records+stride*index)&NONE
    if not 0x1000000<=at<0x2100000:continue
    word(at,(0,1,0x7fff,0x8000,0xffff,0xa123)[(n+position)%6],2)
    if kind=='fault-row-parent':continue
    word(at+0x18,0xcafe0005 if n%2 else NONE)
    word(at+0x4c,0 if kind=='base' and (n+position)%3==0 else NONE)
   for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
   host.write(stack,pack([STOP,(0,0x80000000,0x3d088889,0x7fc12345)[n%4]]));initial_stack=host.read(stack-512,1024).hex()
   gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=stack
   def fp_file(index,mmx):
    fp=dict(cw=CWS[index%len(CWS)],status=0 if mmx else ((index%8)<<11)|((index*0x100)&0x4700)|(index&0x3f),mxcsr=CSRS[index%len(CSRS)],stack=[] if mmx else [EXTENDED[(index+i)%len(EXTENDED)] for i in range(index%7)],xmm=[rng.randbytes(16).hex() for _ in range(8)])
    if mmx:fp['mmx']=[rng.randbytes(8).hex()+'ffff' for _ in range(8)]
    return fp
   fp=fp_file(case,case%5==4);host.prepare(ENTRY,stack,dict(zip(ALIASES,gp)));cpu=EffectCpuState(host);cpu.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags;trace=[];stores=[];calls=[];fault=[];boundary={};defined=NONE&~DEBUG_FLAGS
   def state():return dict(gp=[getattr(regs,name)&NONE for name in ALIASES],eflags=regs.eflags&~DEBUG_FLAGS,fp=cpu.floating_point(),stack=host.read(stack-512,1024).hex())
   for iteration in range(12000):
    pc=regs.rip
    if pc==STOP:break
    if pc in SERVICES:
     before=state();ret=int.from_bytes(host.read(regs.rsp,4),'little');returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=fp_file(case+len(calls)+1,(case+len(calls))%3==1))
     for i in range(3):returned['gp'][i]=rng.getrandbits(32)
     if pc==0x1778d0:returned['gp'][0]=(0,1,0x80,0x100)[(n+len(calls))%4]
     returned['gp'][4]+=4+SERVICES[pc];writes=[]
     if pc==0x177020 and n%2:
      value=REPLACEMENT;writes.append([0x51df74,host.read(0x51df74,4).hex(),value.to_bytes(4,'little').hex()]);host.write(0x51df74,value.to_bytes(4,'little'))
     cpu.load(returned['fp'])
     for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
     regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));calls.append(dict(target=pc,before=before,flags_mask=defined,returned=state(),returned_flags_mask=NONE&~DEBUG_FLAGS,memory_writes=writes,tail=False));defined=NONE&~DEBUG_FLAGS;continue
    assert ENTRY<=pc<0x1778cd,hex(pc);ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw;instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
    def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
    if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4))
    for operand in ins.operands:
     if operand.type!=3:continue
     at=(reg(ins.reg_name(operand.mem.base))+reg(ins.reg_name(operand.mem.index))*operand.mem.scale+operand.mem.disp)&NONE;operands.append((at,operand.size,bool(operand.access&2)))
     if operand.access&2:pending.append((at,operand.size))
    if ins.mnemonic in ('push','call'):operands.append((((regs.rsp-4)&NONE),4,True))
    host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
    if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
     info=ctypes.create_string_buffer(128);host.trace(0x4202,0,ctypes.addressof(info));address=struct.unpack_from('<Q',info.raw,16)[0];assert operands;fault=[dict(instruction=pc,address=address,operand_address=operands[0][0],size=operands[0][1],kind='write' if operands[0][2] else 'read')];break
    assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,n,hex(pc),status)
    for at,width in pending:
     assert (stack-512<=at and at+width<=stack+512) or (0x1000000<=at and at+width<=0x2100000),(kind,n,hex(pc),hex(at));stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
    if ins.mnemonic in ('add','sub','cmp'):defined=NONE&~DEBUG_FLAGS
    elif ins.mnemonic in ('test','and','xor','or'):defined=(NONE&~DEBUG_FLAGS)&~0x10
    elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
    elif ins.mnemonic in ('sar','shr','shl'):defined=(NONE&~DEBUG_FLAGS)&~0x810
    elif ins.mnemonic in ('inc','dec'):defined=(NONE&~DEBUG_FLAGS&~1)|(defined&1)
   else:boundary=dict(kind='instruction-budget',instructions=12000,next_instruction=regs.rip)
   if not fault and not boundary:assert regs.rip==STOP and regs.rsp==stack+8
   if kind=='base':assert not fault and not boundary,(n,fault,boundary)
   rows.append(dict(kind=kind,n=n,entry=ENTRY,regions=regions,memory=memory,stack_base=stack,readonly=[READONLY,4096],initial_stack=initial_stack,gp=gp,eflags=initial_flags,fp=fp,calls=calls,stores=stores,trace=b''.join(struct.pack('<I',pc) for pc in trace).hex(),fault=fault,boundary=boundary,expected=state(),flags_mask=defined))
 data=(json.dumps(dict(schema='H2_TU15_WEAPON_PARTICLE_FRAME_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode();compressed=gzip.compress(data,mtime=0);assert len(data)<=16*1024*1024 and len(compressed)<=1024*1024;args.fixture.write_bytes(compressed)
 report=dict(passed=True,scope=__doc__,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),instruction_map=instructions,original_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),controlled_lower_calls={hex(target):sum(c['target']==target for r in rows for c in r['calls']) for target in SERVICES},instruction_patches=[],returned_frames=sum(not r['fault'] and not r['boundary'] for r in rows),read_stops=sum(bool(r['fault']) and r['fault'][0]['kind']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['kind']=='write' for r in rows),bounded_observation_prefixes=sum(bool(r['boundary']) for r in rows),fault_instructions={hex(pc):sum(bool(r['fault']) and r['fault'][0]['instruction']==pc for r in rows) for pc in sorted({r['fault'][0]['instruction'] for r in rows if r['fault']})},callee_FP_transitions=sum(c['before']['fp']!=c['returned']['fp'] for r in rows for c in r['calls']),raw_bytes=len(data),raw_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(compressed),fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',physical_xbox_proof=False,full_user_goal_complete=False)
 args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ('instruction_map','scope')}))
if __name__=='__main__':main()

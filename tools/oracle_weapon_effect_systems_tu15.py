#!/usr/bin/env python3
"""Physical unchanged179ED0 with declared lower-owner ABI controls.

All outer instructions execute unchanged on Linux i386. Definition lookup,
virtual methods, iteration, system update and location update are explicit
test controls. Their returned GP/flags/FP/bank writes do not qualify those
callees, game services, admission or complete particle output.
"""
import argparse,ctypes,gzip,hashlib,json,os,random,signal,struct,tempfile
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE,pack
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,CSRS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

ENTRY=0x179ed0
EPOOL,CPOOL,LPOOL=0x1000000,0x1001000,0x1002000
EFFECTS,COMPONENTS,LOCATIONS=0x1004000,0x1008000,0x100c000
DESCRIPTION,OBJECT,VTABLE,ITERATORS=0x1010000,0x1011000,0x1012000,0x1014000
LREPLACEMENT,OTHER_LOCATIONS=0x1018000,0x101c000
BAD,READONLY=0x4000000,0x4100000
VIRTUALS={0x13a080,0x13a090}
SERVICES={0x178070:0,0x13a6b0:0,0x13a080:0,0x13a090:0,
          0x178920:4,0x17dce0:12,0x17ba50:4,0x251f70:8}


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);_,image,pin=next(x for x in images() if x[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;rng=random.Random(ENTRY);rows=[];instructions={};steps=0
 recipes=[('base',n) for n in range(160)]
 recipes += [(kind,n) for kind in ['alias-effect','alias-component'] for n in (0,4,0x14,0x38,0x7c,0x108)]
 recipes += [(kind,n) for n,kind in enumerate(('fault-component-pool','fault-effect-pool','fault-component-rows','fault-effect-rows','fault-description','fault-description-count','fault-description-mode','fault-object','fault-vtable','fault-iterator','fault-location-pool','fault-location-rows'))]
 recipes += [('readonly-push',n) for n in range(4)]+[('readonly-local',0),('readonly-call',0),('readonly-late-call',0),('readonly-iterator-argument',0),('readonly-iterator-return',0)]
 with tempfile.TemporaryDirectory(prefix='effect-systems-original-',dir=scratch) as temporary,OriginalAudioCpu(image,Path(temporary)/'original.elf',0x27f,0x1f80,extra_declared_regions=[(READONLY-4096,4096),(READONLY+4096,4096)],read_only_declared_regions=[(READONLY,4096)],source_bank_bytes=0x1100000) as host:
  host.trace(6,host.debug_offset+7*8,0)
  for case,(kind,n) in enumerate(recipes):
   stack=STACK
   if kind=='readonly-push':stack=READONLY+128 if n==0 else READONLY+4096+0x54+n*4
   elif kind=='readonly-local':stack=READONLY+0x54
   elif kind=='readonly-call':stack=READONLY+4096+0x64
   elif kind=='readonly-late-call':stack=READONLY+4096+0x68
   elif kind=='readonly-iterator-argument':stack=READONLY+4096+0x6c
   elif kind=='readonly-iterator-return':stack=READONLY+4096+0x70
   ei=n%3 if kind=='base' else 0;ci=(n//3)%4 if kind=='base' else 0
   effects=stack-0x120-n if kind=='alias-effect' else BAD if kind=='fault-effect-rows' else EFFECTS
   components=stack-0x80-n if kind=='alias-component' else BAD if kind=='fault-component-rows' else COMPONENTS
   effect=(effects+ei*400)&NONE;component=(components+ci*84)&NONE
   count=0 if kind=='base' and n%8==0 else (1,2,0x7fff,0x8000,0xffff)[n%5]
   description=BAD if kind=='fault-description' else 0x2100000-0xc if kind=='fault-description-count' else 0x2100000-0x12 if kind=='fault-description-mode' else DESCRIPTION
   iterations=(n//8)%5 if kind=='base' else 2;locations=n%4 if kind=='base' else 2
   if kind in ('readonly-late-call','readonly-iterator-argument','readonly-iterator-return'):iterations=1
   if kind in ('fault-location-pool','fault-location-rows','fault-iterator'):iterations=1
   regions=[(0x1000000,0x1100000),(STOP,4096)]
   for at,size in regions:host.write(at,bytes(size))
   host.write(stack-512,rng.randbytes(1024));memory=[]
   def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
   word(0x51df74,BAD if kind=='fault-component-pool' else CPOOL);word(CPOOL+0x44,components)
   word(0x4f720c,BAD if kind=='fault-effect-pool' else EPOOL);word(EPOOL+0x44,effects)
   word(0x52d71c,BAD if kind=='fault-location-pool' else LPOOL);word(LPOOL+0x44,BAD if kind=='fault-location-rows' else LOCATIONS);word(LREPLACEMENT+0x44,OTHER_LOCATIONS)
   if components!=BAD:
    word(component+0x38,(NONE,0xa1230003,0x80000001,0x7fff0002)[n%4]);word(component+0x30,0x91230000 if locations else NONE)
   if effects!=BAD:
    for index in range(4):word(effect+0x7c+index*4,rng.getrandbits(32))
    word(effect+0x104,rng.getrandbits(32));word(effect+0x108,rng.getrandbits(32))
   if description!=BAD:
    word(description+4,rng.getrandbits(32));word(description+8,n%4)
    if kind!='fault-description-count':word(description+0xc,count,2)
    if kind not in ('fault-description-count','fault-description-mode'):word(description+0x12,(0,1,0x7fff,0x8000,0xffff)[n%5],2)
   word(OBJECT,BAD if kind=='fault-vtable' else VTABLE);word(VTABLE+0x48,0x13a080);word(VTABLE+0x44,0x13a090)
   for index in range(5):word(ITERATORS+index*16+2,(0,1,0x7fff,0x8000,0xffff)[(n+index)%5],2)
   for index in range(4):
    next_location=(0x81230000|(index+1)) if index+1<locations else NONE
    word(LOCATIONS+index*52+0xc,next_location);word(OTHER_LOCATIONS+index*52+0xc,next_location)
   for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
   argument=((0,1,0x7fff,0x8000,0xa123,0xffff)[n%6]<<16)|ei
   host.write(stack,pack([STOP,argument]));initial_stack=host.read(stack-512,1024).hex();gp=[rng.getrandbits(32) for _ in ALIASES];gp[0]=((0,1,0x7fff,0x8000,0xffff)[n%5]<<16)|ci;gp[4]=stack
   def fp_file(index,mmx):
    fp=dict(cw=CWS[index%len(CWS)],status=0 if mmx else ((index%8)<<11)|((index*0x100)&0x4700)|(index&0x3f),mxcsr=CSRS[index%len(CSRS)],stack=[] if mmx else [EXTENDED[(index+i)%len(EXTENDED)] for i in range(index%7)],xmm=[rng.randbytes(16).hex() for _ in range(8)])
    if mmx:fp['mmx']=[rng.randbytes(8).hex()+'ffff' for _ in range(8)]
    return fp
   fp=fp_file(case,case%5==4);host.prepare(ENTRY,stack,dict(zip(ALIASES,gp)));cpu=EffectCpuState(host);cpu.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags;trace=[];stores=[];calls=[];fault=[];defined=NONE&~DEBUG_FLAGS;iterator_call=0;last_call=0
   def state():return dict(gp=[getattr(regs,name)&NONE for name in ALIASES],eflags=regs.eflags&~DEBUG_FLAGS,fp=cpu.floating_point(),stack=host.read(stack-512,1024).hex())
   for iteration in range(2000):
    pc=regs.rip
    if pc==STOP:break
    if pc in SERVICES:
     before=state();ret=int.from_bytes(host.read(regs.rsp,4),'little');returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=fp_file(case+len(calls)+1,(case+len(calls))%3==1));writes=[]
     for i in range(3):returned['gp'][i]=rng.getrandbits(32)
     if pc==0x178070:returned['gp'][0]=description
     elif pc==0x13a6b0:returned['gp'][0]=BAD if kind=='fault-object' else OBJECT
     elif pc==0x13a080:returned['gp'][0]=(0,1,0x80,0x100,0x123456ff)[n%5]
     elif pc==0x17dce0:
      returned['gp'][0]=BAD if kind=='fault-iterator' else ITERATORS+iterator_call*16 if iterator_call<iterations else 0;iterator_call+=1
     if pc==0x251f70 and n%2 and kind not in ('fault-location-pool','fault-location-rows'):
      value=LREPLACEMENT if len(calls)%2 else LPOOL;writes.append([0x52d71c,host.read(0x52d71c,4).hex(),value.to_bytes(4,'little').hex()]);host.write(0x52d71c,value.to_bytes(4,'little'))
     returned['gp'][4]+=4+SERVICES[pc];cpu.load(returned['fp'])
     for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
     regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));calls.append(dict(target=pc,instruction=last_call,before=before,flags_mask=defined,returned=state(),returned_flags_mask=NONE&~DEBUG_FLAGS,memory_writes=writes,tail=False));defined=NONE&~DEBUG_FLAGS;continue
    assert ENTRY<=pc<0x17a021,hex(pc);ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw;instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
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
    if ins.mnemonic=='call':last_call=pc
    if ins.mnemonic in ('add','sub','cmp'):defined=NONE&~DEBUG_FLAGS
    elif ins.mnemonic in ('test','and','xor','or'):defined=(NONE&~DEBUG_FLAGS)&~0x10
    elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
   else:raise AssertionError(('Unexpected nonreturning controlled input',kind,n,hex(regs.rip)))
   if not fault:assert regs.rip==STOP and regs.rsp==stack+8
   if kind=='base':assert not fault,(n,fault)
   rows.append(dict(kind=kind,n=n,entry=ENTRY,regions=regions,memory=memory,stack_base=stack,readonly=[READONLY,4096],initial_stack=initial_stack,gp=gp,eflags=initial_flags,fp=fp,calls=calls,stores=stores,trace=b''.join(struct.pack('<I',pc) for pc in trace).hex(),fault=fault,boundary={},expected=state(),flags_mask=defined))
 data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_SYSTEMS_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode();compressed=gzip.compress(data,mtime=0);assert len(data)<=64*1024*1024 and len(compressed)<=4*1024*1024;args.fixture.write_bytes(compressed)
 report=dict(passed=True,scope=__doc__,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),instruction_map=instructions,original_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),controlled_lower_calls={hex(target):sum(call['target']==target for row in rows for call in row['calls']) for target in SERVICES},instruction_patches=[],returned_frames=sum(not row['fault'] for row in rows),read_stops=sum(bool(row['fault']) and row['fault'][0]['kind']=='read' for row in rows),write_stops=sum(bool(row['fault']) and row['fault'][0]['kind']=='write' for row in rows),bounded_observation_prefixes=0,callee_FP_transitions=sum(len(row['calls']) for row in rows),initial_mmx=sum('mmx' in row['fp'] for row in rows),fault_instructions={hex(pc):sum(bool(row['fault']) and row['fault'][0]['instruction']==pc for row in rows) for pc in sorted({row['fault'][0]['instruction'] for row in rows if row['fault']})},raw_bytes=len(data),raw_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(compressed),fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',physical_xbox_proof=False,full_user_goal_complete=False)
 args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['instruction_map','scope']}))


if __name__=='__main__':main()

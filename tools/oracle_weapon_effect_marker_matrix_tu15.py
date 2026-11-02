#!/usr/bin/env python3
"""Physical unchanged BAD60 marker-matrix owner with declared lower ABI controls.

The original owner instructions execute unchanged on Linux i386. BACD0 parent
selection and1E11B0/1E1200 marker owners are explicit component boundaries whose
declared GP/FP files and writes are supplied. These controls are not gameplay
outcomes or lower-owner qualification. Actual composed lower execution has its
separate22-case physical loop reference. Run through tools/run_test.py.
"""
import argparse,copy,ctypes,gzip,hashlib,json,os,random,signal,struct
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import _Code,_trace

BASE,END,BAD,ENTRY=0x1000000,0x2100000,0x6000000,0xbad60
POOL,HEADERS,TAGS,OBJECT,ANCESTOR,DEF,MODEL,OUTPUT=[BASE+n for n in (0x1000,0x2000,0x3000,0x6000,0x8000,0xa000,0xb000,0x10000)]
LOWER={0xbacd0:4,0x1e11b0:0,0x1e1200:24}
FLOATS=[0,0x80000000,1,0x80000001,0x007fffff,0x3f800000,0xc0600000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff812345]
class State(EffectCpuState):
    def snapshot(self,regs):
        result=super().snapshot(regs);result['stack']=self.host.read(STACK-512,1024).hex();return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');code=_Code(image,'.text');sites=_trace(code,ENTRY,{ENTRY}).code;configs=[]
    for model in (False,True):
        for direct in (False,True):
            for result in (0,1,0x10000,0xffff0000,0x7fff0001,0x8000fffe):
                for name in (0,0x45670001):
                    for mirror in (False,True):configs.append(('base',dict(model=model,direct=direct,result=result,name=name,mirror=mirror)))
    for value in FLOATS:
        for rounding in range(4):configs.append(('floating',dict(model=False,direct=True,result=0,name=0,mirror=True,value=value,rounding=rounding)))
    for alias in ('same','forward','backward','body-fields','body-flags','header','pool-header','dead-stack','arguments'):
        for backward in (False,True):configs.append(('alias-'+alias,dict(model=False,direct=True,result=0,name=0,mirror=True,backward=backward)))
    for kind in ('fault-first-push','fault-second-push','fault-third-push','fault-argument','fault-pool','fault-header','fault-tags','fault-definition','fault-model','fault-object-fields'):
        configs.append((kind,dict(model=kind in ('fault-tags','fault-definition','fault-model','fault-object-fields'),direct=True,result=0,name=0,mirror=True)))
    for backward in (False,True):
        for progress in (0,1,6,12):configs.append(('fault-matrix-source',dict(model=False,direct=True,result=0,name=0,mirror=True,backward=backward,progress=progress)))
    for progress in (0,1,6,12,13):configs.append(('fault-matrix-destination',dict(model=False,direct=True,result=0,name=0,mirror=True,progress=progress)))
    for available in (0,1,2,4,8,12,24,52):configs.append(('fault-output-header',dict(model=False,direct=True,result=0,name=0,mirror=True,available=available)))
    for model in (False,True):
        for direct in (False,True):configs.append(('active-mmx',dict(model=model,direct=direct,result=0,name=0,mirror=True)))
    rng=random.Random(ENTRY);rows=[];instructions={};write_only=set();steps=0
    with OriginalAudioCpu(image,scratch/'marker-matrix-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=State(host)
        for n,(kind,c) in enumerate(configs):
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK+(n%4)*4;gp[1]=(gp[1]&0xffffff00)|int(c['direct'])
            memory=[];blobs=[];regions=[[BASE,0x20000],[STACK-4096,8192],[END-4096,4096]]
            for at,size in regions:host.write(at,bytes(size))
            def word(at,value,width=4):
                if at>=BASE and at+width>END:return
                memory.append([at,value&((1<<(8*width))-1),width])
            def blob(at,raw):blobs.append([at,raw.hex()])
            unit=((n*31&65535)<<16)|3;ancestor=((n*47&65535)<<16)|5;offset=-0x400 if n%3==0 else 0x400;source=OBJECT+offset;out=OUTPUT
            word(0x4ecbd0,POOL);word(POOL+0x44,HEADERS);word(0x4f0414,TAGS);word(0x46a0d0,0x3f800000)
            for index,body in [(3,OBJECT),(5,ANCESTOR)]:
                word(HEADERS+index*12+8,body);word(body,0x71230006);word(body+4,1024 if c['mirror'] else 0)
                word(body+0x114,(0x8000,0xffff,0,1,52,32767)[n%6],2);word(body+0x116,offset,2);word(body+0x11a,-0x500 if n%2 else 0x500,2)
                for k in range(-12,13):word(body+offset+4*k,rng.getrandbits(32))
                for k in range(4,7):word(body+offset+4*k,c.get('value',FLOATS[(n+k)%len(FLOATS)]))
            word(TAGS+6*16+8,DEF);word(DEF+0x38,0x61340007 if c['model'] else NONE);word(TAGS+7*16+8,MODEL);word(MODEL+4,BASE+0x1c000)
            if kind.startswith('alias-'):out={'same':source-0x38,'forward':source-0x38+4,'backward':source-0x38-4,'body-fields':OBJECT+0x100,'body-flags':OBJECT-4,'header':HEADERS+3*12+8-0x38,'pool-header':POOL+0x10,'dead-stack':STACK-256,'arguments':gp[4]+4}[kind[6:]]
            if kind=='fault-first-push':gp[4]=BASE+8
            elif kind=='fault-second-push':gp[4]=BASE+12
            elif kind=='fault-third-push':gp[4]=BASE+16
            elif kind=='fault-argument':gp[4]=END-8
            elif kind=='fault-pool':word(0x4ecbd0,BAD-0x44)
            elif kind=='fault-header':word(POOL+0x44,BAD-3*12-8)
            elif kind=='fault-tags':word(0x4f0414,BAD-6*16-8)
            elif kind=='fault-definition':word(TAGS+6*16+8,BAD-0x38)
            elif kind=='fault-model':word(TAGS+7*16+8,BAD-4)
            elif kind=='fault-object-fields':
                body=END-0x118;word(HEADERS+3*12+8,body);word(body,0x71230006);word(body+4,1024);word(body+0x116,0x400,2)
            elif kind=='fault-matrix-source':
                k=c['progress'];edge=BASE+4*(k-1) if c.get('backward') else END-4*k;body=BASE+0x400 if c.get('backward') else END-0x400
                word(HEADERS+3*12+8,body);word(body,0x71230006);word(body+4,1024);word(body+0x116,edge-body,2)
            elif kind=='fault-matrix-destination':out=END-0x38-4*c['progress']
            elif kind=='fault-output-header':out=END-c['available']
            gp[3]=out;initial_stack=rng.randbytes(1024).hex();blob(STACK-512,bytes.fromhex(initial_stack));blob(OUTPUT-64,rng.randbytes(256))
            word(gp[4],STOP);word(gp[4]+4,unit);word(gp[4]+8,c['name']);word(gp[4]+12,(0x8000,0xffff,0,1,32767)[n%5])
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            rounding=c.get('rounding',n%4);fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=0x1f80|(rounding<<13)|(0,0x40,0x8000,0x8040)[n%4],stack=[EXTENDED[(n+i)%len(EXTENDED)] for i in range((0,1,3,8)[n%4])],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(ENTRY,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0x8d5)|(1024 if c.get('backward') else 0);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];fault=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(400):
                pc=regs.rip
                if pc==STOP:break
                if pc in LOWER:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little');returned=copy.deepcopy(before);returned['gp'][4]+=4+LOWER[pc]
                    returned['gp'][0]=(NONE if n%3==0 else ancestor) if pc==0xbacd0 else ((0x65210000|n) if pc==0x1e11b0 else c['result'])
                    returned['gp'][1]=rng.getrandbits(32);returned['gp'][2]=rng.getrandbits(32);returned['eflags']=0x202|(rng.getrandbits(12)&0xcd5)
                    newfp=copy.deepcopy(before['fp']);newfp['cw']=CWS[(n+len(calls)+1)%len(CWS)];newfp['mxcsr']=0x1f80|(((n+len(calls))%4)<<13)|(n&0x3f);newfp['xmm']=[rng.randbytes(16).hex() for _ in range(8)];returned['fp']=newfp
                    writes=[]
                    if pc==0x1e1200:
                        raw=struct.pack('<I',c['result']^0x5a5a0000);writes.append([out+0x70,raw.hex()]);host.write(out+0x70,raw)
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));machine.load(newfp);regs=host.registers();returned=machine.snapshot(regs)
                    calls.append(dict(target=pc,before=before,returned=returned,flags_mask=defined,writes=writes,controlled=True));defined=NONE&~DEBUG_FLAGS;continue
                assert pc in sites,(kind,hex(pc));ins=code.at(pc);assert host.read(pc,len(ins.bytes))==bytes(ins.bytes);instructions[f'{pc:08x}']=bytes(ins.bytes).hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                def operand(at,width):
                    at&=NONE
                    if BASE<=at and at+width<=END:assert any(low<=at and at+width<=low+size for low,size in regions),(kind,hex(pc),hex(at),width)
                    reads.append([pc,at,width]);operands.append((at,width))
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4));write_only.add(pc)
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):operand(regs.rsp&NONE,4)
                if pc==0xbaeb0:
                    if regs.rcx&NONE:operand(regs.rsi&NONE,4);pending.append((regs.rdi&NONE,4))
                else:
                    for op in ins.operands:
                        if op.type!=3 or ins.mnemonic=='lea':continue
                        at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                        if op.access&1:operand(at,op.size)
                        if op.access&2:pending.append((at,op.size));write_only.add(pc)
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    info=ctypes.create_string_buffer(128);host.trace(0x4202,0,ctypes.addressof(info));actual=struct.unpack_from('<Q',info.raw,16)[0]
                    failed_reads=[(at,width) for at,width in operands if at<=actual<at+width];access='read' if failed_reads else 'write';at,width=(failed_reads if failed_reads else pending)[-1];fault=[dict(instruction=pc,address=actual,operand_address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic in ('cmp','add','sub'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('and','or','xor','test'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='mul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
                elif ins.mnemonic in ('shl','shr'):defined=(NONE&~DEBUG_FLAGS)&~0x810
            else:raise ValueError('Original marker owner instruction limit')
            if kind.startswith('fault-'):assert fault,(kind,n)
            rows.append(dict(entry=ENTRY,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,regions=regions,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,fault=fault,trace=trace,expected=machine.snapshot(regs),flags_mask=defined))
    assert {int(pc,16) for pc in instructions}==set(sites),(len(instructions),len(sites))
    raw=(json.dumps(dict(schema='H2_TU15_WEAPON_MARKER_MATRIX_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_only),read_modify_write_sites=[0xbaeb0],rows=rows),separators=(',',':'))+'\n').encode();packed=gzip.compress(raw,mtime=0);assert len(packed)<=1024*1024,len(packed);a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_bytes(packed)
    report=dict(passed=True,cases=len(rows),original_sha256=pin,original_instruction_sites=len(instructions),hardware_instructions=steps,read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),backward_cases=sum(bool(r['eflags']&1024) for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),controlled_lower_calls=sum(len(r['calls']) for r in rows),controlled_boundaries=sorted(LOWER),instruction_patches=[],instruction_map=instructions,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_bytes=len(packed),fixture_sha256=hashlib.sha256(packed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP with declared lower ABI controls',whole_lower_execution_proven=False,physical_xbox_proof=False,full_user_goal_complete=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))
if __name__=='__main__':main()

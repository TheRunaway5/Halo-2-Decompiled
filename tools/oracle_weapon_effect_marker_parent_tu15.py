#!/usr/bin/env python3
"""Physical unchanged BACD0 parent selection with an explicit FC510 ABI boundary.

The original44 instruction sites execute unchanged on Linux i386. FC510 has
declared component GP/FP returns, not supplied gameplay outcomes. Production
must call its real owner. Run through tools/run_test.py.
"""
import argparse,copy,ctypes,gzip,hashlib,json,os,random,signal,struct
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_marker_matrix_tu15 import State,BASE,END,BAD
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,EXTENDED
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import _Code,_trace

ENTRY,LOWER=0xbacd0,0xfc510
POOL,HEADERS,TAGS,OBJECT,DEFINITION=[BASE+n for n in (0x1000,0x10000,0x100000,0x120000,0x140000)]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');code=_Code(image,'.text');sites=_trace(code,ENTRY,{ENTRY}).code;configs=[]
    for depth in (0,1,2,5):
        for flags in (0,1):
            for kind in (0,1,2,3,4,5,31,32,33,34,35,36,255):
                for result in (0,1):configs.append(('base',dict(depth=depth,flags=flags,kind=kind,result=result)))
    for kind in ('fault-pool','fault-pool-header','fault-first-push','fault-second-push','fault-argument','fault-header','fault-parent','fault-tag-global','fault-tag-record','fault-definition','fault-flags','fault-kind','fault-result'):
        configs.append((kind,dict(depth=0,flags=1,kind=2,result=1)))
    for alias in ('saved-esi-parent','saved-edi-parent','saved-esi-flags','saved-edi-kind','argument-header'):
        for result in (0,1):configs.append(('alias-'+alias,dict(depth=0,flags=1,kind=2,result=result)))
    for depth in (0,2):
        for result in (0,1):configs.append(('active-mmx',dict(depth=depth,flags=1,kind=2,result=result)))
    rng=random.Random(ENTRY);rows=[];instructions={};write_only=set();steps=0
    with OriginalAudioCpu(image,scratch/'marker-parent-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=State(host)
        for n,(kind,c) in enumerate(configs):
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK+(n%4)*4;memory=[];blobs=[];regions=[[BASE,0x150000],[STACK-4096,8192],[END-4096,4096]]
            for at,size in regions:host.write(at,bytes(size))
            def word(at,value,width=4):
                if at>=BASE and at+width>END:return
                memory.append([at,value&((1<<(8*width))-1),width])
            def blob(at,raw):blobs.append([at,raw.hex()])
            units=[(((n*41+k*19)&65535)<<16)|(3+k) for k in range(c['depth']+2)];bodies=[OBJECT+k*0x400 for k in range(c['depth']+2)]
            word(0x4ecbd0,POOL);word(POOL+0x44,HEADERS);word(0x4f0414,TAGS)
            for k,(unit,body) in enumerate(zip(units,bodies)):
                word(HEADERS+(unit&65535)*12+8,body);word(body,0x73450007);word(body+0x14,units[k+1] if k<c['depth'] else NONE)
                word(body+4,c['flags']);word(body+0xaa,c['kind'],1);word(body+0x154,rng.getrandbits(32));word(TAGS+7*16+8,DEFINITION);word(DEFINITION+2,0x10,1)
            # Reach both parent-present but definition-gate-clear and absent
            # parent cases, including all byte shifts modulo32.
            if c['depth']==0 and n%2:word(bodies[0]+0x14,units[1]);word(DEFINITION+2,0,1)
            if kind in ('fault-tag-global','fault-tag-record','fault-definition'):word(bodies[0]+0x14,units[1])
            if kind=='fault-pool':word(0x4ecbd0,BAD)
            elif kind=='fault-pool-header':word(0x4ecbd0,BAD-0x44)
            elif kind=='fault-first-push':gp[4]=BASE
            elif kind=='fault-second-push':gp[4]=BASE+4
            elif kind=='fault-argument':gp[4]=END-4
            elif kind=='fault-header':word(POOL+0x44,BAD-(units[0]&65535)*12-8)
            elif kind=='fault-parent':word(HEADERS+3*12+8,END-0x14)
            elif kind=='fault-tag-global':word(0x4f0414,BAD)
            elif kind=='fault-tag-record':word(0x4f0414,BAD-7*16-8)
            elif kind=='fault-definition':word(TAGS+7*16+8,BAD-2)
            elif kind=='fault-flags':
                body=BASE-8;word(HEADERS+3*12+8,body);word(body+0x14,NONE)
            elif kind in ('fault-kind','fault-result'):
                body=END-(0xaa if kind=='fault-kind' else 0x154);word(HEADERS+3*12+8,body);word(body+0x14,NONE);word(body+4,1);word(body+0xaa,2,1)
            elif kind.startswith('alias-'):
                alias=kind[6:]
                if alias=='saved-esi-parent':gp[4]=OBJECT+0x18;gp[6]=NONE
                elif alias=='saved-edi-parent':gp[4]=OBJECT+0x1c;gp[7]=NONE
                elif alias=='saved-esi-flags':gp[4]=OBJECT+8;gp[6]=1;gp[7]=0x98760007
                elif alias=='saved-edi-kind':gp[4]=OBJECT+0xb2;gp[7]=2
                else:
                    units[0]=OBJECT;word(HEADERS+(OBJECT&65535)*12+8,OBJECT);gp[4]=HEADERS+(OBJECT&65535)*12+4
            initial_stack=rng.randbytes(1024).hex();blob(STACK-512,bytes.fromhex(initial_stack));word(gp[4],STOP);word(gp[4]+4,units[0])
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4],stack=[EXTENDED[(n+i)%len(EXTENDED)] for i in range((0,1,3,8)[n%4])],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(ENTRY,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];fault=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(600):
                pc=regs.rip
                if pc==STOP:break
                if pc==LOWER:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little');returned=copy.deepcopy(before);returned['gp'][4]+=4
                    returned['gp'][0]=(rng.getrandbits(24)<<8)|c['result'];returned['gp'][1]=rng.getrandbits(32);returned['gp'][2]=rng.getrandbits(32);returned['eflags']=0x202|(rng.getrandbits(12)&0xcd5)
                    newfp=copy.deepcopy(before['fp']);newfp['cw']=CWS[(n+1)%len(CWS)];newfp['mxcsr']=0x1f80|(((n+1)%4)<<13)|(n&0x3f);newfp['xmm']=[rng.randbytes(16).hex() for _ in range(8)];returned['fp']=newfp
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));machine.load(newfp);regs=host.registers();returned=machine.snapshot(regs)
                    calls.append(dict(target=pc,before=before,returned=returned,flags_mask=defined,writes=[],controlled=True));defined=NONE&~DEBUG_FLAGS;continue
                assert pc in sites,(kind,hex(pc));ins=code.at(pc);assert host.read(pc,len(ins.bytes))==bytes(ins.bytes);instructions[f'{pc:08x}']=bytes(ins.bytes).hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                def operand(at,width):
                    at&=NONE
                    if BASE<=at and at+width<=END:assert any(low<=at and at+width<=low+size for low,size in regions),(kind,hex(pc),hex(at),width)
                    reads.append([pc,at,width]);operands.append((at,width))
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4));write_only.add(pc)
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):operand(regs.rsp&NONE,4)
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                    if op.access&1:operand(at,op.size)
                    if op.access&2:pending.append((at,op.size));write_only.add(pc)
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    info=ctypes.create_string_buffer(128);host.trace(0x4202,0,ctypes.addressof(info));actual=struct.unpack_from('<Q',info.raw,16)[0];failed_reads=[(at,width) for at,width in operands if at<=actual<at+width];access='read' if failed_reads else 'write';at,width=(failed_reads if failed_reads else pending)[-1];fault=[dict(instruction=pc,address=actual,operand_address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic in ('cmp','add','sub'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('and','or','xor','test'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic in ('shl','shr'):defined=(NONE&~DEBUG_FLAGS)&~0x810
            else:raise ValueError('Original marker parent instruction limit')
            if kind.startswith('fault-'):assert fault,(kind,n)
            rows.append(dict(entry=ENTRY,kind=kind,n=n,depth=c['depth'],gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,regions=regions,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,fault=fault,trace=trace,expected=machine.snapshot(regs),flags_mask=defined))
    assert {int(pc,16) for pc in instructions}==set(sites),(len(instructions),len(sites))
    raw=(json.dumps(dict(schema='H2_TU15_WEAPON_MARKER_PARENT_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_only),read_modify_write_sites=[],rows=rows),separators=(',',':'))+'\n').encode();packed=gzip.compress(raw,mtime=0);assert len(packed)<=1024*1024,len(packed);a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_bytes(packed)
    report=dict(passed=True,cases=len(rows),original_sha256=pin,original_instruction_sites=len(instructions),hardware_instructions=steps,read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),max_parent_depth=max(r['depth'] for r in rows),controlled_lower_calls=sum(len(r['calls']) for r in rows),controlled_boundaries=[LOWER],instruction_patches=[],instruction_map=instructions,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_bytes=len(packed),fixture_sha256=hashlib.sha256(packed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP with declared FC510 ABI controls',whole_lower_execution_proven=False,physical_xbox_proof=False,full_user_goal_complete=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))
if __name__=='__main__':main()

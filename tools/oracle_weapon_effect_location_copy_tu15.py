#!/usr/bin/env python3
"""Physical unchanged17DD50/17DDC0 location copies and their real lower calls.

No instruction or callee is patched and no lower result is supplied. Real
169376/1693AB/16945E/145560 execute on declared memory, including their complete
returned GP/FP files. The bounded native owner test replays those recorded
lower boundaries; that is not qualification of a native lower replacement.
Run through tools/run_test.py; keep only the compressed reference and report.
"""
import argparse,ctypes,gzip,hashlib,json,os,random,signal,struct
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import _Code,_trace

BASE,END,BAD=0x1000000,0x2100000,0x6000000
POOL,HEADERS,EFFECT,OBJECT,OUTPUT,MODELS=[BASE+n for n in (0x1000,0x2000,0x3000,0x6000,0x10000,0x20000)]
OWNERS={0x17dd50,0x17ddc0};LOWERS={0x169376,0x1693ab}

class State(EffectCpuState):
    def snapshot(self,regs):
        result=super().snapshot(regs);result['stack']=self.host.read(STACK-256,512).hex();return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);_,image,pin=next(r for r in images() if r[0]=='TU1.5');code=_Code(image,'.text')
    owner_sites=set().union(*(_trace(code,e,OWNERS).code for e in OWNERS));configs=[]
    for entry in sorted(OWNERS):
        for selector in (0,1,0xffff,0x8000,0x8001,0xfffe,0x7fff,0x56788001):
            for active in (False,True):
                for direction in (False,True):configs.append((entry,'base',dict(selector=selector,active=active,backward=direction)))
        for kind in ('same','forward','backward','effect','header','dead-stack'):
            for direction in (False,True):configs.append((entry,'alias-'+kind,dict(selector=1,active=False,backward=direction)))
        for direction in (False,True):
            for k in (0,1,6,12):
                configs.append((entry,'fault-copy-source',dict(selector=0,active=False,backward=direction,progress=k)))
                configs.append((entry,'fault-copy-destination',dict(selector=0,active=False,backward=direction,progress=k)))
        for kind in ('fault-pool','fault-header','fault-object-offset','fault-first-push','fault-second-push','fault-call-push','fault-return' if entry==0x17dd50 else 'fault-entry-argument'):
            configs.append((entry,kind,dict(selector=0x8001 if kind=='fault-call-push' else 0,active=kind=='fault-call-push',backward=False)))
        for active in (False,True):configs.append((entry,'active-mmx',dict(selector=0x8001,active=active,backward=False)))
    rng=random.Random(0x17dd50);rows=[];instructions={};lower_instructions={};steps=0;write_only=set();rep_sites={0x17ddb4,0x17ddfb};corrected_access={};wide_stores=0
    with OriginalAudioCpu(image,scratch/'location-copy-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=State(host)
        for n,(entry,kind,settings) in enumerate(configs):
            memory=[];blobs=[];initialized=set();gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK+(n%4)*4
            selector=settings['selector'];node=(selector&0x7fff) if selector&0xffff!=0xffff else -1;node=node if node<0x8000 else node-0x10000
            backward=settings['backward'];animation=(n%2) if settings['active'] else NONE;object_datum=(rng.getrandbits(16)<<16)|3;clip=(n//2)%2
            def word(at,value,width=4):
                assert BASE<=at and at+width<=END or at<BASE,(kind,hex(at),width)
                memory.append([at,value&((1<<(width*8))-1),width])
            def blob(at,raw):blobs.append([at,raw.hex()])
            def matrix(at,backward_extent=False):
                for i in range(-12 if backward_extent else 0,13):word(at+4*i,struct.unpack('<I',struct.pack('<f',(i+1)*(.125 if n%2 else -.25)))[0])
            word(0x4ecbd0,POOL);word(POOL+0x44,HEADERS);word(HEADERS+3*12+8,OBJECT);word(OBJECT+0x116,0x400,2)
            word(EFFECT+0x48,object_datum);word(EFFECT+0x58,animation);word(0x4f6498,MODELS)
            index=animation if animation!=NONE else 0;record=MODELS+index*0x20cc
            word(record+0x10,object_datum if clip==0 else object_datum^0x10000);word(record+0x1020,object_datum)
            matrix(record+0x2060);matrix(record+clip*0x1010+0x314+node*0x34,True)
            source=OBJECT+0x400+node*0x34
            for i in range(-12,13):word(source+4*i,rng.getrandbits(32))
            destination=OUTPUT
            if kind.startswith('alias-'):
                destination={'same':source,'forward':source+4,'backward':source-4,'effect':EFFECT+0x48,'header':HEADERS+3*12+8,'dead-stack':STACK-160}[kind[6:]]
            if kind.startswith('fault-copy-'):
                k=settings['progress'];edge=BASE+4*(k-1) if backward else END-4*k
                blob(BASE if backward else END-4096,rng.randbytes(4096))
                if kind=='fault-copy-source':
                    # Use node0 and a real object whose signed matrix offset
                    # selects the guarded physical-bank edge.
                    obj=BASE+0x400 if backward else END-0x400;word(HEADERS+3*12+8,obj);word(obj+0x116,(edge-obj)&65535,2)
                else:destination=edge
            if kind=='fault-pool':word(0x4ecbd0,BAD-0x44)
            elif kind=='fault-header':word(POOL+0x44,BAD-(3*12+8))
            elif kind=='fault-object-offset':word(HEADERS+3*12+8,BAD-0x116)
            elif kind=='fault-first-push':gp[4]=BASE;blob(BASE,rng.randbytes(4096))
            elif kind=='fault-second-push':gp[4]=BASE+4;blob(BASE,rng.randbytes(4096))
            elif kind=='fault-call-push':gp[4]=BASE+(20 if entry==0x17dd50 else 16);blob(BASE,rng.randbytes(4096))
            elif kind in ('fault-return','fault-entry-argument'):gp[4]=END;blob(END-4096,rng.randbytes(4096))
            initial_stack=rng.randbytes(512).hex();blob(STACK-256,bytes.fromhex(initial_stack));blob(OUTPUT-64,rng.randbytes(256))
            if gp[4]+4<=END:word(gp[4],STOP)
            if entry==0x17dd50:gp[0]=selector
            else:
                gp[0]=(gp[0]&0xffffff00)|(1 if settings['active'] else 0)
                if gp[4]+8<=END:word(gp[4]+4,selector)
            gp[1]=destination;gp[2]=EFFECT
            for at,raw in blobs:host.write(at,bytes.fromhex(raw));initialized.update(range(at,at+len(raw)//2))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'));initialized.update(range(at,at+width))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4],stack=[EXTENDED[(n+i)%len(EXTENDED)] for i in range((0,1,3,8)[n%4])],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(entry,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0x8d5)|(1024 if backward else 0);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];native_calls=[];call_order=[];fault=[];defined=NONE&~DEBUG_FLAGS;external=None;native_return=None
            for _ in range(1500):
                pc=regs.rip
                if external is not None and pc==external['return']:
                    calls[external['index']].update(returned=machine.snapshot(regs),returned_flags_mask=defined);external=None
                if native_return is not None and pc==0x17de0b:
                    native_calls[native_return].update(returned=machine.snapshot(regs),returned_flags_mask=defined);native_return=None
                if pc==STOP:break
                if pc==0x17dd50 and entry==0x17ddc0 and external is None:
                    native_return=len(native_calls);native_calls.append(dict(target=pc,natural=True,before=machine.snapshot(regs),flags_mask=defined,returned=None));call_order.append(['native',native_return])
                if pc in LOWERS and external is None:
                    index=len(calls);ret=int.from_bytes(host.read(regs.rsp,4),'little');calls.append(dict(target=pc,natural=True,before=machine.snapshot(regs),flags_mask=defined,returned=None,writes=[],reads=[]));call_order.append(['lower',index]);external=dict(index=index,return_=ret);external['return']=ret
                ins=code.at(pc);assert host.read(pc,len(ins.bytes))==bytes(ins.bytes)
                parent=external is None;target=instructions if parent else lower_instructions;target[f'{pc:08x}']=bytes(ins.bytes).hex()
                if parent:assert pc in owner_sites,(kind,hex(pc));trace.append(pc)
                pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                def operand(at,width):
                    at&=NONE
                    if BASE<=at and at+width<=END:assert all(p in initialized for p in range(at,at+width)),(n,entry,settings,kind,hex(pc),hex(at),width)
                    (reads if parent else calls[external['index']]['reads']).append([pc,at,width]);operands.append((at,width))
                if ins.mnemonic=='push' or ins.mnemonic=='call':pending.append(((regs.rsp-4)&NONE,4));write_only.add(pc)
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):operand(regs.rsp&NONE,4)
                if pc in rep_sites:
                    if regs.rcx&NONE:operand(regs.rsi&NONE,4);pending.append((regs.rdi&NONE,4))
                else:
                    for operand_index,op in enumerate(ins.operands):
                        if op.type!=3 or ins.mnemonic=='lea':continue
                        at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                        access=op.access
                        if ins.mnemonic=='movhps'and operand_index==0:
                            assert op.size==8 and bytes(ins.bytes).startswith(bytes.fromhex('0f17'))
                            corrected_access[pc]=dict(reported_access=op.access,actual_access=2,opcode=bytes(ins.bytes).hex());access=2;write_only.add(pc)
                        if access&1:operand(at,op.size)
                        if access&2:pending.append((at,op.size))
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert kind.startswith('fault-') and external is None and (operands or pending),(kind,hex(pc),status)
                    def mapped(at,width):
                        if BASE<=at and at+width<=END:return True
                        try:image.read(at,width);return True
                        except Exception:return False
                    stopped_reads=[r for r in operands if not mapped(*r)]
                    access='read' if stopped_reads else 'write';at,width=(stopped_reads if stopped_reads else pending)[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
                for at,width in pending:
                    raw=host.read(at,width);initialized.update(range(at,at+width))
                    if width==8:wide_stores+=1
                    if parent:stores.append([pc,at,width,int.from_bytes(raw,'little')])
                    else:calls[external['index']]['writes'].append([at,raw.hex()])
                if ins.mnemonic in ('cmp','add','sub'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('and','or','xor','test'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
                elif ins.mnemonic=='inc':defined=defined|0x8d4
            else:raise ValueError('Original location-copy instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            rows.append(dict(entry=entry,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,native_calls=native_calls,call_order=call_order,fault=fault,trace=trace,expected=machine.snapshot(regs),flags_mask=defined))
    assert {int(p,16) for p in instructions}==owner_sites,(len(instructions),len(owner_sites));assert len(corrected_access)==4 and wide_stores==36
    raw=(json.dumps(dict(schema='H2_TU15_WEAPON_LOCATION_COPY_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_only),read_modify_write_sites=sorted(rep_sites),rows=rows),separators=(',',':'))+'\n').encode();compressed=gzip.compress(raw,mtime=0);assert len(compressed)<=1024*1024;args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(compressed)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),actual_lower_instruction_sites=len(lower_instructions),capstone_access_corrections=corrected_access,wide_store_events=wide_stores,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),backward_cases=sum(bool(r['eflags']&1024) for r in rows),actual_lower_calls=sum(len(r['calls']) for r in rows),instruction_map=instructions,actual_lower_instruction_map=lower_instructions,fixture_bytes=len(compressed),fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[],all_lower_outcomes_computed_by_original=True,physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['instruction_map','actual_lower_instruction_map']}))
if __name__=='__main__':main()

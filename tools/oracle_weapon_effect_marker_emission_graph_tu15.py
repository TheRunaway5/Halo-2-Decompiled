#!/usr/bin/env python3
"""Physical unchanged TU1.5 marker emission and both real math callees.

Executes1E1200,144910 and145560 without instruction patches or supplied game
services. Inputs cover filtering, capacities, remapping, floating-point files,
aliases and genuine faults. Run through tools/run_test.py.
"""
import argparse,ctypes,gzip,hashlib,json,os,random,signal,struct
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_marker_matrix_tu15 import State,BASE,END,BAD,FLOATS
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,EXTENDED
from weapon_reference_versions import images
from oracle_smg_firing_1_1 import _Code,_trace

ENTRY=0x1e1200
ENTRIES={ENTRY,0x144910,0x145560}
TAGS,MODEL,GROUPS,MARKERS,REMAP,MATRICES,SCENE,OUTPUT=[BASE+n for n in (0x1000,0x12000,0x14000,0x18000,0x20000,0x30000,0x40000,0x50000)]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(r for r in images()if r[0]=='TU1.5');code=_Code(image,'.text');owners={e:set(_trace(code,e,ENTRIES).code)for e in ENTRIES};sites=set().union(*owners.values());assert len(sites)==252
    configs=[]
    for count in (-1,0,1,3):
        for capacity in (-1,0,1,4):
            for filtering in ('none','match','mismatch'):
                for mirror in (False,True):configs.append(('base',dict(count=count,capacity=capacity,filtering=filtering,mirror=mirror)))
    for value in FLOATS:
        for rounding in range(4):configs.append(('floating',dict(count=1,capacity=1,filtering='none',mirror=True,value=value,rounding=rounding)))
    for kind in ('early-none','early-none-bad-tags','fault-first-push','fault-tags','fault-model','fault-group-count','fault-marker','fault-scene','fault-remap','fault-output','fault-quaternion','fault-matrix','fault-return'):
        configs.append((kind,dict(count=1,capacity=1,filtering='match',mirror=True)))
    for kind in ('alias-output-marker','alias-output-matrix','alias-output-matrix-forward','alias-output-matrix-backward','alias-output-group','alias-output-remap','alias-dead-stack'):
        configs.append((kind,dict(count=2,capacity=1,filtering='none',mirror=True)))
    for filtering,mirror in [('none',False),('none',True),('match',True),('mismatch',False)]:configs.append(('active-mmx',dict(count=2,capacity=2,filtering=filtering,mirror=mirror)))
    for node in (-3,-1,0x12340002,0xabcdffff):
        for mirror in (False,True):configs.append(('signed-remap',dict(count=1,capacity=1,filtering='none',mirror=mirror,remap_node=node)))
    configs.append(('alias-output-group-count-one',dict(count=1,capacity=1,filtering='none',mirror=True)))
    rng=random.Random(ENTRY);rows=[];instructions={};write_only=set();read_modify_write=set();steps=0;call_entries=0;call_returns=0;corrected_access={};wide_stores=0;wide_changes=0
    with OriginalAudioCpu(image,scratch/'marker-emission-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000)as host:
        host.trace(6,host.debug_offset+7*8,0);machine=State(host)
        for n,(kind,c)in enumerate(configs):
            gp=[rng.getrandbits(32)for _ in ALIASES];gp[4]=STACK+(n%4)*4;memory=[];blobs=[];regions=[[BASE,0x90000],[STACK-4096,8192],[END-4096,4096]]
            for at,size in regions:host.write(at,bytes(size))
            def word(at,value,width=4):
                if at>=BASE and at+width>END:return
                memory.append([at,value&((1<<(8*width))-1),width])
            def blob(at,raw):blobs.append([at,raw.hex()])
            index=0 if n%2 else 3;tag=((n*17&65535)<<16)|7;body=MODEL;groups=GROUPS;markers=MARKERS;out=OUTPUT;scene=0 if c['filtering']=='none'else SCENE;remap=REMAP if n%2 or'remap_node'in c else 0;matrices=MATRICES;count=c['count']
            if kind.startswith('early-none'):index=NONE
            if kind=='fault-first-push':gp[4]=BASE+12
            elif kind=='fault-model':body=BAD-0x5c
            elif kind=='fault-group-count':groups=BAD-4;index=0
            elif kind=='fault-marker':markers=BAD
            elif kind=='fault-scene':scene=BAD
            elif kind=='fault-remap':remap=BAD
            elif kind=='fault-output':out=BAD
            elif kind=='fault-quaternion':markers=END-16
            elif kind=='fault-matrix':matrices=BAD
            elif kind=='fault-return':gp[4]=END;index=NONE
            elif kind.startswith('alias-'):
                out={'alias-output-marker':MARKERS,'alias-output-matrix':MATRICES,'alias-output-matrix-forward':MATRICES+4,'alias-output-matrix-backward':MATRICES-4,'alias-output-group':GROUPS-4,'alias-output-group-count-one':GROUPS-4,'alias-output-remap':REMAP,'alias-dead-stack':STACK-256}[kind]
            word(0x4f0414,BAD-7*16-8 if kind in ('early-none-bad-tags','fault-tags')else TAGS);word(TAGS+7*16+8,body)
            if kind!='fault-model':word(body+0x5c,groups)
            if kind!='fault-group-count':
                group=groups+(0 if index==NONE else index)*12;word(group+4,count);word(group+8,markers)
            for k in range(4):
                at=markers+36*k
                if kind=='fault-marker':break
                bone=(-128,0,1,127)[(n+k)%4];word(at,bone,1);word(at+1,23+k,1);word(at+2,k%3,1)
                if scene not in (0,BAD):word(scene+bone,23+k if c['filtering']=='match'else 99+k,1)
                for j in range(3):word(at+4+4*j,c.get('value',(0x3f000000,0xbf800000,0x40000000)[j]))
                for j in range(4):word(at+16+4*j,c['value']if'value'in c else(0x3f800000 if j==3 else 0))
                word(at+32,0x3f000000+k)
            for k in range(4):word(REMAP+4*k,c.get('remap_node',k))
            for k in range(-3,7):
                transform=[0x3f800000,0x3f800000,0,0,0,0x3f800000,0,0,0,0x3f800000,0x3f000000,0xbf000000,0x40000000];blob(MATRICES+52*k,struct.pack('<13I',*transform))
            initial_stack=rng.randbytes(1024).hex();blob(STACK-512,bytes.fromhex(initial_stack));blob(OUTPUT-64,rng.randbytes(512));word(gp[4],STOP)
            for k,v in enumerate([scene,remap,matrices,int(c['mirror']),out,c['capacity']],1):word(gp[4]+4*k,v)
            gp[0]=index;gp[1]=tag
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            rounding=c.get('rounding',n%4);fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=0x1f80|(rounding<<13)|(0,0x40,0x8000,0x8040)[n%4],stack=[EXTENDED[(n+i)%len(EXTENDED)]for i in range((0,1,3,8)[n%4])],xmm=[rng.randbytes(16).hex()for _ in range(8)])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex()for i in range(8)])
            host.prepare(ENTRY,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];waiting=[];fault=[];defined=NONE&~DEBUG_FLAGS;static={}
            for _ in range(6000):
                pc=regs.rip
                if waiting and pc==waiting[-1]['return_address']:
                    waiting[-1]['returned']=machine.snapshot(regs);waiting[-1]['returned_flags_mask']=defined;waiting.pop();call_returns+=1
                if pc==STOP:break
                assert pc in sites,(kind,hex(pc))
                if pc in ENTRIES-{ENTRY}:
                    call=dict(target=pc,before=machine.snapshot(regs),flags_mask=defined,return_address=int.from_bytes(host.read(regs.rsp,4),'little'),controlled=False);calls.append(call);waiting.append(call);call_entries+=1
                ins=code.at(pc);assert host.read(pc,len(ins.bytes))==bytes(ins.bytes);instructions[f'{pc:08x}']=bytes(ins.bytes).hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                def operand(at,width):
                    at&=NONE
                    if BASE<=at and at+width<=END:assert any(low<=at and at+width<=low+size for low,size in regions),(kind,hex(pc),hex(at),width)
                    if 0x10000<=at<BASE and at+width<=BASE:
                        raw=host.read(at,width);static[(at,width)]=raw
                        if at!=0x4f0414:assert raw==image.read(at,width),(hex(at),raw.hex())
                    reads.append([pc,at,width]);operands.append((at,width))
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4));write_only.add(pc)
                if ins.mnemonic=='pop'or ins.mnemonic.startswith('ret'):operand(regs.rsp&NONE,4)
                wide_before={}
                for operand_index,op in enumerate(ins.operands):
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                    access=op.access
                    if ins.mnemonic=='movhps'and operand_index==0:
                        # Capstone reports m64 destination access as read.
                        # The authenticated 0F17 opcode is the store form.
                        assert op.size==8 and bytes(ins.bytes).startswith(bytes.fromhex('0f17'))
                        corrected_access[pc]=dict(reported_access=op.access,actual_access=2,opcode=bytes(ins.bytes).hex());access=2
                        if any(low<=at and at+8<=low+size for low,size in regions):wide_before[at]=host.read(at,8)
                    if access&1:operand(at,op.size)
                    if access&2:
                        pending.append((at,op.size))
                        (read_modify_write if access&1 else write_only).add(pc)
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status)and os.WSTOPSIG(status)==signal.SIGSEGV:
                    info=ctypes.create_string_buffer(128);host.trace(0x4202,0,ctypes.addressof(info));actual=struct.unpack_from('<Q',info.raw,16)[0];failed_reads=[(at,width)for at,width in operands if at<=actual<at+width];access='read'if failed_reads else'write';at,width=(failed_reads if failed_reads else pending)[-1];fault=[dict(instruction=pc,address=actual,operand_address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status)and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
                for at,width in pending:
                    actual=host.read(at,width);stores.append([pc,at,width,int.from_bytes(actual,'little')])
                    if width==8:
                        assert at in wide_before;wide_stores+=1;wide_changes+=actual!=wide_before[at]
                if ins.mnemonic in ('cmp','add','sub','comiss','ucomiss'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('and','or','xor','test'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic in ('shl','shr'):defined=(NONE&~DEBUG_FLAGS)&~0x810
                elif ins.mnemonic in ('imul','mul'):defined=(NONE&~DEBUG_FLAGS)&~0xd4
            else:raise ValueError(('Original emission graph instruction limit',kind))
            if kind.startswith('fault-'):assert fault,(kind,n)
            blobs.extend([at,raw.hex()]for(at,width),raw in sorted(static.items()))
            rows.append(dict(entry=ENTRY,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,regions=regions,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,fault=fault,trace=trace,expected=machine.snapshot(regs),flags_mask=defined))
    assert {int(pc,16)for pc in instructions}==sites,(len(instructions),len(sites));assert len(corrected_access)==4 and wide_stores>0 and wide_changes>0
    raw=(json.dumps(dict(schema='H2_TU15_WEAPON_MARKER_EMISSION_GRAPH_REFERENCE_V1',original_sha256=pin,scope=__doc__,entries=sorted(ENTRIES),write_only_sites=sorted(write_only),read_modify_write_sites=sorted(read_modify_write),rows=rows),separators=(',',':'))+'\n').encode();packed=gzip.compress(raw,mtime=0);assert len(packed)<=2*1024*1024,len(packed);a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_bytes(packed)
    report=dict(passed=True,cases=len(rows),original_sha256=pin,original_instruction_sites=len(instructions),owner_instruction_sites={hex(e):len(s)for e,s in owners.items()},hardware_instructions=steps,actual_lower_calls=call_entries,actual_lower_returns=call_returns,actual_lower_faults=call_entries-call_returns,read_stops=sum(bool(r['fault'])and r['fault'][0]['access']=='read'for r in rows),write_stops=sum(bool(r['fault'])and r['fault'][0]['access']=='write'for r in rows),alias_cases=sum(r['kind'].startswith('alias-')for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx'for r in rows),controlled_lower_calls=0,supplied_game_services=[],instruction_patches=[],instruction_map=instructions,capstone_access_corrections=corrected_access,wide_store_events=wide_stores,wide_store_events_with_changed_bytes=wide_changes,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_bytes=len(packed),fixture_sha256=hashlib.sha256(packed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',whole_original_emission_and_math_graph_executed=True,native_graph_qualified=False,physical_xbox_proof=False,full_user_goal_complete=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='instruction_map'}))


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Execute unchanged Vista commit/readiness/phase/zero-stage leaves.

All peers, field values and clock samples are explicit test inputs. No live
session, peer construction, network receive, loading or gameplay is asserted.
"""
from pathlib import Path
import argparse
import hashlib
import itertools
import json
import struct
from unicorn import UC_HOOK_CODE, UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_vista_lobby_countdown import SOURCE_SHA, BASE, STOP, OBJECT, STACK, make_machine, read32, word

SESSION = 0x1A00000
OWNER = 0x1A10000
COUNTDOWN = (0x4C94,0x4C98,0x4C9C,0x4CA0,0x4CA4,0x4CA8,0x4CAC,0x4C60)
PHASE = (0x4C68,0x4C64,0x4C6C,0x796C,0x7970,0x7974)
SERVICES = {0x5C25DF: 0,0x5C225B: 1,0x5D76C5: 2,0x5C596E: 3,0x5C96FB: 4,0x5B3CA0: 5}


def put(uc,address,value,size=4):
    uc.mem_write(address,word(value).to_bytes(4,"little")[:size])


def fields(uc,offsets,byte_offsets):
    return [int.from_bytes(uc.mem_read(SESSION+offset,1 if offset in byte_offsets else 4),"little")
            for offset in offsets]


def setup(image,context=(5,3,3,0),countdown=(1,0,0,0,-1,0,0,0),
          phase=(1,0,0,0,0,0),count=0,statuses=(),peers=()):
    uc=make_machine(image)
    uc.mem_map(SESSION,0x8000);uc.mem_map(OWNER,0x1000)
    for offset,value in zip((0x73A0,0x74,0x71C0,0x4C74),context):
        put(uc,SESSION+offset,value,1 if offset==0x4C74 else 4)
    for offset,value in zip(COUNTDOWN,countdown):put(uc,SESSION+offset,value,1 if offset in (0x4C94,0x4C9C) else 4)
    for offset,value in zip(PHASE,phase):put(uc,SESSION+offset,value,1 if offset==0x796C else 4)
    put(uc,SESSION+0x84,count)
    for i,value in enumerate(statuses):put(uc,SESSION+0x110+i*0x10C,value)
    for i,(present,marked) in enumerate(peers):
        put(uc,SESSION+0x71C5+i*0x1C,present,1);put(uc,SESSION+0x71C6+i*0x1C,marked,1)
    put(uc,OBJECT+8,OWNER);put(uc,OWNER+0x24,SESSION)
    put(uc,0x91ABC8,1,1)
    return uc


def execute(uc,address,args,clocks=()):
    esp=STACK+0x8000
    uc.mem_write(esp,struct.pack("<"+"I"*(len(args)+1),STOP,*map(word,args)))
    uc.reg_write(UC_X86_REG_ESP,esp);uc.reg_write(UC_X86_REG_ECX,OBJECT if address==0x5D9C1A else SESSION)
    calls=[];clock_index=0
    def hook(machine,pc,size,_):
        nonlocal clock_index
        if pc==STOP:machine.emu_stop()
        elif pc in SERVICES:
            service=SERVICES[pc]
            s=machine.reg_read(UC_X86_REG_ESP)
            arguments=[]
            if service==3:arguments=[read32(machine,s+4)]
            if service==4:arguments=[read32(machine,s+4+i*4) for i in range(5)]
            calls.append([service,*arguments])
            if service==5:
                if clock_index>=len(clocks):raise RuntimeError("missing explicit clock sample")
                put(machine,0x91ABCC,clocks[clock_index]);clock_index+=1
    uc.hook_add(UC_HOOK_CODE,hook)
    fault=None
    try:uc.emu_start(address,STOP+1,count=20000)
    except UcError as error:fault=str(error)
    if not fault and uc.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError("original leaf did not return")
    return dict(return_al=uc.reg_read(UC_X86_REG_EAX)&255,clock_count=clock_index,calls=calls,fault=fault)


def output(uc,extent):
    return dict(countdown=fields(uc,COUNTDOWN,(0x4C94,0x4C9C)),
                phase=fields(uc,PHASE,(0x796C,)),
                peers=[[int.from_bytes(uc.mem_read(SESSION+0x71C5+i*0x1C+j,1),"little") for j in (0,1)] for i in range(extent)])


def flat_peers(peers):return list(itertools.chain.from_iterable(peers))


def call_words(calls):
    result=[len(calls)]
    for service,*arguments in calls:result += [service,len(arguments),*arguments]
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe",type=Path,default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--fixture",type=Path,default=Path("tests/fixtures/vista_session_commit_original.tsv"))
    parser.add_argument("--report",type=Path,default=Path("analysis/frontend/vista-session-flow-audit/session-commit-original.json"))
    a=parser.parse_args();image=a.exe.read_bytes()
    assert hashlib.sha256(image).hexdigest()==SOURCE_SHA
    records=[];lines=["# H2V_SESSION_COMMIT_ORIGINAL_V1","# kind label word_count little_endian_u32_semantic_words_hex"]
    def retain(kind,label,inputs,result,words):
        records.append(dict(kind=kind,label=label,inputs=inputs,result=result))
        lines.append("\t".join([kind,label,str(len(words)),*(f"{word(v):08x}" for v in words)]))
    contexts=[(5,3,3,0),(5,3,3,1),(6,0x12345678,0x12345678,0x80),(8,3,3,0),(4,3,2,0)]
    old_states=[(0,-1,0,0,-1,0x11223344,0x55667788,0),
                (1,10,0,0,-1,0,0,0xFFFFFFFF),
                (1,3,1,1,10,0x11223344,0x55667788,0xFFFFFFFE),
                (0,0,0x80,2,1,0xFFFFFFFF,0,17),
                (1,-0x80000000,0xFF,0x80000000,0x7FFFFFFF,7,8,9)]
    identity=(0x89ABCDEF,0x12345678)
    for context,old,sender_local,running,stage,flag,reason in itertools.product(
            contexts,old_states,(False,True),(0,1,0x80),(-0x80000000,-1,0,1,3,10,0x7FFFFFFF),(0,1,0x80),(0,1,2,0x80000000)):
        sender=context[1] if sender_local else word(context[1]+1)
        request=(running,stage,flag,reason,1,*identity)
        uc=setup(image,context,old)
        put(uc,OBJECT+0x2000,identity[0]);put(uc,OBJECT+0x2004,identity[1])
        result=execute(uc,0x5C6853,(sender,running,stage,flag,reason,OBJECT+0x2000))
        assert not result["fault"]
        result.update(output(uc,0))
        label=f"commit_{len(records):05d}"
        retain("C",label,dict(context=context,sender=sender,request=request,before=old),result,
               [*context,sender,*request,*old,*result["countdown"],result["return_al"]])
    # Full 5C96FB local wrapper, not a hook returning true.
    for context,old,running,stage,flag,reason in itertools.product(contexts[:4],old_states,(0,1),(0,1,10),(0,1),(0,1,2)):
        request=(running,stage,flag,reason,1,*identity)
        uc=setup(image,context,old)
        put(uc,OBJECT+0x2000,identity[0]);put(uc,OBJECT+0x2004,identity[1])
        result=execute(uc,0x5C96FB,(running,stage,flag,reason,OBJECT+0x2000))
        assert not result["fault"]
        result.update(output(uc,0))
        retain("L",f"local_{len(records):05d}",dict(context=context,request=request,before=old),result,
               [*context,*request,*old,*result["countdown"],result["return_al"]])
    # Null is safe unless the literal reason-1 retention branch consumes it.
    fault_controls=[]
    for running,old_running,reason in itertools.product((0,1),(0,1),(0,1,2)):
        old=(old_running,3,0,0,-1,0,0,0);request=(running,10,0,reason,0,0,0)
        uc=setup(image,countdown=old);result=execute(uc,0x5C6853,(3,running,10,0,reason,0))
        if result["fault"]:
            fault_controls.append(dict(running=running,old_running=old_running,reason=reason,result=result))
        else:
            result.update(output(uc,0))
            retain("C",f"null_safe_{len(records):05d}",dict(context=(5,3,3,0),sender=3,request=request,before=old),result,
                   [5,3,3,0,3,*request,*old,*result["countdown"],result["return_al"]])
    patterns=[[],[0],[1],[2],[3],[4],[5],[6],[-1],[-0x80000000],[0x7FFFFFFF],
              [3,4,6,-1],[0,1,2,5],[3]*16+[5],[5]+[3]*15+[0]]
    for statuses in patterns:
        count=len(statuses);uc=setup(image,count=count,statuses=statuses)
        result=execute(uc,0x5D76C5,(SESSION,OBJECT+0x2000));assert not result["fault"]
        mask=read32(uc,OBJECT+0x2000)
        retain("R",f"ready_{len(records):05d}",dict(count=count,statuses=statuses),result,
               [count,len(statuses),*statuses,result["return_al"],mask])
    for count in (-1,-0x80000000):
        uc=setup(image,count=count);result=execute(uc,0x5D76C5,(SESSION,OBJECT+0x2000))
        retain("R",f"negative_count_{len(records):05d}",dict(count=count,statuses=[]),result,
               [count,0,result["return_al"],read32(uc,OBJECT+0x2000)])
    for count,peers in [(0,[]),(-1,[]),(1,[(0,0x80)]),(1,[(1,0x80)]),
                        (3,[(0,7),(0x80,9),(1,0)]),(17,[(i%2,0x80+i) for i in range(17)])]:
        for requested,phase in itertools.product((1,2,0xFFFFFFFF),((1,0,0,0,0,0),(2,0xFFFFFFFF,7,0x80,9,10))):
            clocks=(0xFFFFFFF0,0x17);revision=0xFFFFFFFF
            old=(1,0,0,0,-1,0,0,revision)
            uc=setup(image,countdown=old,phase=phase,count=count,peers=peers)
            result=execute(uc,0x5C596E,(requested,),clocks);assert not result["fault"]
            result.update(output(uc,len(peers)))
            retain("P",f"phase_{len(records):05d}",dict(requested=requested,count=count,clocks=clocks,before=phase,revision=revision,peers=peers),result,
                [requested,count,*clocks,*phase,revision,len(peers),*flat_peers(peers),
                 *result["phase"],result["countdown"][-1],*flat_peers(result["peers"]),result["clock_count"]])
    for running,stage,preflight,statuses,phase,local_flag in itertools.product((0,1),(0,1,-1),
            (0,1,2,0xFFFFFFFF),([3],[5],[3,4],[3,0],[]),(1,2),(0,1)):
        context=(5,3,3,local_flag);old=(running,stage,0,1,10,0x11223344,0x55667788,0xFFFFFFFF)
        pbefore=(phase,0xFFFFFFFF,7,0x80,8,9);count=len(statuses);peers=[(1 if i%2==0 else 0,0x80+i) for i in range(count)]
        clocks=(0xFFFFFFF0,0x17)
        uc=setup(image,context,old,pbefore,count,statuses,peers);put(uc,SESSION+0x4C90,preflight)
        result=execute(uc,0x5D9C1A,(),clocks);assert not result["fault"]
        result.update(output(uc,count))
        action=1 if any(call[0]==3 for call in result["calls"]) else 2 if any(call[0]==4 for call in result["calls"]) else 0
        retain("Z",f"zero_{len(records):05d}",dict(context=context,preflight=preflight,count=count,clocks=clocks,countdown=old,phase=pbefore,statuses=statuses,peers=peers),result,
               [*context,preflight,count,*clocks,*old,*pbefore,count,*statuses,*flat_peers(peers),
                *result["countdown"],*result["phase"],*flat_peers(result["peers"]),action,*call_words(result["calls"])])
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text("\n".join(lines)+"\n")
    windows={name:(begin,end,hashlib.sha256(image[begin-BASE:end-BASE]).hexdigest()) for name,begin,end in (
        ("commit",0x5C6853,0x5C6998),("local_wrapper",0x5C96FB,0x5C9833),
        ("readiness",0x5D76C5,0x5D7738),("peer_view",0x5C20C8,0x5C20EB),
        ("phase_write",0x5C596E,0x5C5A08),("zero",0x5D9C1A,0x5D9C6B))}
    report=dict(version="H2V_SESSION_COMMIT_ORIGINAL_V1",source_sha256=SOURCE_SHA,
                tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
                counts={kind:sum(r["kind"]==kind for r in records) for kind in "CLR PZ".replace(" ","")},
                instruction_windows=windows,records=records,fault_controls=fault_controls,
                boundary="Unchanged PC leaves execute against explicit field and peer inputs; actual clock override instructions consume supplied per-call samples. No peer construction, network receive/send, loading or gameplay activation. Fault controls are not serialized as false predicate outputs.")
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({"counts":report["counts"],"fault_controls":len(fault_controls),"fixture_sha256":report["fixture_sha256"]}))


if __name__=="__main__":main()

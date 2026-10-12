#!/usr/bin/env python3
"""Bounded unchanged-PC oracle for normal multiplayer readiness and publication.

External map-file/progress/Windows conversion results are declared inputs.
Whole selector/local writer instructions execute; no live session is asserted.
"""
from pathlib import Path
import argparse
import hashlib
import itertools
import json
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_MXCSR, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG
from oracle_vista_lobby_countdown import SOURCE_SHA, BASE, STOP, OBJECT, STACK, make_machine, read32, word

SP, SESSION, OPTIONS = STACK+0x8000,0x1A00000,0x1A10000
CONVERT,FLOAT_RETURN=STOP+0x100,STOP+0x200
SERVICES={0x5AD660:0,0x5AD736:1,0x5AD423:2,0x5C2321:3,0x5C23A7:4,
          0x4497EA:5,0x449801:6,0x68B052:7,0x44B633:8,CONVERT:9,0x4317D2:10,0x44BABA:11}


def put(u,address,value,size=4):u.mem_write(address,word(value).to_bytes(4,"little")[:size])
def path(text):
    units=list(struct.unpack("<"+"H"*(len(text.encode("utf-16le"))//2),text.encode("utf-16le")))
    assert len(units)<260
    return units+[0]*(260-len(units))
def path_read(u,address):return list(struct.unpack("<260H",bytes(u.mem_read(address,520))))
def ret(u,value=0,pop=0):
    sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,word(value))
    u.reg_write(UC_X86_REG_ESP,sp+4+pop);u.reg_write(UC_X86_REG_EIP,read32(u,sp))
def machine(image):
    u=make_machine(image);u.mem_map(SESSION,0x8000);u.mem_map(OPTIONS,0x2000)
    put(u,SP,STOP);u.reg_write(UC_X86_REG_ESP,SP)
    u.reg_write(UC_X86_REG_FPCW,0x027F);u.reg_write(UC_X86_REG_FPSW,0)
    u.reg_write(UC_X86_REG_FPTAG,0xFFFF);u.reg_write(UC_X86_REG_MXCSR,0x1F80)
    return u
def run(u,start,stop=STOP):
    u.emu_start(start,stop+1,count=100000)
    assert u.reg_read(UC_X86_REG_EIP)==stop
def call_words(calls):
    return [len(calls),*itertools.chain.from_iterable([service,len(args),*args] for service,args in calls)]


def selector(image,label,*,mxcsr=0x1F80,lifecycle=1,present=True,state=5,
             selected_type=1,selected_id=17,selected_text="levels\\declared\\declared",
             loaded=False,loaded_type=1,loaded_id=17,loaded_text=None,
             query=2,fraction=0x3F000000,before=(0x12345678,0x87654321,0x80,0x76543210)):
    u=machine(image);calls=[];u.reg_write(UC_X86_REG_MXCSR,mxcsr)
    put(u,0x820FC0,1,1);put(u,0x820FC4,lifecycle);put(u,0x820FE8,SESSION)
    put(u,SESSION+0x73A0,state if present else 0)
    put(u,SESSION+0x4CB8,selected_type)
    u.mem_write(SESSION+0x4CBC,struct.pack("<260H",*path(selected_text)))
    put(u,SESSION+0x4DD8,selected_id)
    put(u,0x882D3C,OPTIONS);put(u,OPTIONS+0x1190,int(loaded),1)
    put(u,OPTIONS+8+0x80,loaded_type);put(u,OPTIONS+8+0x84,loaded_id)
    u.mem_write(OPTIONS+8+0x88,struct.pack("<260H",*path(selected_text if loaded_text is None else loaded_text)))
    selected_path=path_read(u,SESSION+0x4CBC);loaded_path=path_read(u,OPTIONS+8+0x88)
    for address,value in zip((0x91A620,0x91A624,0x91A588,0x822424),before):put(u,address,value,1 if address==0x91A588 else 4)
    put(u,0x86D7D8,CONVERT);put(u,OBJECT+0x3000,fraction)
    u.mem_write(FLOAT_RETURN,b"\xD9\x05"+struct.pack("<I",OBJECT+0x3000)+b"\xC3")
    raw_calls=[]
    def hook(m,pc,size,_):
        if pc==STOP:m.emu_stop();return
        if pc not in SERVICES:return
        service=SERVICES[pc];s=m.reg_read(UC_X86_REG_ESP);args=[]
        if service==7:args=[read32(m,s+12)]
        elif service==9:args=[read32(m,s+4),read32(m,s+8),read32(m,s+16),read32(m,s+24)]
        elif service==10:args=[read32(m,s+4)]
        elif service==11:args=[read32(m,s+8)]
        calls.append([service,args]);raw_calls.append(dict(pc=f"{pc:08X}",esp=f"{s:08X}",arguments=args))
        if service==8:ret(m,query)
        elif service==9:
            assert args==[65001,0,0xFFFFFFFF,0x104]
            raw=(selected_text+"\0").encode("utf-8");m.mem_write(read32(m,s+20),raw)
            ret(m,len(raw),32)
        elif service==10:m.reg_write(UC_X86_REG_EIP,FLOAT_RETURN)
        elif service==11:ret(m)
    u.hook_add(UC_HOOK_CODE,hook);run(u,0x5B2920)
    after=[read32(u,0x91A620),read32(u,0x91A624),int.from_bytes(u.mem_read(0x91A588,1),"little"),read32(u,0x822424)]
    # The source selector calls the admitted predicate only for a nonzero
    # session pointer state. This represents actual 5AD736 failure as nullopt.
    words=[mxcsr,lifecycle,int(present),state,selected_type,selected_id,*selected_path,
           int(loaded),1,loaded_type,loaded_id,*loaded_path,query,fraction,*before,*after,*call_words(calls)]
    return dict(kind="S",label=label,words=list(map(word,words)),calls=raw_calls)


def comparator(image,label,left,right):
    u=machine(image)
    u.mem_write(OBJECT+0x1000,struct.pack("<260H",*left));u.mem_write(OBJECT+0x2000,struct.pack("<260H",*right))
    u.mem_write(SP,struct.pack("<4I",STOP,OBJECT+0x1000,OBJECT+0x2000,260))
    u.hook_add(UC_HOOK_CODE,lambda m,pc,size,_:m.emu_stop() if pc==STOP else None)
    run(u,0x68B052);value=u.reg_read(UC_X86_REG_EAX)
    return dict(kind="P",label=label,words=[*left,*right,value])


def publication(image,label,state,index,revision,change,status,fraction):
    u=machine(image);count=3
    put(u,SESSION+0x73A0,state);put(u,SESSION+0x71C0,index);put(u,SESSION+0x74,2-index)
    put(u,SESSION+0x70,revision);put(u,SESSION+0x7930,change)
    payload=bytearray((i*37+11)&255 for i in range(0xC8))
    struct.pack_into("<2I",payload,0x60,word(status),word(fraction))
    before=[]
    for i in range(count):
        data=bytes(((i+3)*19+j*7)&255 for j in range(0xCC));before+=list(struct.unpack("<51I",data))
        u.mem_write(SESSION+i*0x10C+0xAC,data)
    u.mem_write(OBJECT+0x2000,bytes(payload));put(u,SP+4,OBJECT+0x2000);u.reg_write(UC_X86_REG_ECX,SESSION)
    u.hook_add(UC_HOOK_CODE,lambda m,pc,size,_:m.emu_stop() if pc==STOP else None);run(u,0x5C83F3)
    after=[]
    for i in range(count):after+=list(struct.unpack("<51I",bytes(u.mem_read(SESSION+i*0x10C+0xAC,0xCC))))
    result=[read32(u,SESSION+0x70),read32(u,SESSION+0x7930),u.reg_read(UC_X86_REG_EAX)&255]
    return dict(kind="W",label=label,words=[state,index,count,revision,change,*struct.unpack("<50I",payload),*before,*after,*result])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe",type=Path,default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--fixture",type=Path,default=Path("tests/fixtures/vista_session_readiness_original.tsv"))
    parser.add_argument("--report",type=Path,default=Path("analysis/frontend/vista-session-flow-audit/local-readiness-original.json"))
    args=parser.parse_args();image=args.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SOURCE_SHA
    rows=[]
    for mxcsr,typ,query,fraction in itertools.product((0x1F80,0x3F80,0x5F80,0x7F80,0x9FC0),
            (1,0xFFFFFFFF),(0,1,2,3,4,-1,-0x80000000),
            (0,0x3E947AE1,0x3F000000,0x3F800000,0xBF800000,0x7F800000,0x7FC12345,1)):
        rows.append(selector(image,f"query_{len(rows):04d}",mxcsr=mxcsr,selected_type=typ,query=query,fraction=fraction))
    for state,lifecycle,present in itertools.product((0,1,2,3,4,5,6,7,8,9,10,0xFFFFFFFF),(0,1,2,3,4,5,0xFFFFFFFF),(False,True)):
        rows.append(selector(image,f"admission_{len(rows):04d}",state=state,lifecycle=lifecycle,present=present))
    for typ,ident,text in ((1,17,None),(2,17,None),(1,18,None),(1,17,"LEVELS\\declared\\declared"),
                           (1,17,"levels\\other\\other"),(1,17,"levels\\declared\\déclared")):
        rows.append(selector(image,f"loaded_{len(rows):04d}",loaded=True,loaded_type=typ,loaded_id=ident,loaded_text=text))
    a=path("abc");b=path("abc")
    rows.append(comparator(image,"same",a,b))
    for index in (0,1,2,3,4,259):
        left=[0x61]*260;right=left.copy();right[index]=0x41
        rows.append(comparator(image,f"different_{index}",left,right))
    for index in (0,1,3,259):
        left=[0x61]*260;right=left.copy();left[index]=right[index]=0
        if index<259:right[index+1]=0x42
        rows.append(comparator(image,f"nul_{index}",left,right))
    for first,second in ((0xFFFF,0x8000),(0x8000,0xFFFF),(0xD800,0xD801),(0xE9,0x65)):
        rows.append(comparator(image,f"codeunit_{first:04x}_{second:04x}",[first]+[0]*259,[second]+[0]*259))
    for state,index,revision,change,status,fraction in itertools.product((5,6,7,8),(0,2),
            (0,0xFFFFFFFF),(0x12345678,0xFFFFFFFF),(0,3,4),(0,100)):
        rows.append(publication(image,f"publish_{len(rows):04d}",state,index,revision,change,status,fraction))
    lines=["# H2V_SESSION_READINESS_ORIGINAL_V1","# kind label word_count little_endian_u32_semantic_words_hex"]
    for row in rows:lines.append("\t".join([row["kind"],row["label"],str(len(row["words"])),*(f"{v:08x}" for v in row["words"])]))
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text("\n".join(lines)+"\n")
    report=dict(version="H2V_SESSION_READINESS_ORIGINAL_V1",source_sha256=SOURCE_SHA,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        counts={kind:sum(row["kind"]==kind for row in rows) for kind in "SPW"},records=rows,
        supplied_x87_context={"control":0x027F,"status":0,"tag":0xFFFF},
        supplied_mxcsr_contexts=[0x1F80,0x3F80,0x5F80,0x7F80,0x9FC0],
        boundary="Whole unchanged normal-map selector/getters/comparator and local record writer execute independently. Map query, post-spill progress and Windows conversion are supplied external results; no actual loader/network/session/lifecycle activation. Five declared SSE contexts; ordinary thread setup unproved.")
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(dict(counts=report["counts"],fixture_sha256=report["fixture_sha256"])))


if __name__=="__main__":main()

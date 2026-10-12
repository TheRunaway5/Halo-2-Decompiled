#!/usr/bin/env python3
"""Execute the unchanged supplied PC announcer queue with explicit playback.

No installer/game/audio execution. Only memset and the external playback request
are service boundaries; the original 47C0DB tick conversion executes unchanged.
Outputs retain all five12-byte slots and the count, including unused/stale bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

BASE=0x400000
STOP=0x1800000
OBJECT=0x1810000
STACK=0x1900000
QUEUE=0x8D8578
COUNT=0x8D8614
SOURCE_SHA="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
METHODS={"I":0x4D0E02,"E":0x4D0E75,"C":0x4D0E62,"U":0x4D0FF4}

def u32(n):return n&0xffffffff
def slot(sound,voice,active,retained,delay):
    return struct.pack("<IIBBh",u32(sound),u32(voice),active,retained,delay)
def state(count,delay=0,sound=0xF42F39C9,active=0,voice=0x12340007):
    return b"".join(slot(sound+i,voice+i,active if i==0 else (i&1),0xa0+i,
                         delay if i==0 else i) for i in range(5))+struct.pack("<I",u32(count))

class Oracle:
    def __init__(self,image):
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32)
        self.uc.mem_map(BASE,(len(image)+0xfff)&~0xfff)
        self.uc.mem_write(BASE,image)
        self.uc.mem_map(STOP,0x1000)
        self.uc.mem_map(OBJECT,0x1000)
        self.uc.mem_map(STACK,0x10000)
        self.uc.mem_write(0x8C06E4,struct.pack("<I",OBJECT))
        self.uc.hook_add(UC_HOOK_CODE,self.hook)
        self.calls=[]
        self.handle=0xffffffff
        self.visits=0
    def read32(self,address):return struct.unpack("<I",self.uc.mem_read(address,4))[0]
    def bank(self):return bytes(self.uc.mem_read(QUEUE,60))+bytes(self.uc.mem_read(COUNT,4))
    def ret(self,value):
        esp=self.uc.reg_read(UC_X86_REG_ESP)
        self.uc.reg_write(UC_X86_REG_EAX,u32(value))
        self.uc.reg_write(UC_X86_REG_EIP,self.read32(esp))
        self.uc.reg_write(UC_X86_REG_ESP,esp+4)
    def hook(self,uc,address,size,_):
        self.visits+=1
        if address==STOP:uc.emu_stop()
        elif address==0x687BA9:
            esp=uc.reg_read(UC_X86_REG_ESP)
            dest,value,length=(self.read32(esp+i) for i in (4,8,12))
            if (dest,value,length)!=(QUEUE,0,0xa0):raise RuntimeError("unexpected original memset")
            uc.mem_write(dest,bytes(length));self.ret(dest)
        elif address==0x4883B6:
            esp=uc.reg_read(UC_X86_REG_ESP)
            self.calls.append({"sound":self.read32(esp+4),"scale_word":self.read32(esp+8),
                               "state_at_call":self.bank().hex()})
            self.ret(self.handle)
    def execute(self,kind,label,a,b,before):
        if len(before)!=64:raise RuntimeError("invalid queue input bank")
        self.uc.mem_write(QUEUE,before[:60]);self.uc.mem_write(COUNT,before[60:])
        self.calls=[];self.handle=a
        esp=STACK+0x8000
        args={"I":[],"E":[b],"C":[0x87654321,a],"U":[]}[kind]
        self.uc.mem_write(esp,struct.pack("<"+"I"*(1+len(args)),STOP,*map(u32,args)))
        self.uc.reg_write(UC_X86_REG_ESP,esp)
        self.uc.reg_write(UC_X86_REG_ECX,u32(a))
        if kind=="I":self.uc.mem_write(OBJECT+2,struct.pack("<H",a&0xffff))
        self.uc.emu_start(METHODS[kind],STOP+1,count=10000)
        if self.uc.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError("original queue did not return")
        return {"kind":kind,"label":label,"a":u32(a),"b":u32(b),"before":before.hex(),
                "after":self.bank().hex(),"calls":list(self.calls)}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe",type=Path,default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--fixture",type=Path,default=Path("tests/fixtures/vista_announcer_queue_original.tsv"))
    parser.add_argument("--report",type=Path,default=Path("analysis/frontend/vista-session-flow-audit/announcer-queue-original.json"))
    args=parser.parse_args();image=args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest()!=SOURCE_SHA:raise SystemExit("unexpected Vista executable")
    oracle=Oracle(image);rows=[]
    def add(kind,a,b,before,label=None):
        row=oracle.execute(kind,label or f"case_{len(rows):04d}",a,b,before);rows.append(row)
        return bytes.fromhex(row["after"])
    for rate in (-32768,-1,0,1,30,60,32767):add("I",rate,0,bytes([0x5a])*64)
    for count in (0,1,4,5,6,0xffffffff):
        for sound in (0xffffffff,0,0xF42F39C9,0xF35D38F7):
            for delay in (-0x80000000,-65537,-32768,-1,0,1,32767,65536,0x7fffffff):
                add("E",sound,delay,state(count))
    for count in (0,1,5):
        for active in (0,1,0x7f,0xff):
            for voice in (0xffffffff,0,0x12340007,0x12340008):
                add("C",voice,0,state(count,active=active))
    for count in range(6):
        for delay in (-32768,-2,-1,0,1,2,32767):
            for active in (0,1,0x7f,0xff):
                for sound in (0xffffffff,0xF42F39C9,0xF35D38F7):
                    for handle in (0xffffffff,0,0xABCD0009):
                        add("U",handle,0,state(count,delay,sound,active))
    retained=bytes([0x66])*64;seq=0
    def sequence(kind,a=0,b=0):
        nonlocal retained,seq
        retained=add(kind,a,b,retained,f"sequential_{seq:03d}");seq+=1
    sequence("I",30);sequence("E",0xF42F39C9,0);sequence("E",0xF35D38F7,2)
    for _ in range(62):sequence("U",0xABCD0009)
    sequence("U",0xABCD0009);sequence("U",0xABCD0009)
    sequence("C",0xABCD0008);sequence("U",0xABCD0009)
    sequence("C",0xABCD0009);sequence("U",0xABCD0009)
    for _ in range(3):sequence("U",0x12340001)
    sequence("U",0x12340001);sequence("C",0x12340001);sequence("U",0x12340001)
    lines=["# H2V_ANNOUNCER_QUEUE_ORIGINAL_V1",
           "# kind label a_word b_word before64hex after64hex call_count cue_word scale_word state_at_call64hex (- when absent)"]
    for r in rows:
        if len(r["calls"])>1:raise RuntimeError("unexpected multiple playback requests")
        call=r["calls"][0] if r["calls"] else None
        fields=[r["kind"],r["label"],f'{r["a"]:08x}',f'{r["b"]:08x}',r["before"],r["after"],str(len(r["calls"]))]
        fields+=([f'{call["sound"]:08x}',f'{call["scale_word"]:08x}',call["state_at_call"]] if call else ["-","-","-"])
        lines.append("\t".join(fields))
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text("\n".join(lines)+"\n")
    ranges={"initialize":(0x4d0e02,0x4d0e62),"completed":(0x4d0e62,0x4d0e75),
            "enqueue":(0x4d0e75,0x4d0ea6),"update":(0x4d0ff4,0x4d108f),
            "tick_conversion":(0x47c0db,0x47c101),"playback_request":(0x4883b6,0x488404),
            "callback_table":(0x7bb538,0x7bb558)}
    report={"version":"H2V_ANNOUNCER_QUEUE_ORIGINAL_V1","source":str(args.exe.resolve()),
            "source_sha256":SOURCE_SHA,"tool_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "fixture_sha256":hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
            "case_count":len(rows),"sequential_count":seq,"instruction_visits":oracle.visits,
            "counts":{k:sum(r["kind"]==k for r in rows) for k in METHODS},
            "instruction_ranges":{k:{"begin":f"{a:08x}","end_exclusive":f"{z:08x}",
                "sha256":hashlib.sha256(image[a-BASE:z-BASE]).hexdigest()} for k,(a,z) in ranges.items()},
            "boundary":{"unchanged_pc_instructions":True,"playback":"4883B6 is an explicit external voice-result boundary; cue/scale/pre-call queue bytes are captured, no audio is played.",
              "memset":"687BA9 is standard memset boundary for exact observed (8D8578,0,A0); original caller instructions unchanged.",
              "tick_rate":"Explicit signed16 source game-time field; unchanged47C0DB conversion executes.",
              "state":"Five literal12-byte slots plus32-bit count. Hash bank reset executes but is outside native queue comparison.",
              "not_proved":["event response/audience implementation","hash suppression implementation","actual tick cadence","actual voice completion timing","audio decode/playback","full lifecycle/return flow"]},
            "cases":rows}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({k:report[k] for k in ("case_count","sequential_count","counts","fixture_sha256","instruction_visits")}))

if __name__=="__main__":main()

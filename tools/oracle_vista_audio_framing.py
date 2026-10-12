#!/usr/bin/env python3
"""Unchanged PC serialized-media parser/packet feeder; codec callbacks explicit.

This executes no installer, game, COM decoder, audio device or native build.
The codec accepts supplied formats and packet submissions without decoding them.
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
CODEC=0x1860000
VTABLE=0x1861000
PAYLOAD=0x1870000
STACK=0x1900000
SOURCE_SHA="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
SOUNDS=[("round_over",0x075EDE00,22387), ("game_over",0x07692E00,26846),
        ("slayer",0x07BFBC00,17928)]

class Oracle:
    def __init__(self,image):
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32)
        self.uc.mem_map(BASE,(len(image)+4095)&~4095)
        self.uc.mem_write(BASE,image)
        self.uc.mem_map(STOP,0x1000)
        self.uc.mem_map(OBJECT,0x42000)
        self.uc.mem_map(CODEC,0x2000)
        self.uc.mem_map(PAYLOAD,0x20000)
        self.uc.mem_map(STACK,0x10000)
        self.uc.mem_write(CODEC,struct.pack("<I",VTABLE))
        for off in (0x10,0x1c,0x20,0x24,0x40,0x44,0x50,0x54):
            self.uc.mem_write(VTABLE+off,struct.pack("<I",STOP+0x100+off))
        self.uc.hook_add(UC_HOOK_CODE,self.hook)
        self.visits=0
    def read32(self,p):return struct.unpack("<I",self.uc.mem_read(p,4))[0]
    def write32(self,p,n):self.uc.mem_write(p,struct.pack("<I",n&0xffffffff))
    def ret(self,n=0,pop=0):
        sp=self.uc.reg_read(UC_X86_REG_ESP)
        self.uc.reg_write(UC_X86_REG_EAX,n&0xffffffff)
        self.uc.reg_write(UC_X86_REG_EIP,self.read32(sp))
        self.uc.reg_write(UC_X86_REG_ESP,sp+4+pop)
    def hook(self,uc,address,size,_):
        self.visits+=1
        sp=uc.reg_read(UC_X86_REG_ESP)
        arg=lambda i:self.read32(sp+4*i)
        if address==STOP:uc.emu_stop()
        elif address==0x687BA9:
            dest,value,length=arg(1),arg(2),arg(3)
            uc.mem_write(dest,bytes([value&255])*length);self.ret(dest)
        elif address==0x687E18:
            dest,source,length=arg(1),arg(2),arg(3)
            uc.mem_write(dest,bytes(uc.mem_read(source,length)));self.ret(dest)
        elif address==0x6875D3:self.ret() # unchanged caller stack cookie boundary
        elif address==0x40E8B6:
            self.ret(int(bytes(uc.mem_read(arg(1),16))==bytes(uc.mem_read(arg(2),16))))
        elif STOP+0x100<=address<STOP+0x200:
            off=address-STOP-0x100
            if arg(1)!=CODEC:raise RuntimeError("unexpected codec object")
            if off==0x20: # IMediaObject::SetInputType(this,0,media,0)
                media=arg(3);raw=bytearray(uc.mem_read(media,72))
                fmt=self.read32(media+68);length=self.read32(media+64)
                raw[68:72]=struct.pack("<I",64) # normalize supplied format pointer
                self.input_media.append({"header":raw.hex(),
                    "format":bytes(uc.mem_read(fmt,length)).hex()})
                self.ret(pop=16)
            elif off==0x1c: # GetOutputType: explicit compatible PCM output
                raw=bytearray(72)
                raw[44:60]=bytes(uc.mem_read(0x7F3DD0,16))
                raw[64:72]=struct.pack("<II",18,CODEC+0x100)
                uc.mem_write(arg(4),bytes(raw));self.ret(pop=16)
            elif off in (0x10,0x50):
                self.write32(arg(3),0 if off==0x10 else 1);self.ret(pop=12)
            elif off==0x54: # ProcessInput; record actual pointer/count only
                packet=arg(3);p=self.read32(packet+4);n=self.read32(packet+8)
                self.packets.append({"offset":p-PAYLOAD,"size":n,
                    "sha256":hashlib.sha256(bytes(uc.mem_read(p,n))).hexdigest()})
                self.ret(pop=32)
            elif off==0x24:self.ret(pop=16)
            elif off==0x40:self.ret(pop=4)
            elif off==0x44:self.ret(pop=8)
            else:raise RuntimeError("unexpected codec callback")
    def call(self,address,args=()):
        sp=STACK+0x8000
        self.uc.mem_write(sp,struct.pack("<"+"I"*(1+len(args)),STOP,*args))
        self.uc.reg_write(UC_X86_REG_ESP,sp)
        self.uc.reg_write(UC_X86_REG_ECX,OBJECT)
        self.uc.emu_start(address,STOP+1,count=100000)
        if self.uc.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError("original did not return")
    def run(self,label,data):
        if len(data)>0x20000:raise RuntimeError("fixture too large")
        self.uc.mem_write(OBJECT,bytes(0x4019c))
        self.write32(OBJECT+8,CODEC)
        self.uc.mem_write(PAYLOAD,data)
        self.input_media=[];self.packets=[]
        self.call(0x4F7404,(PAYLOAD,len(data)))
        accepted=bool(self.read32(OBJECT))
        stalled=False
        if accepted:
            for _ in range(len(data)+2):
                before=self.read32(OBJECT+0x40058)
                self.call(0x4F7355)
                after=self.read32(OBJECT+0x40058)
                if before==after:
                    stalled=before<len(data)-64-self.read32(OBJECT+0x40098)
                    break
            else:raise RuntimeError("packet feeder exceeded bound")
        return {"label":label,"input_hex":data.hex(),"accepted":accepted,
                "stalled":stalled,"input_media":self.input_media,"packets":self.packets}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--exe",type=Path,default=Path("Halo 2 Vista installer/halo2.exe"))
    p.add_argument("--shared",type=Path,default=Path("Halo 2 Vista installer/maps/shared.map"))
    p.add_argument("--fixture",type=Path,default=Path("tests/fixtures/vista_audio_framing_original.tsv"))
    p.add_argument("--report",type=Path,default=Path("analysis/frontend/vista-session-flow-audit/audio-framing-original.json"))
    a=p.parse_args();image=a.exe.read_bytes()
    if hashlib.sha256(image).hexdigest()!=SOURCE_SHA:raise SystemExit("unexpected PC image")
    oracle=Oracle(image);rows=[];actual=[]
    with a.shared.open("rb") as f:
        for name,offset,length in SOUNDS:
            f.seek(offset);data=f.read(length)
            if len(data)!=length:raise RuntimeError("truncated shared sound chunk")
            row=oracle.run(name,data);rows.append(row)
            actual.append({"name":name,"offset":offset,"length":length,
                "sha256":hashlib.sha256(data).hexdigest(),
                "header_hex":data[:64].hex(),"format_hex":data[64:92].hex(),
                "packet_count":len(row["packets"])})
    base=bytes.fromhex(rows[1]["input_hex"])
    def add(label,data):rows.append(oracle.run(label,data))
    for n in (0,1,63,64,65,81,91,92,93,100):add(f"truncate_{n}",base[:n])
    for n in (0,1,17,18,19,255,256,257,0xffffffff):
        data=bytearray(base[:64]+bytes(256))
        struct.pack_into("<I",data,60,n)
        if 18<=n<=256:struct.pack_into("<H",data,80,n-18)
        add(f"format_length_{n}",bytes(data))
    for n in (0,1,9,11,238,65535):
        data=bytearray(base);struct.pack_into("<H",data,80,n);add(f"extra_length_{n}",bytes(data))
    for n in (0,1,2,4458,4459,4460,0xffffffff):
        data=bytearray(base[:92]+base[92:107])
        struct.pack_into("<I",data,40,n);add(f"packet_size_{n}",bytes(data))
    for at in (0,16,44):
        data=bytearray(base[:93]);data[at]^=1;add(f"codec_owned_guid_{at}",bytes(data))
    a.fixture.parent.mkdir(parents=True,exist_ok=True)
    lines=["# H2V_AUDIO_FRAMING_ORIGINAL_V1",
        "# label input_hex accepted stalled normalized_input_media72_hex format_hex packets(offset:size:sha256,...) "]
    for r in rows:
        media=r["input_media"]
        if len(media)>1:raise RuntimeError("unexpected multiple format calls")
        lines.append("\t".join((r["label"],r["input_hex"] or "-",str(int(r["accepted"])),
            str(int(r["stalled"])),media[0]["header"] if media else "-",
            media[0]["format"] if media else "-",
            ",".join(f"{q['offset']}:{q['size']}:{q['sha256']}" for q in r["packets"]) or "-")))
    a.fixture.write_text("\n".join(lines)+"\n")
    report={"source":str(a.exe),"source_sha256":SOURCE_SHA,
        "shared":str(a.shared),"shared_sha256":hashlib.sha256(a.shared.read_bytes()).hexdigest(),
        "fixture":str(a.fixture),"fixture_sha256":hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        "generator_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "original_instruction_visits":oracle.visits,"cases":len(rows),"actual_chunks":actual,
        "source_ranges":[{"start":hex(lo),"end_exclusive":hex(hi),
          "sha256":hashlib.sha256(image[lo-BASE:hi-BASE]).hexdigest()}
          for lo,hi in ((0x4F71D7,0x4F7355),(0x4F7355,0x4F7404),(0x4F7404,0x4F764F),
                        (0x49DB76,0x49DE7B),(0x49DF56,0x49E020))],
        "codec_callbacks":"Accept input/output media and packets with HRESULT 0; supplied PCM output metadata; no codec bytes decoded",
        "boundaries":["Original generic parser delegates GUID/codec validation to external codec; mutated GUIDs can be structurally accepted",
          "Packet size zero admits the format but stalls with unread payload; portable parser rejects this unsupported nonprogress state",
          "No PCM, filtering, audio-device, mixer, cadence, timing or end-to-end session equality claimed"]}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({"cases":len(rows),"original_instruction_visits":oracle.visits,
                      "fixture_sha256":report["fixture_sha256"],"actual":actual},indent=2))
if __name__=="__main__":main()

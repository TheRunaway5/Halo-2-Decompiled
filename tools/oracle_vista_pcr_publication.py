#!/usr/bin/env python3
"""Execute supplied PC variant and PCR publication instructions unchanged.

Localized text, clock, engine/session queries, the caller's actual one-player
pool enumeration and machine metadata are explicit owner services. No native
implementation is imported. This proves the bounded PCR-consumed adapter, not
the complete native session, machine/event bank or multiple-player ranking.
"""
from pathlib import Path
import hashlib, itertools, json, struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import ROOT, SHA, BASE, STOP, STACK, OBJ, machine, u32, w32

BANK=0x8b1c80
VARIANT=OBJ+0x8000
OPTIONS=OBJ+0x10000
PAYLOAD=OBJ+0x2000
LIVE=OBJ+0x18000
ENGINE=OBJ+0xf000
PARAMETERS=(1,2,3,4,5,6,7,8,9,10,17,18,29,30,15,90,89,88,76,91,23,25,26)
VALUES=(-2147483648,-65537,-32769,-1,0,1,2,255,256,32768,65535,2147483647)
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-publication-original'

class Original:
    def __init__(self,image):
        self.image=image;self.u=machine(image);self.executed=set();self.visits=0
        self.mode='';self.clock=0;self.engine=False;self.datum=0;self.iterator_calls=0
        self.ranking_calls=0;self.comparisons=0;self.score_queries=0
        self.u.hook_add(UC_HOOK_CODE,self.hook)

    def ret(self,value=None,pop=0):
        s=self.u.reg_read(UC_X86_REG_ESP)
        if value is not None:self.u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        self.u.reg_write(UC_X86_REG_EIP,u32(self.u,s))
        self.u.reg_write(UC_X86_REG_ESP,s+4+pop)

    def hook(self,u,pc,size,_):
        self.visits+=1;self.executed.add(pc);s=u.reg_read(UC_X86_REG_ESP)
        if pc==STOP:u.emu_stop();return
        if pc==0x6875d3:self.ret();return # Security-cookie service only.
        if pc==0x687ba9:
            dst,value,count=(u32(u,s+4+i*4) for i in range(3))
            u.mem_write(dst,bytes([value&255])*count);self.ret(dst);return
        if pc==0x687e18:
            dst,src,count=(u32(u,s+4+i*4) for i in range(3))
            u.mem_write(dst,bytes(u.mem_read(src,count)));self.ret(dst);return
        if pc==0x46b601: # Absent localized title; output empty UTF16 string.
            u.mem_write(u32(u,s+8),b'\0\0');self.ret(0);return
        if pc==0x44bd27:
            dst,src,count=(u32(u,s+4+i*4) for i in range(3))
            text=bytearray()
            for i in range(count-1):
                c=bytes(u.mem_read(src+2*i,2))
                if c==b'\0\0':break
                text.extend(c)
            u.mem_write(dst,bytes(text)+b'\0\0');self.ret(dst);return
        if pc==0x437e5e:self.ret(self.clock);return
        if pc==0x46ad98:self.ret(int(self.engine));return
        if pc==0x45b15e:self.ret(ENGINE if self.engine else 0);return
        if pc==0x449801:self.ret(OPTIONS);return
        if pc==0x469037:self.ret(0,pop=4);return # External machine lookup; unconsumed.
        if pc==0x5b5ade:self.ret(LIVE+0x1000);return
        if pc in (0x5ae0a9,0x5ae0ba,0x449f8e,0x449823):self.ret(0);return
        if pc in (0x40c8ce,0x452bfd,0x46912e):self.ret();return
        if pc==0x45b171:self.ret(VARIANT);return
        if pc==0x46e858:self.ret(0);return # No teams in the admitted FFA owner.
        if pc==0x466419:
            self.ret(LIVE if u32(u,s+8)==(self.datum&0xffff) else 0);return
        if pc==0x4664e5:
            self.iterator_calls=0;u.mem_write(u32(u,s+4),bytes(12));self.ret();return
        if pc==0x4669cd:
            iterator=u32(u,s+4);self.iterator_calls+=1
            w32(u,iterator+4,self.datum if self.iterator_calls==1 else 0xffffffff)
            self.ret(LIVE if self.iterator_calls==1 else 0);return
        if pc==0x4d3045:self.ranking_calls+=1
        if pc==0x4d2a9c:self.comparisons+=1
        if pc==0x46ff80:self.score_queries+=1

    def call(self,pc,args=(),registers=()):
        s=STACK+0x18000
        self.u.mem_write(s,struct.pack('<'+'I'*(len(args)+1),STOP,*(v&0xffffffff for v in args)))
        self.u.reg_write(UC_X86_REG_ESP,s)
        for r,v in registers:self.u.reg_write(r,v&0xffffffff)
        self.u.emu_start(pc,STOP+1,count=300000)
        assert self.u.reg_read(UC_X86_REG_EIP)==STOP,hex(self.u.reg_read(UC_X86_REG_EIP))
        return self.u.reg_read(UC_X86_REG_EAX)

    def variant(self):
        self.u.mem_write(VARIANT,b'\xa5'*0x130);self.call(0x45b33d,(VARIANT,0))
        return bytes(self.u.mem_read(VARIANT,0x130))

def pattern(n,seed,mult=37):return bytes((i*mult+seed*13)&255 for i in range(n))
def consumed(bank):
    # Explicitly all bytes consumed by40C8CE and its retained numeric getters.
    parts=[bank[0x10:0x12],bank[0x20:0x150]]
    for i in range(16):
        a=0x370+i*0x94
        parts += [bank[a:a+1],bank[a+2:a+10],bank[a+0xc:a+0x90],bank[a+0x90:a+0x91],bank[a+0x92:a+0x94]]
    for i in range(16):
        a=0xcb0+i*0x18;parts.append(bank[a:a+4])
    parts.append(bank[0xe30:0x4e70])
    return b''.join(parts).hex()

def main():
    image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes()
    assert hashlib.sha256(image).hexdigest()==SHA
    original=Original(image);u=original.u;lines=['# Original supplied PC PCR consumed publication; '+SHA]
    default=original.variant();lines.append('D '+default.hex())
    xbox=json.loads((ROOT/'analysis/multiplayer/game-variants/original.json').read_text())['source']
    assert default.hex()==xbox['defaults'][0]['raw']
    descriptors=[]
    for p in range(112):
        entry=u32(u,0x864d88+p*4)
        if entry:
            original.call(entry,(OBJ,OBJ+4,OBJ+8));d=[u32(u,OBJ+i*4) for i in range(3)]
        else:d=[0xffffffff,0,0]
        expected=xbox['descriptors'][p]
        assert d==[expected['kind']&0xffffffff,expected['offset'],expected['operand']]
        descriptors.append(d);lines.append('Q '+' '.join(map(str,[p,*d])))
    for p,value in itertools.product(PARAMETERS,VALUES):
        original.variant();original.call(0x63a8c2,(VARIANT,p,value))
        raw=bytes(u.mem_read(VARIANT,0x130));v=original.call(0x63aa54,(VARIANT,p))
        if v&0x80000000:v-=0x100000000
        lines.append(f'V {p} {value} {raw.hex()} {v}')

    writes=0
    for kind,slot,enabled,final,occupied,score in itertools.product(
            ('P','T'),(-1,0,7,8,15),(0,1,128),(0,1,128),(0,1,128),VALUES):
        if kind=='P' and slot not in (-1,0,15):continue
        if kind=='T' and slot==15:continue
        seed=writes%17;placement=VALUES[(writes+5)%len(VALUES)]
        before=bytearray(pattern(0xdc78,seed));before[0]=enabled;before[0x11]=final
        a=(0x370+max(slot,0)*0x94) if kind=='P' else (0xcb0+max(slot,0)*0x18)
        before[a]=occupied;u.mem_write(BANK,bytes(before))
        original.call(0x466e93 if kind=='P' else 0x466ed4,(slot,score,placement))
        after=bytes(u.mem_read(BANK,0xdc78))
        offsets=[i for i,(x,y) in enumerate(zip(before,after)) if x!=y]
        allowed=({a+0x90,a+0x92,a+0x93} if kind=='P' else {a+1,a+2,a+3})
        assert set(offsets)<=allowed
        fields=after[a+0x90:a+0x91]+after[a+0x92:a+0x94] if kind=='P' else after[a+1:a+4]
        lines.append(f'{kind} {slot} {enabled} {final} {occupied} {score} {placement} {seed} {fields.hex()}')
        writes+=1

    clears=0
    for seed,clock in itertools.product((0,1,16),(0,1,0xffffffff)):
        before=pattern(0xdc78,seed);u.mem_write(BANK,before);original.engine=False;original.clock=clock
        original.call(0x469dfd)
        expected=bytearray(before);expected[0x10:]=bytes(0xdc68)
        struct.pack_into('<II',expected,4,0,clock)
        assert bytes(u.mem_read(BANK,0xdc78))==expected
        lines.append(f'C {seed} {clock}');clears+=1

    flags_cases=0
    w32(u,0x8bf8f8,ENGINE)
    for flags in (0,1,16,32,48,0x80000000,0xffffffff):
        w32(u,ENGINE,flags);result=original.call(0x4d25df)
        assert result==((flags>>5)&1)
        lines.append(f'F {flags} {result}');flags_cases+=1
    scores=0
    for slot,score in itertools.product(range(16),(-32768,-1,0,1,32767)):
        # Explicit PC field owner, not a cast of the Xbox0x1C player records.
        u.mem_write(ENGINE+0x304,pattern(0x294,slot))
        u.mem_write(ENGINE+0x304+4+slot*0x20+2,struct.pack('<h',score))
        datum=0x12340000|slot
        result=original.call(0x4d023a,(datum,1),((UC_X86_REG_ECX,ENGINE+0x304),))&0xffff
        assert result==(score&0xffff)
        lines.append(f'S {datum} {score}');scores+=1
    ranks=0
    original.variant()
    for datum,flag in itertools.product((0x12340000,0x7fff000f,0x80000007),(0,16)):
        original.datum=datum;original.iterator_calls=0;original.ranking_calls=0
        original.comparisons=0;original.score_queries=0
        original.engine=True;w32(u,ENGINE,0x20)
        u.mem_write(BANK,bytes(0xdc78));u.mem_write(BANK,b'\1');u.mem_write(BANK+0x35c,b'\1')
        slot=datum&0xffff;u.mem_write(BANK+0x370+slot*0x94,b'\1')
        u.mem_write(BANK+0x370+slot*0x94+0x90,b'\x7f')
        w32(u,VARIANT+0x48,flag);u.mem_write(LIVE+0x40,pattern(0x84,3))
        original.call(0x4692cc)
        placement=u.mem_read(BANK+0x370+slot*0x94+0x90,1)[0]
        assert placement==0 and original.ranking_calls==1 and original.iterator_calls==2
        assert original.comparisons==original.score_queries==0
        lines.append(f'R {datum} {flag} 32 {placement}');ranks+=1

    snapshots=0
    for seed,enabled,final in itertools.product((0,1,2),(0,1,128),(0,1)):
        default=original.variant();u.mem_write(OPTIONS,bytes(0x2000))
        u.mem_write(OPTIONS+0x2a0,default);u.mem_write(BANK,bytes(0xdc78))
        for i in range(16):
            if (i+seed)%3:continue
            a=OPTIONS+0x448+i*0xd4;u.mem_write(a,bytes([128 if i%2 else 1]))
            u.mem_write(a+0xe,pattern(8,seed+i,29));u.mem_write(a+0x18,pattern(0x84,seed+i,17))
        original.engine=True;original.call(0x469dfd)
        u.mem_write(BANK,bytes([enabled,128]));u.mem_write(BANK+0x11,b'\0')
        for i in range(16):
            original.call(0x466e93,(i,VALUES[(i+seed)%len(VALUES)],VALUES[(i+seed+3)%len(VALUES)]))
        # Original469D6A's actual4040 numeric memcpy, with current PC properties.
        payload=bytearray(0x4bcc);bank=bytes(u.mem_read(BANK,0xdc78))
        payload[0x10:0x950]=bank[0x370:0xcb0]
        payload[0x950:0xad0]=bank[0xcb0:0xe30]
        payload[0xad0:0x4b10]=pattern(0x4040,seed,19)
        u.mem_write(PAYLOAD,bytes(payload));original.call(0x469d6a,(PAYLOAD,))
        if final:original.call(0x469c75)
        bank=bytes(u.mem_read(BANK,0xdc78));lines.append(f'B {seed} {enabled} {final} '+consumed(bank));snapshots+=1

    OUT.mkdir(parents=True,exist_ok=True)
    fixture=ROOT/'tests/fixtures/vista_pcr_publication_original.tsv'
    fixture.write_text('\n'.join(lines)+'\n')
    d=Cs(CS_ARCH_X86,CS_MODE_32);listing=[]
    for pc in sorted(original.executed):
        if not BASE<=pc<BASE+len(image):continue
        ins=next(d.disasm(image[pc-BASE:pc-BASE+15],pc))
        listing.append(f'{pc:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
    (OUT/'executed-original.asm').write_text('\n'.join(listing)+'\n')
    owners=[]
    for begin,end in ((0x45b33d,0x45b6c8),(0x466e93,0x466f43),
                      (0x469dfd,0x46a00a),(0x469d6a,0x469dfd),
                      (0x469c75,0x469cf6),(0x4d25df,0x4d25ed),
                      (0x4d023a,0x4d0252),(0x471487,0x47150d),
                      (0x4d2d0a,0x4d2dcc),(0x469462,0x469497)):
        owners.append(f'; unchanged source owner {begin:08x}..{end:08x}')
        for ins in d.disasm(image[begin-BASE:end-BASE],begin):
            owners.append(f'{ins.address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
    owner_file=OUT/'source-owners.asm';owner_file.write_text('\n'.join(owners)+'\n')
    report=dict(source_sha256=SHA,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        xbox_reference_sha256=hashlib.sha256((ROOT/'analysis/multiplayer/game-variants/original.json').read_bytes()).hexdigest(),
        source_owners_sha256=hashlib.sha256(owner_file.read_bytes()).hexdigest(),
        default_slayer_full304_matches_xbox=True,all112_descriptors_match_xbox=True,
        variant_setter_getter_cases=len(PARAMETERS)*len(VALUES),guarded_writes=writes,
        engine_absent_full_clear_cases=clears,ranking_flag_cases=flags_cases,
        accumulated_player_score_cases=scores,sole_ffa_rank_cases=ranks,consumed_snapshots=snapshots,
        instruction_visits=original.visits,limits=__doc__.strip().splitlines())
    (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()

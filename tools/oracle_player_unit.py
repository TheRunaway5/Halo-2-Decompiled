#!/usr/bin/env python3
"""Original player/unit CPU owners on declared raw API banks.

View, activation, network-dirty and parameter leaves are explicit provided ABI
boundaries here. The separate owned-object-deletion investigation executes
actual joined-player/world teardown without supplied teardown results.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct

import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_XMM0)
from oracle_game_over import SPECS
from oracle_game_variants import Source,ROOT
from oracle_mp_full_scoreboard_1_5 import Xbe,_Code,literal,complete_tokens,references
from oracle_round_lifecycle import compact_raw_records

OUT=ROOT/'analysis/multiplayer/game-variants/player-unit'
FIXTURE=ROOT/'tests/fixtures/player_unit_original.json'
NONE=0xffffffff
P,O,B,G,E,V=0x1100000,0x1110000,0x1120000,0x1130000,0x1140000,0x1150000
OWNERS={
 'retail':dict(bind=0x14cad0,release=0x152cf0,status=0x152340,
    players_slot=0x4e8c24,player_globals_slot=0x4e8c20,objects_slot=0x4e0300,
    game_globals_slot=0x4e6948,engine_globals_slot=0x4e9ae8,engine_table=0x55e4d0,
    services=(0x1872d0,0x1682bf,0x13ac42,0x1584c0,0xb58c0,0xcbf60,0x1588b0,0x187a60)),
 'tu11':dict(bind=0x14e240,release=0x154440,status=0x153a90,
    players_slot=0x4f01f4,player_globals_slot=0x4f01f0,objects_slot=0x4e78d0,
    game_globals_slot=0x4edf18,engine_globals_slot=0x4f10b8,engine_table=0x566084,
    services=(0x188d00,0x169d8a,0x13c3b2,0x159b60,0xb6760,0xcd3b0,0x159ef0,0x189490)),
 'u5':dict(bind=0x14f620,release=0x155820,status=0x154e70,
    players_slot=0x4f54f4,player_globals_slot=0x4f54f0,objects_slot=0x4ecbd0,
    game_globals_slot=0x4f3218,engine_globals_slot=0x4f63b8,engine_table=0x56dfa0,
    services=(0x18a0c0,0x16b16a,0x13d722,0x15af40,0xb7e50,0xce440,0x15b2d0,0x18a850)),
}
NAMES=('bind_view','bind_control','reset_local_state','refresh_user','mark_network_dirty',
       'activate_unit','parameter_bits','forget_view_object')
PARAMETER_BITS=0x3eaaaaab


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def encoded(value):return json.dumps(value,sort_keys=True,separators=(',',':'))+'\n'


class Boundary(Exception):pass


class Trace(Source):
    def __init__(self,spec,owners):
        super().__init__(spec);self.b=owners
        self.g.mem_map(P,0x60000)
        self.service_map=dict(zip(owners['services'],NAMES))
        self.current=[];self.writes=[];self.events=[];self.omit=None
        self.g.hook_add(unicorn.UC_HOOK_CODE,self.hook)
        self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.write)
    def w(self,p,v,n=4):self.g.mem_write(p,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
    def read(self,p,n=4):return int.from_bytes(self.g.mem_read(p,n),'little')
    def hook(self,g,address,size,user):
        if address not in self.service_map:return
        name=self.service_map[address];sp=g.reg_read(UC_X86_REG_ESP)
        eax,ecx,edx,esi=(g.reg_read(r)for r in(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI))
        if name=='bind_view':args=[eax,ecx];argc=0
        elif name=='bind_control':args=[self.u(sp+4),eax,self.u(sp+8)];argc=2
        elif name=='reset_local_state':args=[edx];argc=0
        elif name=='refresh_user':args=[esi];argc=0
        elif name in('mark_network_dirty','activate_unit'):args=[self.u(sp+4),self.u(sp+8)];argc=2
        elif name=='parameter_bits':args=[ecx,self.u(sp+4)];argc=1
        else:args=[edx];argc=0
        self.events.append(dict(name=name,arguments=args))
        if self.omit==name:raise Boundary()
        if name=='parameter_bits':g.reg_write(UC_X86_REG_XMM0,PARAMETER_BITS)
        g.reg_write(UC_X86_REG_EIP,self.u(sp));g.reg_write(UC_X86_REG_ESP,sp+4+argc*4)
    def write(self,g,access,address,size,value,user):
        if any(p<=address and address+size<=p+n for p,n in self.current):
            self.writes.append(dict(address=f'{address:08x}',size=size,
                before=self.read(address,size),after=value&((1<<(size*8))-1)))
    def snapshot(self):
        return [dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())for p,n in self.current]
    def setup(self,slot=0,old=NONE,new=NONE,user=-1,representation=2,game_type=2,present=True,
              bitmap=None,entities=True):
        self.current=[]
        globals_=(('players_slot',P),('player_globals_slot',G),('objects_slot',O),
            ('game_globals_slot',V),('engine_globals_slot',E))
        for key,pointer in globals_:
            self.w(self.b[key],pointer);self.current.append((self.b[key],4))
        for p,n in((P,0x4c+16*0x21c+4),(O,0x4c+3*12+4),(B,3*0x800),
                  (G,0x130),(E,0xc18),(V,0x1200),(self.b['engine_table'],7*4)):
            self.g.mem_write(p,b'\xa5'*n);self.current.append((p,n))
        bits=(1<<slot)if bitmap is None else bitmap
        self.w(P+0x24,0x21c);self.w(P+0x38,bits.bit_length());self.w(P+0x44,P+0x4c)
        self.w(P+0x48,P+0x4c+16*0x21c);self.w(self.u(P+0x48),bits)
        self.w(O+0x44,O+0x4c)
        for i in range(16):
            p=P+0x4c+i*0x21c;self.w(p,0x8700+i,2);self.w(p+2,0,2)
            self.w(p+0x28,NONE,2)
            for off in(0x2c,0x30,0x174):self.w(p+off,NONE)
        for i in range(3):
            self.w(O+0x4c+i*12+8,B+i*0x800)
            self.w(B+i*0x800+0xaa,0,1)
            self.w(B+i*0x800+0xd4,0x90000+i if entities else NONE)
        row=P+0x4c+slot*0x21c
        self.w(row+0x28,user,2);self.w(row+0x88,representation,1)
        self.w(row+0x2c,old);self.w(row+0x30,0x10002)
        self.w(E+0xc14,2);self.w(V+8,game_type)
        for i in range(7):self.w(self.b['engine_table']+i*4,int(present))
        self.writes=[];self.events=[];self.omit=None
        return ((0x8700+slot)<<16)|slot
    def record(self,kind,datum,new=NONE,label='',omit=None):
        self.writes=[];self.events=[];self.omit=omit
        before=self.snapshot();complete=True
        try:
            if kind=='bind':self.call(self.b['bind'],(datum,),((UC_X86_REG_EBX,new),))
            else:self.call(self.b['release'],(datum,))
        except Boundary:complete=False
        return dict(kind=kind,label=label,datum=datum,new_unit=new,omit=omit,
            complete=complete,before=before,after=self.snapshot(),
            writes=list(self.writes),events=list(self.events))


def run(name,spec,b):
    t=Trace(spec,b);records=[]
    transitions=((NONE,NONE),(0x10000,0x10000),(0x10000,NONE),
        (NONE,0x10001),(0x10000,0x10001))
    for slot in(0,3,15):
        for old,new in transitions:
            for user in(-1,0,3):
                datum=t.setup(slot,old,new,user,bitmap=(1<<slot)|1|8)
                records.append(t.record('bind',datum,new,f'slot{slot}_transition{old:x}_{new:x}_user{user}'))
    for representation in(-128,-1,0,1,2,127):
        for game_type in(1,2):
            for present in(False,True):
                datum=t.setup(3,0x10000,0x10001,-32768,representation,game_type,present,entities=False)
                records.append(t.record('bind',datum,0x10001,f'representation{representation}_type{game_type}_engine{present}'))
    for object_type in(0,1,2,32,33,255):
        for scenario in('empty','current','watched','previous','mixed','shared_current'):
            bitmap=0 if scenario=='empty'else 0x8009
            t.setup(bitmap=bitmap)
            t.w(B+0xaa,object_type,1)
            for i in(0,3,15):
                p=P+0x4c+i*0x21c
                if scenario=='current'and i==3 or scenario=='shared_current':t.w(p+0x2c,0x10000)
                if scenario=='watched'and i==15 or scenario=='mixed'and i==3:t.w(p+0x174,0x10000)
                if scenario=='previous'and i==0 or scenario=='mixed'and i==15:t.w(p+0x30,0x10000)
                if scenario=='mixed'and i==0:t.w(p+0x2c,0x10000)
                if i==15:t.w(p+2,8,2)
            records.append(t.record('release',0x10000,label=f'type{object_type}_{scenario}'))
    for omitted in NAMES:
        datum=t.setup(3,0x10000,0x10001,32767,-1,2,True)
        kind='release'if omitted=='forget_view_object'else 'bind'
        records.append(t.record(kind,0x10000 if kind=='release'else datum,
            0x10001,label='missing_'+omitted,omit=omitted))
        assert not records[-1]['complete'],omitted
    section=t.image.section('.text');raw=t.image.section_bytes(section)
    assert bytes(t.g.mem_read(section.va,len(raw)))==raw
    return t,dict(bindings={k:v for k,v in b.items()if k.endswith('_slot')or k=='engine_table'},
        retail=name=='retail',parameter_bits=PARAMETER_BITS,records=records,
        whole_guest_text_unchanged=True)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--fixture',type=Path,default=FIXTURE)
    ap.add_argument('--verify-fixture',type=Path)
    args=ap.parse_args()
    if args.verify_fixture:assert args.fixture.resolve()!=args.verify_fixture.resolve()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    data={};traces={};rows={};listings=[];pairs=[]
    for name,spec in SPECS.items():
        t,data[name]=run(name,spec,OWNERS[name]);traces[name]=t
        code=_Code(t.image,'.text');starts={int(r['address'],16)for r in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
        rows[name]={key:literal(code,OWNERS[name][key],starts|{OWNERS[name][key]})[1]for key in('bind','release','status')}
        for key,rr in rows[name].items():
            p=directory/f'{name}-{key}.asm'
            p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rr))
            listings.append(dict(file=p.name,sha256=sha(p),rows=len(rr)))
    for name in('retail','tu11'):
        for key in('bind','release','status'):
            a,b=rows[name][key],rows['u5'][key]
            equal=complete_tokens(a,traces[name].image)==complete_tokens(b,traces['u5'].image)
            assert equal or(name=='retail'and key=='bind')
            pairs.append(dict(earlier=name,owner=key,complete_shape_equal=equal,
                relocated_operands=references(a,b,traces[name].image,traces['u5'].image)if equal else None))
    fixture=compact_raw_records(dict(schema=1,versions=data,
        boundaries=['Declared raw player/object/game/engine API inputs; source join and complete deactivation are validated separately.',
            'All eight view/control/network-dirty/activation/parameter services are provided ABI leaves here. These calls neither establish those owners nor claim complete object/world teardown.',
            'Missing-service cases stop before the named original leaf; output preserves the actual partial writes and attempted call order.',
            'Retail local control remaps representations0/1 to2/3 in game type2; title updates pass the signed representation byte directly.']))
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    raw=encoded(fixture)
    if args.verify_fixture:assert raw==args.verify_fixture.read_text()
    args.fixture.write_text(raw)
    deps={str(Path(m.__file__).resolve().relative_to(ROOT)):sha(m.__file__)
        for m in list(__import__('sys').modules.values())if getattr(m,'__file__',None)
        and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')}
    report=compact_raw_records(dict(schema=1,oracle_sha256=sha(__file__),source_dependencies=deps,
        fixture_sha256=sha(args.fixture),owners=OWNERS,instruction_listings=listings,
        complete_source_pairs=pairs,instruction_patches=[],
        summary=dict(records=sum(len(v['records'])for v in data.values()),
            partial_records=sum(not r['complete']for v in data.values()for r in v['records']))))
    args.output.write_text(encoded(report))
    print(encoded(dict(report=str(args.output),sha256=sha(args.output),fixture_sha256=sha(args.fixture),summary=report['summary'])),end='')


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Original complete round-reset CPU owner on declared raw API banks.

World services are provided leaves with explicit test mutations to expose
caller policy and iteration. They are not original world teardown; the separate
owned-round-reset probe executes real joined/constructed objects. No provided
leaf from this suite is installed in the native gameplay runtime.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
from oracle_game_variants import Source,ROOT
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import _Code,literal,complete_tokens,references
from oracle_round_lifecycle import compact_raw_records

OUT=ROOT/'analysis/multiplayer/game-variants/round-reset-native'
FIXTURE=ROOT/'tests/fixtures/round_reset_original.json'
NONE=0xffffffff
P,O,B,G,E,T,D,K,L=(0x1100000+i*0x10000 for i in range(9))
NAMES=('deactivate_object','delete_object','reset_decals','reset_contrails','reset_effects',
    'reset_registry','reset_lights','reset_sound','reset_scenario','scenario_callback','place_scenario')
SLOTS={
 'retail':dict(reset=0x159e20,game_globals_slot=0x4e6948,players_slot=0x4e8c24,objects_slot=0x4e0300,
    engine_globals_slot=0x4e9ae8,engine_table=0x55e4d0),
 'tu11':dict(reset=0x15b460,game_globals_slot=0x4edf18,players_slot=0x4f01f4,objects_slot=0x4e78d0,
    engine_globals_slot=0x4f10b8,engine_table=0x566084),
 'u5':dict(reset=0x15c840,game_globals_slot=0x4f3218,players_slot=0x4f54f4,objects_slot=0x4ecbd0,
    engine_globals_slot=0x4f63b8,engine_table=0x56dfa0),
}


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def encoded(value):return json.dumps(value,sort_keys=True,separators=(',',':'))+'\n'
class Boundary(Exception):pass


class Trace(Source):
    def __init__(self,name,spec):
        super().__init__(spec);self.b=dict(SLOTS[name]);self.g.mem_map(P,0x90000)
        self.code=_Code(self.image,'.text')
        self.starts={int(r['address'],16)for r in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
        rows=literal(self.code,self.b['reset'],self.starts|{self.b['reset']})[1]
        calls=[i.operands[0].imm for i in rows if i.mnemonic=='call'and i.operands[0].type==2]
        self.unit_iterator,self.deactivate,self.delete=calls[:3]
        self.object_iterator=calls[4]
        unique=list(dict.fromkeys(calls));assert unique[:4]==[self.unit_iterator,self.deactivate,self.delete,self.object_iterator]
        self.service_map=dict(zip([self.deactivate,self.delete]+unique[4:],NAMES))
        assert len(self.service_map)==len(NAMES)
        # The source weapon branch loads the definition datum into ECX,
        # then the absolute tag-table slot into EDX. Retain that exact operand.
        tag_loads=[rows[n+1]for n,i in enumerate(rows[:-1])if i.mnemonic=='mov'
            and i.op_str=='ecx, dword ptr [eax]'and rows[n+1].mnemonic=='mov'
            and rows[n+1].op_str.startswith('edx, dword ptr [0x')]
        assert len(tag_loads)==1
        self.b['tags_slot']=tag_loads[0].operands[1].mem.disp
        # The sole REP STOSD after the decal call gets its destination from
        # the source's absolute EDI load.
        self.b['mask_slot']=next(i.operands[1].mem.disp for i in rows if i.mnemonic=='mov'
            and i.op_str.startswith('edi, dword ptr [0x'))
        self.regions=[];self.writes=[];self.events=[];self.omit=None;self.relocate=False
        self.g.hook_add(unicorn.UC_HOOK_CODE,self.hook)
        self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.write)
    def w(self,p,v,n=4):self.g.mem_write(p,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
    def read(self,p,n=4):return int.from_bytes(self.g.mem_read(p,n),'little')
    def write(self,g,access,p,n,value,user):
        if any(a<=p and p+n<=a+size for a,size in self.regions):
            self.writes.append(dict(address=f'{p:08x}',size=n,before=self.read(p,n),after=value&((1<<(n*8))-1)))
    def provided_write(self,p,value,n=4):
        self.writes.append(dict(address=f'{p:08x}',size=n,before=self.read(p,n),after=value&((1<<(n*8))-1)))
        self.w(p,value,n)
    def retire(self,datum):
        for i in range(16):
            h=O+0x4c+i*12;p=B+i*0x400
            if self.read(h,2)and self.read(p+0x14)==datum:
                self.retire((self.read(h,2)<<16)|i)
        h=O+0x4c+(datum&65535)*12
        self.provided_write(h,0,2);self.provided_write(h+8,0)
        for i in range(16):
            p=P+0x4c+i*0x21c
            if self.read(p+0x2c)==datum:self.provided_write(p+0x2c,NONE)
    def hook(self,g,address,size,user):
        if address not in self.service_map:return
        name=self.service_map[address];sp=g.reg_read(UC_X86_REG_ESP)
        if name in('deactivate_object','delete_object'):args=[self.u(sp+4),self.u(sp+8)];argc=2
        elif name=='reset_scenario':args=[self.u(sp+4)];argc=1
        elif name=='scenario_callback':args=[g.reg_read(UC_X86_REG_EAX)];argc=0
        else:args=[];argc=0
        self.events.append(dict(name=name,arguments=args))
        if self.omit==name:raise Boundary()
        if name=='delete_object':self.retire(args[0])
        if name=='reset_decals'and self.relocate:self.provided_write(self.b['mask_slot'],L)
        g.reg_write(UC_X86_REG_EIP,self.u(sp));g.reg_write(UC_X86_REG_ESP,sp+4+argc*4)
    def snapshot(self):
        return [dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())for p,n in self.regions]
    def setup(self,mode=1,full=True,present=True,category=0,sparse=False,relocate=False):
        self.regions=[]
        for key,value in(('game_globals_slot',G),('players_slot',P),('objects_slot',O),
            ('tags_slot',T),('mask_slot',K),('engine_globals_slot',E)):
            self.w(self.b[key],value);self.regions.append((self.b[key],4))
        for p,n in((P,0x4c+16*0x21c+4),(O,0x4c+16*12+4),(B,16*0x400),
            (G,0x1200),(E,0xc18),(T,16*16),(D,16*0x400),(K,0x4204),(L,0x4204),(self.b['engine_table'],28)):
            self.g.mem_write(p,b'\xa5'*n);self.regions.append((p,n))
        self.w(G+0xc,mode,1);self.w(G+8,2);self.w(G+0x184,0x90000000|(8 if full else 0))
        for pool,stride,bitmap in((P,0x21c,0x8009),(O,12,0xffff)):
            self.w(pool+0x20,16);self.w(pool+0x24,stride);self.w(pool+0x38,16)
            self.w(pool+0x44,pool+0x4c);self.w(pool+0x48,pool+0x4c+16*stride)
            self.w(self.u(pool+0x48),bitmap)
        types=(0,1,2,3,4,5,6,7,8,9,10,11,12,31,32,255)
        for i,kind in enumerate(types):
            p=P+0x4c+i*0x21c;h=O+0x4c+i*12;body=B+i*0x400
            self.w(p,0x8700+i,2);self.w(p+0x2c,NONE)
            self.w(h,0 if sparse and i in(5,9,12)else 0x9200+i,2)
            self.w(h+3,kind,1);self.w(h+8,body)
            self.w(body,0x66000000|i);self.w(body+0xaa,kind,1)
            self.w(body+0xaf,0 if i in(3,4)else 255,1)
            self.w(body+0x14,0x92030003 if i==4 else NONE)
            self.w(body+0xd4,NONE if i%2==0 else 0x44000000|i)
            self.w(body+4,(1<<14)if category%2 else 0)
            self.w(T+i*16+8,D+i*0x400);self.w(D+i*0x400+0x290,category,2)
        self.w(P+0x4c+0x2c,0x92000000)
        self.w(P+0x4c+3*0x21c+0x2c,0x92010001)
        self.w(E+0xc14,2)
        for i in range(7):self.w(self.b['engine_table']+i*4,int(present))
        self.relocate=relocate;self.writes=[];self.events=[];self.omit=None
    def record(self,label,omit=None):
        self.writes=[];self.events=[];self.omit=omit;before=self.snapshot();complete=True
        try:self.call(self.b['reset'])
        except Boundary:complete=False
        return dict(label=label,omit=omit,relocate_mask=self.relocate,complete=complete,
            before=before,after=self.snapshot(),writes=list(self.writes),events=list(self.events))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--fixture',type=Path,default=FIXTURE)
    ap.add_argument('--verify-fixture',type=Path)
    args=ap.parse_args()
    if args.verify_fixture:assert args.fixture.resolve()!=args.verify_fixture.resolve()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    versions={};traces={};rows={};listings=[];pairs=[]
    for name,spec in SPECS.items():
        t=Trace(name,spec);traces[name]=t;records=[]
        for mode in(1,4):
            for full in(False,True):
                for present in(False,True):
                    for category in(-1,0,1,0x8000):
                        for sparse in(False,True):
                            t.setup(mode,full,present,category,sparse)
                            records.append(t.record(f'mode{mode}_full{full}_engine{present}_category{category}_sparse{sparse}'))
        for omitted in NAMES:
            t.setup();records.append(t.record('missing_'+omitted,omitted));assert not records[-1]['complete']
        t.setup(relocate=True);records.append(t.record('callback_relocates_mask'))
        section=t.image.section('.text');raw=t.image.section_bytes(section)
        assert bytes(t.g.mem_read(section.va,len(raw)))==raw
        versions[name]=dict(bindings={k:v for k,v in t.b.items()if k!='reset'},records=records,
            whole_guest_text_unchanged=True)
        entries=dict(reset=t.b['reset'],unit_iterator=t.unit_iterator,object_iterator=t.object_iterator)
        rows[name]={k:literal(t.code,v,t.starts|{v})[1]for k,v in entries.items()}
        for key,rr in rows[name].items():
            path=directory/f'{name}-{key}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rr))
            listings.append(dict(file=path.name,sha256=sha(path),rows=len(rr)))
    for name in('retail','tu11'):
        for key in rows[name]:
            a,b=rows[name][key],rows['u5'][key]
            assert complete_tokens(a,traces[name].image)==complete_tokens(b,traces['u5'].image),(name,key)
            pairs.append(dict(earlier=name,owner=key,complete_shape_equal=True,
                relocated_operands=references(a,b,traces[name].image,traces['u5'].image)))
    fixture=compact_raw_records(dict(schema=1,versions=versions,boundary=__doc__,
        provided_mutations=['Delete leaf recursively retires declared header subtrees and clears matching player current handles; this is an API test input, not original deletion.',
            'One decal callback redirects the mask global to another retained bank before the original caller reloads it.']))
    args.fixture.parent.mkdir(parents=True,exist_ok=True);raw=encoded(fixture)
    if args.verify_fixture:assert raw==args.verify_fixture.read_text()
    args.fixture.write_text(raw)
    deps={str(Path(m.__file__).resolve().relative_to(ROOT)):sha(m.__file__)
        for m in list(__import__('sys').modules.values())if getattr(m,'__file__',None)
        and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')}
    report=compact_raw_records(dict(schema=1,oracle_sha256=sha(__file__),source_dependencies=deps,
        fixture_sha256=sha(args.fixture),instruction_listings=listings,complete_source_pairs=pairs,
        bindings={name:v['bindings']for name,v in versions.items()},instruction_patches=[],
        summary=dict(records=sum(len(v['records'])for v in versions.values()),
            partial_records=sum(not r['complete']for v in versions.values()for r in v['records']))))
    args.output.write_text(encoded(report))
    print(encoded(dict(report=str(args.output),sha256=sha(args.output),fixture_sha256=sha(args.fixture),summary=report['summary'])),end='')


if __name__=='__main__':main()

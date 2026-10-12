#!/usr/bin/env python3
"""Execute unchanged retail/TU1.1/TU1.5 game-over countdown and class fades.

Game, clock, selected engine and saved audio-class records are declared API
inputs. Original engine registration, substring matching, gain conversion and
fade updates execute without supplied returns or instruction patches. World
phase order is static instruction evidence; this is not a whole original world
tick, hardware playback, session teardown or lobby return.
"""
import argparse, csv, json, random, struct
from pathlib import Path
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_XMM0, UC_X86_REG_XMM1
from oracle_game_variants import Source, ROOT
from oracle_mp_full_scoreboard_1_5 import (NEW, NEW_PIN, PIN, Xbe, _Code,
    literal, complete_tokens, references, sha)

OUT = ROOT/'analysis/multiplayer/game-variants/game-over'
FIXTURE = ROOT/'tests/fixtures/game_over_original.json'
# begin, complete, tick, request-fades, gain-to-db, set-class, advance-class
SPECS = {
    'retail': dict(path='assets/disc/default.xbe',pin='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
        globals=0x4e6948,clock=0x510c54,engine=0x4e9ae8,table=0x55e4d0,classes=0x502118,names=0x470090,
        ctor=0x378500,tsv='analysis/retail/functions.tsv',
        owners=(0x1389c0,0x138a10,0x138eb0,0x188cd0,0x2197f0,0x221980,0x221900),
        constants=(0x45dda8,0x45dc0c,0x45dc5c)),
    'tu11': dict(path='assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',pin=PIN,
        globals=0x4edf18,clock=0x518444,engine=0x4f10b8,table=0x566084,classes=0x5098f0,names=0x477578,
        ctor=0x37eb40,tsv='analysis/map-pack-update/functions.tsv',
        owners=(0x13a130,0x13a180,0x13a620,0x18a700,0x21b900,0x223a90,0x223a10),
        constants=(0x465218,0x46507c,0x4650cc)),
    'u5': dict(path=str(NEW),pin=NEW_PIN,
        globals=0x4f3218,clock=0x51df54,engine=0x4f63b8,table=0x56dfa0,classes=0x50ebf0,names=0x47c7a8,
        ctor=0x3825d0,tsv='analysis/multiplayer/title-update-1.5/decompiled/functions.tsv',
        owners=(0x13b4a0,0x13b4f0,0x13b990,0x18bac0,0x220200,0x228390,0x228310),
        constants=(0x46a2b8,0x46a11c,0x46a16c)),
}
G, C, M, A = 0x1000000, 0x1001200, 0x1010000, 0x1011000
def bits(value): return struct.unpack('<I',struct.pack('<f',value))[0]
def f32(value): return struct.unpack('<f',struct.pack('<f',value))[0]

class CountdownTrace(Source):
    def __init__(self,spec):
        super().__init__(spec)
        self.g.mem_map(M,0x2000)
        self.starts={int(r['address'],16) for r in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
        self.code=_Code(self.image,'.text');self.entries=set();self.calls=[];self.stores=[]
        self.g.hook_add(UC_HOOK_CODE,self.hook)
        self.g.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.call(spec['ctor'])
        self.g.mem_write(spec['globals'],struct.pack('<I',G))
        for global_address,pointer in ((spec['clock'],C),(spec['engine'],M),(spec['classes'],A)):
            self.g.mem_write(global_address,struct.pack('<I',pointer))
    def hook(self,g,a,n,user):
        if a in self.starts:self.entries.add(a)
        if a in self.spec['owners'][3:]:
            sp=g.reg_read(UC_X86_REG_ESP)
            self.calls.append(dict(entry=f'{a:08x}',stack16_hex=bytes(g.mem_read(sp,16)).hex(),
                xmm0=f'{g.reg_read(UC_X86_REG_XMM0):032x}',xmm1=f'{g.reg_read(UC_X86_REG_XMM1):032x}'))
    def write(self,g,access,a,n,value,user):
        if G+0x1128<=a<G+0x1130 or A<=a<A+54*16:
            self.stores.append(dict(address=f'{a:08x}',size=n,value=f'{value&((1<<(n*8))-1):0{n*2}x}'))
    def state(self):
        return dict(active=bool(self.g.mem_read(G+0x1128,1)[0]),
            fade_started=bool(self.g.mem_read(G+0x1129,1)[0]),
            countdown=struct.unpack('<i',self.g.mem_read(G+0x112c,4))[0],
            complete=bool(self.call(self.spec['owners'][1])))
    def classes(self): return bytes(self.g.mem_read(A,54*16)).hex()
    def setup(self,active=False,fade=False,count=0,rate=30,dt=1/30,present=True,raw=None):
        self.g.mem_write(G,bytes(0x1200));self.g.mem_write(M,bytes(0xc18))
        self.g.mem_write(M+0xc14,struct.pack('<I',2 if present else 0))
        self.g.mem_write(G+0x1128,bytes([int(active),int(fade)]))
        self.g.mem_write(G+0x112c,struct.pack('<i',count))
        self.g.mem_write(C+2,struct.pack('<h',rate));self.g.mem_write(C+4,struct.pack('<f',dt))
        self.g.mem_write(A,bytes(raw) if raw is not None else bytes(54*16))
        self.calls=[];self.stores=[]

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    rng=random.Random(0x13b990);data={};reports={};sources={}
    for name,spec in SPECS.items():
        t=CountdownTrace(spec);sources[name]=t
        names=[]
        for i in range(54):
            p=t.u(spec['names']+i*4);names.append(bytes(t.image.read(p,64)).split(b'\0')[0].decode())
        constants=[t.image.read(a,4).hex() for a in spec['constants']]
        starts=[];boundaries=[];sequence=[];fades=[]
        for rate in (-32768,-1,0,1,15,30,60,32767):
            for active in (False,True):
                t.setup(active,True,-27,rate=rate);before=t.state()
                t.call(spec['owners'][0]);after=t.state();t.call(spec['owners'][0]);assert t.state()==after
                starts.append(dict(rate=rate,before=before,after=after))
        for active in (False,True):
            for fade in (False,True):
                for present in (False,True):
                    for count in (-2147483648,-1,0,1,2,59,60,61,62,210,2147483647):
                        for dt in (0,1/60,1/30,f32(1/30)+2**-27,.25):
                            t.setup(active,fade,count,dt=dt,present=present);before=t.state()
                            t.call(spec['owners'][2]);boundaries.append(dict(before=before,dt_bits=f'{bits(dt):08x}',
                                engine_present=present,after=t.state(),classes=t.classes(),requests=len(t.calls)))
        t.setup();t.call(spec['owners'][0])
        sequence.append(dict(step=0,record=t.state(),classes=t.classes()))
        for tick in range(225):
            t.calls=[];t.call(spec['owners'][2])
            # Audio time is separately supplied; no simulation-clock fade step.
            delta=(.011,.023,.041)[tick%3]
            t.call(spec['owners'][6],registers=((UC_X86_REG_XMM1,bits(delta)),))
            sequence.append(dict(step=tick+1,audio_dt_bits=f'{bits(delta):08x}',record=t.state(),
                classes=t.classes(),calls=t.calls.copy()))
        for n in range(24):
            raw=bytearray()
            for i in range(54):raw.extend(struct.pack('<fffI',f32(rng.uniform(-64,0)),f32(rng.uniform(-64,0)),
                f32(rng.uniform(0,3)),rng.getrandbits(32)))
            t.setup(True,False,61,raw=raw,present=n%2==0);before=t.classes()
            t.call(spec['owners'][2]);calls=t.calls.copy();after=t.classes()
            delta=(-.1,0,.001,.25,1,2)[n%6]
            t.call(spec['owners'][6],registers=((UC_X86_REG_XMM1,bits(delta)),))
            final=t.classes()
            for i in range(54):
                assert before[i*32+24:i*32+32]==after[i*32+24:i*32+32]==final[i*32+24:i*32+32]
            fades.append(dict(engine_present=n%2==0,before=before,after_request=after,
                audio_dt_bits=f'{bits(delta):08x}',after_advance=final,calls=calls))
        data[name]=dict(xbe_sha256=spec['pin'],constants=constants,names=names,starts=starts,
            boundaries=boundaries,sequence=sequence,fades=fades)
        reports[name]=dict(source_path=spec['path'],spec=spec,executed_instructions=len(t.executed),
            executed_routines=len(t.entries),all_records=data[name])
    listingdir=args.output.parent/'instructions';listingdir.mkdir(exist_ok=True);listings=[];pairs=[]
    def save(name,t,e):
        f,ins=literal(t.code,e,t.starts);p=listingdir/f'{name}-{e:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
        listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)));return f,ins
    later=sources['u5']
    for name,t in sources.items():
        for index,e in enumerate(t.spec['owners']):
            _,ins=save(name,t,e)
            if name!='u5':
                ne=later.spec['owners'][index];_,ni=literal(later.code,ne,later.starts)
                a,b=complete_tokens(ins,t.image),complete_tokens(ni,later.image)
                differences=[dict(index=i,old=ins[i].op_str,later=ni[i].op_str) for i in range(len(a)) if a[i]!=b[i]]
                expected=[] if index!=5 else [dict(index=45,old=f'edi, {hex(t.spec["names"]+54*4)}',later='edi, 0x47c880')]
                assert len(a)==len(b) and differences==expected,(name,index,differences)
                pairs.append(dict(earlier=name,old=f'{e:08x}',later=f'{ne:08x}',instructions=len(ins),
                    explicit_table_end_difference=differences,relocations=references(ins,ni,t.image,later.image)))
        for e in sorted(t.entries-set(t.spec['owners'])):save(name,t,e)
    # World admission/control/respawn are instruction evidence, not supplied
    # successful executions of their external world/object dependencies.
    world=[]
    for old,new in ((0x139750,0x13aac0),(0x1396e0,0x13aa50),(0x12d600,0x12e9a0),
        (0x14eae0,0x14fec0),(0x14e7e0,0x14fbc0),(0x150dd0,0x1521b0),
        (0x151c90,0x153070),(0x15f3a0,0x160780),(0x15d490,0x15e870)):
        _,oi=save('tu11',sources['tu11'],old);_,ni=save('u5',later,new)
        assert complete_tokens(oi,sources['tu11'].image)==complete_tokens(ni,later.image)
        world.append(dict(old=f'{old:08x}',later=f'{new:08x}',instructions=len(ni),
            calls=[i.op_str for i in ni if i.mnemonic=='call']))
    fixture=dict(schema='original-game-over-v1',boundaries=__doc__,instruction_patches=[],supplied_returns=[],versions=data)
    FIXTURE.write_text(json.dumps(fixture,indent=2,sort_keys=True)+'\n')
    report=dict(fixture,fixture_sha256=sha(FIXTURE),oracle_sha256=sha(__file__),versions=reports,
        dependency_sha256={p:sha(ROOT/p) for p in ('tools/oracle_game_variants.py',
            'tools/oracle_mp_full_scoreboard_1_5.py','tools/oracle_smg_firing_1_1.py',
            'tools/inspect_multiplayer_hud.py','tools/research_mp_lobby_later_update.py',
            'tools/reference/halo2-decompiled/tools/functions.py','tools/reference/halo2-decompiled/tools/xbe.py')},
        complete_source_pairs=pairs,world_instruction_evidence=world,instruction_listings=listings,
        summary=dict(versions=3,starts=sum(len(v['starts']) for v in data.values()),
            boundary_ticks=sum(len(v['boundaries']) for v in data.values()),
            sequence_ticks=675,interrupted_fades=72,complete_countdown_audio_pairs=len(pairs),
            world_instruction_pairs=len(world),supplied_returns=0,instruction_patches=0))
    args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report['summary']))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Two actual weapon records in the original weapon update/postpass phase.

Constructors, per-weapon input, reload commands, complete weapon updates and
both object-pool projectile postpasses execute unchanged. Both weapons share
the actual retained unit/player/inventory ABI storage. Calls explicitly use
primary-then-secondary phase order; this does not prove complete original
unit routing, attachment or world scheduling. Raw storage and unavailable
animation/physics/effect owners are inherited from Trace. For Needler only,
each projectile request executes unchangedFC2E0 in supplied generic storage;
generic allocation still returns NONE. This composes its actual random owner
without claiming successful generic allocation or a retained projectile world. Local
first-person registration is absent; representation/dual timing is original.
"""
import argparse
import gc
import hashlib
import json
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import (Trace,Cache,ROOT,PIN,WEAPON,UNIT,WI,UI,POOL,
    HEADERS,BITMAP,ENTITY_FLAGS,FPS,PLAYER_DATA,TIME,SEED)
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,
    UC_X86_REG_ESP,UC_X86_REG_EIP)
from oracle_smg_firing_1_1 import STOP

OTHER,OI=0x10c0000,0x10002
BIRTH,BIRTH_ERROR,BIRTH_DATUM,BIRTH_RETURN=0x10c2000,0x10c4000,0x10003,STOP+0x100


def export_rows(report,name,representation,pattern):
    lines=[]
    for row in report['rows']:
        words=[row['tick'],*map(int,row['held']),int(row['reload']),int(row['seed'],16)]
        for state in row['after']:
            words.extend(struct.unpack('<4I',bytes.fromhex(state['magazine_hex'])))
            words.extend(struct.unpack('<13I',bytes.fromhex(state['barrel_hex'])))
            words.extend((int(state['heat'],16),int(state['spent'],16),state['energy_flags']))
        start,end=row['event_range'];effects=[e for e in report['events'][start:end] if e.get('service')=='00107b50']
        words.append(len(effects))
        for e in effects:
            words.extend((int(e['registers']['ecx'],16),int(e['registers']['eax'],16),*(int(v,16) for v in e['stack_arguments'][1:3])))
        lines.append(' '.join([name,str(representation),pattern,*(f'{v:08x}' for v in words)]))
    return lines


class DualTrace(Trace):
    def __init__(self,datum,representation,biped,needler):
        self.needler_constructor=needler;self.birth_return=None
        super().__init__(1)
        self.w(UNIT,biped)
        self.w(WEAPON,datum)
        self.g.mem_write(OTHER,bytes(self.g.mem_read(WEAPON,0x600)))
        self.g.mem_write(HEADERS+24,struct.pack('<HBBII',1,3,2,0,OTHER))
        self.w(POOL+0x38,3);self.w(BITMAP,7)
        self.w(OTHER+0xd4,OI);self.g.mem_write(ENTITY_FLAGS+0x44+(OI&0x3ff)*8,b'\x04')
        self.g.mem_write(UNIT+0x212,b'\x00\x01');self.w(UNIT+0x21c,OI)
        self.g.mem_write(PLAYER_DATA+0x88,bytes([representation]))
        self.w(FPS+0x10,0xffffffff)

    def hook(self,g,address,size,user):
        if address==BIRTH_RETURN:
            assert self.birth_return and g.reg_read(UC_X86_REG_EAX)&255==1
            stack,ret,registers,event=self.birth_return
            event['original_FC2E0_output_hex']=bytes(g.mem_read(BIRTH,428)).hex()
            event['seed_after_constructor']=f'{self.u(SEED):08x}'
            for register,value in registers:g.reg_write(register,value)
            g.reg_write(UC_X86_REG_EAX,0xffffffff);g.reg_write(UC_X86_REG_ESP,stack+8);g.reg_write(UC_X86_REG_EIP,ret)
            self.birth_return=None
            return
        if address==0xb8a30 and self.needler_constructor:
            assert self.birth_return is None
            stack=g.reg_read(UC_X86_REG_ESP);ret=self.u(stack);descriptor=self.u(stack+4)
            raw=bytes(g.mem_read(descriptor,0xc4))
            registers=[(r,g.reg_read(r)) for r in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
                UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)]
            super().hook(g,address,size,user)
            event=self.events[-1];event['seed_before_constructor']=f'{self.u(SEED):08x}'
            self.w(POOL+0x38,4);self.w(BITMAP,15)
            g.mem_write(HEADERS+36,struct.pack('<HBBHHI',1,3,5,0xffff,428,BIRTH))
            g.mem_write(BIRTH,bytes(0x1000));self.w(BIRTH,self.u(descriptor));self.w(BIRTH+0x14,0xffffffff);self.sh(BIRTH+0x2c,-1)
            for source,target,length in ((0x1c,0x64,12),(0x28,0x70,12),(0x34,0x7c,12),(0x58,0xa0,4),(0x40,0x88,12)):
                g.mem_write(BIRTH+target,raw[source:source+length])
            nested=stack-0x20
            g.mem_write(nested,struct.pack('<4I',BIRTH_RETURN,BIRTH_DATUM,descriptor,BIRTH_ERROR))
            self.birth_return=(stack,ret,registers,event)
            g.reg_write(UC_X86_REG_ESP,nested);g.reg_write(UC_X86_REG_EIP,0xfc2e0)
            return
        super().hook(g,address,size,user)

    def pair_state(self):
        return [dict(magazine_hex=bytes(self.g.mem_read(p+0x224,16)).hex(),
            barrel_hex=bytes(self.g.mem_read(p+0x1a4,52)).hex(),
            heat=f'{self.u(p+0x180):08x}',spent=f'{self.u(p+0x184):08x}',
            energy_flags=self.u(p+0x16c,2)&6) for p in (WEAPON,OTHER)]

    def execute_pair(self,pattern,reload_tick,loaded,reserve,has_magazine):
        self.active='constructors'
        for datum in (WI,OI):self.run(0x101bb0,(datum,))
        if has_magazine:
            for n,p in enumerate((WEAPON,OTHER)):
                self.sh(p+0x22c,loaded[n]);self.sh(p+0x22a,reserve[n])
        self.rows=[];previous=(False,False)
        for tick,held in enumerate(pattern):
            self.tick=tick;self.w(TIME+8,tick);first=len(self.events)
            self.active='unit_timer_fragment'
            self.g.reg_write(UC_X86_REG_EBP,UNIT);self.g.emu_start(0xc6cd8,0xc6d06,count=50)
            for n,datum in enumerate((WI,OI)):
                self.active=f'input{n}'
                flags=(2 if held[n] else 0)|(8 if held[n] and not previous[n] else 0)
                self.run(0x1045e0,(datum,flags,0x3f800000 if held[n] else 0))
            if tick==reload_tick:
                for datum in (WI,OI):
                    self.active='explicit_reload_command';self.run(0x105640,(datum,),((UC_X86_REG_ECX,0),))
            for n,datum in enumerate((WI,OI)):
                self.active=f'update{n}';self.run(0x1021d0,(datum,))
            self.active='projectile_pass';self.run(0x1019f0);after=self.pair_state();seed=self.u(SEED)
            last=len(self.events);self.active='second_projectile_pass';self.run(0x1019f0)
            assert after==self.pair_state() and seed==self.u(SEED) and len(self.events)==last
            self.rows.append(dict(tick=tick,held=held,reload=tick==reload_tick,after=after,seed=f'{seed:08x}',event_range=[first,last]))
            previous=held
        return dict(scope=__doc__,rows=self.rows,events=self.events,
            supplied_services=self.services,weapon_datums=[f'{WI:08x}',f'{OI:08x}'],
            representation=self.u(PLAYER_DATA+0x88,1),instruction_patches=[])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--export-only',action='store_true')
    parser.add_argument('--weapon',action='append',choices=('magnum','needler','plasma_rifle','smg'))
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');cases=[];lines=['# name representation pattern tick left right reload seed; two magazine/barrel/energy records; effect tuples.']
    if args.export_only:
        summary=json.loads((args.output/'report.json').read_text())
        for case in summary['cases']:
            path=Path(case['path']);assert hashlib.sha256(path.read_bytes()).hexdigest()==case['sha256']
            report=json.loads(path.read_text())
            lines.extend(export_rows(report,case['name'],case['representation'],case['pattern']))
        args.fixture.write_text('\n'.join(lines)+'\n')
        summary.setdefault('generator_tool_sha256',summary['tool_sha256'])
        summary['exporter_tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        summary['fixture_sha256']=hashlib.sha256(args.fixture.read_bytes()).hexdigest()
        (args.output/'report.json').write_text(json.dumps(summary,indent=2,sort_keys=True)+'\n')
        return
    globals_=next(t for t in cache.tags.values() if t['group']=='matg')
    representations=cache.block(globals_['offset']+0x138,0xbc)
    for name in args.weapon or ('magnum','needler','plasma_rifle'):
        tag=next(t for t in cache.tags.values() if t['group']=='weap' and t['name'].split('\\')[-1]==name)
        magazines=cache.block(tag['offset']+0x2c0,0x5c);capacity=cache.h(magazines[0]+10) if magazines else 0
        for representation in (2,3):
            biped=cache.tags[cache.u(representations[representation]+0xb4)]
            patterns={
                'left':([(False,False)]*5+[(True,False)]*140+[(False,False)]*100,None,[capacity,capacity],[4,4]),
                'both':([(False,False)]*5+[(True,True)]*140+[(False,False)]*100,None,[capacity,capacity],[4,4]),
                'pulses':([(False,False)]*5+[(True,False),(False,True)]*90+[(False,False)]*100,None,[capacity,capacity],[4,4]),
                'shared':([(False,False)]*5+[(True,True)]*140+[(False,False)]*120,30,[1,capacity],[0,4]),
            }
            for pattern,(held,reload,loaded,reserve) in patterns.items():
                trace=DualTrace(int(tag['datum'],16),representation,int(biped['datum'],16),name=='needler')
                trace.services[0x177a10]=('generic effect allocation: explicitly unavailable',0xffffffff,4)
                report=trace.execute_pair(held,reload,loaded,reserve,bool(magazines))
                report.update(actual_weapon=tag,actual_unit=biped,initial_loaded=loaded,initial_reserve=reserve,
                    ammo_controls_applied=bool(magazines),xbe_sha256=hashlib.sha256(trace.image.data).hexdigest())
                path=args.output/f'{name}-{representation}-{pattern}.json';path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
                lines.extend(export_rows(report,name,representation,pattern))
                cases.append(dict(name=name,representation=representation,pattern=pattern,ticks=len(report['rows']),
                    path=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
                print(name,representation,pattern,len(report['rows']),flush=True)
                del trace,report;gc.collect()
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    summary=dict(scope=__doc__,cases=cases,instruction_patches=[],
        xbe_sha256=PIN,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inherited_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),cache_sha256=hashlib.sha256(cache.data).hexdigest())
    (args.output/'report.json').write_text(json.dumps(summary,indent=2,sort_keys=True)+'\n')


if __name__=='__main__':main()

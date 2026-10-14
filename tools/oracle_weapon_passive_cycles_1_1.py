#!/usr/bin/env python3
"""Original complete weapon clocks with declared holstered/dropped storage.

The original constructor and complete1021D0 update execute unchanged, followed
by both original projectile postpasses. Object/player/time/clock controls and
the inherited unavailable animation/effect/projectile/world services are
explicit inputs. Holstered storage retains its unit owner and inventory slot,
but has no selected hand or parent; dropped storage has neither. This does not
execute the full original holster/drop callers or object-world scheduler.
"""
import argparse
import gc
import hashlib
import json
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import (Trace,Cache,ROOT,PIN,WEAPON,UNIT,WI,FPS,TIME,
    PLAYER_DATA,SEED)


def state_words(state):
    return [*state['clock_words'],*struct.unpack('<4I',bytes.fromhex(state['magazine_state_hex'])),
        *struct.unpack('<13I',bytes.fromhex(state['selected_barrel_hex'])),int(state['seed'],16)]


def export_case(report,name,context,pattern):
    lines=[' '.join(['I',name,context,pattern,*(f'{w:08x}' for w in state_words(report['initial']))])]
    for row in report['rows']:
        words=[row['tick'],*state_words(row['after'])];first,last=row['event_range']
        effects=[e for e in report['events'][first:last] if e.get('service')=='00107b50']
        words.append(len(effects))
        for e in effects:words.extend(int(v,16) for v in (e['registers']['eax'],*e['stack_arguments']))
        lines.append(' '.join(['T',name,context,pattern,*(f'{w:08x}' for w in words)]))
    return lines


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--weapon',action='append')
    parser.add_argument('--export-only',action='store_true')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    names=('magnum','battle_rifle','covenant_carbine','shotgun','sniper_rifle',
        'brute_shot','needler','rocket_launcher','smg','plasma_rifle','brute_plasma_rifle','beam_rifle')
    if args.weapon:
        assert set(args.weapon)<=set(names);names=tuple(n for n in names if n in args.weapon)
    cases=[];lines=['# weapon context pattern tick; ready heat spent inhibitor flags blocking; magazine4 barrel13 seed effects.']
    if args.export_only:
        report_path=args.output/'report.json';summary=json.loads(report_path.read_text())
        for case in summary['cases']:
            path=Path(case['path']);assert hashlib.sha256(path.read_bytes()).hexdigest()==case['sha256']
            lines.extend(export_case(json.loads(path.read_text()),case['weapon'],case['context'],case['pattern']))
        args.fixture.write_text('\n'.join(lines)+'\n')
        summary.setdefault('generator_tool_sha256',summary['tool_sha256'])
        summary.setdefault('original_fixture_sha256',summary['fixture_sha256'])
        summary['exporter_tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        summary['fixture_sha256']=hashlib.sha256(args.fixture.read_bytes()).hexdigest()
        report_path.write_text(json.dumps(summary,indent=2,sort_keys=True)+'\n');return
    for name in names:
        tag=next(t for t in cache.tags.values() if t['group']=='weap' and t['name'].split('\\')[-1]==name)
        triggers=cache.block(tag['offset']+0x2c8,0x40);index=cache.h(triggers[0]+8)
        magazines=cache.block(tag['offset']+0x2c0,0x5c)
        battery=bool(cache.u(tag['offset']+0x12c)&(1<<11))
        patterns=['ready','recovery']+(['heat'] if battery else ['reload','empty'])
        for context in ('holstered','dropped'):
            for pattern in patterns:
                class PassiveTrace(Trace):
                    def state(self):
                        state=super().state();state['selected_barrel_hex']=bytes(self.g.mem_read(WEAPON+0x1a4+index*52,52)).hex()
                        state['clock_words']=[self.u(WEAPON+0x19e,2),self.u(WEAPON+0x180),self.u(WEAPON+0x184),
                            self.u(WEAPON+0x188),self.u(WEAPON+0x16c,2),self.u(WEAPON+0x17c,2)]
                        return state
                trace=PassiveTrace(1);trace.w(WEAPON,int(tag['datum'],16));trace.w(FPS+0x10,0xffffffff)
                trace.g.mem_write(PLAYER_DATA+0x88,b'\x02')
                trace.services[0x177a10]=('generic effect allocation: explicitly unavailable',0xffffffff,4)
                trace.active='constructor';trace.run(0x101bb0,(WI,))
                trace.w(WEAPON+0x14,0xffffffff);trace.g.mem_write(UNIT+0x212,b'\xff\xff')
                trace.g.mem_write(WEAPON+0x12c,bytes([11 if context=='holstered' else 0]))
                if context=='dropped':trace.w(WEAPON+0x154,0xffffffff);trace.w(UNIT+0x218,0xffffffff)
                trace.sh(WEAPON+0x19e,10 if pattern=='ready' else 0)
                trace.sh(WEAPON+0x16c,0 if pattern=='ready' else 0x20)
                trace.sh(WEAPON+0x17c,7)
                trace.sh(WEAPON+0x16e,0)
                if pattern=='heat':
                    trace.f(WEAPON+0x180,1.);trace.f(WEAPON+0x184,.25);trace.f(WEAPON+0x188,.125)
                if pattern in ('reload','empty'):
                    capacity=cache.h(magazines[0]+10);trace.sh(WEAPON+0x22c,capacity-2 if pattern=='reload' else 0)
                    trace.sh(WEAPON+0x22a,4)
                    if pattern=='reload':
                        for offset,value in ((0x224,1),(0x226,30),(0x230,12),(0x232,30)):trace.sh(WEAPON+offset,value)
                if pattern=='recovery':
                    at=WEAPON+0x1a4+index*52;trace.g.mem_write(at+1,b'\x02');trace.sh(at+2,5)
                    for offset,value in ((0x10,.5),(0x1c,.5),(0x20,.5),(0x28,.25)):trace.f(at+offset,value)
                initial=trace.state();rows=[]
                for tick in range(180):
                    trace.tick=tick;trace.w(TIME+8,tick);first=len(trace.events)
                    trace.active='passive_update';trace.run(0x1021d0,(WI,))
                    trace.active='projectile_pass';trace.run(0x1019f0);after=trace.state();last=len(trace.events)
                    trace.active='second_projectile_pass';trace.run(0x1019f0)
                    assert trace.state()==after and len(trace.events)==last
                    row=dict(tick=tick,after=after,event_range=[first,last]);rows.append(row)
                report=dict(scope=__doc__,weapon=tag,context=context,pattern=pattern,initial=initial,rows=rows,
                    events=trace.events,supplied_services=trace.services,selected_barrel=index,instruction_patches=[],
                    xbe_sha256=PIN,cache_sha256=hashlib.sha256(cache.data).hexdigest())
                lines.extend(export_case(report,name,context,pattern))
                path=args.output/f'{name}-{context}-{pattern}.json';path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
                cases.append(dict(weapon=name,context=context,pattern=pattern,ticks=len(rows),path=str(path),
                    sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
                print(name,context,pattern,len(rows),flush=True);del trace,report;gc.collect()
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    (args.output/'report.json').write_text(json.dumps(dict(scope=__doc__,cases=cases,xbe_sha256=PIN,
        cache_sha256=hashlib.sha256(cache.data).hexdigest(),fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inherited_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),instruction_patches=[]),indent=2,sort_keys=True)+'\n')


if __name__=='__main__':main()

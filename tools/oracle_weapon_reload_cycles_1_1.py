#!/usr/bin/env python3
"""Original complete ordinary reload/firing sequences with declared services.

Constructor, input, reload command, full weapon update and both projectile
postpasses execute unchanged. Raw equipped storage, model/animation service
boundaries and failed projectile/effect allocation are inherited explicitly.
Both registered and unregistered views use unchanged original graph lookup
from the owning player's representation. This is TU1.1 whole-cycle evidence,
not a complete original game world.
"""
import argparse
import gc
import hashlib
import json
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,Trace,Cache,WEAPON,FPS


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--weapon',action='append')
    args=parser.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    names=('magnum','battle_rifle','covenant_carbine','shotgun',
           'sniper_rifle','brute_shot','needler','rocket_launcher')
    if args.weapon:
        assert set(args.weapon)<=set(names)
        names=tuple(name for name in names if name in args.weapon)
    patterns={
        'partial':([False]*5+[True]*20+[False]*200,30,'full'),
        'empty_auto':([False]*5+[True]*160+[False]*130,None,'two'),
        'interrupt':([False]*5+[True]*12+[False]*10+[True]*160+[False]*60,20,'partial'),
        'redundant':([False]*220,20,'full'),
        'empty_idle':([False]*250,None,'zero'),
        'last_round':([False]*5+[True]*160+[False]*130,None,'one'),
    }
    args.output.mkdir(parents=True,exist_ok=True)
    lines=['# name local pattern; tick held reload; 4 magazine words, 13 selected barrel words, effect count and triples.']
    cases=[];requests=0;ticks=0
    for name in names:
        tag=next(tag for tag in cache.tags.values() if tag['group']=='weap' and tag['name'].split('\\')[-1]==name)
        trigger=cache.block(tag['offset']+0x2c8,0x40)[0]
        index=cache.h(trigger+8)
        barrel=cache.block(tag['offset']+0x2d0,0xec)[index]
        magazine=cache.block(tag['offset']+0x2c0,0x5c)[cache.h(barrel+0x28)]
        capacity=cache.h(magazine+0xa)
        for local in (0,1):
            for pattern_name,(pattern,reload_tick,initial) in patterns.items():
                class ReloadTrace(Trace):
                    def state(self):
                        result=super().state()
                        result['selected_barrel_hex']=bytes(self.g.mem_read(WEAPON+0x1a4+index*0x34,52)).hex()
                        return result
                trace=ReloadTrace(1)
                trace.w(WEAPON,int(tag['datum'],16))
                if not local:trace.w(FPS+0x10,0xffffffff) # Actual167D32 registration is absent;169F3B still executes.
                loaded=capacity if initial=='full' else 2 if initial=='two' else 1 if initial=='one' else max(0,capacity-2) if initial=='partial' else 0
                report=trace.execute(pattern=pattern,reload_tick=reload_tick,loaded_ammo=loaded,reserve_ammo=4)
                report.update(scope=__doc__,actual_weapon_tag=tag,selected_barrel=index,local=bool(local),actual_pattern_name=pattern_name)
                path=args.output/f'{name}-{local}-{pattern_name}.json'
                path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
                for row in report['rows']:
                    after=row['post_dispatch']
                    values=[row['tick'],int(row['held']),int(row['explicit_reload_command'])]
                    values.extend(struct.unpack('<4I',bytes.fromhex(after['magazine_state_hex'])))
                    values.extend(struct.unpack('<13I',bytes.fromhex(after['selected_barrel_hex'])))
                    first,last=row['event_range']
                    effects=[event for event in report['events'][first:last] if event.get('service')=='00107b50']
                    values.append(len(effects))
                    for event in effects:
                        values.extend(int(word,16) for word in (event['registers']['eax'],*event['stack_arguments'][1:3]))
                    lines.append(' '.join([name,str(local),pattern_name,*(f'{value:08x}' for value in values)]))
                    requests+=len(effects);ticks+=1
                cases.append(dict(name=name,local=local,pattern=pattern_name,loaded=loaded,reserve=4,
                    ticks=len(report['rows']),path=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
                print(name,local,pattern_name,len(report['rows']),flush=True)
                del trace,report;gc.collect()
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    (args.output/'report.json').write_text(json.dumps(dict(scope=__doc__,cases=cases,ticks=ticks,effect_requests=requests,
        xbe_sha256=PIN,cache_sha256=CACHE_PIN,instruction_patches=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inherited_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest()),indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(cases),ticks=ticks,effect_requests=requests)),flush=True)
    cache.data.close();cache.stream.close()


if __name__=='__main__':main()

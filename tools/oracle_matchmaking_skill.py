#!/usr/bin/env python3
"""Unchanged retail/TU1.5 XP, collection and rank controls for the Rust port.

Original initializer bytes supply all tables. Only declared synthetic result
records/registers and fixture delta limits are written; no executable byte,
callee or instruction is replaced. No Live result producer is inferred here.
Run through run_test.py; fixture/report files are small reference evidence.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI
from oracle_game_variants import Source, Xbe, VARIANT, STOP, ROOT, VERSIONS
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(data): return hashlib.sha256(data).hexdigest()
def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, separators=(',', ':'))+'\n')


def run(name, path, pin, init, pair, player, clan, rank, base):
    image=Xbe(path)
    assert sha(image.data)==pin
    # Reuse only the bounded original call convention. Loading the image here
    # does not change unrelated globals (variant initializers need none here).
    source=Source.__new__(Source)
    source.g=Uc(UC_ARCH_X86,UC_MODE_32);g=source.g
    g.mem_map(0x10000,0x800000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(VARIANT,0x2000);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
    executed={}
    g.hook_add(UC_HOOK_CODE,lambda uc,at,size,unused:executed.__setitem__(at,size))
    source.call(init)
    def table(off,fmt,count):return list(struct.unpack('<'+fmt*count,g.mem_read(base+off,struct.calcsize(fmt)*count)))
    tables=dict(win_exchange=table(0x378,'h',255),loss_exchange=table(0x576,'h',255),
        xp_thresholds=table(0x774,'i',128),loss_percent=table(0x974,'B',128),win_percent=table(0x9f4,'B',128),
        loss_offsets=table(0xa74,'h',128),win_offsets=table(0xb74,'h',128),delta_bounds=table(0xc78,'i',2))
    original=ROOT/'rust/online_protocol/data'/('retail-launch-2004-rank.json' if name=='retail-launch-2004' else 'title-update-1.5-rank.json')
    assert tables==json.loads(original.read_text())['tables']
    rng=random.Random(0x7480078620)
    controls=[];calls=1
    def add(op, values, actual, limits=None):
        controls.append(dict(op=op,input=values,actual=actual,limits=limits))
    for i in range(512):
        own,other=rng.randrange(128),rng.randrange(128)
        a,b=rng.choice((-1,0,1,7,15,127)),rng.choice((-1,0,1,7,15,127))
        actual=source.call(pair,registers=((UC_X86_REG_ESI,own),(UC_X86_REG_EBX,other),(UC_X86_REG_EDX,a),(UC_X86_REG_EAX,b)))
        calls+=1;add('rank_pair',dict(own=own,other=other,place=a,other_place=b),actual)
    for i in range(128):
        count=1+i%16;team_mode=i%2==1
        ratings=[]
        for index in range(count):
            ratings.append(dict(skill=rng.randrange(128),place=rng.choice((-1,0,1,3,15)),
                team=rng.randrange(4) if team_mode else index,xp=rng.choice((0,3,5000,0x3fffffff-3,0x3fffffff)),
                key=3 if index==0 or rng.randrange(4) else 7,flags=0 if index==0 else rng.randrange(4),
                active=index==0 or bool(rng.randrange(2))))
        places=[rng.choice((-1,0,1,3)) for _ in range(4)] if team_mode else None
        limits=[7,-9] if i%8==0 else None
        if limits:g.mem_write(base+0xc78,struct.pack('<ii',*limits))
        g.mem_write(VARIANT,bytes(0xf44));g.mem_write(VARIANT+4,struct.pack('<i',3));g.mem_write(VARIANT+0x12b,bytes([team_mode]))
        for slot,p in enumerate(ratings):
            at=VARIANT+0x384+slot*0xa4
            for off,fmt,value in [(0,'B',int(p['active'])),(0xa,'B',p['flags']),(0x8c,'b',p['team']),
                                  (0x94,'i',p['key']),(0x98,'h',p['skill']),(0x9c,'i',p['xp']),(0xa0,'b',p['place'])]:
                g.mem_write(at+off,struct.pack('<'+fmt,value))
        for team,place in enumerate(places or []):g.mem_write(VARIANT+0xdc4+team*24,b'\1'+struct.pack('<b',place))
        actual=source.call(player,(VARIANT,0));calls+=1
        add('rank_player',dict(players=ratings,selected=0,key=3,team_places=places),actual,limits)
        if limits:g.mem_write(base+0xc78,struct.pack('<ii',*tables['delta_bounds']))
    for i in range(64):
        ratings=[dict(skill=rng.randrange(128),place=rng.choice((-1,0,1,7,15)),xp=rng.choice((0,3,5000,0x3fffffff-3)),
                      active=index==0 or bool(rng.randrange(2))) for index in range(1+i%16)]
        g.mem_write(VARIANT,bytes(0xf44))
        for slot,p in enumerate(ratings):
            at=VARIANT+0xdc4+slot*24
            for off,fmt,value in [(0,'B',int(p['active'])),(1,'b',p['place']),(4,'h',p['skill']),(8,'i',p['xp'])]:
                g.mem_write(at+off,struct.pack('<'+fmt,value))
        add('rank_clan',dict(clans=ratings,selected=0),source.call(clan,(VARIANT,0)));calls+=1
    points={0,1,0x3fffffff}
    for a,b in zip(tables['xp_thresholds'],tables['xp_thresholds'][1:]):
        points.update(p for p in (a-1,a,a+1,(a+b)//2,(a+b)//2+1) if 0<=p<=0x3fffffff)
    for xp in sorted(points):
        previous=rng.randrange(128)
        add('rank_raw',dict(xp=xp,previous=previous),source.call(rank,registers=((UC_X86_REG_ESI,xp),(UC_X86_REG_EDI,previous))))
        calls+=1
    for at,size in executed.items():
        section=next(section for section in image.sections if section.va<=at<section.va+section.vsize)
        assert bytes(g.mem_read(at,size))==image.section_bytes(section)[at-section.va:at-section.va+size]
    return dict(policy=name,xbe_sha256=pin,table_sha256=sha(original.read_bytes()),tables=tables,controls=controls,
        calls=calls,unique_original_instructions=len(executed),
        original_executed_bytes_sha256=sha(b''.join(struct.pack('<IH',at,executed[at])+bytes(g.mem_read(at,executed[at])) for at in sorted(executed))))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    cases=[run('retail-launch-2004',ROOT/VERSIONS['retail']['path'],VERSIONS['retail']['pin'],0x66330,0x746b0,0x74800,0x74720,0x74970,0x4ce040),
           run('title-update-1.5-defaults',NEW_DEFAULT,NEW_PIN,0x67400,0x784d0,0x78620,0x78540,0x78790,0x4da8c8)]
    save(args.fixture,dict(schema='H2_MATCHMAKING_SKILL_ORIGINAL_V1',scope=__doc__,policies=cases))
    assert args.fixture.stat().st_size<1024*1024
    save(args.report,dict(passed=True,scratch=str(scratch),fixture_sha256=sha(args.fixture.read_bytes()),fixture_bytes=args.fixture.stat().st_size,
        policies=[{k:v for k,v in case.items() if k not in ('tables','controls')}|dict(controls=len(case['controls'])) for case in cases],
        limits='Synthetic comparison-skill/result records only. No effective skill producer, downloaded profile, original Live arbitration, native played flow or delivery proof.',retained_large_artifacts=[]))
    print(args.report.read_text())

if __name__=='__main__':main()

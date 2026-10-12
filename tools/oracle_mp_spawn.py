#!/usr/bin/env python3
"""Run untouched original Xbox multiplayer spawn instructions with explicit worlds.

The original collector, iterators, Slayer callbacks, scoring, RNG and chooser
execute. Fixture pools supply their state. A requested collision-tag test records
the original transformed query and supplies the shape-service result; ordinary
cases supply tag NONE and take the original no-collision branch. This is not a
live Xbox world, global RNG initialization proof, or collision-system oracle.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer import Cache, MAPS
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import capstone
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG,
    UC_X86_REG_XMM0)

VERSIONS = {
    'base': dict(path='assets/disc/default.xbe',
        pin='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
        choose=0x14ef00, collect=0x23bc40, settings=0x23aea0,
        score=0x23ba90, sum=0x23b060, collision=0x23bb70, shape=0x1c5210,
        globals=0x4e034c, scenario=0x4e0350, tags=0x4e3b44,
        players=0x4e8c24, objects=0x4e0300, options=0x4e6948,
        engine_globals=0x4e9ae8, engine_table=0x55e4d0, engine=0x47fc80,
        seed=0x4e7408, settings_data=0x502258, score_return=0x14efb6,
        collect_return=0x14ef71, random_scale=0x45dc50),
    'tu11': dict(path='assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
        pin='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
        choose=0x150640, collect=0x23dd40, settings=0x23cfa0,
        score=0x23db90, sum=0x23d160, collision=0x23dc70, shape=0x1c8b40,
        globals=0x4e791c, scenario=0x4e7920, tags=0x4eb114,
        players=0x4f01f4, objects=0x4e78d0, options=0x4edf18,
        engine_globals=0x4f10b8, engine_table=0x566084, engine=0x487188,
        seed=0x4ee9d8, settings_data=0x509a30, score_return=0x1506f6,
        collect_return=0x1506b1, random_scale=0x4650c0),
}
CACHE_PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
MEM = 0x1000000
MATG, SCENARIO, MULG, RUNTIME, CONSTANTS, TAGS, STARTS, SPAWN, RESPAWN_ZONES, INITIAL_ZONES, OVERRIDES = [MEM + n * 0x4000 for n in range(11)]
PLAYER_POOL, PLAYERS, BITMAP, OBJECT_POOL, HEADERS, OBJECTS, ENGINE_GLOBALS, OPTIONS, SEED, LIST, POINT, SHAPE_DEF, SHAPE_DATA = [MEM + (11+n) * 0x4000 for n in range(13)]
STACK, STOP = 0x200f000, 0x3000000
SESSION, CONFIG = MEM+0x70000, MEM+0x80000

def f32(v): return struct.unpack('<f', struct.pack('<f', v))[0]
def bitstr(v): return f'{struct.unpack("<I", struct.pack("<f", v))[0]:08x}'
def advance(seed): return (seed * 0x19660d + 0x3c6ef35f) & 0xffffffff
def expected_sum(point, influences):
    total = f32(0)
    for x, y, z, inner, outer, upper, lower, weight in influences:
        dx, dy, dz = [f32(a-b) for a,b in zip(point, (x,y,z))]
        d2 = f32(f32(dx*dx) + f32(dy*dy))
        part = f32(0)
        if f32(outer*outer) > d2 and upper >= dz and dz >= f32(-lower):
            if f32(inner*inner) > d2 or inner >= outer:
                part = weight
            else:
                part = f32((1.0 - (math.sqrt(d2) - inner) / (outer - inner)) * weight)
        total = f32(total + part)
    return total


class Oracle:
    def __init__(self, version):
        self.version, self.a = version, VERSIONS[version]
        self.x = Xbe(ROOT / self.a['path'])
        assert hashlib.sha256(self.x.data).hexdigest() == self.a['pin']
        self.g = Uc(UC_ARCH_X86, UC_MODE_32)
        self.g.mem_map(0x10000, 0x800000)
        for section in self.x.sections:
            self.g.mem_write(section.va, self.x.section_bytes(section))
        self.g.mem_map(MEM, 0x100000)
        self.g.mem_map(0x2000000, 0x10000)
        self.g.mem_map(STOP, 0x1000)
        self.current_scores, self.collision_queries = [], []
        self.blocked = set()
        self.g.hook_add(UC_HOOK_CODE, self.hook)

    def w(self, at, v): self.g.mem_write(at, struct.pack('<I', v & 0xffffffff))
    def sh(self, at, v): self.g.mem_write(at, struct.pack('<h', v))
    def f(self, at, v): self.g.mem_write(at, struct.pack('<f', v))
    def u(self, at): return struct.unpack('<I', self.g.mem_read(at,4))[0]
    def h(self, at): return struct.unpack('<h', self.g.mem_read(at,2))[0]
    def floats(self, at, n): return list(struct.unpack('<'+'f'*n, self.g.mem_read(at,n*4)))
    def reset_cpu(self):
        self.g.reg_write(UC_X86_REG_FPCW, 0x027f)
        self.g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        self.g.reg_write(UC_X86_REG_FPSW, 0)
        self.g.reg_write(UC_X86_REG_FPTAG, 0xffff)

    def call(self, address, argv=(), registers=(), end=STOP):
        self.reset_cpu()
        self.w(STACK, STOP)
        for i,v in enumerate(argv): self.w(STACK+4+i*4,v)
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        for r,v in registers: self.g.reg_write(r,v)
        try:
            self.g.emu_start(address,end,count=1000000)
        except Exception as ex:
            raise RuntimeError(f'{self.version} entry {address:08x}, EIP {self.g.reg_read(UC_X86_REG_EIP):08x}: {ex}') from ex
        assert self.g.reg_read(UC_X86_REG_EIP)==end
        return self.g.reg_read(UC_X86_REG_EAX)

    def hook(self, uc, address, size, _):
        sp = uc.reg_read(UC_X86_REG_ESP)
        if address == self.a['collect_return']:
            self.collected = [self.floats(sp+0x2c+i*32,8) for i in range(self.u(sp+0x28))]
        elif address == self.a['score_return']:
            index = self.u(sp+0x14)
            bits = uc.reg_read(UC_X86_REG_XMM0) & 0xffffffff
            self.current_scores.append(dict(index=index, bits=f'{bits:08x}',
                value=struct.unpack('<f', struct.pack('<I',bits))[0],
                private_seed=f'{self.u(sp+0x1c):08x}',
                global_spawn_seed=f'{self.u(SEED+4):08x}'))
        elif address == self.a['shape']:
            matrix = uc.reg_read(UC_X86_REG_EAX)
            pos = self.floats(matrix+0x28,3)
            index = self.current_scores[-1]['index']
            hit = index in self.blocked
            self.collision_queries.append(dict(index=index, transform=self.floats(matrix,13),
                position=pos, shape=f'{self.u(sp+4):08x}',
                excluded=f'{self.u(sp+8):08x}', filter=self.u(sp+12), supplied_hit=hit))
            uc.reg_write(UC_X86_REG_EAX,int(hit))
            uc.reg_write(UC_X86_REG_EIP,self.u(sp))
            uc.reg_write(UC_X86_REG_ESP,sp+16)

    def setup(self, cache, initial=True, count=0, players=(), objects=(),
              recent_deaths=(), random_bias=None, starts=None):
        self.g.mem_write(MEM,bytes(0x100000))
        self.g.mem_write(0x2000000,bytes(0x10000))
        s=cache.scenario
        matg=next(t['offset'] for t in cache.tags.values() if t['group']=='matg')
        mulg=cache.tags[cache.u(matg+0x16c)]['offset']
        runtime=cache.block(mulg+8,0x568)[0]
        constants=cache.block(runtime+0x530,0x160)[0]
        self.g.mem_write(MATG,bytes(cache.data[matg:matg+0x170]))
        self.w(MATG+0x16c,0)
        self.g.mem_write(MULG,bytes(cache.data[mulg:mulg+16]));self.w(MULG+12,RUNTIME)
        self.g.mem_write(RUNTIME,bytes(cache.data[runtime:runtime+0x568]));self.w(RUNTIME+0x534,CONSTANTS)
        self.g.mem_write(CONSTANTS,bytes(cache.data[constants:constants+0x160]))
        if random_bias is not None:self.f(CONSTANTS,random_bias)
        self.w(TAGS+8,MULG)
        self.g.mem_write(SCENARIO,bytes(cache.data[s:s+0x320]))
        self.start_offsets=cache.block(s+0x100,0x34)
        if starts is None:
            raw=b''.join(bytes(cache.data[p:p+0x34]) for p in self.start_offsets)
            self.start_points=[list(struct.unpack_from('<3f',raw,i*0x34)) for i in range(len(self.start_offsets))]
        else:
            raw=b''.join(struct.pack('<3f',*p)+bytes(0x34-12) for p in starts)
            self.start_points=[list(p) for p in starts]
        assert len(raw)<0x4000
        self.g.mem_write(STARTS,raw);self.w(SCENARIO+0x100,len(self.start_points));self.w(SCENARIO+0x104,STARTS)
        data=cache.block(s+0x318,0x60)
        if data:
            p=data[0];self.g.mem_write(SPAWN,bytes(cache.data[p:p+0x60]));self.w(SCENARIO+0x31c,SPAWN)
            for off,stride,target in [(0x48,0x10,OVERRIDES),(0x50,0x30,RESPAWN_ZONES),(0x58,0x30,INITIAL_ZONES)]:
                rows=cache.block(p+off,stride)
                assert len(rows)*stride<0x4000
                if rows:self.g.mem_write(target,b''.join(bytes(cache.data[q:q+stride]) for q in rows))
                self.w(SPAWN+off+4,target)
        for key,target in [('globals',MATG),('scenario',SCENARIO),('tags',TAGS),
            ('players',PLAYER_POOL),('objects',OBJECT_POOL),('engine_globals',ENGINE_GLOBALS),
            ('options',OPTIONS),('seed',SEED)]:self.w(self.a[key],target)
        self.w(self.a['engine_table']+8,self.a['engine'])
        self.w(ENGINE_GLOBALS+0xc14,2)
        self.w(OPTIONS+8,2);self.w(OPTIONS+0x10,0x13579bdf);self.w(OPTIONS+0x14,0x2468ace0)
        self.w(SEED,0x12345678);self.w(SEED+4,0x87654321)
        self.w(PLAYER_POOL+0x20,16);self.w(PLAYER_POOL+0x24,0x21c)
        self.w(PLAYER_POOL+0x38,16);self.w(PLAYER_POOL+0x44,PLAYERS);self.w(PLAYER_POOL+0x48,BITMAP)
        self.w(BITMAP,(1<<(len(players)+1))-1)
        for i in range(16):
            p=PLAYERS+i*0x21c;self.w(p+0x2c,-1);self.g.mem_write(p+0xc0,bytes([i]));self.sh(p,1)
        self.g.mem_write(PLAYERS+2,bytes([8 if initial else 0]));self.sh(PLAYERS+0x218,count)
        all_objects=[]
        for i,p in enumerate(players,1):
            team,point,dead=p
            self.g.mem_write(PLAYERS+i*0x21c+0xc0,bytes([team]))
            self.w(PLAYERS+i*0x21c+0x2c,0x10000+len(all_objects))
            all_objects.append(dict(type=0,position=point,dead=dead))
        all_objects.extend(objects)
        self.w(OBJECT_POOL+0x38,len(all_objects));self.w(OBJECT_POOL+0x44,HEADERS)
        for i,obj in enumerate(all_objects):
            p=OBJECTS+i*0x400
            self.g.mem_write(HEADERS+i*12,struct.pack('<HBBHHI',1,3,obj['type'],0xffff,0x400,p))
            self.w(p,1);self.w(p+0xc,-1);self.w(p+0x10,-1);self.w(p+0x14,-1)
            self.g.mem_write(p+0xaa,bytes([obj['type']]))
            self.g.mem_write(p+0x10a,bytes([4 if obj.get('dead') else 0]))
            self.g.mem_write(p+0x30,struct.pack('<3f',*obj['position']))
            self.g.mem_write(p+0x88,struct.pack('<3f',*obj.get('velocity',(0,0,0))))
        # Positive authored explosive bounds enable original projectile danger.
        grenade=next(t for t in cache.tags.values() if t['group']=='proj' and t['name'].endswith('frag_grenade'))
        self.g.mem_write(SHAPE_DEF,bytes(cache.data[grenade['offset']:grenade['offset']+0x2a0]));self.w(TAGS+24,SHAPE_DEF)
        self.w(SHAPE_DEF+0x28c,1);self.w(SHAPE_DEF+0x290,SHAPE_DATA)
        for i,point,timer in recent_deaths:
            p=ENGINE_GLOBALS+0x558+i*0x18
            self.g.mem_write(p,b'\1');self.g.mem_write(p+4,struct.pack('<3f',*point));self.sh(p+0x10,timer)
        self.call(self.a['settings'])
        self.bias=self.floats(CONSTANTS,1)[0]
        self.random_scale=struct.unpack('<f',self.x.read(self.a['random_scale'],4))[0]
        self.setup_state=dict(initial=initial,spawn_count=count,players=list(players),
            objects=list(objects),recent_deaths=list(recent_deaths),
            spawning_player_datum='00010000',active_player_bitmap=f'{self.u(BITMAP):08x}',
            options_state=self.u(OPTIONS+8),options_mode=int(self.g.mem_read(OPTIONS+12,1)[0]),
            options_10=f'{self.u(OPTIONS+0x10):08x}',options_14=f'{self.u(OPTIONS+0x14):08x}',
            options_team_mode=False,maximum_random_bias=self.bias,
            random_bias_overridden=random_bias is not None,starts_overridden=starts is not None,
            spawn_settings=self.floats(self.a['settings_data'],38))

    def choose(self, deterministic=True, preview=False, blocked=None):
        self.current_scores=[];self.collision_queries=[];self.collected=[]
        self.blocked=set(blocked or ())
        count=self.h(PLAYERS+0x218)
        private_initial=((1^self.u(OPTIONS+0x10)^self.u(OPTIONS+0x14))+count)&0xffffffff
        global_initial=self.u(SEED+4)
        tag=1 if blocked is not None else 0xffffffff
        actual=self.call(self.a['choose'],(tag,int(deterministic),int(preview)),[(UC_X86_REG_ECX,0x10000)])
        actual=actual if actual<0x80000000 else actual-0x100000000
        assert len(self.current_scores)==len(self.start_points)
        seed=private_initial if deterministic else global_initial
        best=-1000.0;expected=-1
        for row,point in zip(self.current_scores,self.start_points):
            seed=advance(seed)
            assert row['private_seed']==f'{seed if deterministic else private_initial:08x}'
            assert row['global_spawn_seed']==f'{global_initial if deterministic else seed:08x}'
            score=f32(expected_sum(point,self.collected)+self.bias*((seed>>16)*self.random_scale))
            assert row['bits']==bitstr(score),(self.version,row,score,self.collected)
            if score>best and row['index'] not in self.blocked:
                best=score;expected=row['index']
        assert actual==expected,(actual,expected)
        expected_count=((count+int(actual!=-1 and not preview)+32768)&0xffff)-32768
        assert self.h(PLAYERS+0x218)==expected_count
        assert self.u(SEED)==0x12345678
        assert self.u(SEED+4)==(global_initial if deterministic else seed)
        return dict(input_state=self.setup_state,selected=actual, selected_score_bits=bitstr(best),
            deterministic=deterministic, preview=preview, collision_tag_none=blocked is None,
            blocked_indices=sorted(self.blocked), private_initial_seed=f'{private_initial:08x}',
            global_initial_seed=f'{global_initial:08x}', final_seed=f'{seed:08x}',
            global_final_seed=f'{self.u(SEED+4):08x}', weapon_seed=f'{self.u(SEED):08x}',
            spawn_count_before=count,spawn_count_after=self.h(PLAYERS+0x218),
            influence_count=len(self.collected), influences=self.collected,
            influence_bits=[[bitstr(v) for v in p] for p in self.collected],
            scores=self.current_scores,collision_queries=self.collision_queries)

    def kernel_cases(self):
        cases=[]
        base=[0,0,0,2,4,1,3,-5]
        positions=[(0,0,0),(2,0,0),(3,0,0),(4,0,0),(-4,0,0),
                   (1,0,1),(1,0,f32(1+2**-23)),(1,0,-3),(1,0,f32(-3-2**-22)),
                   (3,0,-2),(0,3,0),(2,2,0)]
        self.w(self.a['engine_globals'],ENGINE_GLOBALS);self.w(ENGINE_GLOBALS+0xc14,2)
        self.w(self.a['engine_table']+8,self.a['engine'])
        for n,point in enumerate(positions):
            for influences in ([base],[base,[0,0,0,1,5,2,2,3]],[[0,0,0,4,2,1,3,-5]]):
                self.w(LIST,len(influences))
                self.g.mem_write(LIST+4,b''.join(struct.pack('<8f',*row) for row in influences))
                self.g.mem_write(POINT,struct.pack('<3f',*point))
                self.call(self.a['sum'],registers=[(UC_X86_REG_EAX,LIST),(UC_X86_REG_EDX,POINT)])
                bits=self.g.reg_read(UC_X86_REG_XMM0)&0xffffffff
                expected=expected_sum(point,influences)
                assert bits==int(bitstr(expected),16)
                cases.append(dict(point=list(point),influences=influences,score=expected,bits=f'{bits:08x}'))
        return cases

    def property_cases(self, cache):
        assert self.version=='tu11'
        cases=[]
        for phase,teams,requested in [(1,False,0),(1,False,-1),(1,True,99),(1,True,-2),(0,False,0)]:
            self.setup(cache)
            self.g.mem_write(0x4d5188,bytes(0x130));self.w(0x4d517c,0)
            self.w(SESSION+0x741c,5);self.w(SESSION+0x14,phase)
            self.w(SESSION+0x4db0,1);self.w(SESSION+0x111c,0x8009)
            self.g.mem_write(SESSION+0x4df8,bytes([int(teams)]))
            for i in (0,3,15):
                self.g.mem_write(SESSION+0x1138+i*0x13c+0x7c,bytes([requested&255]))
                self.g.mem_write(SESSION+0x11c8+i*0x13c+0x7c,bytes([6]))
            self.call(0x65dd0,(SESSION,))
            records=[]
            for i in (0,3,15):
                team=self.g.mem_read(SESSION+0x11c8+i*0x13c+0x7c,1)[0]
                expected=(6 if phase!=1 else 255 if requested==-1 else max(0,min(7,requested)) if teams else i)
                assert team==expected
                self.g.mem_write(CONFIG,bytes(self.g.mem_read(SESSION+0x11c8+i*0x13c,0x90)))
                # Exact config-copy prefix; notification/color/name events after
                # the copy are outside this state-mapping proof.
                self.call(0x14d600,(0x10000,),[(UC_X86_REG_EAX,CONFIG)],end=0x14d687)
                actual=self.g.mem_read(PLAYERS+0xc0,1)[0]
                assert actual==team
                records.append(dict(membership_slot=i,assigned_property_7c=team,player_c0_after_copy=actual))
            cases.append(dict(session_state=5,session_phase_14=phase,parameters_team_flag=teams,
                requested_team=requested,current_team=6,active_membership_mask='00008009',records=records))
        return cases


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/spawn-source/report.json')
    args=parser.parse_args()
    oracles={name:Oracle(name) for name in VERSIONS}
    kernels={name:o.kernel_cases() for name,o in oracles.items()}
    paths=[ROOT/f'assets/disc/maps/{name}.map' for name in MAPS]
    paths+=sorted((ROOT/'assets/multiplayer-map-pack/demos/dlc').glob('*/*.map'))
    maps=[];rows=[]
    properties=[]
    for path in paths:
        cache=Cache(path)
        digest=hashlib.sha256(cache.data).hexdigest()
        if path.name=='lockout.map':assert digest==CACHE_PIN
        scenarios=[('empty_initial',dict(initial=True)),('empty_respawn',dict(initial=False)),
            ('enemy_at_start0_initial',dict(initial=True,players=[(1,tuple(struct.unpack_from('<3f',cache.data,cache.block(cache.scenario+0x100,0x34)[0])),False)])),
            ('enemy_at_start0_respawn',dict(initial=False,players=[(1,tuple(struct.unpack_from('<3f',cache.data,cache.block(cache.scenario+0x100,0x34)[0])),False)]))]
        for case,setup in scenarios:
            results={}
            for name,o in oracles.items():
                o.setup(cache,**setup);results[name]=o.choose()
            assert results['base']==results['tu11'],(path.name,case)
            rows.append(dict(map=path.name,case=case,**results['tu11']))
        s=cache.scenario;p=cache.block(s+0x318,0x60)
        matg=next(t['offset'] for t in cache.tags.values() if t['group']=='matg')
        mulg=cache.tags[cache.u(matg+0x16c)]['offset']
        runtime=cache.block(mulg+8,0x568)[0]
        constants=cache.block(runtime+0x530,0x160)[0]
        authored=dict(runtime_file_offset=f'{runtime:08x}',constants_file_offset=f'{constants:08x}',
            dynamic_upper=cache.f(runtime+0x150),dynamic_lower=cache.f(runtime+0x154),
            types=[dict(index=i,file_offset=f'{runtime+0x180+i*0x1c:08x}',
                inner=cache.f(runtime+0x180+i*0x1c),outer=cache.f(runtime+0x184+i*0x1c),
                weight=cache.f(runtime+0x188+i*0x1c)) for i in range(12)],
            constants_first_11=list(struct.unpack_from('<11f',cache.data,constants)))
        if p:
            at=p[0]
            authored['scenario_heights']=list(struct.unpack_from('<2f',cache.data,at))
            authored['overrides']=[dict(file_offset=f'{q:08x}',type=cache.h(q),
                inner=cache.f(q+4),outer=cache.f(q+8),weight=cache.f(q+12))
                for q in cache.block(at+0x48,0x10)]
            for name,off in [('initial_zones',0x58),('respawn_zones',0x50)]:
                authored[name]=[dict(file_offset=f'{q:08x}',name_stringid=f'{cache.u(q):08x}',
                    teams=f'{cache.u(q+4):08x}',games=f'{cache.u(q+8):08x}',flags=f'{cache.u(q+12):08x}',
                    position=list(struct.unpack_from('<3f',cache.data,q+0x10)),
                    lower=cache.f(q+0x1c),upper=cache.f(q+0x20),inner=cache.f(q+0x24),
                    outer=cache.f(q+0x28),weight=cache.f(q+0x2c)) for q in cache.block(at+off,0x30)]
        maps.append(dict(map=path.name,path=str(path.relative_to(ROOT)),sha256=digest,
            scenario_file_offset=f'{s:08x}',start_file_offsets=[f'{p:08x}' for p in cache.block(s+0x100,0x34)],
            start_positions=[list(struct.unpack_from('<3f',cache.data,q))
                for q in cache.block(s+0x100,0x34)],
            spawn_data_file_offset=f'{p[0]:08x}' if p else None,authored=authored))
        if path.name=='lockout.map':
            o=oracles['tu11'];point=list(struct.unpack_from('<3f',cache.data,cache.block(s+0x100,0x34)[0]))
            properties=o.property_cases(cache)
            fixtures=[('global_rng',dict(initial=False),dict(deterministic=False)),
                ('private_rng_preview',dict(initial=False,count=3),dict(preview=True)),
                ('signed_spawn_count_negative',dict(initial=False,count=-1),{}),
                ('signed_spawn_count_wrap',dict(initial=False,count=32767),{}),
                ('blocked_all',dict(initial=False),dict(blocked=range(len(cache.block(s+0x100,0x34))))),
                ('blocked_random_best',dict(initial=False),dict(blocked=[7])),
                ('enemy_dead_unit',dict(initial=False,players=[(1,point,True)]),{}),
                ('friend_live',dict(initial=False,players=[(0,point,False)]),{}),
                ('death_history',dict(initial=False,players=[(1,point,True)],recent_deaths=[(0,point,150)]),{}),
                ('death_history_zero_timer',dict(initial=False,recent_deaths=[(0,point,0)]),{}),
                ('death_history_negative_timer',dict(initial=False,recent_deaths=[(0,point,-1)]),{}),
                ('projectile_danger',dict(initial=False,objects=[dict(type=5,position=point,velocity=(1,2,3))]),{}),
                ('empty_vehicle',dict(initial=False,objects=[dict(type=1,position=point)]),{}),
                ('moving_empty_vehicle',dict(initial=False,objects=[dict(type=1,position=point,velocity=(2,0,0))]),{}),
                ('equal_scores_authored_order',dict(initial=False,random_bias=0),{}),
                ('zero_starts',dict(initial=False,starts=[]),{}),
                ('threshold_negative',dict(initial=False,starts=[point],random_bias=-2000),{})]
            for case,setup,choose in fixtures:
                o.setup(cache,**setup);result=o.choose(**choose)
                rows.append(dict(map=path.name,case=case,fixture_overrides=setup,**result))
        cache.data.close();cache.stream.close()
    report=dict(passed=True,oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        dependencies=dict(python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__),
        fixture_cpu_state=dict(x87_control_word='027f',x87_status_word='0000',
            x87_tag_word='ffff',mxcsr='00001f80'),
        authorities={name:dict(path=v['path'],sha256=v['pin']) for name,v in VERSIONS.items()},
        scope='Original settings/collector/iterators/Slayer callbacks/sum/random/selection instructions are unchanged. State pools are fixtures; normal chooser calls supply collision tag NONE. Collision cases supply recorded shape results. No full-world collision, global RNG startup, integrated match/world scheduling, team mode, objective engine or live Xbox parity is proved.',
        kernel_case_count=sum(len(v) for v in kernels.values()),kernels=kernels,
        authored_map_count=len(maps),paired_base_tu_chooser_case_count=len(maps)*4,
        maps=maps,chooser_case_count=len(rows),rows=rows)
    report['player_property_assignment_cases']=properties
    args.output.parent.mkdir(parents=True,exist_ok=True)
    md=Cs(CS_ARCH_X86,CS_MODE_32)
    listings={
        'base':[(0x14ef00,0x14f040),(0x23aea0,0x23b170),(0x23b170,0x23b500),
                (0x23b500,0x23b8e0),(0x23b8e0,0x23bc90),(0x24dc50,0x24dc53),(0xa5750,0xa5760)],
        'tu11':[(0x150640,0x150780),(0x150a4b,0x150a82),(0x23cfa0,0x23d270),
                (0x23d270,0x23d600),(0x23d600,0x23d9e0),(0x23d9e0,0x23dd90),
                (0x37eb40,0x37eb96),(0x2b4590,0x2b4593),(0xa1110,0xa1120),
                (0x65dd0,0x66170),(0x14d600,0x14d6f0),(0x14d210,0x14d350),
                (0x14d350,0x14d3d0),
                (0x14d800,0x14dda0),(0x150d2f,0x150dc0)]}
    for name,ranges in listings.items():
        o=oracles[name];lines=[f'{name} {o.a["pin"]}']
        for start,end in ranges:
            lines.append(f'\nRANGE {start:08x}..{end:08x} SHA256 {hashlib.sha256(o.x.read(start,end-start)).hexdigest()}')
            lines.extend(f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in md.disasm(o.x.read(start,end-start),start))
        (args.output.parent/f'{name}-instructions.txt').write_text('\n'.join(lines)+'\n')
    args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','kernel_case_count','authored_map_count','paired_base_tu_chooser_case_count','chooser_case_count')}))

if __name__=='__main__':main()

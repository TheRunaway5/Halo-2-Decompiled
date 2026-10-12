#!/usr/bin/env python3
"""Original early-player queue ordering with explicit pending-player fixtures.

The unchanged active caller, full early-player stage, eligibility functions,
normal spawn and chooser execute. The authored-start count is explicitly zero,
so original spawn fails before creation and no unit/head pose is supplied.
Input gathering, representation and later-world services retain the timing
oracle's boundaries. This proves queue attempts/RNG, not successful world spawn.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import capstone
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX

from oracle_mp_spawn import ROOT, VERSIONS, Cache, CACHE_PIN, PLAYERS, BITMAP, SEED, SCENARIO
from oracle_mp_spawn_lifecycle import TIME
from oracle_mp_spawn_timing import TimingOracle, i32


def source_queue_model(queue,seed):
    queue=list(queue);steps=[]
    while queue:
        before=seed
        if len(queue)>1:
            seed=(seed*0x19660d+0x3c6ef35f)&0xffffffff
            index=(seed>>16)%len(queue)
        else:index=0
        chosen=queue[index]
        steps.append(dict(queue_before=[f'{d:08x}' for d in queue],
            seed_before=f'{before:08x}',seed_after=f'{seed:08x}',
            index=index,chosen=f'{chosen:08x}',draw=len(queue)>1))
        queue[index]=queue[-1];queue.pop()
    return steps,seed


class QueueOracle(TimingOracle):
    def __init__(self,version):
        super().__init__(version)
        md=Cs(CS_ARCH_X86,CS_MODE_32)
        self.spawn_return=next(i.address+i.size for i in md.disasm(
            self.x.read(self.timing['early'],0x200),self.timing['early'])
            if i.mnemonic=='call' and int(i.op_str,16)==self.owner['spawn'])
        self.attempts=[];self.pending_attempt=None

    def player_state(self,datum):
        p=PLAYERS+(datum&0xffff)*0x21c
        return dict(full_player=f'{(self.h(p)&0xffff)<<16|(datum&0xffff):08x}',
            c0=bytes(self.g.mem_read(p+0xc0,1))[0],flags=f'{self.h(p+2)&0xffff:04x}',
            current=f'{self.u(p+0x2c):08x}',previous=f'{self.u(p+0x30):08x}',
            countdown=i32(self.u(p+0x164)),penalty=i32(self.u(p+0x168)),
            displayed_seconds=i32(self.u(p+0x170)),spawn_count=self.h(p+0x218))

    def hook(self,uc,address,size,unused):
        if hasattr(self,'spawn_return'):
            if address==self.owner['spawn']:
                datum=self.u(uc.reg_read(UC_X86_REG_ESP)+4)
                self.pending_attempt=dict(player=f'{datum:08x}',game_tick=self.u(TIME+8),
                    before=self.player_state(datum),weapon_rng=f'{self.u(SEED):08x}',
                    spawn_rng=f'{self.u(SEED+4):08x}')
            if address==self.spawn_return:
                row=self.pending_attempt;assert row is not None
                row.update(result=uc.reg_read(UC_X86_REG_EAX)&255,
                    after=self.player_state(int(row['player'],16)))
                self.attempts.append(row);self.pending_attempt=None
        super().hook(uc,address,size,unused)

    def queue_case(self,cache,name,slots,seed=0x12345678,clock=0,initial=True,countdowns=None,ticks=1):
        self.fixture(cache)
        self.w(BITMAP,sum(1<<s for s in slots))
        self.w(SEED,seed);self.w(SCENARIO+0x100,0);self.w(TIME+8,clock)
        datums=[]
        for slot in slots:
            p=PLAYERS+slot*0x21c;salt=7+5*slot;datum=salt<<16|slot
            self.sh(p,salt);self.sh(p+2,9 if initial else 1);self.sh(p+0x1ac,-1)
            self.w(p+0x164,(countdowns or {}).get(slot,0))
            self.w(p+0x168,0);self.w(p+0x170,0);self.sh(p+0x218,0)
            datums.append(datum)
            self.watch.update({p+off:f'player{slot}.{label}' for off,label in
                ((0x164,'countdown'),(0x218,'spawn_count'),(2,'flags'),(0x2c,'current'))})
        self.watch[SEED]='global_weapon_rng'
        source_rows=[]
        for _ in range(ticks):
            self.attempts=[];self.pending_attempt=None
            before={f'{d:08x}':self.player_state(d) for d in datums}
            seed_before=self.u(SEED);spawn_before=self.u(SEED+4)
            row=self.tick()
            scan=[e for e in self.eligibility if e['stage']=='scan']
            assert [e['player'] for e in scan]==[f'{d:08x}' for d in datums]
            queued=[int(e['player'],16) for e in scan if e['result']]
            model,expected_seed=source_queue_model(queued,seed_before)
            assert [a['player'] for a in self.attempts]==[s['chosen'] for s in model]
            assert [a['weapon_rng'] for a in self.attempts]==[s['seed_after'] for s in model]
            assert self.u(SEED)==expected_seed and self.u(SEED+4)==spawn_before
            rng_writes=[w for w in self.writes if w['field']=='global_weapon_rng']
            assert [w['value'] for w in rng_writes]==[s['seed_after'] for s in model if s['draw']]
            rechecks=[e for e in self.eligibility if e['stage']=='recheck']
            assert [e['player'] for e in rechecks]==[a['player'] for a in self.attempts]
            assert all(e['result'] and e['before']==e['after']==0 for e in rechecks)
            assert all(a['result']==0 and a['before']==a['after'] for a in self.attempts)
            after={f'{d:08x}':self.player_state(d) for d in datums}
            assert all(s['current']=='ffffffff' and s['spawn_count']==0 for s in after.values())
            assert all(bool(int(s['flags'],16)&8)==initial for s in after.values())
            assert not self.marker_calls and not self.collision_queries and not self.shape_requests
            source_rows.append(dict(game_tick_before=row['before']['game_tick'],
                game_tick_after=row['after']['game_tick'],players_before=before,players_after=after,
                eligibility=self.eligibility,source_spawn_attempts=self.attempts,
                source_rng_writes=rng_writes,source_notifications=self.notices,
                literal_queue_model=model,weapon_rng_after=f'{self.u(SEED):08x}',
                spawn_rng_after=f'{self.u(SEED+4):08x}'))
        return dict(case=name,slots=slots,initial_flag=initial,input_clock=clock,
            input_weapon_rng=f'{seed:08x}',input_countdowns=countdowns or {},
            authored_start_count_override=0,source_ticks=source_rows)

    def cases(self,cache):
        rows=[]
        for slots in ([0,3],[0,3,15],list(range(16))):
            for seed in (0,1,0x12345678):
                rows.append(self.queue_case(cache,f'initial_{len(slots)}_players_seed_{seed:08x}',slots,seed))
        rows.append(self.queue_case(cache,'respawn_three_at_start',[0,3,15],initial=False))
        rows.append(self.queue_case(cache,'one_player_no_queue_draw',[3]))
        rows.append(self.queue_case(cache,'negative_countdowns_no_queue',[0,3,15],countdowns={0:-1,3:-1,15:-1}))
        rows.append(self.queue_case(cache,'positive_countdown_admission',[0,3,15],countdowns={0:2,3:1,15:0},ticks=2))
        rows.append(self.queue_case(cache,'failed_initial_attempts_retry',[0,3,15],ticks=5))
        rows.append(self.queue_case(cache,'sixteen_at_elapsed_three',list(range(16)),clock=3))
        rows.append(self.queue_case(cache,'sixteen_at_elapsed_four',list(range(16)),clock=4))
        rows.append(self.queue_case(cache,'sixteen_respawns_at_elapsed_thirtyfive',list(range(16)),clock=35,initial=False))
        return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/spawn-source/queue-report.json')
    args=parser.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    oracles={v:QueueOracle(v) for v in VERSIONS}
    rows={v:o.cases(cache) for v,o in oracles.items()}
    def clean(x):
        if isinstance(x,dict):return {k:clean(v) for k,v in x.items() if k!='instruction'}
        if isinstance(x,list):return [clean(v) for v in x]
        return x
    assert clean(rows['base'])==clean(rows['tu11'])
    report=dict(passed=True,oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        imported_tool_sha256={n:hashlib.sha256((ROOT/f'tools/{n}.py').read_bytes()).hexdigest() for n in
            ('oracle_mp_spawn','oracle_mp_spawn_lifecycle','oracle_mp_spawn_ownership','oracle_mp_spawn_timing')},
        dependencies=dict(python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__),
        authorities={v:dict(path=a['path'],sha256=a['pin']) for v,a in VERSIONS.items()},
        scope=__doc__,cache=dict(path='assets/disc/maps/lockout.map',sha256=CACHE_PIN),
        service_boundaries=['Authored-start count is explicitly overridden to zero; original spawn returns false before creation.',
            'No unit/head transform is supplied; current unit is NONE and death history is empty for every pending player.',
            'Input gathering, representation and later-world services retain frozen timing-oracle boundaries.',
            'Eligibility, queue selection/removal, RNG writes, complete no-starts spawn and footer execute unchanged.'],
        case_count=sum(len(r) for r in rows.values()),
        tick_count=sum(len(c['source_ticks']) for r in rows.values() for c in r),
        spawn_attempt_count=sum(len(t['source_spawn_attempts']) for r in rows.values() for c in r for t in c['source_ticks']),
        rows=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','case_count','tick_count','spawn_attempt_count')}))


if __name__=='__main__':main()

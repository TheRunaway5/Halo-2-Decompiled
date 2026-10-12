#!/usr/bin/env python3
"""Source weapon interaction candidates and type-20 requests on real tag data.

Runtime positions/radius, spatial candidates, collision visibility and animation
services are explicit fixture inputs. Selection/admission/ranking and action
request assembly execute unchanged original TU1.1 instructions.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_mp_inventory_1_1 import (
    InventoryTrace, ROOT, PIN, CACHE_PIN, SCRATCH, SECOND, THIRD, W2, W3,
    UNIT, WEAPON, WI, UI, POOL, GAME, HEADERS, SEED, STOP,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI,
    UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_ESP, _trace,
)

OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/target'
SMG = 'objects\\weapons\\rifle\\smg\\smg'
BR = 'objects\\weapons\\rifle\\battle_rifle\\battle_rifle'
BEST = SCRATCH + 0x300
MODE_OUT = SCRATCH + 0x400
ENTRIES = (0x1527c0, 0x152e90, 0x152a30, 0xbacc0, 0x153090,
           0xbbd10, 0x152080, 0xcec00, 0xceaf0, 0xe7f30, 0xea660,
           0xce510, 0xcfed0, 0x15b6d0, 0xc57a0, 0x139ff0,
           0x150f00, 0x1884e0, 0xa1110, 0x9d5a0, 0x2c22a0,
           0xbede0, 0xbeb30, 0x1441d0, 0xbdf10, 0xbdf80, 0xb8a30,
           0xe0c20, 0xfbfa0, 0x16bac0, 0x16b240, 0x14e780, 0x14eae0)


class TargetTrace(InventoryTrace):
    def __init__(self, target=BR, trigger=0):
        super().__init__()
        self.inventory_services()
        self.target_events = []
        self.admission_returns = []
        self.spatial = [W2]
        self.collision_results = None
        self.w(POOL + 0x38, 4)
        self.world_weapon(WI, WEAPON, SMG)
        self.world_weapon(W2, SECOND, target)
        self.world_weapon(W3, THIRD, BR)
        # Original mode-2 engine table, including original unconditional pickup
        # admission slot 16. Runtime engine object address remains a fixture.
        self.w(SCRATCH, 0x463b48)
        self.w(UNIT + 0x13c, 0)
        self.services[0xc57a0] = ('supplied collision visibility gate: clear', 1, 0)
        self.action('attach_initial_smg_with_player_adjustment', 0xce510, (UI, WI, 1))
        self.g.mem_write(WEAPON + 0x20c, bytes([trigger]))
        # Runtime radius is an explicit input, not inferred from character art
        # or a native capsule. Position and transformed center are distinct.
        self.f(UNIT + 0x3c, 0.5)
        for obj, x in ((UNIT, 0.0), (SECOND, 0.125), (THIRD, 0.25)):
            for field in (0x30, 0x64):
                self.f(obj + field, x)
                self.f(obj + field + 4, 0)
                self.f(obj + field + 8, 0)
        self.g.mem_write(BEST, bytes(4) + b'\xff'*8)

    def hook(self, g, a, size, user):
        for ret, result in self.admission_returns[:]:
            if ret == a:
                result['accepted'] = bool(g.reg_read(UC_X86_REG_EAX) & 255)
                result['modes'] = list(g.mem_read(result.pop('_out'), 4))
                self.admission_returns.remove((ret, result))
        if a == 0xcec00:
            sp = g.reg_read(UC_X86_REG_ESP)
            result = dict(phase=self.active, entry='000cec00',
                          unit=f'{self.u(sp+4):08x}', weapon=f'{self.u(sp+8):08x}',
                          _out=self.u(sp+12))
            self.target_events.append(result)
            self.admission_returns.append((self.u(sp), result))
        if a == 0xe7f30:
            request = g.reg_read(UC_X86_REG_EDI)
            self.target_events.append(dict(phase=self.active, entry='000e7f30',
                                          unit=f'{g.reg_read(UC_X86_REG_EAX):08x}',
                                          type=self.u(request), weapon=f'{self.u(request+4):08x}',
                                          mode=self.u(request+8, 2),
                                          request_hex=bytes(g.mem_read(request, 32)).hex()))
        if a == 0xbbf40:
            sp = g.reg_read(UC_X86_REG_ESP)
            mask = self.u(sp+8)
            out, maximum = self.u(sp+24), self.u(sp+28)
            values = self.spatial[:maximum] if mask & 4 else []
            for n, value in enumerate(values):
                self.w(out+n*4, value)
            self.target_events.append(dict(phase=self.active, supplied=True, entry='000bbf40',
                                          description='supplied spatial index enumeration',
                                          type_mask=f'{mask:08x}', origin=self.vec(self.u(sp+16)),
                                          radius_bits=f'{self.u(sp+20):08x}', maximum=maximum,
                                          returned=[f'{v:08x}' for v in values]))
            self.services[a] = ('supplied spatial index enumeration', len(values), 28)
        if a == 0x16b240 and self.collision_results is not None:
            sp=g.reg_read(UC_X86_REG_ESP)
            result=self.collision_results.pop(0) if len(self.collision_results)>1 else self.collision_results[0]
            out=self.u(sp+24)
            self.g.mem_write(out,bytes(0x5c))
            self.f(out+4,1.0)
            # Supplied hit position is the requested endpoint. This is a
            # declared collision fixture result, not a world intersection.
            start=struct.unpack('<3f',g.mem_read(self.u(sp+8),12))
            delta=struct.unpack('<3f',g.mem_read(self.u(sp+12),12))
            for n in range(3):self.f(out+8+n*4,start[n]+delta[n])
            self.target_events.append(dict(phase=self.active,supplied=True,entry='0016b240',
                                          type_mask=f'{self.u(sp+4):08x}',start=self.vec(self.u(sp+8)),
                                          delta=self.vec(self.u(sp+12)),ignored_objects=[f'{self.u(sp+16):08x}',f'{self.u(sp+20):08x}'],
                                          collision_hit=bool(result),supplied_point=self.vec(out+8)))
            self.services[a]=('supplied collision segment result with endpoint fixture',result,24)
        super().hook(g, a, size, user)

    def candidate(self):
        return dict(priority=struct.unpack('<h', self.g.mem_read(BEST,2))[0],
                    modes_flags=self.u(BEST+2,1), object=f'{self.u(BEST+4):08x}',
                    bytes=bytes(self.g.mem_read(BEST,12)).hex())

    def choose(self, name='choose_weapon_candidate'):
        self.action(name, 0x1527c0, (0, BEST))
        return self.candidate()

    def request(self, primary=True, name='request_candidate_pickup'):
        start = len(self.target_events)
        self.action(name, 0x153090, (int(primary),),
                    ((UC_X86_REG_EAX, 0), (UC_X86_REG_ECX, BEST)))
        return dict(accepted=bool(self.g.reg_read(UC_X86_REG_EAX)&255),
                    emitted=[e for e in self.target_events[start:] if e['entry']=='000e7f30'])

    def bounds(self, index, obj, scale=1.0):
        self.f(obj+0xa0,scale)
        self.sh(obj+0x116,0x500)
        self.g.mem_write(obj+0x70,struct.pack('<3f',1,0,0))
        self.g.mem_write(obj+0x7c,struct.pack('<3f',0,0,1))
        self.action('build_supplied_root_pose_with_original_math',0xbeb30,
                    (obj+0x64,struct.unpack('<I',struct.pack('<f',scale))[0],0,0,obj+0x500),
                    ((UC_X86_REG_EAX,obj+0x7c),(UC_X86_REG_ECX,obj+0x70),(UC_X86_REG_EBX,0)))
        self.action('original_metadata_bounding_sphere',0xbede0,(index,))
        definition=self.cache.tags[self.u(obj)]
        p=definition['offset']
        expected=struct.unpack('<f',struct.pack('<f',scale*self.cache.f(p+4)))[0]
        assert self.fv(obj+0x3c)==expected
        return dict(definition=definition,runtime_scale=scale,
                    authored_radius=dict(file_offset=f'{p+4:08x}',bits=f'{self.cache.u(p+4):08x}',value=self.cache.f(p+4)),
                    authored_bounding_offset=[self.cache.f(p+8+n*4) for n in range(3)],
                    runtime_bounds_center=self.vec(obj+0x30),
                    runtime_radius=dict(bits=f'{self.u(obj+0x3c):08x}',value=self.fv(obj+0x3c)),
                    root_pose_hex=bytes(self.g.mem_read(obj+0x500,52)).hex())


def run_cases():
    traces, results = [], []
    def save(t, name, **fields):
        assert not t.admission_returns
        traces.append(t)
        results.append(dict(name=name, **fields, candidate=t.candidate(),
                            final_inventory=t.inventory_state(), rows=t.inventory_rows,
                            target_events=t.target_events, events=t.events,
                            invoked_supplied_services=sorted({e['service'] for e in t.events if 'service' in e})))
    for target in (BR, SMG):
        for trigger in (0,1,2):
            for primary in (True, False):
                t=TargetTrace(target,trigger)
                candidate=t.choose()
                request=t.request(primary)
                if target==BR:
                    assert candidate['priority']==1
                    assert candidate['modes_flags']==(1 if trigger==0 else 2)
                    assert request['accepted']==primary
                    if primary:
                        assert request['emitted'][0]['mode']==(3 if trigger==0 else 4)
                        assert request['emitted'][0]['weapon']==f'{W2:08x}'
                save(t,f'{target.rsplit(chr(92),1)[-1]}_trigger_{trigger}_primary_{primary}',
                     target_definition=target, initial_trigger=trigger, primary=primary,
                     chosen_before_request=candidate, request=request)
    for kind in ('inside','exact_boundary','outside','inventory_flag','last_holder','blocked_collision',
                 'same_distance_aim_reverse','nearer_center','equal_center','higher_priority_occupied',
                 'free_slot','zero_salt','stale_salt','wrong_object_type','owned','bad_priority'):
        t=TargetTrace()
        radius=struct.unpack('<f',struct.pack('<f',0.5+t.fv(0x452000)))[0]
        if kind=='inside':t.f(SECOND+0x30,radius-0.0001)
        if kind=='exact_boundary':t.f(SECOND+0x30,radius)
        if kind=='outside':t.f(SECOND+0x30,radius+0.0001)
        if kind=='inventory_flag':t.w(SECOND+0x12c,1)
        if kind=='last_holder':t.w(SECOND+0x14c,UI)
        if kind=='blocked_collision':t.services[0xc57a0]=('supplied collision visibility gate: blocked',0,0)
        if kind=='same_distance_aim_reverse':t.f(UNIT+0x168,-1)
        if kind in ('nearer_center','equal_center','higher_priority_occupied'):
            t.spatial=[W2,W3]
            # The nearer transformed center wins even with the farther position.
            t.f(THIRD+0x64,0.05 if kind=='nearer_center' else 0.125)
        candidate=t.choose()
        expected=0 if kind in ('exact_boundary','outside','inventory_flag','last_holder','blocked_collision') else 1
        assert candidate['priority']==expected
        if kind=='nearer_center':assert candidate['object']==f'{W3:08x}'
        if kind=='equal_center':assert candidate['object']==f'{W2:08x}'
        if kind=='higher_priority_occupied':
            t.sh(BEST,8)
            t.action('lower_priority_weapon_preserves_existing_target',0x152e90,(0,BEST),((UC_X86_REG_EAX,W3),))
            assert t.candidate()['priority']==8
        if kind=='free_slot':t.w(POOL+0x38,2)
        if kind=='zero_salt':t.sh(HEADERS+(W2&0xffff)*12,0)
        if kind=='stale_salt':t.w(BEST+4,0x20002)
        if kind=='wrong_object_type':t.g.mem_write(HEADERS+(W2&0xffff)*12+3,b'\x03')
        if kind=='owned':t.w(SECOND+0x154,UI)
        if kind=='bad_priority':t.sh(BEST,2)
        request=t.request()
        rejected=kind in ('exact_boundary','outside','inventory_flag','last_holder','blocked_collision',
                         'higher_priority_occupied','free_slot','zero_salt','stale_salt','wrong_object_type','owned','bad_priority')
        assert request['accepted']==(not rejected)
        if rejected:assert request['emitted']==[]
        save(t,kind, runtime_unit_radius=0.5, pickup_radius_bits=f'{struct.unpack("<I",struct.pack("<f",radius))[0]:08x}',
             chosen_before_request=candidate, request=request)
    for index,obj,name,scale in ((UI,UNIT,'stock_unit',1.0),(UI,UNIT,'scaled_unit',0.5),
                                (W2,SECOND,'stock_br',1.0),(W2,SECOND,'stock_smg',1.0)):
        t=TargetTrace(SMG if name=='stock_smg' else BR)
        bounds=t.bounds(index,obj,scale)
        save(t,name,bounds=bounds)
    for kind in ('inside','exact_boundary','outside'):
        t=TargetTrace()
        unit_bounds=t.bounds(UI,UNIT)
        t.bounds(W2,SECOND)
        radius=struct.unpack('<f',struct.pack('<f',t.fv(UNIT+0x3c)+t.fv(0x452000)))[0]
        # Change the already source-produced world bounding-center field as a
        # labeled geometric input to test the exact distance threshold.
        t.f(SECOND+0x30,t.fv(UNIT+0x30)+radius+(-0.0001 if kind=='inside' else 0.0001 if kind=='outside' else 0))
        t.f(SECOND+0x38,t.fv(UNIT+0x38))
        candidate=t.choose()
        assert candidate['priority']==(1 if kind=='inside' else 0)
        save(t,'authored_radius_'+kind,bounds=unit_bounds,
             pickup_radius_bits=f'{struct.unpack("<I",struct.pack("<f",radius))[0]:08x}',
             chosen_before_request=candidate)
    for kind,collision_sequence in (('first_clear',[0]),('second_clear',[1,0]),('all_blocked',[1])):
        t=TargetTrace()
        t.bounds(UI,UNIT)
        t.bounds(W2,SECOND)
        del t.services[0xc57a0]
        t.collision_results=collision_sequence[:]
        candidate=t.choose('choose_with_original_collision_visibility_geometry')
        assert candidate['priority']==(0 if kind=='all_blocked' else 1)
        collisions=[e for e in t.target_events if e['entry']=='0016b240']
        assert len(collisions)==(1 if kind=='first_clear' else 2 if kind=='second_clear' else 8)
        save(t,'original_visibility_'+kind,collision_results=collision_sequence,
             chosen_before_request=candidate,collision_segments=len(collisions))
    for target,flags in ((BR,2),(BR,0x20),(SMG,2),(SMG,0x20)):
        t=TargetTrace(target)
        candidate=t.choose()
        action_buffer=SCRATCH+0x700
        t.action('original_player_action_initialize',0x14e780,registers=((UC_X86_REG_EDX,action_buffer),))
        t.sh(action_buffer+0x1c,flags)
        t.fragment('original_candidate_player_action_marshalling',0x188a94,0x188aae,
                   ((UC_X86_REG_EBX,BEST),(UC_X86_REG_ESI,action_buffer)))
        assert bytes(t.g.mem_read(action_buffer+0x28,12))==bytes(t.g.mem_read(BEST,12))
        event_start=len(t.target_events)
        t.action('original_player_action_process_interaction',0x150f00,(0,UI,action_buffer))
        emitted=[e for e in t.target_events[event_start:] if e['entry']=='000e7f30' and e['type']==20]
        accepted=(target==BR and flags==2) or (target==SMG and flags==0x20)
        assert bool(emitted)==accepted
        assert t.u(action_buffer+0x1c,2)==(0 if target==BR and flags==2 else flags)
        if accepted:assert emitted[0]['weapon']==f'{W2:08x}' and emitted[0]['mode']==(3 if flags==2 else 5)
        save(t,f'player_action_{target.rsplit(chr(92),1)[-1]}_flags_{flags}',
             chosen_before_request=candidate,player_action_flags_before=flags,
             player_action_flags_after=t.u(action_buffer+0x1c,2),emitted=emitted)
    return traces, results


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,default=OUT/'report.json')
    args=parser.parse_args()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    traces,cases=run_cases()
    t=traces[0]
    instructions=[]
    directory=args.output.parent/'instructions'
    directory.mkdir(parents=True,exist_ok=True)
    executed=set().union(*(t.executed for t in traces))
    for entry in ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(ENTRIES))
        ins=[t.code.at(a) for a in sorted(routine.code)]
        path=directory/f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in ins)+'\n')
        instructions.append(dict(entry=f'{entry:08x}',listing=str(path.relative_to(ROOT)),
                                 sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                 instruction_count=len(ins),executed_instruction_count=sum(i.address in executed for i in ins),
                                 original_instruction_bytes_sha256=hashlib.sha256(b''.join(i.bytes for i in ins)).hexdigest()))
    report=dict(xbe_sha256=PIN,cache_sha256=CACHE_PIN,
                oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                inventory_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_mp_inventory_1_1.py').read_bytes()).hexdigest(),
                original_constants={f'{a:08x}':dict(bits=f'{t.u(a):08x}',value=t.fv(a))
                                    for a in (0x452000,0x4650d8,0x4651a0,0x465044)},
                original_engine_table=dict(address='00463b48',mode_query=f'{t.u(0x463b48):08x}',
                                           pickup_query=f'{t.u(0x463b48+0x40):08x}',
                                           bytes=bytes(t.g.mem_read(0x463b48,0xcc)).hex()),
                weapon_definition_type={name:t.cache.h(next(tag for tag in t.cache.tags.values()
                                                          if tag['group']=='weap' and tag['name']==name)['offset']+0x290)
                                        for name in (SMG,BR)},
                cases=cases,instructions=instructions,
                fixture='Original mode-2 engine vtable and its pickup leaf; player 0 owns fixture unit. '
                        'Runtime unit radius explicitly supplied 0.5; distinct source position/transformed center fields. '
                        'Spatial index enumeration and collision visibility supplied and labeled. '
                        'Original constructor and original player mode adjustments execute; animation/world services supplied.',
                scope='Complete original target scan/weapon distance-admission/ranking/valid-datum request/type20 pickup instructions. '
                      'Collision visibility, spatial enumeration, runtime bound construction, physics and animation timing excluded.')
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),routine_rows=sum(len(t.inventory_rows) for t in traces))))


if __name__=='__main__':main()

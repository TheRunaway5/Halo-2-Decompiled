#!/usr/bin/env python3
"""Separate unchanged TU1.1 equipped death cleanup continuation.

Real unit animation components, allocated inventory, cache animation and marker
poses are source-produced. Heap backing, resident delivery and world/audio
services remain explicit. Earlier frozen suites and production are untouched.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_mp_weapon_attachment_1_1 import AttachmentTrace,observe_pose
from oracle_smg_constructor_equipped_1_1 import ConstructorTrace
from oracle_spartan_equipped_unit_phase_1_1 import UnitPhaseTrace,MANAGER_OFFSET,REQUEST
from oracle_mp_inventory_1_1 import (ROOT,PIN,CACHE_PIN,UNIT,UI,POOL,HEADERS,
    TIME,SEED,STOP,SCRATCH,_trace,UC_X86_REG_EAX,UC_X86_REG_EBX,
    UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,
    UC_X86_REG_ESP,UC_X86_REG_EIP)
from oracle_smg_firing_1_1 import PLAYER_DATA,GAME

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/death-tail'
FROZEN={
 'analysis/multiplayer/integration/spawn-source/owner-counter-report.json':'e7492035911eb502c77b5154b3a35a9ad3c8decac254f3c5f18758d9a0d56dac',
 'analysis/multiplayer/integration/spawn-source/corpse-schedule-report.json':'649a0761e442ff5698476a18d4fcef36ebe89d4431ecb8619de497a46e357f6d',
 'analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json':'657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f'}
ENTRIES=(0xb78c0,0xd7e00,0xcbdc0,0xe6050,0xe5680,0xe6dc0,0x1e8e60,
    0xbc840,0x14e240,0x153a90,0xd22b0,0x116640,0x1045e0,
    0x1058f0,0x105750,0x106d40,0x104d30,0x104aa0,0x10a1d0,
    0x1042e0,0x108010,0x108230,0x109c30,
    0xd0140,0xcfd80,0xcfed0,0xcf8c0,0xcfb00,0xcff90,
    0xb8820,0xb8a30,0xbd170,0xbd270,0x16d2c0,0xfc1e0,
    0xfb120,0xfb360,0xfb3f0,0xfb490,0xb9430,0xb9c20,0x1dc8f0,0x1dc940,
    0xba980,0xba780,0xba7d0,0xb8490,0xb86c0,0x16b240,0x246a50,
    0xb8250,0xea440,0xe7ff0,0xe89f0,0xe9a90,0xe9d50,0xe9a50,0xeb830,0x115a00)
STAGES={0xcbf21:'tail_component_entry',0xcbf40:'selected_weapon_stop_loop',
    0xcbf8e:'clear_unit_flag21',0xcc005:'drop_unselected_inventory',
    0xcc019:'extra_weapon_reference_gate',0xcc04a:'grenade_drop',
    0xcc054:'selected_drop_actions_gate',0xcc06a:'action19',0xcc10e:'action9',
    0xcc1b2:'interrupt_actions',0xcc1d9:'clear_action26',
    0xcc20c:'clear_action27',0xcc23f:'clear_action22',0xcc254:'complete_return'}

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def s16(t,at):return struct.unpack('<h',t.g.mem_read(at,2))[0]

class DeathTailTrace(AttachmentTrace):
    control=UnitPhaseTrace.control
    weapon_snapshot=ConstructorTrace.weapon_snapshot

    def __init__(self,row=1,near=False):
        self.tail_active=False;self.tail_calls=[];self.tail_returns=[];self.tail_writes=[]
        self.stages=[];self.preparation=[]
        super().__init__(row,near)
        # Exact new-map garbage stores, without resetting the existing object
        # pools in the earlier portion of B78C0. Prefix-owned register values
        # are explicit slice inputs; complete world initialization is excluded.
        gp=self.u(0x4e58c4);garbage_before=bytes(self.g.mem_read(gp,0x84)).hex()
        self.fragment('original_new_map_garbage_clear_slice',0xb7972,0xb7995,
            ((UC_X86_REG_EAX,gp),(UC_X86_REG_EBX,0)))
        self.garbage_initialization=dict(start='000b7972',stop_exclusive='000b7995',
            registers=dict(eax=f'{gp:08x}',ebx='00000000'),before_hex=garbage_before,
            after_hex=bytes(self.g.mem_read(gp,0x84)).hex(),complete_world_initializer=False)
        assert self.u(gp+8)==0xffffffff and self.u(gp+4,2)==0

    def capture(self):
        return dict(weapon=self.weapon_snapshot(self.initial_datum),unit_manager=self.manager_state(),
            unit_object_400_hex=bytes(self.g.mem_read(UNIT,0x400)).hex())

    def compact(self):
        extra=UNIT+s16(self,UNIT+0x342);control=UNIT+s16(self,UNIT+0x346)
        return dict(tick=self.u(TIME+8),unit=f'{UI:08x}',dead=bool(self.u(UNIT+0x10a,1)&4),
            owner=f'{self.u(UNIT+0x13c):08x}',previous_owner=f'{self.u(UNIT+0x140):08x}',
            player_current=f'{self.u(PLAYER_DATA+0x2c):08x}',player_previous=f'{self.u(PLAYER_DATA+0x30):08x}',
            unit_flags=f'{self.u(UNIT+0x134):08x}',biped_state=self.u(UNIT+0x3dc,1),
            current=[struct.unpack('<b',self.g.mem_read(UNIT+0x212+i,1))[0]for i in range(2)],
            requested=[struct.unpack('<b',self.g.mem_read(UNIT+0x216+i,1))[0]for i in range(2)],
            slots=[f'{self.u(UNIT+0x218+i*4):08x}'for i in range(4)],
            extra_weapon=f'{self.u(UNIT+0x238):08x}',grenades=list(self.g.mem_read(UNIT+0x23e,2)),
            selected_drop_gate=self.u(UNIT+0x1f5,1),extra_mode=s16(self,extra+0xc),
            extra_sound=f'{self.u(extra+0x1c):08x}',control_flags=f'{self.u(control+4):08x}',
            control_byte_c=self.u(control+0xc,1),control_reference_2c=f'{self.u(control+0x2c):08x}',
            primary=f'{self.u(UNIT+MANAGER_OFFSET+4):08x}',
            secondary=f'{self.u(UNIT+s16(self,UNIT+0x33e)+0xa0):08x}',
            weapons=[dict(datum=f'{self.u(HEADERS+i*12,2)<<16|i:08x}',
                item_flags=f'{self.u(self.u(HEADERS+i*12+8)+0x12c,2):04x}',
                input=f'{self.u(self.u(HEADERS+i*12+8)+0x16e,2):04x}',
                input_byte=self.u(self.u(HEADERS+i*12+8)+0x170,1),
                state=self.u(self.u(HEADERS+i*12+8)+0x178),
                loaded=s16(self,self.u(HEADERS+i*12+8)+0x22c),reserve=s16(self,self.u(HEADERS+i*12+8)+0x22a),
                magazine_mode=s16(self,self.u(HEADERS+i*12+8)+0x224),
                transfer_ticks=s16(self,self.u(HEADERS+i*12+8)+0x230),
                total_transfer_ticks=s16(self,self.u(HEADERS+i*12+8)+0x232))
                for i in range(2,self.u(POOL+0x38))if self.u(HEADERS+i*12+3,1)==2 and self.u(HEADERS+i*12+8)],
            object_count=self.u(POOL+0x3c),high_water=self.u(POOL+0x38),
            shared_rng=f'{self.u(SEED):08x}',local_rng=f'{self.u(SEED+4):08x}')

    def state_full(self):
        compact=self.compact()
        compact.update(unit_prefix_hex=bytes(self.g.mem_read(UNIT,0x478)).hex(),
            player_record_hex=bytes(self.g.mem_read(PLAYER_DATA,0x21c)).hex(),
            manager=self.manager_state(),blocks={f'{off:03x}':dict(size=self.u(UNIT+off,2),
                offset=f'{self.u(UNIT+off+2,2):04x}',
                hex=bytes(self.g.mem_read(UNIT+self.u(UNIT+off+2,2),self.u(UNIT+off,2))).hex())for off in(0x33c,0x340,0x344)},
            unit_node_matrices_hex=bytes(self.g.mem_read(UNIT+self.u(UNIT+0x116,2),self.u(UNIT+0x114,2))).hex(),
            objects=[self.object_state(self.u(HEADERS+i*12,2)<<16|i)
                for i in range(2,self.u(POOL+0x38))if self.u(HEADERS+i*12,2)and self.u(HEADERS+i*12+8)],
            garbage_hex=bytes(self.g.mem_read(self.u(0x4e58c4),12)).hex())
        return compact

    def finish_tail_returns(self,a):
        for ret,row in reversed(self.tail_returns[:]):
            if ret==a:
                row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                row['after']=self.compact();self.tail_returns.remove((ret,row));break

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs);self.finish_tail_returns(STOP)

    def hook(self,g,a,size,user):
        if self.tail_active:
            self.finish_tail_returns(a);sp=g.reg_read(UC_X86_REG_ESP)
            if a in STAGES:self.stages.append(dict(stage=STAGES[a],entry=f'{a:08x}',state=self.compact()))
            if a in ENTRIES:
                row=dict(entry=f'{a:08x}',phase=self.active,return_address=f'{self.u(sp):08x}',
                    registers=self.registers(),stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)],before=self.compact())
                if a==0xea440:
                    request=self.u(sp+8);row['request_hex']=bytes(g.mem_read(request,32)).hex();row['action_type']=self.u(request)
                if a==0xe7ff0:
                    row['interrupt_type']=g.reg_read(UC_X86_REG_EDI)
                    row['interrupt_unit']=f'{g.reg_read(UC_X86_REG_ECX):08x}'
                if a==0xb8a30:row['descriptor_hex']=bytes(g.mem_read(self.u(sp+4),0xc4)).hex()
                self.tail_calls.append(row);self.tail_returns.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,a,size,value,user):
        if self.tail_active:
            record=None
            for i in range(self.u(POOL+0x38)):
                p=self.u(HEADERS+i*12+8)
                if not p:continue
                if p<=a<p+0x478:
                    off=a-p
                    if i==1 and ((off in(4,0x10,0x134,0x13c,0x140,0x238,0x2e0,0x3dc))or 0x210<=off<0x240):record=(i,off)
                    elif i>=2 and (off in(4,0xc,0x10,0x14,0x18,0x12c,0x14c,0x150,0x154)or
                        (self.u(HEADERS+i*12+3,1)==2 and 0x16c<=off<0x260)):record=(i,off)
                if i==1:
                    for f in (0x33e,0x342,0x346):
                        start=p+s16(self,p+f)
                        length={0x33e:0x124,0x342:0x58,0x346:0x44}[f]
                        if start<=a<start+length:record=(i,a-p)
                if record:break
            if record:
                i,off=record;self.tail_writes.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                    datum=f'{self.u(HEADERS+i*12,2)<<16|i:08x}',offset=f'{off:04x}',size=size,
                    before=bytes(g.mem_read(a,size)).hex(),value=f'{value&((1<<(size*8))-1):0{size*2}x}'))
        super().mem_write(g,access,a,size,value,user)

    def prepare_weapon(self,ticks,scenario):
        # Same bounded root ordering as the frozen allocated-SMG suite.
        self.services[0xb8a30]=('supplied failed projectile creation during preparation only',-1,4)
        self.services[0x1e20c0]=('supplied clear muzzle segment during preparation only',0,16)
        for tick in range(ticks):
            self.tick=tick;self.w(TIME+8,tick);before=self.capture()
            held=scenario=='held' or(scenario=='reload'and tick<45)
            control=self.control((0x10000 if held else 0)|(0x40000 if held and tick==0 else 0),
                buttons=1 if scenario=='reload'and tick==46 else 0)
            self.active='preparation_source_manager_blend';self.run(0x1d0ed0,regs=((UC_X86_REG_ESI,UNIT+MANAGER_OFFSET),))
            self.active='preparation_source_unit_wrapper';self.run(0xc7e90,(UI,),((UC_X86_REG_EAX,0xc7e90),))
            self.active='preparation_source_weapon_update';self.run(0x1021d0,(self.initial_datum,))
            self.active='preparation_source_action_dispatch';self.run(0xe7e60,(UI,))
            self.active='preparation_source_animation_update';self.run(0x112e10,(UI,REQUEST))
            self.active='preparation_source_pose';self.run(0xbdf80,(UI,))
            self.active='preparation_source_projectile_postpass';self.run(0x1019f0)
            self.preparation.append(dict(tick=tick,held=held,control=control,before=before,after=self.capture()))
        self.services.pop(0xb8a30);self.services.pop(0x1e20c0)

    def death(self):
        before=self.state_full();event_start=len(self.events);live_start=len(self.live_events)
        self.tail_active=True
        self.tick=max(self.tick+1,200);self.w(TIME+8,self.tick)
        self.action('source_body_depletion',0xd7e00,(UI,0xffffffff,1,0));after_depletion=self.state_full()
        # These semantic arguments match the frozen actual stock SMG fatal
        # cleanup: classNONE, knocked0, force0. The vector pointer is an explicit
        # null boundary; NONE class makes E6050 skip its optional vector copy.
        self.action('complete_source_death_cleanup',0xcbdc0,(UI,0xffffffff,0,0,0))
        after=self.state_full();self.tail_active=False
        assert self.g.reg_read(UC_X86_REG_EIP)==STOP and not self.tail_returns
        assert after['dead']and after['biped_state']==3 and after['player_current']=='ffffffff'
        assert after['owner']=='ffffffff'and after['current']==[-1,-1]
        assert after['slots']==['ffffffff']*4 and after['grenades']==[0,0]
        assert int(after['control_flags'],16)&(1<<26|1<<27|1<<22)==0
        assert after['manager']['secondary_channel_hex'][0:8]=='ffffffff'
        events=[{k:v for k,v in e.items()if k!='state'}for e in self.events[event_start:]]
        used={e['service']for e in events if 'service'in e}
        assert not used&{'000fb560','001dc8f0','001dc940','000b8a30','001e20c0'}
        selected=[before['slots'][slot]for slot in before['current']if slot!=-1]
        assert [r['stack'][:3]for r in self.tail_calls if r['entry']=='001045e0']==[
            [d,'00000000','00000000']for d in selected]
        assert [r['action_type']for r in self.tail_calls if r['entry']=='000ea440']==[19,9]
        order=[r['stage']for r in self.stages]
        assert order.index('drop_unselected_inventory')<order.index('grenade_drop')<order.index('action19')<order.index('action9')<order.index('interrupt_actions')
        before_weapons=[o for o in before['objects']if int(o['datum'],16)in {int(d,16)for d in before['slots']if d!='ffffffff'}]
        drops=[r['stack'][0]for r in self.tail_calls if r['entry']=='000cfb00']
        allocs=[r for r in self.tail_calls if r['entry']=='000b8a30']
        assert len(allocs)==before['grenades'][0]
        success=sum(r['return_eax']!='ffffffff'for r in allocs)
        assert len(drops)==success+len(before_weapons)
        assert sum('rng_write_instruction'in e for e in events)==len(drops)*4
        if before['weapons'][0]['magazine_mode']==1:
            transfers=[r for r in self.tail_calls if r['entry']=='00106d40']
            late=before['weapons'][0]['transfer_ticks']*2<before['weapons'][0]['total_transfer_ticks']
            assert bool(transfers)==late
            if late:
                assert transfers[0]['before']['weapons'][0]['item_flags']=='0005'
                assert transfers[0]['after']['weapons'][0]['loaded']==60
                assert transfers[0]['after']['weapons'][0]['reserve']==120
                assert not any(r['entry']=='00104d30'for r in self.tail_calls)
        return dict(before=before,after_depletion=after_depletion,after=after,complete_original_return=True,
            arguments=[f'{v:08x}'for v in(UI,0xffffffff,0,0,0)],
            component_producer=self.initial_equipped['startup'],initial_loadout=self.initial_equipped,
            garbage_initialization=self.garbage_initialization,initial_weapon_resources=self.weapon_resources,
            authored_marker_metadata=self.marker_metadata,
            root_boundary=self.player_pose,preparation=self.preparation,calls=self.tail_calls,stages=self.stages,
            stores=self.tail_writes,services=events,resource_events=self.live_events[live_start:],
            attachment_calls=[r for r in self.attachment_calls if r['phase']=='complete_source_death_cleanup'],
            collision_queries=[r for r in self.collision_queries if r['phase']=='complete_source_death_cleanup'])

def cases():
    traces=[];rows=[]
    for name in('initial_smg_idle','held_smg','reload_smg','late_reload_smg','secondary_dual_smg','holstered_smg_current_br','source_pool_exhausted'):
        t=DeathTailTrace(12 if name=='secondary_dual_smg'else 1,near=name in('secondary_dual_smg','holstered_smg_current_br'))
        if name=='held_smg':t.prepare_weapon(10,'held')
        elif name=='reload_smg':t.prepare_weapon(47,'reload')
        elif name=='late_reload_smg':t.prepare_weapon(72,'reload')
        elif name in('secondary_dual_smg','holstered_smg_current_br'):
            button=3 if name=='secondary_dual_smg'else 2
            t.seeded_duration(button,232);t.poll('source_233ms_pickup',{button:255},milliseconds=1)
            observe_pose(t,'source_current_equipped_pose')
            assert t.u(UNIT+0x21c)==t.target_datum
        elif name=='source_pool_exhausted':
            # Complete original allocator fills remaining headers. These are
            # explicit body-less reservation inputs, not world-created items.
            while t.u(POOL+0x3c)<t.u(POOL+0x20):
                t.run(0x16d2c0,regs=((UC_X86_REG_EDX,POOL),))
                assert t.g.reg_read(UC_X86_REG_EAX)!=0xffffffff
            t.run(0x16d2c0,regs=((UC_X86_REG_EDX,POOL),))
            assert t.g.reg_read(UC_X86_REG_EAX)==0xffffffff
        rows.append(dict(case=name,**t.death()));traces.append(t)
    return traces,rows

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
    for p,h in FROZEN.items():assert sha(ROOT/p)==h,p
    traces,rows=cases();t=traces[0];args.output.parent.mkdir(parents=True,exist_ok=True)
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
    executed=set().union(*(t.executed for t in traces))
    for entry in ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(routine.code)]
        for i in ins:assert t.image.read(i.address,len(i.bytes))==bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
        p=directory/f'{entry:08x}.asm';p.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in ins)+'\n')
        listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),instructions=len(ins),executed=sum(i.address in executed for i in ins)))
    deps=('oracle_mp_weapon_attachment_1_1.py','oracle_smg_constructor_equipped_1_1.py',
        'oracle_mp_inventory_equipped_1_1.py','oracle_spartan_equipped_unit_phase_1_1.py',
        'oracle_mp_inventory_1_1.py','oracle_mp_inventory_controls_1_1.py','oracle_spartan_equipped_animation_1_1.py',
        'oracle_spartan_live_pose_1_1.py','oracle_spartan_default_pose_1_1.py','oracle_smg_firing_1_1.py')
    table=[]
    for n in(9,0x13,0x16,0x1a,0x1b,0x1e):
        at=0x46ec68+n*4;p=t.u(at);raw=bytes(t.g.mem_read(p,16))
        assert t.image.read(at,4)==bytes(t.g.mem_read(at,4)) and t.image.read(p,16)==raw
        table.append(dict(type=n,table_pointer=f'{at:08x}',pointer_hex=bytes(t.g.mem_read(at,4)).hex(),entry=f'{p:08x}',record_hex=raw.hex()))
    dependencies={f'tools/{p}':sha(ROOT/'tools'/p)for p in deps}
    attachment=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json').read_text())
    for dependency in attachment['dependencies']:
        assert sha(ROOT/dependency['path'])==dependency['sha256']
        dependencies[dependency['path']]=dependency['sha256']
    matg=next(tag for tag in t.cache.tags.values()if tag['group']=='matg');bipd=t.cache.tags[t.u(UNIT)]
    authored_grenades=dict(globals_tag=matg,block=t.cache_record(matg['offset']+t.cache.magic+0x100,8),
        records=[t.cache_record(at+t.cache.magic,0x2c)for at in t.cache.block(matg['offset']+0x100,0x2c)],
        biped_starting_fields=t.cache_record(bipd['offset']+t.cache.magic+0x1b4,4))
    result=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=sha(__file__),frozen_reports=FROZEN,
        shared_cache_sha256=t.equipped_metadata['shared_cache_sha256'],
        dependencies=dependencies,instruction_patches=[],new_absent_component_or_pose_fixtures=[],
        instruction_listings=listings,action_tables=table,authored_grenade_inputs=authored_grenades,case_count=len(rows),cases=rows,
        boundaries=['Inherited preallocated player/unit/dormant object arena and64-header object pool; complete source startup/datum order is outside this suite.',
            'Original full unit type constructor/BD270 produce extension/extra/control blocks; actual source graph and cache pose sampling produce matrices.',
            'Initial biped state0, physical componentNONE and inactive physical flags are inherited body-arena inputs; full biped/Havok construction and contacts are not executed.',
            'Heap backing/growth, exact resource residency, explicit roots/input/phase scheduling, audio/effects and FPS notification remain named inherited services.',
            'Complete D7E00 then CBDC0 roots execute with classNONE/knocked0/force0 matching the frozen actual SMG fatal cleanup; the inactive optional damage-vector pointer is supplied null.',
            'This new suite does not replay D9180 or the prior lethal attribution/death writer; their synchronous scheduling remains the separate frozen corpse contract.',
            'Source grenade/default loadout outputs execute; exhausted-pool case reserves body-less original headers explicitly and proves original allocation failure, not complete world pressure.',
            'B7972..B7995 executes original new-map garbage stores with explicit prefix-owned register values; earlier B78C0 world/pool resets are not run.',
            'Drop terrain calls execute against authored BSP, but246A50 Havok shape test is supplied false; no completed world settling, active unit broadphase, biped Havok body or Xbox reciprocal bit parity is claimed.',
            'No new direct component/pose writes, instruction patches, earlier frozen changes or production edits.'])
    args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(rows),report=str(args.output),sha256=sha(args.output))))

if __name__=='__main__':main()

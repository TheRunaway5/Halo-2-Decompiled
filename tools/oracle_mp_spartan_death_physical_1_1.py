#!/usr/bin/env python3
"""Separate genuine allocated Spartan death/body transition diagnostic.

Explicit ordinary diagnostic death roots use original current-owner production.
No pose, physical mode, constraint frame, contact, world/PVS, timer or successful
return is supplied. The inherited selected setup/platform scope stays explicit.
"""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_sandbox_owner_1_1 as sandbox
import oracle_mp_spawn_player_visibility as player
from oracle_mp_spawn_biped_physics import PLACEMENT, RAW_COMPONENTS, RAW_HAVOK
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import ROOT, STACK, STOP, PIN, CACHE_PIN
from functions import _trace
import oracle_mp_command_locomotion_pose_1_1 as motion
import oracle_smg_effect_owner_1_1 as effect

ENTRIES = set(player.ENTRIES) | {0xd7cd0,0xd7e00,0xcbdc0,0xe6050,0xe6dc0,0x1e8e60,
    0x1c70d0,0x1d4c50,0x1d3bb0,0x1d2b10,0x1da570,0x1da6f0,0x1d9330,0x1d95e0,
    0x1d98b0,0x1daac0,0x1db1c0,0x1db8e0,0x1d4f30,0x1eead0,0x1eeb70,0x1eeb20,
    0xe2520,0xe2ce0,0x1f08e0,0x13a710,0x110ee0,0xbdf80,0x1d7310,0x1d42d0,0x1d7710}
ENTRIES |= effect.ENTRIES


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def write(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        assert path.read_bytes() == raw, path
    else:
        path.write_bytes(raw)


class DeathTrace(sandbox.SandboxTrace):
    def __init__(self, data):
        self.death_enabled = False
        self.death_calls = []
        self.death_pending = []
        self.death_stores = []
        self.death_reads = []
        self.death_stages = []
        self.death_executed = set()
        self.death_order = 0
        super().__init__(data)
        self.g.hook_add(unicorn.UC_HOOK_MEM_READ, self.death_read)

    def physical_snapshot(self, backing=False):
        result = dict(world=self.state(),player=self.player_state(),
            clock_pointer=f'{self.u(0x518444):08x}',
            clock0c_hex=bytes(self.g.mem_read(self.u(0x518444),12)).hex(),
            source_globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
                for p,n in ((0x517fdc,2),(0x526248,4),(0x4e78d0,4),(0x526234,4))])
        if backing:
            result['raw_physical_backing'] = [dict(pointer=f'{p:08x}',size=n,
                raw_hex=bytes(self.g.mem_read(p,n)).hex())
                for p,n in ((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b))]
        if hasattr(self,'spawned_unit'):
            head=self.u(self.u(0x4e78d0)+0x44)+(self.spawned_unit&0xffff)*12
            p=self.u(head+8)
            result['unit_biped_fields'] = dict(pointer=f'{p:08x}',
                fields348_hex=bytes(self.g.mem_read(p+0x348,8)).hex(),
                physical_state3dc28_hex=bytes(self.g.mem_read(p+0x3dc,0x28)).hex())
        return result

    def returns_at(self, address):
        matches=[row for row in self.death_pending if row[0]==address]
        if not matches:
            return
        stack=matches[-1][1]
        for target,sp,row in reversed(matches):
            if sp!=stack:
                continue
            row.update(return_order=self.death_order,
                returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
                return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp)
            self.death_order+=1
            self.death_pending.remove((target,sp,row))

    def hook(self,g,address,size,user):
        if self.death_enabled:
            self.death_executed.add(address)
            self.returns_at(address)
            if address in ENTRIES:
                sp=g.reg_read(reg.UC_X86_REG_ESP)
                row=dict(id=len(self.death_calls),phase=self.active,order=self.death_order,
                    entry=f'{address:08x}',caller=f'{self.u(sp):08x}',
                    registers=self.registers(),entry_esp=f'{sp:08x}',
                    stack16=[f'{self.u(sp+4+i*4):08x}' for i in range(16)])
                self.death_calls.append(row)
                self.death_pending.append((self.u(sp),sp,row))
                self.death_order+=1
        super().hook(g,address,size,user)

    def death_read(self,g,access,address,size,value,user):
        if self.death_enabled and self.death_pending:
            self.death_reads.append(dict(order=self.death_order,call=self.death_pending[-1][2]['id'],
                instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',
                address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()))
            self.death_order+=1

    def mem_write(self,g,access,address,size,value,user):
        if self.death_enabled:
            self.death_stores.append(dict(order=self.death_order,phase=self.active,
                instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',
                address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),
                written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
            self.death_order+=1
        super().mem_write(g,access,address,size,value,user)

    def phase(self,name,entry,args=(),regs=()):
        self.active=name
        self.new_invalid=None
        start=len(self.events)
        before=self.physical_snapshot(True)
        try:
            self.run(entry,args,regs)
            self.returns_at(STOP)
            complete=True
        except (unicorn.UcError,KernelBoundary):
            assert self.new_invalid is not None
            complete=False
        self.death_stages.append(dict(name=name,entry=f'{entry:08x}',
            arguments=[f'{a:08x}' for a in args],
            input_registers={str(k):f'{v:08x}' for k,v in regs},
            before=before,after=self.physical_snapshot(True),completed=complete,
            stopped_eip=f'{self.g.reg_read(reg.UC_X86_REG_EIP):08x}',
            invalid=self.new_invalid,used_services=self.events[start:]))
        print(json.dumps(dict(stage=name,completed=complete,
            eip=self.death_stages[-1]['stopped_eip'],invalid=self.new_invalid)),flush=True)
        return complete


class MovingDeathTrace(DeathTrace, motion.CommandPoseTrace):
    """Observer-only composition; both inherited original owners stay unchanged."""


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--phmo-fixup',action='store_true')
    ap.add_argument('--effect-storage',action='store_true')
    ap.add_argument('--moving',action='store_true')
    ap.add_argument('--context-replay',type=Path)
    args=ap.parse_args()
    pins={**sandbox.FROZEN,
        'tools/oracle_mp_sandbox_owner_1_1.py':'579e6c9a5d2ef33fd31527a440f3b8ee77d72a20af458517cadbcfab7bd34acd',
        'analysis/multiplayer/integration/death-animation-20261010/frozen/manifest.json':
            '416482b29993c11cab75794e835598e2fe6a5be694f4b0cbb02c4ab0f11588c0',
        'tools/oracle_mp_command_locomotion_pose_1_1.py':
            '807e303f942380bc809816e00b28053cc1c78e8542a1ac92516ab26cc25645ad',
        'analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/report.json':
            '12b7411de3bfa03f089de889d860011fb57cf20f9128cadaa4a1e272b1b65dd0'}
    for path,expected in pins.items():
        assert digest(ROOT/path)==expected,path
    args.output.parent.mkdir(parents=True,exist_ok=True)
    proc=None
    if args.context_replay:
        data=json.loads(args.context_replay.read_text())
    else:
        stderr=(args.output.parent/'runtime-stderr.txt').open('w')
        proc=subprocess.Popen(['wine',str(player.CONTEXT_OUT/'generator.exe')],
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=stderr,text=True,
            env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(player.CONTEXT_OUT/'wine-prefix')))
        data=json.loads(proc.stdout.readline())
    try:
        movement=None
        if args.moving:
            motion.CommandPoseTrace=MovingDeathTrace
            t,correlation=motion.prepare(data)
            for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
            t.g.context_restore(t.source_fixture_cpu)
            movement=motion.probe(t,'authored_heading_forward',0x3f800000,0,True)
            prior=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/report.json').read_text())
            expected=next(row for row in prior['cases'] if row['case']=='authored_heading_forward')
            assert len(expected['stages'])==len(movement['stages'])
            for a,b in zip(expected['stages'],movement['stages']):
                for key in ('before_state','after_state'):
                    left={k:v for k,v in prior['states'][a[key]].items() if k!='id'}
                    right={k:v for k,v in t.states[b[key]].items() if k!='id'}
                    assert left==right,(a['name'],key)
            movement=dict(case=movement,prior_runtime_correlation=correlation,
                prior_case_states_byte_identical=True,states=t.states,calls=t.calls,
                reads=t.reads,stores=t.stores,points=t.points)
            print(json.dumps(dict(source_movement_complete=True,prior_case_states_byte_identical=True)),flush=True)
        else:
            t=player.prepare(data,trace_type=DeathTrace)
            assert t.player_stages[-1]['completion']=='complete_original_return'
            assert player.spawn_and_activate(t)
        if args.phmo_fixup:
            t.selected_physics_fixup(0xe9f2086d)
        initial=t.physical_snapshot(True)
        t.death_enabled=True
        complete=True
        if args.effect_storage:
            for name,entry in (('original_effect_storage_constructor',0x1775d0),
                    ('original_effect_map_reset',0x177690)):
                if not t.phase(name,entry):
                    complete=False
                    break
        # Exact attribution is source-produced for this current unit. The
        # diagnostic root is explicitly separate from incoming-hit admission.
        owner=PLACEMENT+0x800
        if complete:
            complete=t.phase('original_current_damage_owner',0xd7cd0,(t.spawned_unit,owner))
        owner_hex=bytes(t.g.mem_read(owner,10)).hex()
        if complete:
            complete=t.phase('explicit_original_death_request',0xd7e00,(t.spawned_unit,owner,1,0))
        if complete:
            complete=t.phase('explicit_original_ordinary_death_cleanup',0xcbdc0,
                (t.spawned_unit,0xffffffff,0,0,0))
        if complete:
            for name,entry in (('object_prephysics',0xb7e00),('world_physics',0x1c7660),
                    ('object_postphysics',0xb7f50),('object_newflags',0xb8040)):
                if not t.phase('original_after_death_'+name,entry):
                    break
    finally:
        if proc:
            proc.stdin.write('x');proc.stdin.flush();proc.stdin.close()
            proc.wait(timeout=60);proc.stdout.close();stderr.close()
            assert proc.returncode==0
    write(args.output.parent/'raw-host-context.json',encoded(data))
    directory=args.output.parent/'instructions'
    listings=[]
    def emit(name,addresses):
        rows=[t.code.at(a) for a in sorted(addresses)]
        for row in rows:
            assert row and t.image.read(row.address,len(row.bytes))==row.bytes==bytes(t.g.mem_read(row.address,len(row.bytes)))
        raw=''.join(f'{row.address:08x} {row.bytes.hex()} {row.mnemonic} {row.op_str}\n' for row in rows).encode()
        write(directory/name,raw)
        return dict(file='instructions/'+name,sha256=hashlib.sha256(raw).hexdigest(),rows=len(rows),
            executed_rows=sum(row.address in t.death_executed for row in rows))
    for entry in sorted(ENTRIES):
        routine=_trace(t.code,entry,t.starts|ENTRIES)
        listings.append(dict(entry=f'{entry:08x}',**emit(f'{entry:08x}.asm',routine.code)))
    listings.append(dict(kind='complete_new_execution',**emit('executed-death.asm',t.death_executed)))
    report=dict(schema=1,oracle_sha256=digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
        prior_pins=pins,source_preparation=t.producers,initial=initial,
        selected_phmo_postprocess=getattr(t,'selected_fixup',None),
        source_movement=movement,
        original_damage_owner10_hex=owner_hex,stages=t.death_stages,calls=t.death_calls,
        reads=t.death_reads,stores=t.death_stores,instructions=listings,
        source_spawn_player=f'{t.spawned_player:08x}',source_spawn_unit=f'{t.spawned_unit:08x}',
        all_services=t.events,instruction_patches=[],new_supplied_gameplay_or_physics_results=[],
        boundaries=['The original selected player/world/platform preparation and inherited30Hz clock input remain explicit; no natural full frame/time progression is claimed.',
            'Optional masterchief phmo postprocess executes the existing exact tag iterator/dispatcher with actual tag input; it is not whole map startup.',
            'D7CD0 produces the actual current-unit owner. D7E00/CBDC0 are explicit ordinary diagnostic roots, not original incoming fatal-hit admission.',
            'No physical state, pose, root, constraints, contacts, PVS membership, tick, effect or successful source result is supplied.',
            'Whole ordinary roots after death follow existing source object-phase order without inventing successive ticks. Genuine first unavailable owner is retained.',
            'All original calls/reads/writes and full before/after physical backing are retained; source emulator FP output is not claimed to be Xbox hardware parity.'])
    for path,expected in pins.items():
        assert digest(ROOT/path)==expected,path
    write(args.output,encoded(report))
    print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),
        calls=len(t.death_calls),stores=len(t.death_stores),reads=len(t.death_reads))),flush=True)


if __name__=='__main__':
    main()

#!/usr/bin/env python3
"""Ordinary retained grenade updates after the frozen effect-owned admission.

The complete frozen begin composition is reproduced unchanged before continuing
original C7E90/E7E60 dispatch. No clip, action, time, mode, grenade or return is
supplied. Each later neutral host command is explicitly declared.
"""
import argparse
import hashlib
import json
import os
import struct
import subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EIP
import oracle_mp_grenade_effect_pool_1_1 as begin
from functions import _trace

ROOT=begin.ROOT
OUT=ROOT/'analysis/multiplayer/integration/grenade-owned-release'
motion=begin.motion
# Direct owners retained in the original grenade release listing and the
# previously executed stock-projectile constructor. Broader services stay out.
EXTRA_ENTRIES={0xba980,0xa8460,0xa9fa0,0xfe1e0,0xfcf90,0xfe4a0,
    0xb9430,0xbbcb0,0xbbe70}
PIN='93ff6717374679dea8687de2619d25909970e7701eb40e925642ddac99289721'


class OwnedReleaseTrace(begin.GrenadeEffectTrace):
    def snapshot(self):
        # A genuine allocator first occupies the header, then produces body
        # storage. Keep the whole raw intermediate state; no object type or
        # weapon field is readable until its actual body bytes exist.
        value=begin.phase.base.GrenadeTrace.snapshot(self)
        value['carried_weapon_own_states']=[dict(datum=o['datum'],
            state_178=f'{struct.unpack_from("<I",bytes.fromhex(o["body_hex"]),0x178)[0]:08x}',
            blocking_17c=struct.unpack_from('<h',bytes.fromhex(o['body_hex']),0x17c)[0])
            for o in value['objects'] if len(bytes.fromhex(o['body_hex']))>=0x17e and bytes.fromhex(o['body_hex'])[0xaa]==2]
        whole=begin.effect.EffectTrace.snapshot(self)
        value['effect_context']={key:whole[key] for key in ('game','camera','pools','shared_seed')}
        return value


def prepare(context):
    for name,digest in begin.PINS.items():assert motion.digest(ROOT/name)==digest,name
    freeze=ROOT/'analysis/multiplayer/integration/grenade-effect-pool/manifest.json'
    assert motion.digest(freeze)==PIN
    frozen=json.loads(freeze.read_text())
    for collection in (frozen['source_files'],frozen['verified_prior_pins']):
        for name,digest in collection.items():assert motion.digest(ROOT/name)==digest,name
    motion.ENTRIES.update(begin.ENTRIES)
    motion.STATE_ENTRIES.update(begin.ENTRIES)
    motion.CommandPoseTrace=OwnedReleaseTrace
    trace,correlation=motion.prepare(context)
    for lo,hi,permissions,raw in trace.source_fixture:trace.g.mem_write(lo,raw)
    trace.g.context_restore(trace.source_fixture_cpu)
    trace.probe='genuine_owner_effect_pool_constructor';trace.observe=True
    constructor_stages=[]
    before_pools=begin.pools(trace)
    for label,entry in [('source_effect_storage_constructor',0x1775d0),('source_effect_map_reset',0x177690)]:
        before=trace.state_id();start=len(trace.events)
        assert trace.stage(label,entry)
        trace.probe_returns(motion.STOP)
        constructor_stages.append(dict(name=label,entry=f'{entry:08x}',before_state=before,after_state=trace.state_id(),
            completion=trace.player_stages[-1]['completion'],stopped_eip=f'{trace.g.reg_read(UC_X86_REG_EIP):08x}',
            invalid=trace.new_invalid,services=trace.events[start:],complete_pools=begin.pools(trace)))
    removed={}
    for entry in (0x115650,0x179b90):
        if entry in trace.services:removed[f'{entry:08x}']=trace.services.pop(entry)
    cases=[begin.phase.execute(trace,'genuine_owner_effect_initialized_neutral_then_Q',1)]
    old=json.loads((begin.OUT/'measured/report.json').read_text())
    values=dict(prior_runtime_correlation=correlation,before_pools=before_pools,constructor_stages=constructor_stages,
        removed_inherited_services=removed,cases=cases,final_pools=begin.pools(trace),final_state=trace.snapshot(),
        calls=trace.calls,states=trace.states,reads=trace.reads,stores=trace.stores,points=trace.points)
    values=json.loads(motion.encoded(values))
    for key,value in values.items():assert value==old[key],('Frozen admitted grenade composition changed',key)
    prefix=dict(report='analysis/multiplayer/integration/grenade-effect-pool/measured/report.json',
        report_sha256=motion.digest(begin.OUT/'measured/report.json'),manifest_sha256=PIN,
        byte_identical_fields=list(values),runtime_sha256=hashlib.sha256(motion.encoded(values)).hexdigest(),
        ranges={key:[0,len(getattr(trace,key))] for key in ('calls','states','reads','stores','points')})
    # Only after exact prior correlation, expand the observer to retain actual
    # world/allocator/release callback bytes if reached. No guest state changes.
    motion.ENTRIES.update(EXTRA_ENTRIES);motion.STATE_ENTRIES.update(EXTRA_ENTRIES)
    for entry in EXTRA_ENTRIES:
        routine=_trace(trace.code,entry,trace.starts|motion.ENTRIES)
        trace.routines[entry]=routine
        for address in routine.code:trace.owners[address]=entry
    return trace,prefix


def continue_updates(trace,count):
    trace.probe='genuine_admitted_grenade_ordinary_continuation';trace.observe=True
    start={key:len(getattr(trace,key)) for key in ('calls','states','reads','stores','points')}
    stages=[];inputs=[];rows=[]
    def pointer(datum):return trace.u(trace.u(trace.u(0x4e78d0)+0x44)+(datum&0xffff)*12+8)
    weapon=trace.u(pointer(trace.spawned_unit)+0x218)
    def run(label,entry,args=(),regs=()):
        before=trace.state_id();at=len(trace.events)
        ok=trace.stage(label,entry,args,regs);trace.probe_returns(motion.STOP)
        stages.append(dict(name=label,entry=f'{entry:08x}',arguments=[f'{v:08x}' for v in args],
            registers={str(k):f'{v:08x}' for k,v in regs},before_state=before,after_state=trace.state_id(),
            completion=trace.player_stages[-1]['completion'],stopped_eip=f'{trace.g.reg_read(UC_X86_REG_EIP):08x}',
            invalid=trace.new_invalid,services=trace.events[at:]))
        return ok
    def blend(label,datum):
        p=pointer(datum);offset=struct.unpack('<h',trace.g.mem_read(p+0x12a,2))[0]
        if offset==-1 or trace.u(p+offset+0x68)==0xffffffff:return True
        return run(label,0x1d0ed0,regs=((UC_X86_REG_ESI,p+offset),))
    for phase in range(count):
        before=trace.state_id();label=f'continuation_{phase}'
        if not run(label+'_neutral_initializer',0x14e780,regs=((UC_X86_REG_EDX,motion.command.COMMAND),)):break
        raw=trace.raw(motion.command.COMMAND,0x5c)
        trace.w(motion.command.COMMAND,0)
        trace.g.mem_write(motion.command.COMMAND+0x22,struct.pack('<h',0))
        inputs.append(dict(phase=phase,flags='00000000',selector=0,before5c_hex=raw,
            after5c_hex=trace.raw(motion.command.COMMAND,0x5c),declared_fields=['incoming command+0 flags','incoming command+22 grenade selector retained from originalCD310']))
        record=trace.u(0x4d6d44)+0x80+(trace.spawned_player&0xffff)*0x88
        if not all(run(label+'_'+name,entry,args) for name,entry,args in (
            ('submission',0x852a0,(record,motion.command.COMMAND)),
            ('assembly',0x69350,(trace.u(0x4d6d44),motion.command.PACKET)),
            ('player',0x14eae0,(motion.command.PACKET,)))):break
        after_player=trace.state_id()
        if not blend(label+'_source_unit_blend',trace.spawned_unit):break
        if not run(label+'_source_biped_control',0xc7e90,(trace.spawned_unit,),((UC_X86_REG_EAX,0xc7e90),)):break
        after_control=trace.state_id()
        if not run(label+'_source_biped_motion_animation',0xde960,(trace.spawned_unit,),((UC_X86_REG_EAX,0xde960),)):break
        if not blend(label+'_source_weapon_blend',weapon):break
        if not run(label+'_source_weapon_update',0x1021d0,(weapon,),((UC_X86_REG_EAX,0x1021d0),)):break
        if not run(label+'_source_unit_pose',0xbdf80,(trace.spawned_unit,)):break
        if not run(label+'_source_weapon_pose',0xbdf80,(weapon,)):break
        if not run(label+'_source_projectile_postpass',0x1019f0):break
        rows.append(dict(phase=phase,before_state=before,after_player=after_player,after_control=after_control,after_pose=trace.state_id()))
        print(json.dumps(dict(phase=phase,grenade=trace.grenade_state())),flush=True)
    trace.observe=False
    unreturned=[row['id'] for _,_,row in trace.pending]
    if unreturned:assert stages[-1]['completion']=='blocked_original_instruction'
    return dict(requested_phases=count,rows=rows,inputs=inputs,stages=stages,
        ranges={key:[value,len(getattr(trace,key))] for key,value in start.items()},unreturned_original_calls=unreturned,
        final_state=trace.snapshot(),final_cpu=trace.cpu(),final_pools=begin.pools(trace))


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--phases',type=int,default=12)
    args=parser.parse_args();args.output=args.output.resolve();assert not args.output.exists() and 1<=args.phases<=32
    pin=motion.digest(__file__)
    process=subprocess.Popen(['wine',str(motion.command.CONTEXT_OUT/'generator.exe')],
        stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
        env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        context=json.loads(process.stdout.readline());trace,prefix=prepare(context)
        continuation=continue_updates(trace,args.phases)
        assert motion.digest(__file__)==pin and motion.digest(begin.OUT/'manifest.json')==PIN
        report=dict(schema='source-grenade-owned-release-v1',oracle_sha256=pin,original_xbe_sha256=begin.effect.PIN,
            original_cache_sha256=begin.effect.CACHE_PIN,prior_admitted_begin_correlation=prefix,
            continuation=continuation,source_metadata=begin.metadata(trace),calls=trace.calls,states=trace.states,
            reads=trace.reads,stores=trace.stores,points=trace.points,instructions=motion.listings(trace,args.output.parent),
            inherited_services=trace.events,inherited_physical_services=trace.physical_services,
            new_supplied_gameplay_fields=[],new_supplied_source_returns=[],instruction_patches=[],
            boundaries=[
                'The immutable admitted original grenade composition is byte-identically reproduced first. Only observer entries are expanded after that correlation.',
                'Subsequent declared neutral incoming command records preserve selector0 produced by originalCD310. Unchanged original player/control/blend/weapon/pose/postpass roots are explicitly composed; no recursive full-world traversal or complete advancing source match clock is inferred.',
                'No animation clip, frame, timer, phase, action, count, held object, projectile, PVS, clock, seed, allocator, admission or return is assigned by this extension. First genuine unavailable instruction/service remains a stop.',
                'Inherited actual allocation/cache/resources/raw host memory/compatibility context/physics/optional feedback boundaries remain declared. Local audio/device success, original1.5 execution and native Q gameplay integration remain open.'])
        motion.frozen_write(args.output,motion.encoded(report))
        print(json.dumps(dict(report=str(args.output),sha256=motion.digest(args.output),
            last=continuation['stages'][-1],grenade=continuation['final_state']['grenade'])),flush=True)
    finally:
        if process.poll() is None:
            try:process.stdin.write('x');process.stdin.flush();process.wait(timeout=15)
            except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
        for stream in (process.stdin,process.stdout,process.stderr):stream.close()


if __name__=='__main__':main()

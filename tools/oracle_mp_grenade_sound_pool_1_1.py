#!/usr/bin/env python3
"""Small original looping-sound pool constructor/reset on the grenade owner.

Reproduce the immutable admitted grenade first, then execute only18A200 and
18A2A0. No boot/device chain, initialized/valid flag or sound return is supplied.
Stop at the first genuine unavailable driver store.
"""
import argparse
import json
import os
import subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EIP
import oracle_mp_grenade_owned_release_1_1 as release
from functions import _trace

ROOT=release.ROOT
OUT=ROOT/'analysis/multiplayer/integration/grenade-sound-pool'
motion=release.motion
ENTRIES={0x18a200,0x18a2a0,0x18a790,0x18a9c0,0x16d070,0x16d220,0x2230e0}


class SoundPoolTrace(release.OwnedReleaseTrace):
    def __init__(self,*args,**kwargs):
        self.sound_observe=False
        super().__init__(*args,**kwargs)

    def sound_state(self):
        allocator=self.u(0x51841c);pool=self.u(0x4f485c);state=self.u(0x4f4858)
        value=dict(allocator_pointer=f'{allocator:08x}',allocator4_hex=self.raw(allocator,4) if allocator else None,
            arena_pointer=f'{self.u(0x4ed650):08x}',arena_used=f'{self.u(0x4ed654):08x}',arena_checksum=f'{self.u(0x4ed65c):08x}',
            owner_pool_pointer=f'{pool:08x}',owner_state_pointer=f'{state:08x}',driver_pointer=f'{self.u(0x526474):08x}')
        if pool:value['owner_pool60cc_hex']=self.raw(pool,0x60cc)
        if state:value['owner_state244_hex']=self.raw(state,0x244)
        return value

    def snapshot(self):
        value=super().snapshot()
        if self.sound_observe:value['sound_owner_context']=self.sound_state()
        return value


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve();assert not args.output.exists()
    checkpoint=ROOT/'analysis/multiplayer/integration/grenade-owned-release/checkpoint-manifest.json'
    checkpoint_pin=motion.digest(checkpoint)
    frozen=json.loads(checkpoint.read_text())
    for collection in (frozen['source_files'],frozen['verified_prior_pins']):
        for name,digest in collection.items():assert motion.digest(ROOT/name)==digest,name
    pin=motion.digest(__file__)
    release.OwnedReleaseTrace=SoundPoolTrace # Observer-only subclass, no guest service override.
    process=subprocess.Popen(['wine',str(motion.command.CONTEXT_OUT/'generator.exe')],
        stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
        env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        context=json.loads(process.stdout.readline());trace,prefix=release.prepare(context)
        motion.ENTRIES.update(ENTRIES);motion.STATE_ENTRIES.update(ENTRIES)
        for entry in ENTRIES:
            routine=_trace(trace.code,entry,trace.starts|motion.ENTRIES);trace.routines[entry]=routine
            for address in routine.code:trace.owners[address]=entry
        trace.sound_observe=True;trace.observe=True;trace.probe='genuine_grenade_owner_sound_pool_prefix'
        before=trace.sound_state();stages=[]
        begin={key:len(getattr(trace,key)) for key in ('calls','states','reads','stores','points')}
        for label,entry in [('source_looping_sound_pool_constructor',0x18a200),('source_looping_sound_map_reset',0x18a2a0)]:
            prior=trace.state_id();at=len(trace.events)
            ok=trace.stage(label,entry);trace.probe_returns(motion.STOP)
            stages.append(dict(name=label,entry=f'{entry:08x}',before_state=prior,after_state=trace.state_id(),
                completion=trace.player_stages[-1]['completion'],stopped_eip=f'{trace.g.reg_read(UC_X86_REG_EIP):08x}',
                invalid=trace.new_invalid,services=trace.events[at:],sound_context=trace.sound_state()))
            if not ok:break
        trace.observe=False
        assert motion.digest(__file__)==pin and motion.digest(checkpoint)==checkpoint_pin
        value=dict(schema='source-grenade-sound-pool-v1',oracle_sha256=pin,previous_continuation_checkpoint_sha256=checkpoint_pin,
            original_xbe_sha256=release.begin.effect.PIN,original_cache_sha256=release.begin.effect.CACHE_PIN,
            prior_admitted_begin_correlation=prefix,before=before,stages=stages,final_state=trace.snapshot(),
            ranges={key:[start,len(getattr(trace,key))] for key,start in begin.items()},
            calls=trace.calls,states=trace.states,reads=trace.reads,stores=trace.stores,points=trace.points,
            unreturned_original_calls=[row['id'] for _,_,row in trace.pending],
            instructions=motion.listings(trace,args.output.parent),inherited_services=trace.events,
            inherited_physical_services=trace.physical_services,new_supplied_gameplay_fields=[],
            new_supplied_source_returns=[],instruction_patches=[],
            boundaries=[
                'The immutable source-owned effect-pool/grenade begin is reproduced before adding only original18A200 and18A2A0. Complete boot/DSP/title-heap initialization is not repeated or asserted.',
                'Existing original51841C allocator owns pool storage and state allocation. Original16D070/16D220 and18A2B5 own all header/pointer/valid/reset stores; no pool or admission field is assigned by the tool.',
                'Actual driver526474 remains absent. Original device/capabilities/initialized/audio flags, voice and playback returns are not supplied. Any original driver store fault remains an unreturned map-reset boundary.',
                'This separate prefix does not resume the earlier partialB8A30, allocate a second grenade, release a held object or establish completed sound-map activation. Original fixed clock/seed/owner/action/PVS remain preserved.'])
        motion.frozen_write(args.output,motion.encoded(value))
        print(json.dumps(dict(report=str(args.output),sha256=motion.digest(args.output),
            stages=[dict(name=s['name'],completion=s['completion'],stopped_eip=s['stopped_eip'],invalid=s['invalid']) for s in stages],
            final_sound={key:val for key,val in trace.sound_state().items() if not key.endswith('_hex')})),flush=True)
    finally:
        if process.poll() is None:
            try:process.stdin.write('x');process.stdin.flush();process.wait(timeout=15)
            except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
        for stream in (process.stdin,process.stdout,process.stderr):stream.close()


if __name__=='__main__':main()

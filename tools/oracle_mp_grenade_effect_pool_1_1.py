#!/usr/bin/env python3
"""Original grenade begin composed with actual effect storage/reset producers.

The unchanged allocated-owner command fixture is reproduced before restoration.
Only original effect constructors/reset roots are added before the exact bounded
neutral/Q sequence. No game pool, PVS, clock, seed, channel or return is supplied.
"""
import argparse
import hashlib
import json
import os
import struct
import subprocess
from pathlib import Path

from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP
import oracle_mp_grenade_bounded_phases_1_1 as phase
import oracle_smg_effect_owner_1_1 as effect

ROOT = phase.ROOT
OUT = ROOT/'analysis/multiplayer/integration/grenade-effect-pool'
motion = phase.motion
ENTRIES = phase.ENTRIES | effect.ENTRIES | {0x16d220}
PINS = {**phase.PINS,
 'tools/oracle_mp_grenade_bounded_phases_1_1.py': '0d10fc01bfb10aad5341346f8de29241e2f19f412668b1fd5fccb9cf22de2b82',
 'analysis/multiplayer/integration/grenade-bounded-phases/manifest.json': '636e172a83834e2695e3890fd0677ab7472c2c60477cbdc96c50c8d235b2929f'}


class GrenadeEffectTrace(phase.PhaseTrace):
    def return_deltas(self, entry, seen=None):
        # 1775D0 ends in JMP175860. The source tail's RET ABI closes the
        # preserved original caller frame; a nested alias alone never does.
        seen=set() if seen is None else seen
        if entry in seen:return set()
        seen.add(entry)
        routine=self.routines[entry]
        deltas=set()
        for address in routine.code:
            instruction=self.code.at(address)
            if instruction.mnemonic=='ret':
                deltas.add(4+(int(instruction.op_str,0) if instruction.op_str else 0))
            elif instruction.mnemonic=='jmp' and instruction.op_str.startswith('0x'):
                target=int(instruction.op_str,0)
                if target not in routine.code and target in self.routines:
                    deltas.update(self.return_deltas(target,seen))
        return deltas

    def probe_returns(self, address):
        current_sp=self.g.reg_read(UC_X86_REG_ESP)
        for ret,entry_sp,row in self.pending[:]:
            if ret!=address or current_sp-entry_sp not in self.return_deltas(int(row['entry'],16)):continue
            row.update(return_order=self.order,return_cpu=self.cpu(),return_esp_delta=current_sp-entry_sp,
                after_packets=[dict(p,raw_hex=self.raw(int(p['pointer'],16),p['size']) if int(p['pointer'],16) else None)
                    for p in row['packets']])
            self.order+=1
            if int(row['entry'],16) in motion.STATE_ENTRIES:row['after_state']=self.state_id()
            self.pending.remove((ret,entry_sp,row))

    def snapshot(self):
        value = super().snapshot()
        whole = effect.EffectTrace.snapshot(self)
        value['effect_context'] = {key:whole[key] for key in ('game','camera','pools','shared_seed')}
        return value

    def packets(self, entry, registers, stack):
        return super().packets(entry, registers, stack) + effect.EffectTrace.packets(self, entry, registers, stack)


def pools(trace):
    result = []
    for row in effect.EffectTrace.snapshot(trace)['pools']:
        value = dict(row)
        if int(row['pointer'],16):
            value['complete_storage_hex'] = trace.raw(int(row['data_pointer'],16), row['capacity']*row['stride'])
        result.append(value)
    return result


def metadata(trace):
    cache = trace.cache
    result = phase.base.metadata(trace)
    tags = []
    for datum in (0xe7a60621, 0xe7b0062b):
        tag = cache.tags[datum]
        assert tag['group'] == 'effe'
        root = tag['offset']
        events = []
        for at in cache.block(root+0x14,0x38):
            parts = []
            for part in cache.block(at+0x18,0x38):
                parts.append(dict(address=f'{part+cache.magic:08x}',raw38_hex=trace.raw(part+cache.magic,0x38),
                    reference=cache.reference(cache.u(part+0x10))))
            systems = cache.block(at+0x30,0x38)
            events.append(dict(address=f'{at+cache.magic:08x}',raw38_hex=trace.raw(at+cache.magic,0x38),
                parts=parts,particle_system_count=len(systems)))
        tags.append(dict(tag=tag,address=f'{root+cache.magic:08x}',root1c_hex=trace.raw(root+cache.magic,0x1c),
            locations=[dict(address=f'{p+cache.magic:08x}',raw4_hex=trace.raw(p+cache.magic,4)) for p in cache.block(root+0xc,4)],events=events))
    result['throw_begin_effects'] = tags
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.output=args.output.resolve()
    assert not args.output.exists()
    # Supplement hard pins with the immutable effect freeze's actual source pin.
    freeze=ROOT/'analysis/multiplayer/integration/inventory-source/smg-effect-owner/manifest.json'
    frozen=json.loads(freeze.read_text())
    pins={**PINS,'tools/oracle_smg_effect_owner_1_1.py':frozen['pins']['tools/oracle_smg_effect_owner_1_1.py'],
          str(freeze.relative_to(ROOT)):motion.digest(freeze)}
    for name,digest in pins.items():assert motion.digest(ROOT/name)==digest,name
    oracle_pin=motion.digest(__file__)
    motion.ENTRIES.update(ENTRIES)
    motion.STATE_ENTRIES.update(ENTRIES)
    motion.CommandPoseTrace=GrenadeEffectTrace
    process=subprocess.Popen(['wine',str(motion.command.CONTEXT_OUT/'generator.exe')],
        stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
        env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        context=json.loads(process.stdout.readline())
        trace,correlation=motion.prepare(context)
        for lo,hi,permissions,raw in trace.source_fixture:trace.g.mem_write(lo,raw)
        trace.g.context_restore(trace.source_fixture_cpu)
        trace.probe='genuine_owner_effect_pool_constructor'
        trace.observe=True
        constructor_stages=[]
        before_pools=pools(trace)
        for label,entry in [('source_effect_storage_constructor',0x1775d0),('source_effect_map_reset',0x177690)]:
            before=trace.state_id();start=len(trace.events)
            ok=trace.stage(label,entry)
            trace.probe_returns(motion.STOP)
            constructor_stages.append(dict(name=label,entry=f'{entry:08x}',before_state=before,after_state=trace.state_id(),
                completion=trace.player_stages[-1]['completion'],stopped_eip=f'{trace.g.reg_read(UC_X86_REG_EIP):08x}',
                invalid=trace.new_invalid,services=trace.events[start:],complete_pools=pools(trace)))
            if not ok:break
        removed={}
        cases=[]
        if all(s['completion']=='complete_original_return' for s in constructor_stages):
            # Frozen preparation may include optional unavailable effect leaves.
            # Removing a leaf executes its original body; no replacement result.
            for entry in (0x115650,0x179b90):
                if entry in trace.services:removed[f'{entry:08x}']=trace.services.pop(entry)
            cases.append(phase.execute(trace,'genuine_owner_effect_initialized_neutral_then_Q',1))
        trace.observe=False
        for name,digest in pins.items():assert motion.digest(ROOT/name)==digest,name
        assert oracle_pin==motion.digest(__file__)
        value=dict(schema='source-grenade-effect-pool-v1',oracle_sha256=oracle_pin,source_pins=pins,
            original_xbe_sha256=effect.PIN,original_cache_sha256=effect.CACHE_PIN,
            prior_runtime_correlation=correlation,source_metadata=metadata(trace),before_pools=before_pools,
            constructor_stages=constructor_stages,removed_inherited_services=removed,cases=cases,
            final_pools=pools(trace),final_state=trace.snapshot(),calls=trace.calls,states=trace.states,
            reads=trace.reads,stores=trace.stores,points=trace.points,
            instructions=motion.listings(trace,args.output.parent),inherited_services=trace.events,
            inherited_physical_services=trace.physical_services,new_supplied_gameplay_fields=[],
            new_supplied_source_returns=[],instruction_patches=[],
            boundaries=[
                'Explicit composition of original effect storage/reset producers before the unchanged bounded neutral/Q source sequence on the byte-identically reproduced, genuinely allocated/joined/loadout owner. A whole original boot/map callback order is not claimed.',
                'Original1775D0 tailcalls175860; original177690 owns validity/reset. No effect pool pointer, bitmap, handle, allocation result, visibility, PVS, local membership, channel, clock, seed, audio flag or success is written by the new tool.',
                'The unchanged source clock tick0/dt3D088889 remains inherited. Bounded root phases are not naturally advancing complete original world ticks.',
                'Only the same declared incoming command flags and source-produced grenade selector are supplied. Original effect/animation bodies execute when their inherited unavailable leaves are removed; all remaining platform/feedback/resource boundaries remain recorded.',
                'Original1.1 execution; a 1.5 owning runtime, grenade creation/release/fuse/bounce/explosion and live native Q handler are not inferred from begin admission.'])
        motion.frozen_write(args.output,motion.encoded(value))
        print(json.dumps(dict(report=str(args.output),sha256=motion.digest(args.output),
            constructor_stages=[dict(name=s['name'],completion=s['completion'],invalid=s['invalid']) for s in constructor_stages],
            cases=[dict(name=c['name'],last=c['stages'][-1],final=c['final']['grenade']) for c in cases])),flush=True)
    finally:
        if process.poll() is None:
            try:process.stdin.write('x');process.stdin.flush();process.wait(timeout=15)
            except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
        for stream in (process.stdin,process.stdout,process.stderr):stream.close()


if __name__=='__main__':main()

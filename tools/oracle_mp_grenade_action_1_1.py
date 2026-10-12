#!/usr/bin/env python3
"""Separate native grenade intents through actual allocated player/world owners.

Only incoming command flags/selector are declared. Original unit ownership,
counts, current selection, action state, animation, allocation and release
execute without new supplied returns. The frozen source clock is retained;
successive composed phases are not a recovered advancing match clock.
"""
import argparse
import json
import os
import struct
import subprocess
from pathlib import Path

from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP
import oracle_mp_command_locomotion_pose_1_1 as motion

ROOT = motion.ROOT
ENTRIES = {0xe7f30, 0xe7e60, 0xe7ff0, 0xe95e0, 0xe99b0, 0xe9a50,
           0xe8d60, 0xe8f30, 0xcd310, 0xcf440, 0x1055f0, 0x112270,
           0xefee0, 0x115650, 0x1ca8d0, 0x148160, 0xefe30, 0xc9a40,
           0xb8820, 0xb8a30, 0xba2a0, 0x1781f0, 0xbcd80, 0xa99f0,
           0xa87e0, 0xcc320, 0xfe900, 0x10df90, 0xbd0c0, 0xa8620,
           0xb9430, 0xd2310, 0x101640}


class GrenadeTrace(motion.CommandPoseTrace):
    def grenade_state(self):
        pool = self.u(0x4e78d0)
        unit = self.u(self.u(pool+0x44)+(self.spawned_unit & 0xffff)*12+8)
        component = unit+struct.unpack('<h', self.g.mem_read(unit+0x346, 2))[0]
        return dict(unit_pointer=f'{unit:08x}', action_pointer=f'{component:08x}',
                    current=struct.unpack('<b', self.g.mem_read(unit+0x23c, 1))[0],
                    requested=struct.unpack('<b', self.g.mem_read(unit+0x23d, 1))[0],
                    counts=list(self.g.mem_read(unit+0x23e, 2)),
                    action44_hex=self.raw(component, 0x44),
                    active_words=[f'{self.u(component+4+n*4):08x}' for n in range(2)],
                    phase=self.u(component+0xc, 1), timer=self.u(component+0xe, 1),
                    release_timer=self.u(component+0xf, 1),
                    held_object=f'{self.u(component+0x10):08x}',
                    held_type=struct.unpack('<h', self.g.mem_read(component+0x14, 2))[0])

    def snapshot(self):
        result = super().snapshot()
        result['grenade'] = self.grenade_state()
        return result

    def packets(self, entry, registers, stack):
        result = super().packets(entry, registers, stack)
        def add(name, pointer, size):
            result.append(dict(field=name, pointer=f'{pointer:08x}', size=size,
                               raw_hex=self.raw(pointer, size) if pointer else None))
        if entry == 0xe7f30:
            add('edi_original_action_request20', registers['edi'], 0x20)
        if entry == 0xe95e0:
            add('stack1_original_grenade_request20', stack[1], 0x20)
        if entry == 0xb8820:
            add('esi_original_generic_descriptor_c4', registers['esi'], 0xc4)
            add('stack2_optional_location_c', stack[2], 12)
        if entry == 0xb8a30:
            add('stack0_original_creation_descriptor_c4', stack[0], 0xc4)
        if entry == 0x115650:
            add('stack3_selected_original_channel_output4', stack[3], 4)
        if entry == 0xe8f30:
            add('stack2_optional_point12', stack[2], 12)
            add('stack3_optional_direction12', stack[3], 12)
        return result


def metadata(trace):
    cache = trace.cache
    global_pointer = trace.u(0x4e791c)
    records = []
    for i in range(trace.u(global_pointer+0x100)):
        address = trace.u(global_pointer+0x104)+i*0x2c
        datum = trace.u(address+0x28)
        tag = cache.tags.get(datum)
        records.append(dict(index=i, pointer=f'{address:08x}', raw2c_hex=trace.raw(address, 0x2c),
                            projectile=tag,
                            raw_projectile_hex=None if not tag else trace.raw(tag['offset']+cache.magic, 0x2a0)))
    return dict(matg_pointer=f'{global_pointer:08x}', grenade_block8_hex=trace.raw(global_pointer+0x100, 8),
                records=records, dispatch_table=f'{trace.u(0x46ec68+22*4):08x}',
                dispatch_record10_hex=trace.raw(trace.u(0x46ec68+22*4), 16))


def run_case(trace, name, selected, frames):
    trace.probe = name
    trace.observe = True
    begin = {key:len(getattr(trace, key)) for key in ('calls', 'states', 'reads', 'stores', 'points')}
    stages = []
    inputs = []
    def run(label, entry, args=(), regs=()):
        before = trace.state_id()
        ok = trace.stage(name+'_'+label, entry, args, regs)
        trace.probe_returns(motion.STOP)
        stages.append(dict(name=label, entry=f'{entry:08x}', before_state=before, after_state=trace.state_id(),
                           completion=trace.player_stages[-1]['completion'], invalid=trace.new_invalid,
                           stopped_eip=f'{trace.g.reg_read(UC_X86_REG_EIP):08x}'))
        return ok

    # A valid positive selector is obtained by the unchanged source count scan.
    selector = -1
    if selected:
        assert run('source_available_grenade_selector', 0xcd310,
                   regs=((UC_X86_REG_EAX, trace.spawned_unit), (UC_X86_REG_ECX, 0xffffffff), (UC_X86_REG_EBX, 0)))
        selector = struct.unpack('<h', (trace.g.reg_read(UC_X86_REG_EAX)&0xffff).to_bytes(2, 'little'))[0]
    for frame in range(frames):
        if not run(f'frame_{frame}_neutral_command_initializer', 0x14e780,
                   regs=((UC_X86_REG_EDX, motion.command.COMMAND),)):
            break
        # Select on the first neutral phase; Q is one later host pulse. The
        # unselected control instead sends Q immediately against constructor NONE.
        flags = 0x04000000 if frame == int(selected) else 0
        before = trace.raw(motion.command.COMMAND, 0x5c)
        trace.w(motion.command.COMMAND, flags)
        trace.g.mem_write(motion.command.COMMAND+0x22, struct.pack('<h', selector))
        inputs.append(dict(frame=frame, supplied_native_throw_pulse=bool(flags),
                           delivered_selector=selector, before5c_hex=before,
                           after5c_hex=trace.raw(motion.command.COMMAND, 0x5c),
                           only_declared_fields=['incoming command+0 flags', 'incoming command+22 selector']))
        record = trace.u(0x4d6d44)+0x80+(trace.spawned_player&0xffff)*0x88
        phases = [('submission', 0x852a0, (record, motion.command.COMMAND)),
                  ('assembly', 0x69350, (trace.u(0x4d6d44), motion.command.PACKET)),
                  ('player', 0x14eae0, (motion.command.PACKET,)),
                  ('prephysics', 0xb7e00, ()), ('world', 0x1c7660, ()),
                  ('postphysics', 0xb7f50, ()), ('newflags', 0xb8040, ())]
        if not all(run(f'frame_{frame}_{label}', entry, args) for label, entry, args in phases):
            break
        print(json.dumps(dict(case=name, frame=frame, grenade=trace.grenade_state())), flush=True)
    trace.observe = False
    unreturned = [row['id'] for _, _, row in trace.pending]
    if unreturned:
        assert stages[-1]['completion'] == 'blocked_original_instruction'
    trace.pending = []
    return dict(name=name, inputs=inputs, source_count_scan_selector=selector,
                ranges={key:[start, len(getattr(trace, key))] for key, start in begin.items()},
                stages=stages, unreturned_original_calls=unreturned, final=trace.grenade_state())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--frames', type=int, default=12)
    args = parser.parse_args()
    assert args.frames >= 2
    pins = {**motion.FROZEN, 'tools/oracle_mp_command_locomotion_pose_1_1.py':motion.digest(ROOT/'tools/oracle_mp_command_locomotion_pose_1_1.py')}
    motion.ENTRIES.update(ENTRIES)
    motion.STATE_ENTRIES.update(ENTRIES)
    motion.CommandPoseTrace = GrenadeTrace
    process = subprocess.Popen(['wine', str(motion.command.CONTEXT_OUT/'generator.exe')],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                               env=dict(os.environ, WINEDEBUG='-all', WINEPREFIX=str(motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        context = json.loads(process.stdout.readline())
        trace, correlation = motion.prepare(context)
        original_metadata = metadata(trace)
        backing, cpu = trace.source_fixture, trace.source_fixture_cpu
        cases = []
        print(json.dumps(dict(prior_runtime_identical=True, source_allocated_player=f'{trace.spawned_player:08x}',
                              source_allocated_unit=f'{trace.spawned_unit:08x}')), flush=True)
        for name, selected in [('constructor_unselected_Q', False), ('source_selected_frag_Q', True)]:
            for lo, hi, permissions, raw in backing:
                trace.g.mem_write(lo, raw)
            trace.g.context_restore(cpu)
            cases.append(run_case(trace, name, selected, args.frames))
        for path, digest in pins.items():
            assert motion.digest(ROOT/path) == digest, path
        report = dict(schema=1, tool_sha256=motion.digest(__file__), source_pins=pins,
                      prior_runtime_correlation=correlation, source_metadata=original_metadata,
                      cases=cases, calls=trace.calls, states=trace.states, reads=trace.reads,
                      stores=trace.stores, points=trace.points, instructions=motion.listings(trace, args.output.parent),
                      inherited_services=trace.events, inherited_physical_services=trace.physical_services,
                      new_supplied_gameplay_fields=[], new_supplied_source_returns=[], instruction_patches=[],
                      boundaries=['Original constructors/join/spawn/loadout/header/world/physics own the player, Spartan, carried SMG, two starting frag grenades and action component. Only declared incoming command flags/selector are changed.',
                                  'The valid selector is obtained by unchanged CD310 consuming actual source counts, then delivered as a host command. Local polling/member/readiness/session ownership remains unresolved.',
                                  'Complete ordinary player/prephysics/world/postphysics/newflags order executes with the inherited fixed clock. Repeated composed phases do not establish natural advancing match time or animation duration.',
                                  'Existing real raw storage/cache/resource/compatibility boundaries remain inherited. No grenade allocator, attachment, action, admission, count or release success is supplied; first unavailable instructions remain recorded.'])
        motion.frozen_write(args.output, motion.encoded(report))
        print(json.dumps(dict(report=str(args.output), sha256=motion.digest(args.output),
                              cases=[dict(name=c['name'], last=c['stages'][-1], final=c['final']) for c in cases])), flush=True)
    finally:
        if process.poll() is None:
            try:
                process.stdin.write('x')
                process.stdin.flush()
                process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        for stream in (process.stdin, process.stdout, process.stderr):
            stream.close()


if __name__ == '__main__':
    main()

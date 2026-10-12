#!/usr/bin/env python3
"""Observe original round phases on a source-joined player and constructed world.

Uses the frozen source API preparation from the repeated full-reset probe.
An explicit original phase-request API stimulus starts each inter-round wait;
this is not a combat-produced round ending or a whole world/session tick.
Clock delivery, startup, raw compatibility backing, cold audio contexts and
optional creation services retain that probe's declared scope. All inherited
gameplay/resource returns are guarded during new phase/post-world calls.
Respawn admission is observed independently, including rejection; subsequent
direct spawning does not resume or establish an admitted player-update flow.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_EIP
import oracle_round_reset_scenario as scenario
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_mp_full_scoreboard_1_5 import _Code

ROOT = scenario.ROOT
OUT = ROOT / 'analysis/multiplayer/game-variants/owned-round-phase'
PINS = {
    'tools/oracle_round_reset_scenario.py': '2675c1410054df1aa7471071541615b30fca855ef71a41ee25c748ef12e0f419',
    'analysis/multiplayer/game-variants/round-reset-scenario/report.json': '43060421aa45c8a4fa36b570f94a641cfb234dad2b1db2fe899ebb0baa4319cd',
    'analysis/multiplayer/game-variants/round-reset-scenario/verification.json': 'f36906262d673de3283f3b6ce50695f9e9167844325eca6dbc7015671368f1e0',
}


class PhaseTrace(scenario.ScenarioTrace):
    full_tag_postprocess = True

    def __init__(self, data):
        super().__init__(data)
        self.starts.update((0x15dbc0, 0x15e070, 0x159180, 0x15f2a0,
                            0x159120, 0x15c370, 0x1614f0, 0xa85c0, 0x15f3a0,
                            0x14e9e0, 0x688f0))

    def phase_state(self):
        engine = self.u(0x4f10b8)
        clock = self.u(0x518444)
        raw = bytes(self.g.mem_read(engine, 0xc18))
        signed = lambda off, size: int.from_bytes(raw[off:off+size], 'little', signed=True)
        return dict(engine=f'{engine:08x}', engine_c18_hex=raw.hex(),
                    clock=f'{clock:08x}', clock_24_hex=bytes(self.g.mem_read(clock, 0x24)).hex(),
                    tick=self.u(clock+8), tick_rate=self.u(clock+2, 2),
                    phase=signed(0x6c, 2), index=signed(0x6e, 2),
                    countdown=signed(0xc00, 4), status=signed(0xc04, 4),
                    game_over=self.u(self.u(0x4edf18)+0x1128, 1),
                    recording_hex=bytes(self.g.mem_read(0x518490, 2)).hex(),
                    players=self.player_state(), world=self.raw_state())

    def phase_call(self, name, entry, regs=()):
        before = self.phase_state()
        start_events, start_calls, start_frees = len(self.events), len(self.teardown_calls), len(self.free_records)
        self.active = name
        self.new_invalid = None
        self.teardown = True
        try:
            self.run(entry, regs=regs)
            completion = 'complete_original_return'
        except (unicorn.UcError, KernelBoundary):
            assert self.new_invalid is not None
            completion = 'stopped_original_instruction'
        finally:
            self.teardown = False
        return dict(name=name, entry=f'{entry:08x}', completion=completion,
                    explicit_register_inputs=[dict(register=reg, value=value)for reg, value in regs],
                    before=before, after=self.phase_state(), stop=self.new_invalid,
                    eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}', observed_instructions=self.count,
                    returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
                    services=self.events[start_events:], calls=self.teardown_calls[start_calls:],
                    storage_releases=len(self.free_records)-start_frees, guarded=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT/'report.json')
    args = parser.parse_args()
    for path, pin in PINS.items():
        assert scenario.prior.sha(ROOT/path) == pin, path
    args.output.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.Popen(['wine', str(scenario.prior.CONTEXT_OUT/'generator.exe')],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True, env=dict(os.environ, WINEDEBUG='-all',
                            WINEPREFIX=str(scenario.prior.CONTEXT_OUT/'wine-prefix')))
    events, cycles = [], []
    try:
        data = json.loads(proc.stdout.readline())
        scenario.prior.ResetTrace = PhaseTrace
        t = scenario.prior.prepare(data, True)
        player = scenario.prior.join(t, 0)
        events.append(t.phase_call('explicit_original_request_first_active', 0x1614f0,
                                  ((UC_X86_REG_EDX, 1),)))
        events.append(t.phase_call('original_first_active_phase', 0x15dbc0))
        print(json.dumps(dict(stage=events[-1]['name'], completion=events[-1]['completion'],
                              stop=events[-1]['stop'], phase=events[-1]['after']['phase'])), flush=True)
        if events[-1]['completion'] == 'complete_original_return':
            assert events[-1]['after']['phase'] == 1
            events.append(t.phase_call('original_respawn_admission_before_activation', 0x15f3a0,
                                      ((UC_X86_REG_EAX, player),)))
            events.append(t.phase_call('original_player_activation_scan', 0x14e9e0))
            print(json.dumps(dict(stage=events[-1]['name'], completion=events[-1]['completion'],
                                  stop=events[-1]['stop'])), flush=True)
        if events[-1]['completion'] == 'complete_original_return':
            for cycle in range(2):
                start = len(events)
                eligible = t.phase_call('original_respawn_admission_before_direct_spawn', 0x15f3a0,
                                        ((UC_X86_REG_EAX, player),))
                events.append(eligible)
                if eligible['completion'] != 'complete_original_return':
                    break
                assert int(eligible['returned_eax'], 16)&255 == 1
                assert t.stage('original_direct_player_spawn', 0x1509c0, (player,))
                assert t.g.reg_read(UC_X86_REG_EAX)&255 == 1
                spawn = t.player_stages[-1]
                events.append(t.phase_call('explicit_original_request_phase2', 0x1614f0,
                                          ((UC_X86_REG_EDX, 2),)))
                events.append(t.phase_call('original_phase2_post_world', 0x15e070))
                if events[-1]['completion'] != 'complete_original_return':
                    break
                for tick in range(180):
                    # Declared monotonically delivered simulation-clock input.
                    clock = t.u(0x518444)
                    t.w(clock+8, t.u(clock+8)+1)
                    event = t.phase_call(f'original_inter_round_post_world_{tick+1}', 0x15e070)
                    events.append(event)
                    if event['completion'] != 'complete_original_return':
                        break
                cycles.append(dict(index=cycle, events=list(range(start, len(events))),
                                   spawn=spawn, constructed_world_after=t.state(),
                                   audio_after=t.audio_state(),
                                   direct_spawn_is_independent_of_admission=True))
                print(json.dumps(dict(stage=events[-1]['name'], completion=events[-1]['completion'],
                                      stop=events[-1]['stop'], phase=events[-1]['after']['phase'],
                                      index=events[-1]['after']['index'], deleted=len(t.free_records))), flush=True)
                if events[-1]['completion'] != 'complete_original_return':
                    break
        section = t.image.section('.text')
        text = t.image.section_bytes(section)
        assert bytes(t.g.mem_read(section.va, len(text))) == text
    finally:
        _, error = proc.communicate('x', timeout=45)
    assert proc.returncode == 0, error
    for path, pin in PINS.items():
        assert scenario.prior.sha(ROOT/path) == pin, path
    directory = args.output.parent/'instructions'
    directory.mkdir(exist_ok=True)
    code = _Code(t.image, '.text')
    rows = [code.at(address)for address in sorted(t.executed)
            if section.va <= address < section.va+len(text)]
    listing = directory/'tu11-executed-original.asm'
    listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
    dependencies = {str(Path(m.__file__).resolve().relative_to(ROOT)): scenario.prior.sha(m.__file__)
                    for m in list(sys.modules.values())if getattr(m, '__file__', None)
                    and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')}
    report = scenario.prior.compact_raw_records(dict(
        schema=1, boundary=__doc__, prior_pins=PINS, source_dependencies=dependencies,
        source_sha256=scenario.prior.PIN, cache_sha256=scenario.prior.CACHE_PIN,
        oracle_sha256=scenario.prior.sha(__file__),
        generator_sha256=scenario.prior.sha(scenario.prior.CONTEXT_OUT/'generator.exe'),
        variant=t.variant, source_player=f'{player:08x}', preparation=t.producers,
        activation_context=dict(simulation_initialized_byte=t.u(0x4d6d38, 1),
                                simulation_global_pointer=f'{t.u(0x4d6d44):08x}'),
        sound_initializer=t.sound_prefix, independent_device_constructor=t.cold_device,
        tag_postprocess=t.tag_postprocess, tag_index_delivery=t.tag_index_delivery,
        new_preparation=t.new_preparation, events=events, cycles=cycles,
        storage_release_records=t.free_records, whole_guest_text_unchanged=True,
        instruction_patches=[], new_supplied_gameplay_returns=[],
        executed_instruction_listing=dict(file=listing.name, sha256=scenario.prior.sha(listing), rows=len(rows))))
    args.output.write_text(json.dumps(report, sort_keys=True, separators=(',', ':'))+'\n')
    print(json.dumps(dict(report=str(args.output), sha256=scenario.prior.sha(args.output),
                         completion=events[-1]['completion'], stop=events[-1]['stop'])), flush=True)


if __name__ == '__main__':
    main()

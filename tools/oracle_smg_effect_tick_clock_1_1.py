#!/usr/bin/env python3
"""Natural effect frame with source-produced ordinal156 backing after time proof."""
import argparse
import json
import os
import subprocess
from pathlib import Path
from unicorn.x86_const import *
import oracle_smg_effect_system_time_1_1 as system
from oracle_smg_effect_owner_1_1 import sha, save, encoded

ROOT = system.ROOT
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-tick-clock'
FROZEN = {**system.FROZEN,
    'tools/oracle_smg_effect_system_time_1_1.py': 'f2ad079599f86f9d1c15fc5f69635942005f59b4476aeeaea2c15a7e3e27fcb7',
    'docs/MP_SMG_EFFECT_SYSTEM_TIME_RESEARCH.md': 'ae2e02702443c07224ec16f488024cf9b81c167c2b50d81fc3b9c0d8c7b6fdbb',
    'analysis/multiplayer/integration/inventory-source/smg-effect-system-time/measured-real/report.json': '8c88dab9c22d3569c23b1f780f094583e567f29112d67ccc4d9812799e74704b',
    'analysis/multiplayer/integration/inventory-source/smg-effect-system-time/manifest.json': 'f0da5157fa4f33a7c1b900f06eb2a1de52fe83753da35e890b4973f0b34c9ede'}
BACKING = 0x0b000000


def provider(out):
    prior = json.loads((system.OUT / 'measured-real/report.json').read_text())
    contract = prior['provider_contract']
    original = system.OUT / 'provider/compiled-system-clock.cpp'
    assert sha(original) == contract['compiled_source_sha256']
    for name, pin in contract['inputs'].items():
        assert sha(ROOT / name) == pin, name
    source = original.read_text()
    old = '}else std::abort();'
    assert source.count(old) == 1
    source = source.replace(old, '}else if(op==\'p\') {auto now=get_now();auto next=pit_next(now);Report("update",now,next);}else std::abort();')
    cpp, exe = out / 'compiled-tick-clock.cpp', out / 'compiled-tick-clock.exe'
    save(cpp, source.encode())
    command = ['i686-w64-mingw32-g++', '-std=c++20', '-O0', '-static', '-Wl,--no-insert-timestamp', str(cpp), '-o', str(exe)]
    result = subprocess.run(command, capture_output=True, text=True)
    save(out / 'compiler.log', (result.stdout + result.stderr).encode())
    assert result.returncode == 0, result.stderr
    assert 'Contains the number of milliseconds elapsed since the system was started.' in system.SDK.read_text()
    ke = (system.NATIVE / 'primary-source/EmuKrnlKe.cpp').read_text()
    assert 'XBSYSAPI EXPORTNUM(156) xbox::dword_xt VOLATILE xbox::KeTickCount = 0;' in ke
    metadata = dict(schema='source-tick-clock-provider-v1', compiled_source_sha256=sha(cpp), compiled_binary_sha256=sha(exe), compiler_command=command, prior_system_time_provider=contract, unchanged_timer_and_query_bodies=True, added_transport='p: actual get_now→pit_next→KiClockIsr, then read actual volatile KeTickCount; no clock/dt or game value chosen.', tick_abi='ordinal156 volatile DWORD data, milliseconds since source startup; original337A40/45/47 return EAX with no arguments.', startup_scope='Replay actual prior captured initialization and first query through unchanged bodies. Additional observations use actual host QPC, with full replay for independent reproduction. No complete event-loop/title-startup claim.')
    save(out / 'contract.json', encoded(metadata))
    return exe, metadata


class NativeClock:
    def __init__(self, path, replay=None):
        self.rows, self.private, self.replay = [], [], replay
        self.prefix = json.loads((system.OUT / 'measured-real/native-clock-inputs.json').read_text())
        self.p = subprocess.Popen(['wine', str(path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=dict(os.environ, WINEDEBUG='-all', WINEPREFIX=str(system.life.birth.local.base.owner.player_source.CONTEXT_OUT / 'wine-prefix')))
        self.request('i')

    def request(self, op):
        index = len(self.rows)
        old = self.replay[index] if self.replay is not None else self.prefix[index] if index < len(self.prefix) else None
        phase = {'i': 'initialize', 'q': 'query', 'p': 'update'}[op]
        if old is None:
            self.p.stdin.write(op + '\n')
        else:
            assert old['phase'] == phase
            observations = old['observations']
            self.p.stdin.write('r ' + str(len(observations)) + '\n' + ''.join(f"{row['kind']} {row['value']}\n" for row in observations) + op + '\n')
        self.p.stdin.flush()
        line = self.p.stdout.readline()
        assert line, line
        row = json.loads(line)
        self.private.append(row.pop('private_native_records'))
        if op == 'q':
            query = json.loads(self.p.stdout.readline())
            assert query['output8_hex'] == row['system12_hex'][:16]
            assert query['system_before12_hex'] == query['system_after12_hex'] == row['system12_hex']
            row['actual_query'] = query
        if old is not None:
            assert old == row, ('clock replay changed', index)
        self.rows.append(row)
        return row

    def close(self):
        _, stderr = self.p.communicate('x\n', timeout=15)
        assert self.p.returncode == 0, stderr
        return stderr


class TickTrace(system.TimeTrace):
    def __init__(self, *args, **kwargs):
        self.tick_deliveries, self.tick_returns, self.import_relocations = [], [], []
        super().__init__(*args, **kwargs)

    def hook(self, guest, address, size, user):
        if self.life_on and self.native_clock is not None:
            if address == 0x800000b6:
                stack = guest.reg_read(UC_X86_REG_ESP)
                self.new_invalid = dict(boundary='New original MmSetAddressProtect request is outside the inherited finite physical-backing provider. Stop before that older provider can supply a result.', kernel_ordinal=182, target='800000b6', original_return=f'{self.u(stack):08x}', args=[f'{self.u(stack + 4 + index * 4):08x}' for index in range(3)], cpu=self.cpu(), stack40_hex=self.raw(stack, 0x40), pending_original_calls=[row['id'] for _, _, row in self.life_pending], kernel_target_unexecuted=True)
                raise system.KernelBoundary()
            if address == 0x337a40 and self.u(0x417cd4) == 0x8000009c:
                assert self.image.read(0x417cd4, 4).hex() == '9c000080'
                guest.mem_map(BACKING, 0x1000)
                self.import_relocations.append(dict(order=self.life_order, instruction='00337a40', import_address='00417cd4', original4_hex=self.raw(0x417cd4, 4), relocated4_hex=BACKING.to_bytes(4, 'little').hex(), explicit_backing=f'{BACKING:08x}', before_cpu=self.cpu(), before_state=self.life_state(), scope='Existing verified ordinal156 compatibility data relocation; no original instruction or loaded EAX is patched. Original337A40 owns loading the relocated address. Backing value remains unconsumed until source producer delivery at337A45.'))
                self.w(0x417cd4, BACKING)
            if address == 0x337a45:
                assert guest.reg_read(UC_X86_REG_EAX) == BACKING
                before = self.raw(BACKING, 4)
                cpu = self.cpu()
                row = self.native_clock.request('p')
                value = bytes.fromhex(row['clock4_hex'])
                guest.mem_write(BACKING, value)
                self.tick_deliveries.append(dict(order=self.life_order, phase=self.active, instruction='00337a45', original_return=f'{self.u(guest.reg_read(UC_X86_REG_ESP)):08x}', output_pointer=f'{BACKING:08x}', before4_hex=before, actual_output4_hex=value.hex(), native_observation_index=len(self.native_clock.rows) - 1, before_cpu=cpu, after_cpu=self.cpu(), scope='Only actual source-owned volatile KeTickCount bytes after real captured performance-counter producer update; original load/return remain executing.'))
                self.life_order += 1
            if address == 0x337a47:
                self.tick_returns.append(dict(order=self.life_order, instruction='00337a47', cpu=self.cpu(), native_observation_index=len(self.native_clock.rows) - 1, eax4_hex=guest.reg_read(UC_X86_REG_EAX).to_bytes(4, 'little').hex()))
        super().hook(guest, address, size, user)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'measured/report.json')
    parser.add_argument('--clock-replay', type=Path)
    args = parser.parse_args()
    pin = sha(__file__)
    for file, expected in FROZEN.items():
        assert sha(ROOT / file) == expected, file
    executable, contract = provider(OUT / 'provider')
    source = system.life.birth.local.base.owner.player_source
    worker = subprocess.Popen(['wine', str(source.CONTEXT_OUT / 'generator.exe')], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=dict(os.environ, WINEDEBUG='-all', WINEPREFIX=str(source.CONTEXT_OUT / 'wine-prefix')))
    clock = None
    try:
        data = json.loads(worker.stdout.readline())
        # Select only the extension's observer class for the immutable helper.
        # This changes no file, owning root, source state or source result.
        previous_type = system.TimeTrace
        system.TimeTrace = TickTrace
        try:
            trace = system.prepare(data)
        finally:
            system.TimeTrace = previous_type
        assert trace.life_stage('original_fixed_effect_phase_after_actual_birth', 0x1788b0)
        prior = json.loads((system.OUT / 'measured-real/report.json').read_text())
        assert trace.life_stages[0] == prior['stages'][0]
        fixed = [row for row in prior['calls'] if row['phase'] == prior['stages'][0]['name']]
        assert trace.life_calls == fixed and trace.life_states == prior['states']
        clock = NativeClock(executable, json.loads(args.clock_replay.read_text()) if args.clock_replay else None)
        trace.native_clock = clock
        trace.life_stage('original_outer_frame_with_source_platform_system_time', 0x12cbc0)
        print(json.dumps(trace.life_stages[-1]), flush=True)
        listings = []
        entries = set(system.life.ENTRIES) | {int(row['entry'], 16) for row in trace.life_calls}
        listing_inputs = [(f'{entry:08x}', system.life._trace(trace.code, entry, trace.starts | entries).code) for entry in sorted(entries)]
        listing_inputs += [('all-executed', trace.life_executed), ('all-stores', {int(row['instruction'], 16) for row in trace.life_stores if trace.code.at(int(row['instruction'], 16)) is not None})]
        for label, addresses in listing_inputs:
            instructions = [trace.code.at(address) for address in sorted(addresses)]
            for instruction in instructions:
                assert instruction.bytes == trace.image.read(instruction.address, instruction.size) == bytes(trace.g.mem_read(instruction.address, instruction.size))
            path = args.output.parent / 'instructions' / f'{label}.asm'
            save(path, ''.join(f'{item.address:08x} {item.bytes.hex()} {item.mnemonic} {item.op_str}\n' for item in instructions).encode())
            listings.append(dict(file=str(path.relative_to(args.output.parent)), sha256=sha(path), rows=len(instructions), executed_rows=sum(item.address in trace.life_executed for item in instructions)))
        save(args.output.parent / 'native-clock-inputs.json', encoded(clock.rows))
        save(args.output.parent / 'native-private-records.json', encoded(clock.private))
        save(args.output.parent / 'native-clock-stderr.log', clock.close().encode())
        clock = None
        for file, expected in FROZEN.items():
            assert sha(ROOT / file) == expected, file
        assert sha(__file__) == pin
        # The measured caller prefix, including system-time/CRT returns, stays
        # exact up to the deliberate data-import relocation at337A40.
        for old, new in zip(prior['calls'], trace.life_calls):
            assert all(new[key] == value for key, value in old.items()), ('pre-tick call prefix changed', old['id'])
        assert trace.clock_inputs == prior['actual_platform_clock_delivery']
        report = dict(schema='source-smg-effect-tick-clock-v1', oracle_sha256=pin, frozen_pins=FROZEN, provider_contract=contract, native_clock_inputs_sha256=sha(args.output.parent / 'native-clock-inputs.json'), prior_item_local_birth_fixed_and_pre_tick_call_prefix_identical=True, stages=trace.life_stages, calls=trace.life_calls, states=trace.life_states, reads=trace.life_reads, stores=trace.life_stores, points=trace.life_points, actual_platform_clock_delivery=trace.clock_inputs, actual_tick_deliveries=trace.tick_deliveries, actual_tick_returns=trace.tick_returns, data_import_relocations=trace.import_relocations, instructions=listings, outside_source_text=trace.life_external, new_game_or_particle_state_inputs=[], instruction_patches=[], boundaries=['Actual captured platform-clock observations and untouched original producers only; no dt, game ticks, particle state or admission supplied.', 'Explicit existing ordinal156 backing relocation is a compatibility data import; source instructions and original load/return stay unchanged.', 'First system-time producer observations are exact captured prior inputs; later QPC observations are actual host inputs, with replay through the same bodies for independent reproduction.', 'Natural outer-frame root stops at its first unavailable owner; no successful startup, elapsed, continuous aging, rendering or Xbox hardware parity inferred.'])
        save(args.output, encoded(report))
        print(json.dumps(dict(output=str(args.output), sha256=sha(args.output), tick_reads=len(trace.tick_deliveries), calls=len(trace.life_calls), states=len(trace.life_states), stores=len(trace.life_stores))), flush=True)
    finally:
        if clock is not None:
            clock.close()
        try:
            _, stderr = worker.communicate('x', timeout=15)
        except subprocess.TimeoutExpired:
            worker.kill()
            worker.wait(timeout=5)
            stderr = 'context worker shutdown timeout'
    if worker.returncode:
        raise RuntimeError((worker.returncode, stderr))


if __name__ == '__main__':
    main()

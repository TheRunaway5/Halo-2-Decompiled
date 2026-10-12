#!/usr/bin/env python3
"""Natural original effect frame after its real configuration-capacity owner.

The immutable preceding frame failure is preserved. This extension inserts the
existing independently executed whole663B0 owner as a selected startup root.
No capacity, state, elapsed, heap result or particle field is supplied.
"""
import argparse
import json
import os
import subprocess
from pathlib import Path
import oracle_smg_effect_frame_admission_1_1 as frame
from oracle_smg_effect_owner_1_1 import sha, save, encoded

ROOT = frame.ROOT
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-frame-configuration'
PRIOR = {**frame.PRIOR,
    'tools/oracle_smg_effect_frame_admission_1_1.py': '39c85a85234cbb3a9f2544ba55cd1632d1d5cfaa3c844ccd6b3afa30d7038695',
    'tools/research_smg_frame_admission_1_1.py': 'ff971a1888aff5b6b6c16b9d95b1a389f2d3c3bbb0f19b5622856c17114f521a',
    'tools/audit_smg_effect_frame_admission.py': 'ac90c376d694b422acddda593269e70b36689370a8165d0d33d27c17fa3e5b5d',
    'docs/MP_SMG_EFFECT_FRAME_ADMISSION_RESEARCH.md': 'a94bb3fe572d1fe80a0de855de5b917e0307ffb4fe7397056e12e9fea13220cd',
    'analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json': '5026aa2ab63cfd35f613942e57467cc8763b13ccb0ab9b353286451e469f04a4',
    'analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/manifest.json': '36bd453f171fc5114b36b8a9ae514c43422f6cd640d96e5318e053012d8547c0',
    'tools/oracle_mp_spawn_command_input.py': 'de382cb5f14ccbc64fbc5609419382f2a574fc1f46ba00e371efbfa3a641f4fc',
    'analysis/multiplayer/integration/spawn-source/command-input/report.json': '1fc1b58c65fd804477ac9bbdcd3bcad1ad10bd954f98b0f5ea17348891ba0a76'}
EXTRA = {0x663b0, 0x663a0, 0x663f0, 0x67a70, 0x66f50, 0x67230, 0x673a0}


class ConfigurationTrace(frame.AdmissionTrace):
    def __init__(self, *args, **kwargs):
        self.configuration_records = []
        self.configuration_inserted = False
        super().__init__(*args, **kwargs)
        # Extend only this independent observer's original call/operand coverage.
        self.starts |= EXTRA
        for entry in sorted(EXTRA):
            flow = frame.tick.system.life._trace(self.code, entry, self.starts)
            self.life_routines[entry] = flow
            for address in flow.code:
                self.life_owners[address] = entry

    def configuration_state(self):
        return dict(handle47e548_8_hex=self.raw(0x47e548, 8),
                    literal4d0000_7000_hex=self.raw(0x4d0000, 0x7000),
                    heap_capacity4d6d24_8_hex=self.raw(0x4d6d24, 8),
                    frame=self.frame_state(), current=self.snapshot())

    def life_stage(self, name, entry):
        if entry == 0x12cbc0 and not self.configuration_inserted:
            self.configuration_inserted = True
            before = self.configuration_state()
            first_call, first_store = len(self.life_calls), len(self.life_stores)
            ok = super().life_stage('original_configuration_default_handler_selected_startup_root', 0x663b0)
            self.configuration_records.append(dict(
                entry='000663b0', before=before, after=self.configuration_state(),
                first_call=first_call, final_call=len(self.life_calls),
                first_store=first_store, final_store=len(self.life_stores),
                complete_original_return=ok,
                scope='Existing original whole configuration owner selected before natural frame; complete upstream title constructor order remains explicit. No capacity or success is written by the observer.'))
            if not ok:
                return False
        return super().life_stage(name, entry)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'measured/report.json')
    parser.add_argument('--clock-replay', type=Path)
    args = parser.parse_args()
    pin = sha(__file__)
    for name, expected in PRIOR.items():
        assert sha(ROOT / name) == expected, name
    executable, contract = frame.tick.provider(OUT / 'provider')
    source = frame.tick.system.life.birth.local.base.owner.player_source
    worker = subprocess.Popen(['wine', str(source.CONTEXT_OUT / 'generator.exe')], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                              env=dict(os.environ, WINEDEBUG='-all', WINEPREFIX=str(source.CONTEXT_OUT / 'wine-prefix')))
    clock = None
    try:
        data = json.loads(worker.stdout.readline())
        previous_type = frame.tick.system.TimeTrace
        frame.tick.system.TimeTrace = ConfigurationTrace
        try:
            trace = frame.tick.system.prepare(data)
        finally:
            frame.tick.system.TimeTrace = previous_type
        assert trace.life_stage('original_fixed_effect_phase_after_actual_birth', 0x1788b0)
        prior = json.loads((frame.tick.system.OUT / 'measured-real/report.json').read_text())
        assert trace.life_stages[0] == prior['stages'][0]
        fixed = [row for row in prior['calls'] if row['phase'] == prior['stages'][0]['name']]
        assert trace.life_calls == fixed and trace.life_states == prior['states']
        clock = frame.tick.NativeClock(executable, json.loads(args.clock_replay.read_text()) if args.clock_replay else None)
        trace.native_clock = clock
        trace.life_stage('original_outer_frame_after_source_configuration_owner', 0x12cbc0)
        print(json.dumps(dict(stages=[dict(name=row['name'], completion=row['completion'], eip=row['stopped_eip']) for row in trace.life_stages], stop=trace.life_stages[-1]['invalid'])), flush=True)
        listings = []
        entries = set(frame.tick.system.life.ENTRIES) | EXTRA | {int(row['entry'], 16) for row in trace.life_calls}
        listing_inputs = [(f'{entry:08x}', frame.tick.system.life._trace(trace.code, entry, trace.starts | entries).code) for entry in sorted(entries)]
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
        # Configuration intentionally changes original state before the frame.
        # Earlier item/local/birth and fixed phase were compared above; the old
        # outer call IDs/registers are not asserted identical after this owner.
        report = dict(schema='source-smg-effect-frame-configuration-v1', oracle_sha256=pin,
                      underlying_frame_oracle_sha256=sha(frame.__file__), underlying_tick_oracle_sha256=sha(frame.tick.__file__),
                      source_xbe_sha256=frame.tick.system.life.birth.local.base.PIN,
                      frozen_pins=PRIOR, provider_contract=contract,
                      native_clock_inputs_sha256=sha(args.output.parent / 'native-clock-inputs.json'),
                      prior_item_local_birth_and_fixed_phase_fields_identical=True,
                      stages=trace.life_stages, calls=trace.life_calls, states=trace.life_states,
                      reads=trace.life_reads, stores=trace.life_stores, points=trace.life_points,
                      configuration_records=trace.configuration_records, frame_records=trace.frame_records,
                      actual_memory_protection=trace.protection_calls, permission_provider=trace.permission_metadata,
                      actual_platform_clock_delivery=trace.clock_inputs, actual_tick_deliveries=trace.tick_deliveries,
                      actual_tick_returns=trace.tick_returns, data_import_relocations=trace.import_relocations,
                      instructions=listings, outside_source_text=trace.life_external,
                      final_frame_state=trace.frame_state(), final_configuration_state=trace.configuration_state(),
                      new_game_or_particle_state_inputs=[], instruction_patches=[],
                      boundaries=['Earlier actual item/local/birth/fixed phase remains exactly verified before the selected configuration owner.',
                                  'Whole original663B0 establishes capacity through actual original stores; full authored title constructor scheduling remains unproved.',
                                  'Actual platform-clock producer and applied permission operations only; no hand-entered capacity/dt/tick/readiness/pose/particle/contact/success.',
                                  'Natural outer-frame stops at its first unavailable owner. No admitted multiplayer elapsed/aging/rendered flash or Xbox hardware parity claim.'])
        save(args.output, encoded(report))
        print(json.dumps(dict(output=str(args.output), sha256=sha(args.output), calls=len(trace.life_calls), states=len(trace.life_states), stores=len(trace.life_stores), clock_reads=len(trace.tick_deliveries))), flush=True)
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
    assert sha(__file__) == pin
    for name, expected in PRIOR.items():
        assert sha(ROOT / name) == expected, name


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Capture actual world-construction input and its unchanged original prefix.

This is private original-execution evidence. Neither the checkpoint nor the
expected results are production resources or substitutes for native startup.
"""
import argparse
import hashlib
import json
import zlib
from pathlib import Path
import unicorn
from unicorn import x86_const as R
import oracle_physics_world_event_runtime_20261011 as retained
from oracle_smg_firing_1_1 import Xbe, PIN
from export_mp_physics_contact_lifecycle import segments

ROOT = retained.ROOT


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--correspondence', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    tool_pin = sha(__file__)
    pair_pin = sha(args.correspondence)
    paired = json.loads(args.correspondence.read_text())
    assert paired['disc_1_1_sha256'] == PIN
    known = {int(p, 16) for p in paired['instruction_map']}
    assert 0x1c5ff0 in known
    authenticated, drift = retained.verify_retained()
    power = retained.prior
    while not hasattr(power, 'power'):
        power = power.prior
    power = power.power
    source_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(source_path) == PIN
    image = Xbe(source_path)
    text = image.section('.text')
    args.output.mkdir(parents=True, exist_ok=False)
    memory = args.output / 'memory'
    memory.mkdir()
    captured, cases = [], []

    class BootstrapTrace(retained.Trace):
        def __init__(self, data):
            self.bootstrap_active = None
            super().__init__(data)
            self.g.hook_add(unicorn.UC_HOOK_MEM_READ, self.bootstrap_read)

        def hook(self, g, address, size, user):
            case = self.bootstrap_active
            if case and (address == case['return_address'] or address not in known or address in self.services):
                case.update(exit=retained.captured_abi(g), exit_entry=address,
                    complete=address == case['return_address'],
                    explicit_service_boundary=address in self.services)
                self.bootstrap_active = None
            if address == 0x1c5ff0:
                assert not captured, 'Multiple actual world constructions need independent checkpoints'
                assert bytes(g.mem_read(text.va, text.rsize)) == image.section_bytes(text)
                record = dict(entry=address, entry_abi=retained.captured_abi(g), regions=[],
                    gdtr=list(g.reg_read(R.UC_X86_REG_GDTR)), ldtr=list(g.reg_read(R.UC_X86_REG_LDTR)),
                    segment_selectors={name: g.reg_read(getattr(R, 'UC_X86_REG_' + name))
                        for name in ('CS', 'DS', 'ES', 'SS', 'FS', 'GS')})
                for index, (start, end, protection) in enumerate(g.mem_regions()):
                    raw = bytes(g.mem_read(start, end - start + 1))
                    compressed = zlib.compress(raw, level=1)
                    path = memory / f'{index:04d}.zlib'
                    path.write_bytes(compressed)
                    record['regions'].append(dict(start=start, size=len(raw), protection=protection,
                        file=path.name, sha256=hashlib.sha256(raw).hexdigest(),
                        compressed_sha256=sha(path), compressed_bytes=len(compressed)))
                captured.append(record)
                case = dict(id=0, phase=self.active, entry=address, entry_abi=record['entry_abi'],
                    return_address=self.u(g.reg_read(R.UC_X86_REG_ESP)), events=[],
                    instructions=[], estimates=[], timestamps=[], packets=[])
                cases.append(case)
                self.bootstrap_active = case
            if self.bootstrap_active:
                assert address in known and address not in self.services
                self.bootstrap_active['instructions'].append(address)
                instruction = self.runtime_code.at(address)
                assert instruction.mnemonic not in ('rsqrtss', 'rdtsc'), 'New CPU observation owner required'
            super().hook(g, address, size, user)

        def bootstrap_read(self, g, _access, address, size, _value, _user):
            if self.bootstrap_active:
                self.bootstrap_active['events'].append(dict(kind='R',
                    instruction=g.reg_read(R.UC_X86_REG_EIP), address=address,
                    raw=bytes(g.mem_read(address, size)).hex()))

        def mem_write(self, g, access, address, size, value, user):
            if self.bootstrap_active:
                self.bootstrap_active['events'].append(dict(kind='W',
                    instruction=g.reg_read(R.UC_X86_REG_EIP), address=address,
                    before=bytes(g.mem_read(address, size)).hex(),
                    written=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))
            super().mem_write(g, access, address, size, value, user)

    context = power.OWNER / 'raw-host-context.json'
    context_pin = sha(context)
    trace = power.player.prepare(json.loads(context.read_text()), trace_type=BootstrapTrace)
    assert len(captured) == len(cases) == 1 and trace.bootstrap_active is None
    assert bytes(trace.g.mem_read(text.va, text.rsize)) == image.section_bytes(text)
    assert trace.player_stages[-1]['completion'] == 'complete_original_return'
    for case in cases:
        initial, current = {}, {}
        for event in case['events']:
            address = event['address']
            raw = bytes.fromhex(event['raw'] if event['kind'] == 'R' else event['before'])
            for offset, value in enumerate(raw):
                p = address + offset
                if p not in current:
                    initial[p] = current[p] = value
                assert current[p] == value
            if event['kind'] == 'W':
                for offset, value in enumerate(bytes.fromhex(event['written'])):
                    current[address + offset] = value
        case['initial_banks'] = [dict(address=p, hex=b.hex()) for p, b in segments(initial)]
        case['final_banks'] = [dict(address=p, hex=b.hex()) for p, b in segments(current)]
        case['writes'] = [e for e in case.pop('events') if e['kind'] == 'W']
    assert sha(__file__) == tool_pin and sha(args.correspondence) == pair_pin and sha(context) == context_pin
    fixture = dict(schema=1, image='1.1', tool_sha256=tool_pin, original_sha256=PIN,
        correspondence_sha256=pair_pin, input_context_sha256=context_pin, cases=cases,
        authenticated_parent_manifest_entries=authenticated, instruction_patches=[],
        source_instruction_patches=[], native_expected_data_used=False,
        live_gameplay_proof=False, production_checkpoint_replay=False,
        scope='Actual original1C5FF0 prefix in the retained selected startup order; no service return admitted within the measured prefix.')
    fixture_path = args.output / 'fixture.json'
    fixture_path.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':')) + '\n')
    checkpoint = dict(schema=1, tool_sha256=tool_pin, original_sha256=PIN,
        parent_fixture_sha256=sha(fixture_path), unchanged_original_prefix_sha256=sha(fixture_path),
        correspondence_sha256=pair_pin, captured_entry=captured[0], original_text_unpatched=True,
        native_expected_data_used=False, production_checkpoint_replay=False,
        live_gameplay_proof=False, original_prepare_completed=True,
        input_context_sha256=context_pin)
    checkpoint_path = args.output / 'checkpoint.json'
    checkpoint_path.write_text(json.dumps(checkpoint, sort_keys=True, indent=2) + '\n')
    (args.output / 'parent-native-drift.json').write_text(json.dumps(drift, sort_keys=True, indent=2) + '\n')
    case = cases[0]
    print(json.dumps(dict(checkpoint_sha256=sha(checkpoint_path), fixture_sha256=sha(fixture_path),
        instructions=len(case['instructions']), writes=len(case['writes']),
        complete=case['complete'], stop=hex(case['exit_entry']),
        explicit_service_boundary=case['explicit_service_boundary'])))


if __name__ == '__main__':
    main()

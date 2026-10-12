#!/usr/bin/env python3
"""Execute unchanged original world code from an authenticated actual entry state.

The previous original prefix must match before any newly admitted owner runs.
Full checkpoint memory is input only; fixtures retain observed first-before
bytes, never recorded expected outputs inserted as initial state.
"""
import argparse
import hashlib
import json
import zlib
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from capstone.x86_const import X86_OP_REG, X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from oracle_physics_world_event_runtime_20261011 import captured_abi
from export_mp_physics_contact_lifecycle import segments


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('checkpoint', 'parent', 'parent-correspondence', 'correspondence', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    tool_pin = sha(__file__)
    checkpoint = json.loads(args.checkpoint.read_text())
    parent = json.loads(args.parent.read_text())
    old_pair = json.loads(args.parent_correspondence.read_text())
    paired = json.loads(args.correspondence.read_text())
    assert checkpoint['tool_sha256'] == sha(ROOT / 'tools/capture_physics_game_frame_20261011.py')
    assert checkpoint['original_sha256'] == PIN and checkpoint['original_text_unpatched']
    assert not checkpoint['native_expected_data_used'] and not checkpoint['production_checkpoint_replay']
    assert sha(args.checkpoint) == '6a64d10d3ac19526287985c2495d9c7262e8a5a881392b529eeb2f9aa74f230e'
    assert sha(args.parent) == '65cef8f47f5cd88931e8b999b1293121017767d8416e0382d2584396cd7f77fa'
    assert checkpoint['parent_fixture_sha256'] == checkpoint['unchanged_original_prefix_sha256'] == parent['parent_fixture_sha256'] == '203e132fa7ccc83744682ba113f7d575366f6164cd38bc7e1327b28a8d4d033c'
    assert parent['tool_sha256'] == checkpoint['tool_sha256']
    assert checkpoint['correspondence_sha256'] == parent['correspondence_sha256'] == sha(args.parent_correspondence)
    assert paired['disc_1_1_sha256'] == old_pair['disc_1_1_sha256'] == PIN
    for key in ('entries', 'instruction_map', 'address_map'):
        for old, later in old_pair[key].items():
            assert paired[key][old] == later, (key, old, 'Original correspondence changed')
    assert len(parent['cases']) == 1
    prior = parent['cases'][0]
    entry = checkpoint['captured_entry']
    assert entry['entry'] == prior['entry'] and entry['entry_abi'] == prior['entry_abi']
    image_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(image_path) == PIN
    image = Xbe(image_path)
    runtime_code = _Code(image, '.text')
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    for region in entry['regions']:
        path = args.checkpoint.parent / 'memory' / region['file']
        assert sha(path) == region['compressed_sha256']
        raw = zlib.decompress(path.read_bytes())
        assert len(raw) == region['size'] and hashlib.sha256(raw).hexdigest() == region['sha256']
        g.mem_map(region['start'], region['size'], region['protection'])
        g.mem_write(region['start'], raw)
    section = image.section('.text')
    assert bytes(g.mem_read(section.va, section.rsize)) == image.section_bytes(section)
    g.reg_write(R.UC_X86_REG_GDTR, tuple(entry['gdtr']))
    if any(entry['ldtr']):
        g.reg_write(R.UC_X86_REG_LDTR, tuple(entry['ldtr']))
    for name, value in entry['segment_selectors'].items():
        g.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
    state = entry['entry_abi']
    for name, value in zip(('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI'), state['gp']):
        g.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
    for i, value in enumerate(state['xmm']):
        g.reg_write(getattr(R, f'UC_X86_REG_XMM{i}'), int(value, 16))
    for name, key in (('EFLAGS', 'eflags'), ('FPCW', 'x87_control_word'), ('MXCSR', 'mxcsr'), ('FPSW', 'x87_status'), ('FPTAG', 'x87_tag')):
        g.reg_write(getattr(R, 'UC_X86_REG_' + name), state[key])
    top = (state['x87_status'] >> 11) & 7
    for i, value in enumerate(state['x87_values']):
        raw = bytes.fromhex(value)
        g.reg_write(getattr(R, f'UC_X86_REG_FP{(top + i) & 7}'),
            (int.from_bytes(raw[:8], 'little'), int.from_bytes(raw[8:], 'little')))
    assert captured_abi(g) == state, 'Restored original entry ABI differs'
    known = {int(p, 16) for p in paired['instruction_map']}
    trace, events, packets, estimates, timestamps = [], [], [], [], []
    pending = None
    exit_entry = None
    prefix_checked = False

    def read(_g, _a, address, size, _v, _u):
        events.append(dict(kind='R', instruction=g.reg_read(R.UC_X86_REG_EIP), address=address,
            raw=bytes(g.mem_read(address, size)).hex()))

    def write(_g, _a, address, size, value, _u):
        events.append(dict(kind='W', instruction=g.reg_read(R.UC_X86_REG_EIP), address=address,
            before=bytes(g.mem_read(address, size)).hex(),
            written=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))

    def code(_g, address, _size, _u):
        nonlocal pending, exit_entry, prefix_checked
        if pending:
            kind, site, target, value = pending
            pending = None
            if kind == 'rsqrtss':
                estimates.append(dict(instruction=site, input=value,
                    output=g.reg_read(getattr(R, 'UC_X86_REG_' + target.upper())) & 0xffffffff))
            else:
                timestamps.append(dict(instruction=site,
                    output=(g.reg_read(R.UC_X86_REG_EDX) << 32) | g.reg_read(R.UC_X86_REG_EAX)))
        if not prefix_checked and len(trace) == len(prior['instructions']):
            assert address == prior['exit_entry'] and trace == prior['instructions']
            assert captured_abi(g) == prior['exit'], 'Original prefix ABI changed'
            assert [e for e in events if e['kind'] == 'W'] == prior['writes']
            assert packets == prior['packets'] and estimates == prior['estimates'] and timestamps == prior['timestamps']
            for bank in prior['final_banks']:
                assert bytes(g.mem_read(bank['address'], len(bank['hex']) // 2)).hex() == bank['hex']
            prefix_checked = True
        if address == prior['return_address'] or address not in known:
            exit_entry = address
            g.emu_stop()
            return
        trace.append(address)
        instruction = runtime_code.at(address)
        if address == 0x300100:
            sp = g.reg_read(R.UC_X86_REG_ESP)
            packet = int.from_bytes(g.mem_read(sp + 4, 4), 'little')
            packets.append(dict(input=packet, destination=g.reg_read(R.UC_X86_REG_ECX),
                input_hex=bytes(g.mem_read(packet, 128)).hex()))
        if instruction.mnemonic == 'rsqrtss':
            operand = instruction.operands[1]
            if operand.type == X86_OP_REG:
                value = g.reg_read(getattr(R, 'UC_X86_REG_' + instruction.reg_name(operand.reg).upper())) & 0xffffffff
            else:
                assert operand.type == X86_OP_MEM and not operand.mem.segment
                location = operand.mem.disp
                if operand.mem.base:
                    location += g.reg_read(getattr(R, 'UC_X86_REG_' + instruction.reg_name(operand.mem.base).upper()))
                if operand.mem.index:
                    location += operand.mem.scale * g.reg_read(getattr(R, 'UC_X86_REG_' + instruction.reg_name(operand.mem.index).upper()))
                value = int.from_bytes(g.mem_read(location & 0xffffffff, 4), 'little')
            pending = ('rsqrtss', address, instruction.reg_name(instruction.operands[0].reg), value)
        elif instruction.mnemonic == 'rdtsc':
            pending = ('rdtsc', address, None, None)

    g.hook_add(UC_HOOK_CODE, code)
    g.hook_add(UC_HOOK_MEM_READ, read)
    g.hook_add(UC_HOOK_MEM_WRITE, write)
    g.emu_start(prior['entry'], 0, count=1000000)
    assert exit_entry is not None and prefix_checked, 'Original world prefix was not preserved'
    initial, current = {}, {}
    for event in events:
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
    case = dict(id=0, phase=prior['phase'], entry=prior['entry'], entry_abi=state,
        return_address=prior['return_address'], exit=captured_abi(g), exit_entry=exit_entry,
        complete=exit_entry == prior['return_address'],
        initial_banks=[dict(address=p, hex=b.hex()) for p, b in segments(initial)],
        final_banks=[dict(address=p, hex=b.hex()) for p, b in segments(current)],
        writes=[e for e in events if e['kind'] == 'W'], instructions=trace,
        packets=packets, estimates=estimates, timestamps=timestamps)
    assert sha(__file__) == tool_pin
    result = dict(schema=1, image='1.1', tool_sha256=tool_pin,
        original_sha256=PIN, checkpoint_sha256=sha(args.checkpoint),
        parent_fixture_sha256=sha(args.parent), correspondence_sha256=sha(args.correspondence),
        cases=[case], previous_original_prefix_identical=True, original_prefix_stopped_before_cpu_observation=True,
        source_instruction_patches=[], native_expected_data_used=False,
        production_checkpoint_replay=False, live_gameplay_proof=False,
        scope='Unchanged original outer physics-frame execution from its actual retained entry; the earlier CPU-boundary prefix independently compares before continuing. Emulator CPU outputs are diagnostic observations, not Xbox hardware or production providers.')
    args.output.mkdir(parents=True, exist_ok=False)
    path = args.output / 'fixture.json'
    path.write_text(json.dumps(result, sort_keys=True, separators=(',', ':')) + '\n')
    print(json.dumps(dict(sha256=sha(path), instructions=len(trace), writes=len(case['writes']),
        packets=len(packets), complete=case['complete'], stop=hex(exit_entry))))


if __name__ == '__main__':
    main()

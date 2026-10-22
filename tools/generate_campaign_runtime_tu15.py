#!/usr/bin/env python3
"""Lower whole TU1.5 campaign squad, command and HS storage owners.

Entries were recovered from actual Cairo placement and actor/command/HS lifecycle
failures. Existing AI-frame helpers remain in their canonical module. Every
selected body retains its whole original CFG; no gameplay return is supplied.
"""
import argparse
import json
import re
import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha
from mcc_script_builtin_hooks import add_hooks

ENTRIES = (
    0x27d190, 0x2097e0, 0x209720, 0x2096c0, 0x20bc90, 0x2095f0,
    0x2092d0, 0x2091a0, 0x20ba40, 0x1ec820, 0x209e30, 0x2094b0,
    0x20d070, 0x20d0d0, 0x1e8b00, 0x218b20, 0x1e8960, 0x120970,
    0x1e8750, 0xd81e0, 0xb1d60, 0x261be0, 0x261a90, 0x2117e0,
    0x211a80, 0x2129d0, 0x2112b0, 0x212470, 0x212a20, 0x1e6fb0,
    0x16e7f0, 0x211360, 0x16e510, 0x2aa6d0, 0x211cb0, 0x212560,
    0x211b10, 0x2105d0, 0x211da0, 0x2123c0, 0x211e80, 0x212340, 0x212360, 0x2123e0,
    0x2123a0, 0x263b80, 0x262870, 0x2629d0, 0x2093c0, 0x20b150,
    0x1ec7b0, 0x1eb590, 0x29b450, 0x20c650, 0x20d180, 0x20d2d0,
    0x1d5db0, 0x2b4a40,
)
SHARED_FRAME_HELPERS = (0x20b620, 0x20a160, 0x272550)


def admit_canonical_owners():
    pattern = r'^static void run_([0-9a-f]+)\(WorldState &s\) \{'
    overlaps = []
    for file in (ROOT / 'src').rglob('*_generated.cpp'):
        if file.name == 'source_campaign_runtime_tu15_generated.cpp':
            continue
        owned = {int(p, 16) for p in re.findall(pattern, file.read_text(), re.M)}
        overlaps.extend({'entry': f'{p:08x}', 'file': str(file.relative_to(ROOT))}
                        for p in set(ENTRIES) & owned)
    if overlaps:
        raise RuntimeError(f'Original campaign bodies already have owners: {overlaps}')
    frame = (ROOT / 'src/campaign/ai/source_ai_frame_tu15_generated.cpp').read_text()
    owned_frame = {int(p, 16) for p in re.findall(pattern, frame, re.M)}
    assert set(SHARED_FRAME_HELPERS) <= owned_frame


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--directory', type=Path, required=True)
    args = ap.parse_args()
    admit_canonical_owners()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in sorted(ENTRIES):
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, w, n in body.tables if w == 4],
            byte_tables=[dict(address=p, width=w, count=n, raw_hex=image.read(p, w*n).hex())
                         for p, w, n in body.tables if w == 1]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}': value for p, value in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest,
        correspondence=correspondence, additional_entries=Path(__file__)), paired)
    text = args.output.read_text()
    replacements = {
        'tools/generate_physics_live_support_tu15.py': 'tools/generate_campaign_runtime_tu15.py',
        'halo2/physics_live_support_tu15.hpp': 'halo2/source_campaign_runtime_tu15.hpp',
        'source_run_physics_live_support_tu15': 'source_run_campaign_runtime_tu15',
        'source_physics_live_support_tu15_owns_entry': 'source_campaign_runtime_tu15_owns_entry',
    }
    for before, after in replacements.items():
        text = text.replace(before, after)
    args.output.write_text(add_hooks(text))
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)),
        scope=__doc__, reused_existing_frame_helpers=list(SHARED_FRAME_HELPERS),
        tool_dependency_sha256={name: sha(ROOT/'tools'/name) for name in (
            'generate_physics_live_support_tu15.py', 'oracle_smg_firing_1_1.py',
            'research_mp_sandbox_update_functions.py', 'mcc_script_builtin_hooks.py')})
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete original campaign owners:', len(ENTRIES), 'routines',
          len(instructions), 'instructions')


if __name__ == '__main__':
    main()

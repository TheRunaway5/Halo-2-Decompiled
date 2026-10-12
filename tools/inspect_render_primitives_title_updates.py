#!/usr/bin/env python3
"""Primary-byte correspondence for retail/TU1.1/TU1.5 render primitives.

Masked prefixes only locate candidates. Complete recursively reached owner
instructions, internal branches, switch topology, direct owner-call constraints,
read-only operand bytes and literal descriptor/method tables establish the
reported correspondence. Mutable globals remain relocated caller inputs.
No title-update execution or captured title-update render world is claimed.
"""
import argparse
import csv
import hashlib
import json
import re
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import _Code, literal, complete_tokens, references
from research_mp_lobby_later_update import table_pair
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/game-variants/render-primitives-title-updates-20261011'
OWNERS = dict(vertex_extern=0x19be0, state_extern=0x1a170, rgb_pack=0x131ed0,
              value_provider=0x1b2c0, immediate_state=0x3f5cf0, fog_color=0x3f5370,
              line_width=0x3f5730, trunc_float=0x3fcf00, bank_reset=0x16b10)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def section(image, address):
    return next(s for s in image.sections if s.va <= address < s.va + s.vsize)


def prefix(image, entry, size):
    raw = image.read(entry, min(size, 64))
    mask = bytearray([255] * len(raw))
    cs = Cs(CS_ARCH_X86, CS_MODE_32)
    cs.detail = True
    for ins in cs.disasm(raw, entry):
        for operand in ins.operands:
            memory = operand.type == X86_OP_MEM and any(s.va <= operand.mem.disp < s.va + s.vsize for s in image.sections)
            immediate = operand.type == X86_OP_IMM and (ins.mnemonic == 'call' or ins.mnemonic.startswith('j') or ins.mnemonic in ('mov', 'push') and any(s.va <= operand.imm < s.va + s.vsize for s in image.sections))
            if memory:
                at = ins.address - entry + ins.disp_offset
                mask[at:at + ins.disp_size] = bytes(ins.disp_size)
            if immediate:
                at = ins.address - entry + ins.imm_offset
                mask[at:at + ins.imm_size] = bytes(ins.imm_size)
    return b''.join(re.escape(bytes([value])) if keep else b'.' for value, keep in zip(raw, mask))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    images, starts, codes = {}, {}, {}
    for name, spec in SPECS.items():
        image = Xbe(ROOT / spec['path'])
        assert sha(image.data) == spec['pin']
        images[name] = image
        starts[name] = {int(r['address'], 16) for r in csv.DictReader((ROOT / spec['tsv']).open(), delimiter='\t')}
        codes[name] = {s.name: _Code(image, s.name) for s in image.sections if s.name in ('.text', 'D3D')}
    indexed = read_rows(FUNCTIONS_CSV)
    old = images['retail']
    functions, entries, locators = {}, {'retail': OWNERS.copy()}, []
    for owner, entry in OWNERS.items():
        sec = section(old, entry).name
        functions['retail', owner] = literal(codes['retail'][sec], entry, starts['retail'])
        pattern = prefix(old, entry, int(indexed[entry]['size']))
        for name in ('tu11', 'u5'):
            candidates, accepted = [], []
            image = images[name]
            for s in image.sections:
                if s.name not in ('.text', 'D3D'):
                    continue
                for match in re.finditer(pattern, image.section_bytes(s), re.S):
                    candidate = s.va + match.start()
                    candidates.append(candidate)
                    f, ins = literal(codes[name][s.name], candidate, starts[name])
                    of, oi = functions['retail', owner]
                    tables = table_pair(of, f, oi, ins, old, image)
                    if complete_tokens(oi, old) == complete_tokens(ins, image) and tables is not None and all(x['topology_equal'] for x in tables):
                        accepted.append((candidate, f, ins))
            if owner == 'trunc_float':
                # Identical leaf copies exist in .text and D3D. The admitted
                # one is the actual call target of this image's line owner.
                line = functions[name, 'line_width'][1]
                target = next(int(i.op_str, 16) for i in line if i.mnemonic == 'call' and i.op_str.startswith('0x'))
                accepted = [row for row in accepted if row[0] == target]
            assert len(accepted) == 1, (owner, name, candidates, [r[0] for r in accepted])
            target, f, ins = accepted[0]
            entries.setdefault(name, {})[owner] = target
            functions[name, owner] = (f, ins)
            locators.append(dict(owner=owner, image=name, prefix_candidates=candidates, accepted_entry=target))
    listing_records, comparisons = [], []
    for name, image in images.items():
        for owner, entry in entries[name].items():
            f, ins = functions[name, owner]
            path = args.output / f'{name}-{owner}-{entry:08x}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            listing_records.append(dict(image=name, owner=owner, entry=entry, instructions=len(ins), listing=path.name, listing_sha256=sha(path.read_bytes())))
            if name == 'retail':
                continue
            of, oi = functions['retail', owner]
            refs = references(oi, ins, old, image)
            constants = [r for r in refs if r['old']['section'] == '.rdata']
            assert all(r['old']['bytes_hex'] == r['later']['bytes_hex'] for r in constants), (owner, name, constants)
            mapped = dict(zip(entries['retail'].values(), entries[name].values()))
            calls = []
            for a, b in zip(oi, ins):
                if a.mnemonic == 'call' and a.op_str.startswith('0x') and int(a.op_str, 16) in mapped:
                    assert int(b.op_str, 16) == mapped[int(a.op_str, 16)]
                    calls.append(dict(old_site=a.address, later_site=b.address, old_target=int(a.op_str, 16), later_target=int(b.op_str, 16)))
            comparisons.append(dict(image=name, owner=owner, old_entry=OWNERS[owner], later_entry=entry,
                                    complete_shape_internal_branches_equal=True, instructions=len(ins),
                                    switch_tables=table_pair(of, f, oi, ins, old, image), relocated_operands=refs,
                                    all_referenced_rdata_bytes_equal=True, owner_direct_calls=calls))
    literal_tables = []
    for name in images:
        refs = references(functions['retail', 'state_extern'][1], functions[name, 'state_extern'][1], old, images[name]) if name != 'retail' else []
        render = next((int(r['later_address'], 16) for r in refs if int(r['old_address'], 16) == 0x467020), 0x467020 if name == 'retail' else None)
        methods = next((int(r['later_address'], 16) for r in refs if int(r['old_address'], 16) == 0x43e6f0), 0x43e6f0 if name == 'retail' else None)
        assert render is not None and methods is not None
        raw = images[name].read(render, 28)
        method_raw = images[name].read(methods, 92 * 4)
        assert raw == old.read(0x467020, 28) and method_raw == old.read(0x43e6f0, 92 * 4)
        literal_tables.append(dict(image=name, render_descriptors_address=render, render_seven_rows_hex=raw.hex(),
                                   immediate_methods_address=methods, immediate92_methods_hex=method_raw.hex()))
    report = dict(schema='original-render-primitives-title-updates-v1', boundary=__doc__, entries=entries,
                  images={n: dict(path=str(Path(SPECS[n]['path'])), sha256=SPECS[n]['pin']) for n in images},
                  prefix_locators=locators, owners=listing_records, complete_comparisons=comparisons, literal_tables=literal_tables,
                  dependencies={str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in (Path(__file__).resolve(), ROOT / 'tools/oracle_mp_full_scoreboard_1_5.py', ROOT / 'tools/research_mp_lobby_later_update.py', ROOT / 'tools/oracle_game_over.py')},
                  instruction_patches=[], supplied_returns=[],
                  limits=['Static whole-owner correspondence with all reached instructions/internal branches/switch topology, owning direct-call constraints and referenced read-only bytes checked.',
                          'Mutable D3D/context/provider/cache banks remain title-specific relocated live inputs; initial file bytes do not prove a live state.',
                          'No whole title-update primitive execution, title-update world/resource capture, graphics submission or framebuffer output is claimed.',
                          'Dimension/periodic provider branches and nonfinite inputs remain outside the native admitted vertex seam regardless of static correspondence.'])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(owners=len(OWNERS), complete_comparisons=len(comparisons), entries=entries, report_sha256=sha(path.read_bytes())), indent=2))


if __name__ == '__main__':
    main()

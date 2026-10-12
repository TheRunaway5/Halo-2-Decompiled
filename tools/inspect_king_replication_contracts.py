#!/usr/bin/env python3
"""Original registration plus static complete King replication contracts in three XBEs.

No replication callback, network codec, service or native implementation is
executed here. Full-owner correspondence is static primary-byte evidence.
"""
from pathlib import Path
import argparse, csv, hashlib, json, re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from oracle_game_over import SPECS
from oracle_game_variants import Source
from oracle_mp_full_scoreboard_1_5 import _Code, literal, complete_tokens, references
from research_mp_lobby_later_update import table_pair
from inspect_render_primitives_title_updates import prefix, section
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/game-variants/king-replication-source-20261012'
SLOTS = dict(base_initialize=41, base_difference=42, base_apply=43,
             king_initialize=44, king_difference=45, king_apply=46)
CALLERS = dict(initialize_dispatch=0xa45e0, difference_dispatch=0xa4650,
               apply_dispatch=0xa4730, creation_apply=0xa4880,
               shared_write=0xa4ab0, shared_read=0xa4e20)

def sha(raw): return hashlib.sha256(raw).hexdigest()
def cstring(image, pointer):
    return image.read(pointer, 128).split(b'\0')[0].decode('ascii', errors='replace')
def calls(ins):
    return [int(i.op_str, 16) for i in ins if i.mnemonic == 'call' and i.op_str.startswith('0x')]

def main():
    ap = argparse.ArgumentParser(description=__doc__); ap.add_argument('--output', type=Path, default=OUT)
    args = ap.parse_args(); args.output.mkdir(parents=True, exist_ok=False)
    images, starts, codes, entries, functions, registrations, entity_tables = {}, {}, {}, {}, {}, [], []
    indexed = read_rows(FUNCTIONS_CSV)
    for name, spec in SPECS.items():
        source = Source(spec); source.call(spec['ctor']); image = source.image
        images[name] = image
        starts[name] = {int(row['address'], 16) for row in csv.DictReader((ROOT / spec['tsv']).open(), delimiter='\t')}
        codes[name] = {s.name: _Code(image, s.name) for s in image.sections if s.name in ('.text', 'D3D')}
        obj = source.u(spec['table'] + 4 * 4); vtable = source.u(obj)
        table = [source.u(vtable + slot * 4) for slot in range(51)]
        entries[name] = {key: table[slot] for key, slot in SLOTS.items()}
        registrations.append({'image': name, 'constructor': spec['ctor'], 'engine_table': spec['table'],
            'king_source_type': 4, 'object': obj, 'vtable': vtable, 'vtable51': table,
            'executed_registration_instruction_count': len(source.executed)})
        # Find the genuine entity vtable by its literal name getter and all
        # 27 executable pointers; no entity pointer is supplied to a callback.
        tables = []
        for s in image.sections:
            if s.name != '.rdata': continue
            raw = image.section_bytes(s)
            for at in range(0, len(raw) - 27 * 4, 4):
                getter = int.from_bytes(raw[at + 4:at + 8], 'little')
                try:
                    body = image.read(getter, 6)
                    if body[:1] != b'\xb8' or body[5:] != b'\xc3': continue
                    string = int.from_bytes(body[1:5], 'little')
                    if cstring(image, string) != 'king-engine-globals': continue
                    pointers = [int.from_bytes(raw[at + n * 4:at + n * 4 + 4], 'little') for n in range(27)]
                    if not all(any(t.va <= pointer < t.va + t.vsize and t.flags & 4 for t in image.sections) for pointer in pointers): continue
                    tables.append((s.va + at, pointers, string))
                except (ValueError, StopIteration): continue
        assert len(tables) == 1, (name, tables)
        address, pointers, string = tables[0]
        entity_tables.append({'image': name, 'vtable': address, 'pointers27': pointers,
                              'name_pointer': string, 'literal_name': cstring(image, string)})
        entries[name].update(king_wire_size=pointers[11], king_wire_write=pointers[14], king_wire_read=pointers[15])
        for owner, entry in entries[name].items():
            sec = section(image, entry).name
            functions[name, owner] = literal(codes[name][sec], entry, starts[name])
    old = images['retail']; locators = []
    for owner, entry in CALLERS.items():
        oi = literal(codes['retail']['.text'], entry, starts['retail'])
        entries['retail'][owner] = entry; functions['retail', owner] = oi
        pattern = prefix(old, entry, int(indexed[entry]['size']))
        for name in ('tu11', 'u5'):
            accepted, candidates = [], []
            image = images[name]
            for s in image.sections:
                if s.name not in codes[name]: continue
                for match in re.finditer(pattern, image.section_bytes(s), re.S):
                    candidate = s.va + match.start(); candidates.append(candidate)
                    f, ins = literal(codes[name][s.name], candidate, starts[name])
                    topology = table_pair(oi[0], f, oi[1], ins, old, image)
                    if complete_tokens(oi[1], old) == complete_tokens(ins, image) and topology is not None and all(r['topology_equal'] for r in topology):
                        accepted.append((candidate, f, ins))
            assert len(accepted) == 1, (name, owner, candidates, [v[0] for v in accepted])
            target, f, ins = accepted[0]
            entries[name][owner] = target; functions[name, owner] = (f, ins)
            locators.append({'image': name, 'owner': owner, 'candidates': candidates, 'accepted': target})
    service_names = ['refresh_player_properties', 'refresh_recording', 'phase_change', 'game_over_begin']
    for name, image in images.items():
        targets = calls(functions[name, 'base_apply'][1]); assert len(targets) == 4, targets
        for service, target in zip(service_names, targets):
            entries[name][service] = target
            functions[name, service] = literal(codes[name][section(image, target).name], target, starts[name])
        targets = calls(functions[name, 'refresh_recording'][1]); assert len(targets) == 1, targets
        target = targets[0]
        entries[name]['recording_team_is_active'] = target
        functions[name, 'recording_team_is_active'] = literal(codes[name][section(image, target).name], target, starts[name])
    listings, comparisons = [], []
    for name, image in images.items():
        for owner, entry in entries[name].items():
            f, ins = functions[name, owner]; p = args.output / f'{name}-{owner}-{entry:08x}.asm'
            p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            listings.append({'image': name, 'owner': owner, 'entry': entry, 'instructions': len(ins),
                'listing': p.name, 'sha256': sha(p.read_bytes()), 'direct_calls': calls(ins)})
            if name == 'retail': continue
            of, oi = functions['retail', owner]
            refs = references(oi, ins, old, image)
            topology = table_pair(of, f, oi, ins, old, image)
            equal = complete_tokens(oi, old) == complete_tokens(ins, image)
            explicit_bound = None
            if owner == 'refresh_recording':
                old_tokens, later_tokens = complete_tokens(oi, old), complete_tokens(ins, image)
                differences = [j for j, (a, b) in enumerate(zip(old_tokens, later_tokens)) if a != b]
                assert len(old_tokens) == len(later_tokens) and differences == [20]
                assert oi[20].mnemonic == ins[20].mnemonic == 'cmp'
                assert oi[20].op_str.startswith('esi, 0x') and ins[20].op_str.startswith('esi, 0x')
                old_begin = int(next(i.op_str.split(', ')[1] for i in oi if i.mnemonic == 'mov' and i.op_str.startswith('esi, 0x')), 16)
                later_begin = int(next(i.op_str.split(', ')[1] for i in ins if i.mnemonic == 'mov' and i.op_str.startswith('esi, 0x')), 16)
                old_end, later_end = int(oi[20].op_str.split(', ')[1], 16), int(ins[20].op_str.split(', ')[1], 16)
                assert old_end - old_begin == later_end - later_begin == 16 * 0x18
                explicit_bound = {'instruction': 20, 'old_begin': old_begin, 'old_end': old_end,
                    'later_begin': later_begin, 'later_end': later_end, 'rows': 16, 'stride': 0x18,
                    'reason': 'Generic token helper leaves CMP immediate addresses literal; explicit identical bank-relative bound checked.'}
                old_tokens[20] = later_tokens[20] = 'cmp esi, RECORD_BANK_PLUS_384'
                equal = old_tokens == later_tokens
            if owner not in service_names:
                assert equal and topology is not None and all(r['topology_equal'] for r in topology), (name, owner)
            constants = [r for r in refs if r['old']['section'] == '.rdata']
            constants_equal = all(r['old']['bytes_hex'] == r['later']['bytes_hex'] for r in constants)
            if owner not in service_names:
                assert constants_equal, (name, owner, constants)
            mapped = dict(zip(entries['retail'].values(), entries[name].values()))
            constraints = []
            for a, b in (zip(oi, ins) if equal else []):
                if a.mnemonic == 'call' and a.op_str.startswith('0x') and int(a.op_str, 16) in mapped:
                    target = int(a.op_str, 16); assert int(b.op_str, 16) == mapped[target]
                    constraints.append({'old_site': a.address, 'later_site': b.address, 'old_target': target, 'later_target': mapped[target]})
            comparisons.append({'image': name, 'owner': owner, 'whole_shape_internal_branches_equal': equal,
                'switch_topology': topology, 'relocated_operands': refs,
                'explicit_recording_loop_bound': explicit_bound,
                'referenced_rdata_prefixes_equal': constants_equal, 'owning_direct_call_constraints': constraints})
    report = {'schema': 'king-replication-primary-contracts-v1', 'scope': __doc__,
        'images': {name: {'path': spec['path'], 'sha256': spec['pin']} for name, spec in SPECS.items()},
        'registered_tables': registrations, 'entity_definition_tables': entity_tables,
        'entries': entries, 'prefix_locators': locators, 'listings': listings,
        'complete_comparisons': comparisons, 'instruction_patches': [], 'supplied_returns': [],
        'dependencies': {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in [Path(__file__),
            ROOT/'tools/inspect_render_primitives_title_updates.py', ROOT/'tools/oracle_game_over.py',
            ROOT/'tools/oracle_game_variants.py', ROOT/'tools/oracle_mp_full_scoreboard_1_5.py',
            ROOT/'tools/research_mp_lobby_later_update.py']}}
    (args.output/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps({'owners':len(entries['retail']), 'listings':len(listings),
        'comparisons':len(comparisons), 'entries': entries, 'report_sha256':sha((args.output/'report.json').read_bytes())}))

if __name__ == '__main__': main()

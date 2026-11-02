"""Inventory original declarations and existing generated evaluator definitions.

This source audit identifies implementation candidates for the recorded mod
requirements. Definitions and World route mentions are not execution, link or
effect acceptance. Run through run_test.py; retain only its bounded report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

from audit_workshop_compatibility import Pe64, xbox_catalog
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sweep', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(scratch)
    sweep = json.loads(args.sweep.read_text())
    assert sweep['completed'] and sweep['schema'] == 'H2_ALL_INSTALLED_WORKSHOP_SWEEP_V1'
    needs = set(sweep['summary']['script_requirements'])
    inventory = root / 'docs/WORKSHOP_COMPATIBILITY_SWEEP.md'
    text = inventory.read_text().split('## Missing declaration inventory', 1)[1].split('## Native failures', 1)[0]
    declarations = [(name, int(opcode)) for name, opcode in re.findall(r'^\| `([^`]+)` \| (\d+) \|', text, re.M)
                    if name in needs]
    assert {name for name, _ in declarations} == needs
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    pe = Pe64(Path.home() / '.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    catalog, code = xbox_catalog(image), _Code(image, '.text')
    rows, entries = [], set()
    for name, opcode in declarations:
        declaration = pe.builtin(opcode)
        assert declaration['name'] == name
        candidates = [row for row in catalog.get(name, []) if row['xbox_return_type'] == declaration['return_type']
                      and row['xbox_arguments'] == declaration['arguments']]
        rows.append(dict(declaration=declaration, candidates=candidates))
        entries.update(int(row['xbox_evaluator'], 16) for row in candidates)
    definitions = {entry: [] for entry in entries}
    pattern = re.compile(r'static void run_([0-9a-f]+)\(WorldState &s\) \{')
    world = (root / 'src/physics/physics_live_world_tu15.cpp').read_text()
    source_pins = {}
    for path in sorted((root / 'src').rglob('*generated.cpp')):
        source = path.read_text()
        selected = [match for match in pattern.finditer(source) if int(match[1], 16) in entries]
        if not selected:
            continue
        name = path.relative_to(root).as_posix()
        source_pins[name] = hashlib.sha256(path.read_bytes()).hexdigest()
        owners = re.findall(r'bool (\w+_owns_entry)\(', source)
        for match in selected:
            end = source.find('\nstatic void ', match.end())
            body = source[match.start():end if end != -1 else len(source)]
            sites = sorted({int(pc, 16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)', body)})
            definitions[int(match[1], 16)].append(dict(source=name, sites=sites,
                body_sha256=hashlib.sha256(body.encode()).hexdigest(),
                exported_owner_symbols=owners, owner_symbol_mentioned_in_world=any(owner in world for owner in owners)))
    for row in rows:
        for candidate in row['candidates']:
            entry = int(candidate['xbox_evaluator'], 16)
            original = complete_body(code, entry)
            candidate['original_instruction_sites'] = len(original.code)
            candidate['generated_definitions'] = definitions[entry]
            for definition in definitions[entry]:
                definition['same_instruction_site_set'] = set(definition['sites']) == set(original.code)
            candidate['engine_effects_qualified'] = False
    report = dict(scope=__doc__,sweep=str(args.sweep), sweep_sha256=hashlib.sha256(args.sweep.read_bytes()).hexdigest(),
        inventory_sha256=hashlib.sha256(inventory.read_bytes()).hexdigest(),
        xbe_sha256=NEW_PIN, mcc_sha256=hashlib.sha256(pe.data).hexdigest(),
        distinct_requirements=len(needs), declarations=len(rows), generated_source_hashes=source_pins,
        rows=rows, engine_effects_qualified=False, owned_scratch=str(scratch), retained_large_artifacts=[])
    raw = json.dumps(report, indent=2) + '\n'
    assert len(raw.encode()) < 1024 * 1024
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(raw)
    print(json.dumps(dict(requirements=len(needs), declarations=len(rows),
        declarations_with_generated_evaluator=sum(any(c['generated_definitions'] for c in row['candidates']) for row in rows),
        report_bytes=len(raw.encode()))))


if __name__ == '__main__':
    main()

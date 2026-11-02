"""Inventory the full authenticated MCC builtin registry beyond installed mods.

The original name-search loop supplies the registry bound. Profile membership
is metadata admission only; signature matches are candidates for native owners,
not effect acceptance. Run through run_test.py; retain only a bounded report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from audit_workshop_compatibility import Pe64, xbox_catalog
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(scratch)
    pe = Pe64(Path.home() / '.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    # Exact original search loop: index starts at0, addresses table[index],
    # increments, and continues while signed index < this encoded count.
    guards = [(0x1806e2cf8, '33db'), (0x1806e2cfd, '391de1fb4300'),
              (0x1806e2d08, '4c8d3d411b7200'), (0x1806e2d10, '4863c3'),
              (0x1806e2d16, '498b34c7'), (0x1806e2d40, 'ffc3'),
              (0x1806e2d42, '3b1d9cfb4300'), (0x1806e2d48, '7cc6')]
    for address, encoded in guards:
        assert pe.read(address, len(encoded) // 2).hex() == encoded
    assert 0x1806e2cfd + 6 + int.from_bytes(pe.read(0x1806e2cfd + 2, 4), 'little', signed=True) == 0x180b228e4
    assert 0x1806e2d42 + 6 + int.from_bytes(pe.read(0x1806e2d42 + 2, 4), 'little', signed=True) == 0x180b228e4
    assert 0x1806e2d08 + 7 + int.from_bytes(pe.read(0x1806e2d08 + 3, 4), 'little', signed=True) == 0x180e04850
    count = pe.word(0x180b228e4, 4)
    assert 0 < count <= 2048
    profile_path = root / 'tools/mcc-script-layout.json'
    profile = json.loads(profile_path.read_text())
    admitted = {row['mcc_opcode']: row for row in profile['builtins']}
    assert len(admitted) == len(profile['builtins']) and set(admitted) <= set(range(count))
    catalog = xbox_catalog(image)
    rows = []
    for opcode in range(count):
        address = pe.word(0x180e04850 + opcode * 8)
        arity = pe.word(address + 56, 2)
        assert arity <= 256  # Audit storage bound; not runtime type admission.
        name = pe.string(pe.word(address + 8))
        assert name and len(name) <= 4096
        args_ = list(struct.unpack('<' + 'H' * arity, pe.read(address + 58, arity * 2)))
        row = dict(mcc_opcode=opcode, name=name, return_type=pe.word(address, 2),
            arguments=args_, mcc_flags=pe.word(address + 16, 2), mcc_descriptor=hex(address),
            mcc_evaluator=hex(pe.word(address + 32)), profiled=opcode in admitted)
        if opcode in admitted:
            assert all(row[key] == admitted[opcode][key] for key in
                ('name', 'return_type', 'arguments', 'mcc_descriptor', 'mcc_evaluator'))
        row['xbox_signature_candidates'] = [candidate for candidate in catalog.get(name, [])
            if candidate['xbox_return_type'] == row['return_type'] and candidate['xbox_arguments'] == args_]
        rows.append(row)
    report = dict(scope=__doc__, mcc_sha256=hashlib.sha256(pe.data).hexdigest(), xbe_sha256=NEW_PIN,
        source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        profile_sha256=hashlib.sha256(profile_path.read_bytes()).hexdigest(),
        count_cell='0x180b228e4', table='0x180e04850', bound_instructions=[[hex(a), b] for a, b in guards],
        declaration_count=count, profiled_declarations=len(admitted),
        unprofiled_declarations=count - len(admitted), maximum_arity=max(len(row['arguments']) for row in rows),
        unique_names=len({row['name'] for row in rows}), rows=rows,
        external_globals_and_value_types_inventoried=False, engine_effects_qualified=False,
        owned_scratch=str(scratch), retained_large_artifacts=[])
    raw = json.dumps(report, indent=2) + '\n'
    assert len(raw.encode()) < 1024 * 1024
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(raw)
    print(json.dumps({key: report[key] for key in ('declaration_count', 'profiled_declarations',
        'unprofiled_declarations', 'maximum_arity', 'unique_names')}))


if __name__ == '__main__':
    main()

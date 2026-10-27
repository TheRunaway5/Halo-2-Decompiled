"""Actual original program-bank graph and declared-storage mutation controls."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native', type=Path, required=True)
    a = p.parse_args()
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'tools'))
    from native_program_storage_graph import storage_graph, canonical
    if not a.native.is_dir():
        return 77
    policy = json.loads((root / 'tools/data/mcc-native-library-closure.json').read_bytes())
    passed = code_controls = rejects = padding_controls = 0
    for source in policy['libraries']:
        folder = a.native / source['map']
        library = json.loads((folder / 'library.json').read_bytes())
        assert hashlib.sha256(canonical(library)).hexdigest() == source['canonical_sha256']
        for record in library['shader_tags']:
            if record['group'] not in ('stem', 'spas', 'vrtx'):
                continue
            payload = (folder / 'payloads' / record['payload']).read_bytes()
            graph = storage_graph(record, payload)
            passed += 1
            if record['group'] != 'vrtx' or source['map'] != 'dune':
                continue
            classes, address = struct.unpack_from('<II', payload, 4)
            for index in range(classes):
                count, at = struct.unpack_from('<II', payload, address + index * 28 + 12)
                if not count:
                    continue
                changed = bytearray(payload); changed[at] ^= 1
                owner = copy.deepcopy(record)
                owner['payload'] = hashlib.sha256(changed).hexdigest() + '.bin'
                other = storage_graph(owner, changed)
                assert graph['graph'] != other['graph'], 'Another class program byte was ignored'
                code_controls += 1
            if record['pointers']:
                owner = copy.deepcopy(record);owner['pointers'].pop()
                try:
                    storage_graph(owner, payload)
                except ValueError:
                    rejects += 1
                else:
                    raise AssertionError('Unowned original pointer was accepted')
            for gap in graph['storage']['alignment_padding'][:1]:
                changed = bytearray(payload);changed[gap['offset']] ^= 1
                owner = copy.deepcopy(record);owner['payload'] = hashlib.sha256(changed).hexdigest() + '.bin'
                if gap['offset'] % 4:
                    other = storage_graph(owner, changed)
                    assert graph['graph'] == other['graph'] and graph['storage'] != other['storage']
                    padding_controls += 1
                else:
                    try:
                        storage_graph(owner, changed)
                    except ValueError:
                        rejects += 1
                    else:
                        raise AssertionError('Nonzero copied-allocation alignment padding was accepted')
    assert passed > 2000 and code_controls > 50 and rejects and padding_controls
    print(f'{passed} complete typed program owners; {code_controls} all-class code-word controls; '
          f'{rejects} missing-pointer rejects; {padding_controls} separately retained padding controls')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

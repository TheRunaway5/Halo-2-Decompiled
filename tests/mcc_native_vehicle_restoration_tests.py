"""Decode the pinned finite vehicle source fixture inside an owned test lifecycle."""
import argparse
import copy
import hashlib
import json
import lzma
import os
from pathlib import Path
import subprocess
import tempfile


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()


def update_preceding_metadata(fixture, receipt, policy, programs):
    if (receipt['schema'] != 'HALO2_MCC_NATIVE_VEHICLE_PRECEDING_METADATA_V2' or
            [r['map'] for r in receipt['maps']] != ['needle', 'street_sweeper'] or
            [r['map'] for r in fixture['cases']] != ['needle', 'street_sweeper']):
        raise ValueError('Unreviewed vehicle fixture preceding-metadata census')
    result = copy.deepcopy(fixture)
    for row, delta in zip(result['cases'], receipt['maps']):
        name = row['map']
        before = row['preceding_library']
        profile = next(r for r in policy['maps'] if r['map'] == name)
        pc = next(r for r in programs['maps'] if r['map'] == name)
        if (hashlib.sha256(canonical(before)).hexdigest() != delta['original_canonical_sha256'] or
                len(before['shader_tags']) != delta['original_records'] or
                len(delta['insertions']) != (1 if name == 'needle' else 2)):
            raise ValueError('Original literal vehicle preceding bank changed')
        updated = copy.deepcopy(before)
        previous_index = -1
        for insertion in delta['insertions']:
            index, record = insertion['index'], insertion['record']
            reviewed = [r['record'] for r in pc['records'] if r['record'] == record]
            if (not previous_index < index <= len(updated['shader_tags']) or
                    record['group'] != 'stem' or record['strings'] or len(reviewed) != 1 or
                    any(r['group'] == record['group'] and r['name'] == record['name']
                        for r in before['shader_tags'])):
                raise ValueError('Vehicle fixture delta is not an exact new template metadata record')
            existing = {(r['group'], r['name']) for r in before['shader_tags']}
            if any(group not in ('spas', 'vrtx') or (group, target) not in existing
                   for _, (group, target) in record['references']):
                raise ValueError('Vehicle fixture template requires an unowned program')
            updated['shader_tags'].insert(index, record)
            previous_index = index
        if (len(updated['shader_tags']) != delta['updated_records'] or
                hashlib.sha256(canonical(updated)).hexdigest() != delta['updated_canonical_sha256'] or
                delta['updated_canonical_sha256'] != profile['preceding_canonical_sha256']):
            raise ValueError('Updated literal vehicle preceding bank lacks its exact policy prefix')
        row['obsolete_preceding_library'] = before
        row['preceding_insertions'] = delta['insertions']
        row['preceding_library'] = updated
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, required=True)
    parser.add_argument('--test-executable', type=Path, required=True)
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    metadata_path = root / 'tests/fixtures/mcc-native-vehicle-restoration-v1.json'
    metadata = json.loads(metadata_path.read_bytes())
    if (set(metadata) != {'schema', 'compressed_file', 'compressed_bytes', 'compressed_sha256',
                          'decoded_bytes', 'decoded_sha256', 'scope'} or
            metadata['schema'] != 'HALO2_MCC_NATIVE_VEHICLE_FIXTURE_OWNER_V1' or
            metadata['compressed_file'] != 'mcc-native-vehicle-restoration-v1.json.xz' or
            not 0 < metadata['compressed_bytes'] <= 1024 * 1024 or
            not 0 < metadata['decoded_bytes'] <= 32 * 1024 * 1024):
        raise ValueError('Invalid finite vehicle source fixture owner')
    packed = (metadata_path.parent / metadata['compressed_file']).read_bytes()
    if len(packed) != metadata['compressed_bytes'] or hashlib.sha256(packed).hexdigest() != metadata['compressed_sha256']:
        raise ValueError('Complete compressed vehicle source fixture changed')
    decoder = lzma.LZMADecompressor()
    raw = decoder.decompress(packed, max_length=metadata['decoded_bytes'] + 1)
    if (not decoder.eof or decoder.unused_data or len(raw) != metadata['decoded_bytes'] or
            hashlib.sha256(raw).hexdigest() != metadata['decoded_sha256']):
        raise ValueError('Complete literal vehicle fixture changed or has unowned trailing bytes')
    receipt = json.loads((metadata_path.parent / 'mcc-native-vehicle-preceding-metadata-v2.json').read_bytes())
    program_path = root / 'tools/data/mcc-native-pc-world-programs.json'
    if (hashlib.sha256(metadata_path.read_bytes()).hexdigest() != receipt['original_fixture_owner_sha256'] or
            hashlib.sha256(program_path.read_bytes()).hexdigest() != receipt['pc_program_policy_sha256']):
        raise ValueError('Vehicle fixture delta lost its original source/policy proof')
    prepared = update_preceding_metadata(json.loads(raw), receipt,
        json.loads((root / 'tools/data/mcc-native-vehicle-restoration.json').read_bytes()),
        json.loads(program_path.read_bytes()))
    with tempfile.TemporaryDirectory(prefix='vehicle-restoration-', dir=os.environ['HALO2_TEST_SCRATCH']) as folder:
        fixture = Path(folder) / 'fixture.json'
        fixture.write_bytes(canonical(prepared))
        return subprocess.run([str(args.test_executable.resolve(strict=True)), str(fixture),
            str(root / 'tools/data/mcc-native-vehicle-restoration.json')], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())

"""Restore the exact declared camera cohort after its prior owner terminated.

Authenticate the selected code archive and every combined overlay. Copy only
hash-matching Root inputs. Reconstruct the two intentionally different CMake/
Cache files from the sealed base and narrow recorded material additions, then
require the full prior14652-file inventory exactly. No runtime is published.
Run through run_test.py; --session starts a new finite owned coordinator.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from build_launcher_engine import source_hashes


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace(text, before, after):
    assert text.count(before) == 1 and after not in text
    return text.replace(before, after)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preparation', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--session', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = owner / 'source'
    prep = json.loads(args.preparation.read_text())
    assert not Path(prep['owned_scratch']).exists(), 'Prior owner must be authoritatively absent before recovery'
    receipt_path = Path(prep['source_refresh_receipt'])
    assert digest(receipt_path) == prep['source_refresh_sha256']
    receipt = json.loads(receipt_path.read_text())
    assert receipt['passed'] and receipt['changes'] == prep['overlays']
    assert len(receipt['changes']) == 58 and prep['production_files'] == 14652
    reference = json.loads(args.reference.read_text())
    assert reference['passed'] and reference['runtime'] == prep['selected_runtime']
    assert reference['runtime_sha256'] == prep['runtime_sha256']
    selected = json.loads((root / 'gameplay-runtime-selection.json').read_text())
    assert selected['runtime'] == prep['selected_runtime'] and selected['runtime_sha256'] == prep['runtime_sha256']
    args.output.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, '-B', str(root / 'tools/prepare_qualified_workshop_snapshot.py'),
        '--reference', str(args.reference), '--output', str(args.output / 'base')], check=True)
    base_pins = source_hashes(source)
    expected = dict(base_pins)
    planned, reconstructed = {}, []
    for row in receipt['changes']:
        name = row['path']
        target = source / name
        before = digest(target) if target.exists() else None
        assert before == row['before'], (name, before, row['before'])
        original = root / name
        if digest(original) == row['sha256']:
            raw = original.read_bytes()
        elif name == 'CMakeLists.txt':
            text = target.read_text()
            anchor = '    src/rendering/mcc_native_world_default_roles.cpp src/rendering/mcc_native_illum_three_channel.cpp)\n'
            text = replace(text, anchor, anchor + ''.join(
                'target_sources(halo2_cache PRIVATE src/rendering/mcc_native_' + family + '.cpp)\n'
                for family in ('env_illum_three_channel', 'tiling_specular', 'alpha_environment')))
            anchor = '    foreach(world_profile world_default_roles illum_no_specular illum_three_channel character_shared)\n'
            text = replace(text, anchor, ''.join('    include(cmake/' + module + '.cmake)\n'
                for module in ('MccNativeEnvIllumThreeChannel', 'MccNativeTilingSpecular', 'MccNativeAlphaEnvironment')) + anchor)
            raw = text.encode()
            reconstructed.append(name)
        elif name == 'src/rendering/mcc_native_shaders.cpp':
            text = target.read_text()
            anchor = '#include "halo2/mcc_native_illum_three_channel.hpp"\n'
            text = replace(text, anchor, anchor + ''.join('#include "halo2/mcc_native_' + family + '.hpp"\n'
                for family in ('env_illum_three_channel', 'tiling_specular', 'alpha_environment')))
            start = '            if(!projected)projected=project_mcc_env_illum_three_channel('
            end = '            if(!projected)projected=project_mcc_world_default_roles('
            current = original.read_text()
            assert current.count(start) == current.count(end) == 1
            block = current[current.index(start):current.index(end)]
            assert block.count('alias.address') == 3 and block.count('retained_tag_allocations_') == 3
            raw = replace(text, end, block + end).encode()
            reconstructed.append(name)
        else:
            raise ValueError('Recorded overlay no longer has its exact source input: ' + name)
        assert hashlib.sha256(raw).hexdigest() == row['sha256'], name
        planned[name] = raw
        if row['production']:
            expected[name] = row['sha256']
    assert hashlib.sha256(json.dumps(expected, sort_keys=True).encode()).hexdigest() == prep['production_inventory_sha256']
    for name, raw in planned.items():
        target = source / name
        assert target.parent.resolve().is_relative_to(source) and not target.is_symlink()
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    actual = source_hashes(source)
    assert actual == expected and len(actual) == 14652
    assert all(digest(source / row['path']) == row['sha256'] for row in receipt['changes'])
    assert digest(root / prep['selected_runtime']) == prep['runtime_sha256']
    proof = dict(scope=__doc__, passed=True, source=str(source), original_preparation=str(args.preparation),
        original_preparation_sha256=digest(args.preparation), original_refresh=str(receipt_path),
        original_refresh_sha256=digest(receipt_path), complete_source_inventory_reproduced=True,
        production_inventory_sha256=prep['production_inventory_sha256'], production_files=len(actual),
        overlays=receipt['changes'], reconstructed_sealed_files=reconstructed,
        prior_owner_removed=True, selected_runtime_unchanged=True, owned_scratch=str(owner), retained_large_artifacts=[])
    (args.output / 'recovery.json').write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps(dict(passed=True, production_files=len(actual), source=str(source),
                         production_inventory_sha256=prep['production_inventory_sha256'])), flush=True)
    if args.session:
        os.execv(sys.executable, [sys.executable, '-u', '-B', str(root / 'tools/macos/test_session.py'),
            '--output', str(args.output / 'session')])


if __name__ == '__main__':
    main()

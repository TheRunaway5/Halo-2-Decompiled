"""Check original shared-map routing without copying map or program banks."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile


def verify(repo_root, owner, cache=None):
    tools = Path(repo_root) / 'tools'
    sys.path.insert(0, str(tools))
    spec = importlib.util.spec_from_file_location('character_shared_export',
                                                tools / 'export_mcc_native_character_shared.py')
    exporter = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(exporter)
    with tempfile.TemporaryDirectory(prefix='shared-source-', dir=owner) as temporary:
        prepared = Path(temporary)
        original = prepared / 'original-halo2'
        declared = original / 'h2_maps_win64_dx11/shared.map'
        override = prepared / 'explicit-shared.map'
        assert exporter.resolve_shared_source(prepared) == prepared / 'halo2/h2_maps_win64_dx11/shared.map'
        (prepared / 'provenance.json').write_text(json.dumps({'source': str(original)}))
        assert exporter.resolve_shared_source(prepared) == declared
        assert exporter.resolve_shared_source(prepared, override) == override
        override.write_bytes(b'not the authenticated original MCC shared source')
        try:
            exporter.authenticated_shared_source(prepared, override)
        except ValueError as error:
            assert 'complete actual shared source changed' in str(error)
        else:
            raise AssertionError('Changed original shared source was admitted')
        (prepared / 'provenance.json').write_text(json.dumps({'source': 'relative-donor'}))
        try:
            exporter.resolve_shared_source(prepared)
        except ValueError as error:
            assert 'declared original MCC source directory' in str(error)
        else:
            raise AssertionError('Relative prepared-cache donor was admitted')
        if cache is not None:
            cache = Path(cache)
            shared = exporter.authenticated_shared_source(cache)
            expected = json.loads((tools / 'data/mcc-native-character-shared.json').read_bytes())
            assert exporter.generate(cache)[0] == expected
            assert exporter.generate(cache, shared)[0] == expected
            try:
                exporter.generate(cache, override)
            except ValueError as error:
                assert 'complete actual shared source changed' in str(error)
            else:
                raise AssertionError('Explicit changed source bypassed complete source authentication')
            library = prepared / 'native-shaders/coagulation/library.json'
            library.parent.mkdir(parents=True)
            library.write_text('{}')
            try:
                exporter.generate(prepared, shared)
            except ValueError as error:
                assert 'original complete library changed' in str(error)
            else:
                raise AssertionError('Original source bypassed complete native-library authentication')
    print('Original shared source routing and strict source/library controls passed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, required=True)
    parser.add_argument('--cache', type=Path)
    args = parser.parse_args()
    verify(args.repo_root, Path(os.environ['HALO2_TEST_SCRATCH']), args.cache)

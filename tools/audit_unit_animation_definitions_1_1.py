#!/usr/bin/env python3
"""Audit immutable original metadata, independent repeat and native test evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from oracle_unit_animation_manager_1_1 import sha, save, encoded

BASE = ROOT / 'analysis/multiplayer/integration/unit-animation-definitions-20261008'
PRIOR = ROOT / 'analysis/multiplayer/integration/inventory-source/unit-animation-clock/manifest.json'


def audit(out):
    measured, repeated = BASE / 'measured', BASE / 'repeated'
    files = sorted(p for p in measured.rglob('*') if p.is_file())
    for p in files:
        assert p.read_bytes() == (repeated / p.relative_to(measured)).read_bytes()
    r = json.loads((measured / 'report.json').read_text())
    for path, expected in r['source_pins'].items():
        assert sha(ROOT / path) == expected, path
    assert r['tool_sha256'] == sha(ROOT / 'tools/oracle_unit_animation_definitions_1_1.py')
    assert r['fixture_sha256'] == sha(measured / 'fixtures.txt')
    assert len(r['queries']) == r['definitions'] == r['counts']['direct'] + r['counts']['inherited']
    assert len({(q['graph'], q['packed']) for q in r['queries']}) == r['definitions']
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    instruction_rows = 0
    for p in (measured / 'instructions').glob('*.asm'):
        for line in p.read_text().splitlines():
            address, raw, _ = line.split(' ', 2)
            b = bytes.fromhex(raw)
            assert image.read(int(address, 16), len(b)) == b
            instruction_rows += 1
    original_owner = measured / 'instructions/001ca5a0.asm'
    listed = {line.split()[0] for line in original_owner.read_text().splitlines()}
    assert set(r['executed_instructions']) <= listed
    assert all(set(q['instructions']) <= listed for q in r['queries'])
    graph_dir = BASE / 'graph-metadata/measured'
    gr = json.loads((graph_dir / 'report.json').read_text())
    assert gr['exporter_sha256'] == sha(ROOT / 'tools/export_unit_animation_graph_metadata.py')
    assert gr['source_resolution_report_sha256'] == sha(measured / 'report.json')
    assert gr['fixture_sha256'] == sha(graph_dir / 'fixtures.txt')
    graph_repeat_files = 0
    for p in graph_dir.rglob('*'):
        if p.is_file():
            assert p.read_bytes() == (BASE / 'graph-metadata/repeated' / p.relative_to(graph_dir)).read_bytes()
            graph_repeat_files += 1
    u5 = BASE / 'u5-correspondence/report.json'
    mapping = json.loads(u5.read_text())
    assert mapping['driver_sha256'] == sha(ROOT / 'tools/research_unit_animation_definitions_u5.py')
    assert mapping['matcher_sha256'] == sha(ROOT / 'tools/research_unit_animation_manager_u5.py')
    assert mapping['raw_report_sha256'] == sha(BASE / 'u5-correspondence/raw/report.json')
    pins = mapping['image_sha256']
    assert pins[str(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')] == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    assert pins['/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'] == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    for path, expected in pins.items():
        assert sha(path) == expected, path
    assert all(q['normalized_complete_body_and_internal_branch_equal'] and q['unique_full_body_match']
               for q in mapping['owners'])
    tests = []
    for build, sanitized in [('build-unit-animation-definitions-20261008', False),
                             ('build-unit-animation-definitions-asan-20261008', True)]:
        path = ROOT / build / 'Testing/Temporary/LastTest.log'
        text = path.read_text()
        assert 'Test Passed.' in text and 'ERROR:' not in text and 'runtime error:' not in text
        output = re.search(r'^definitions=.*$', text, re.M).group()
        counts = dict((k, int(v)) for k, v in re.findall(r'(\w+)=(\d+)', output))
        assert counts['definitions'] == r['definitions'] + 5
        assert counts['inherited'] == r['counts']['inherited']
        assert counts['sounds'] == r['counts']['sounds']
        assert counts['graphs'] == len(gr['graphs'])
        assert counts['graph_pointer_boundaries'] == len(gr['graphs']) * 3
        tests.append(dict(sanitized=sanitized, original_output=output,
                          log_sha256=sha(path),
                          test_binary_sha256=sha(ROOT / build / 'unit_animation_definition_tests'),
                          linked_cache_archive_sha256=sha(ROOT / build / 'libhalo2_cache.a')))
    prior = json.loads(PRIOR.read_text())['pins']
    for path, expected in prior.items():
        assert sha(ROOT / path) == expected, ('Prior frozen owner changed', path)
    result = dict(schema='source-unit-animation-definition-audit-v1', auditor_sha256=sha(__file__),
                  original_report_sha256=sha(measured / 'report.json'), repeat_files=len(files),
                  original_instruction_rows=instruction_rows, definitions=r['definitions'],
                  owned_graph_metadata=len(gr['graphs']),
                  graph_metadata_repeat_files=graph_repeat_files,
                  original_graph_metadata_bytes=sum(q['size'] for q in gr['graphs']),
                  owner_correspondence_sha256=sha(u5), native_tests=tests,
                  preserved_prior_pins=len(prior), prior_manifest_sha256=sha(PRIOR),
                  mismatches=0,
                  scope='Literal source metadata and original graph-resolution queries; native importer compared to all records plus five prior live-owner definitions. This is not native action selection, live character animation, current collision pose or successful audio/effect playback.')
    save(out, encoded(result))
    print(json.dumps(dict(definitions=r['definitions'], repeat_files=len(files),
                          preserved_prior_pins=len(prior), mismatches=0, audit_sha256=sha(out))))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    audit(p.parse_args().output)

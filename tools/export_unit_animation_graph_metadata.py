#!/usr/bin/env python3
"""Export owned authored jmad banks, keeping original virtual words opaque.

Literal cache payloads only. The prior original1CA5A0 returned addresses are
checked against these exact tags. No runtime graph postprocess bytes are added.
"""
import argparse
import json
from pathlib import Path
from oracle_unit_animation_definitions_1_1 import ROOT, MAP, sha, save
from inspect_multiplayer import Cache

SOURCE = ROOT / 'analysis/multiplayer/integration/unit-animation-definitions-20261008/measured/report.json'


def export(out):
    assert sha(MAP) == '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
    assert sha(SOURCE) == '291d62a14321ffcc9dcf436f3054290baf74838ce6c652ae3bc34d7aa4bdbecf'
    original = json.loads(SOURCE.read_text())
    c = Cache(MAP)
    for q in original['queries']:
        target = c.tags[int(q['resolved_graph'], 16)]
        assert q['original_return'] == f'{target["offset"] + c.magic:08x}'
        p = int(q['definition_pointer'], 16) - c.magic
        assert target['offset'] <= p and p + 0x6c <= target['offset'] + target['size']
    rows, graphs = [], []
    for datum, t in sorted(c.tags.items()):
        if t['group'] != 'jmad' or t['offset'] is None:
            continue
        assert t['size'] >= 0xc4
        raw = bytes(c.data[t['offset']:t['offset'] + t['size']])
        assert len(raw) == t['size']
        address = t['offset'] + c.magic
        rows.append(f'G {datum:08x} {address:08x} {len(raw)} {raw.hex()}')
        graphs.append(dict(datum=f'{datum:08x}', address=f'{address:08x}', size=len(raw)))
    save(out / 'fixtures.txt', ('\n'.join(rows) + '\n').encode())
    r = dict(schema='source-unit-animation-graph-metadata-fixtures-v1', exporter_sha256=sha(__file__),
             map_sha256=sha(MAP), source_resolution_report_sha256=sha(SOURCE),
             fixture_sha256=sha(out / 'fixtures.txt'), graphs=graphs,
             correlated_original_resolution_queries=len(original['queries']),
             scope='Literal authored full graph metadata. Resource flags/notifications and other runtime postprocessing remain separate owners; no active residency/channel/variant success is supplied.')
    save(out / 'report.json', (json.dumps(r, sort_keys=True, indent=2) + '\n').encode())
    c.data.close()
    c.stream.close()
    print(json.dumps(dict(graphs=len(graphs), bytes=sum(x['size'] for x in graphs),
                          report_sha256=sha(out / 'report.json'))))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    export(p.parse_args().output)

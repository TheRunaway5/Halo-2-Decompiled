#!/usr/bin/env python3
"""Export whole original cleanup witnesses and their literal mutable bindings."""
import hashlib
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, verify, actual

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/'analysis/multiplayer/integration/sound-driver-cleanup-production-20261009'
SOURCE = 'analysis/multiplayer/integration/sound-driver-owners-native/measured/report.json'
PAIR = 'analysis/multiplayer/integration/sound-driver-owners-native/paired/report.json'
PINS = {
    SOURCE: 'b106eae5bc39633b181fffb838cf088cdf286de57987687a0067a51fe91a0993',
    PAIR: '314b994fcc7ab20b7ca9efa888fd253187beb5e93536967efc9044e6f5e3f3b9',
}


def main():
    verify(PINS)
    report = json.loads(actual(SOURCE).read_text())
    paired = json.loads(actual(PAIR).read_text())
    cleanup = next(q for q in paired['owners'] if q['old'] == '00220bf0')
    records = cleanup['memory']
    rows = ['# Whole unchanged cleanup witnesses; retained source-created backing, no platform returns.']
    count = stores = output_bytes = 0
    for image in report['images']:
        tag = image['image']; side = 'old' if tag == 'old' else 'new'
        driver_binding = next(q for q in records if q['old_site'] == '00220bf9')
        queue_binding = next(q for q in records if q['old_site'] == '00220cbf')
        driver_global = driver_binding[side+'_va']; queue_global = queue_binding[side+'_va']
        regions = {int(q['address'], 16): bytearray.fromhex(q['raw_hex']) for q in image['initial_regions']}
        regions[int(driver_global, 16)] = bytearray(int(image['driver_address'], 16).to_bytes(4, 'little'))
        regions[int(queue_global, 16)] = bytearray.fromhex(queue_binding[side+'_hex'])
        case = next(q for q in image['cases'] if q['operation'] == 'cleanup_witness')
        assert not case['controls'] and not case['invalid'] and not case['platform_request']
        assert case['outcome'] == 'returned' and not case['source_return_supplied']
        pool = regions[int(image['cache_address'], 16)]
        allocator = int.from_bytes(pool[0x30:0x34], 'little')
        rows.append('\t'.join(['image', tag, driver_global, image['cache_address'],
                               queue_global, image['queue_address'], f'{allocator:08x}']))
        for address, raw in sorted(regions.items()):
            rows.append('\t'.join(['input', tag, f'{address:08x}', raw.hex()]))
        for store in case['stores']:
            address = int(store['address'], 16); extent = store['bytes']
            owner = next(a for a, raw in regions.items() if a <= address and address+extent <= a+len(raw))
            offset = address-owner; raw = regions[owner]
            assert raw[offset:offset+extent] == int(store['before'], 16).to_bytes(extent, 'little')
            raw[offset:offset+extent] = int(store['after'], 16).to_bytes(extent, 'little')
            rows.append('\t'.join(['store', tag, store['instruction'], store['address'],
                                   str(extent), store['before'], store['after']]))
            stores += 1
        for output in case['output_regions']:
            assert hashlib.sha256(regions[int(output['address'], 16)]).hexdigest() == output['sha256']
        for address, raw in sorted(regions.items()):
            rows.append('\t'.join(['output', tag, f'{address:08x}', raw.hex()]))
            output_bytes += len(raw)
        count += 1
    assert count == 2 and stores == 6
    destination = BASE/'measured'; destination.mkdir(parents=True, exist_ok=True)
    fixture = destination/'fixtures.tsv'
    fixture_bytes = ('\n'.join(rows)+'\n').encode()
    if fixture.exists():
        assert fixture.read_bytes() == fixture_bytes
    else:
        fixture.write_bytes(fixture_bytes)
    metadata = dict(schema='sound-driver-cleanup-witness-fixtures-v1', exporter_sha256=sha(Path(__file__)),
                    source_pins=PINS, cases=count, stores=stores, complete_output_bytes=output_bytes,
                    fixture_sha256=sha(fixture), source_platform_returns=[],
                    limits=['Retained prior allocation-prefix arena/cache and literal static queue, not successful driver startup.',
                            'Only naturally empty full cleanup execution; nonempty releases remain mandatory.'])
    content = (json.dumps(metadata, sort_keys=True, indent=2)+'\n').encode()
    path = destination/'report.json'
    if path.exists():
        assert path.read_bytes() == content
    else:
        path.write_bytes(content)
    print(json.dumps(dict(cases=count, stores=stores, full_output_bytes=output_bytes,
                         fixture_sha256=sha(fixture), report_sha256=sha(path))))


if __name__ == '__main__':
    main()

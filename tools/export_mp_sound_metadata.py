#!/usr/bin/env python3
"""Export literal owned-map sound/class records for retained native providers."""
import argparse
import json
from pathlib import Path
from inspect_multiplayer import Cache, ROOT
from inventory_mp_weapon_effects import sha


def export(reports, providers, output):
    paths = set()
    for report in reports:
        for item in json.loads(report.read_text())['maps']:
            value = Path(item['path'])
            paths.add(value if value.is_absolute() else ROOT / value)
    rows, maps = [], []
    for path in sorted(paths):
        c = Cache(path)
        sounds = [(datum, tag) for datum, tag in sorted(c.tags.items())
                  if tag['group'] == 'snd!' and tag['offset'] is not None]
        classes = [tag for tag in c.tags.values() if tag['group'] == 'sncl' and tag['offset'] is not None]
        assert len(classes) == 1
        bank = c.block(classes[0]['offset'], 92)
        label = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
        rows.append(f'M {json.dumps(label)} {len(sounds)} {len(bank)}')
        for datum, tag in sounds:
            at = tag['offset']
            rows.append(f'S {datum:08x} {c.data[at:at+20].hex()}')
        for index, at in enumerate(bank):
            rows.append(f'C {index} {c.data[at:at+92].hex()}')
        maps.append(dict(path=label, sha256=sha(path), sounds=len(sounds), classes=len(bank)))
        c.data.close()
        c.stream.close()
    authority = json.loads(providers.read_text())
    position, forward = authority['vectors']
    ordinary, special = sorted(authority['callbacks'], key=lambda item: int(item['old_address'],16))
    for image, prefix in ((1,'old'), (2,'u5')):
        rows.append(f'V {image} {position["raw12_hex"]} {forward["raw12_hex"]} '
                    f'{ordinary[prefix+"_address"]} {special[prefix+"_address"]}')
    output.mkdir(parents=True, exist_ok=True)
    fixture = output / 'fixtures.tsv'
    fixture.write_text('\n'.join(rows)+'\n')
    report = dict(schema='owned-sound-metadata-provider-v1', maps=maps,
                  source_reports={str(path):sha(path) for path in reports},
                  immutable_provider_authority=dict(path=str(providers), sha256=sha(providers)),
                  tool_sha256=sha(Path(__file__)), cache_reader_sha256=sha(ROOT/'tools/inspect_multiplayer.py'),
                  fixture_sha256=sha(fixture), sounds=sum(m['sounds'] for m in maps),
                  classes=sum(m['classes'] for m in maps), live_audio_parity=False)
    (output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(maps=len(maps),sounds=report['sounds'],classes=report['classes'],
                         fixture_sha256=sha(fixture),report_sha256=sha(output/'report.json'))))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--resource-report',type=Path,action='append',required=True)
    parser.add_argument('--providers',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    export(args.resource_report,args.providers,args.output)

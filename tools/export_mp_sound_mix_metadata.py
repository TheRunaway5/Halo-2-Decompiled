#!/usr/bin/env python3
"""Retain complete owned snmx mix payloads, distinct from sncl classes."""
import argparse
import hashlib
import json
from pathlib import Path
from inspect_multiplayer import Cache, ROOT


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(inventory_path, output):
    expected='f750772e6951ff065711dd3c91b207d657e85fe69c43c89d06135eb717e072d3'
    if sha(inventory_path)!=expected:
        raise ValueError('Authenticated map inventory changed')
    inventory=json.loads(inventory_path.read_text())
    rows,maps=[],[]
    for item in inventory['maps']:
        path=Path(item['path'])
        path=path if path.is_absolute() else ROOT/path
        if sha(path)!=item['sha256']:
            raise ValueError('Owned map changed: '+str(path))
        cache=Cache(path)
        selected=[(datum,tag)for datum,tag in sorted(cache.tags.items())
                  if tag['group']=='snmx' and tag['offset'] is not None]
        if not selected:
            raise ValueError('Owned map lacks resident sound mix metadata')
        label=str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
        rows.append(f'M {json.dumps(label)} {len(selected)}')
        size=0
        for datum,tag in selected:
            offset,extent=tag['offset'],tag['size']
            if not extent or not (0<=offset<=offset+extent<=len(cache.data)):
                raise ValueError('Incomplete actual sound mix payload')
            address=offset+cache.magic
            raw=cache.data[offset:offset+extent]
            rows.append(f'T {datum:08x} snmx {address:08x} {raw.hex()}')
            size+=extent
        maps.append(dict(path=label,sha256=item['sha256'],tags=len(selected),bytes=size))
        cache.data.close();cache.stream.close()
    output.mkdir(parents=True,exist_ok=True)
    fixture=output/'fixtures.tsv'
    fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='owned-complete-sound-mix-metadata-v1',maps=maps,
                inventory=dict(path=str(inventory_path),sha256=expected),
                tool_sha256=sha(Path(__file__)),cache_reader_sha256=sha(ROOT/'tools/inspect_multiplayer.py'),
                fixture_sha256=sha(fixture),tags=sum(m['tags']for m in maps),
                exact_bytes=sum(m['bytes']for m in maps),
                semantic_owner='snmx sound mix; separate sncl class data unchanged',
                native_map_activation_or_source_defaults=False)
    (output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in ['tags','exact_bytes','fixture_sha256']}))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    export(args.inventory,args.output)

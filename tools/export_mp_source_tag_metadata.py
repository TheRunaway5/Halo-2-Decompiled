#!/usr/bin/env python3
"""Export complete original resident constructor tag payloads and node banks."""
import argparse
import hashlib
import json
from pathlib import Path
from inspect_multiplayer import Cache, ROOT


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(source_report, output):
    if sha(source_report) != 'f750772e6951ff065711dd3c91b207d657e85fe69c43c89d06135eb717e072d3':
        raise ValueError('Owned map inventory changed; authenticate new inputs first')
    inventory=json.loads(source_report.read_text())
    rows, maps=[],[]
    for item in inventory['maps']:
        path=Path(item['path'])
        path=path if path.is_absolute() else ROOT/path
        if sha(path)!=item['sha256']:
            raise ValueError(f'Owned map changed: {path}')
        c=Cache(path)
        selected=[(datum,tag) for datum,tag in sorted(c.tags.items())
                  if tag['group'] in ('bipd','hlmt','udlg') and tag['offset'] is not None]
        label=str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
        rows.append(f'M {json.dumps(label)} {len(selected)}')
        exact=queries=cross=0
        for datum,tag in selected:
            at,size=tag['offset'],tag['size']
            if not size or not (0<=at<=at+size<=len(c.data)):
                raise ValueError('Invalid complete resident tag extent')
            rows.append(f'T {datum:08x} {tag["group"]} {at+c.magic:08x} {c.data[at:at+size].hex()}')
            exact+=size
        # Original hlmt+78/+7C node bank. This includes the measured forwarding
        # pointer from masterchief_mp to masterchief's full owned hlmt payload.
        for datum,tag in selected:
            if tag['group']!='hlmt' or tag['size']<0x80:
                continue
            at=tag['offset']
            count,address=c.u(at+0x78),c.u(at+0x7c)
            if not count:
                continue
            extent=count*0x5c
            owners=[(d,t) for d,t in selected if t['offset']+c.magic<=address and
                    address+extent<=t['offset']+c.magic+t['size']]
            if len(owners)!=1:
                raise ValueError('Original model node bank has no unique owned tag backing')
            source=address-c.magic
            rows.append(f'Q {datum:08x} {count} {address:08x} {owners[0][0]:08x} '
                        f'{c.data[source:source+extent].hex()}')
            queries+=1;cross+=owners[0][0]!=datum
        maps.append(dict(path=label,sha256=item['sha256'],tags=len(selected),bytes=exact,
                         node_queries=queries,cross_tag_queries=cross))
        c.data.close();c.stream.close()
    output.mkdir(parents=True,exist_ok=True)
    fixture=output/'fixtures.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='owned-constructor-tag-metadata-v1',maps=maps,
                inventory=dict(path=str(source_report),sha256=sha(source_report)),
                tool_sha256=sha(Path(__file__)),cache_reader_sha256=sha(ROOT/'tools/inspect_multiplayer.py'),
                fixture_sha256=sha(fixture),tags=sum(m['tags'] for m in maps),
                exact_bytes=sum(m['bytes'] for m in maps),node_queries=sum(m['node_queries'] for m in maps),
                cross_tag_queries=sum(m['cross_tag_queries'] for m in maps),live_constructors=False)
    (output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('tags','exact_bytes','node_queries','cross_tag_queries','fixture_sha256')}))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    export(args.inventory,args.output)

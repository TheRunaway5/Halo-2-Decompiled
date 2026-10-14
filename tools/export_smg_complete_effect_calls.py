#!/usr/bin/env python3
"""Export all original equipped SMG effect calls, including magazine owners.

Read-only derivation from pinned original execution reports. The previous
prepared-effect fixture deliberately excluded reload/chamber calls. This
supplement preserves their full original order without changing that fixture.
"""
import argparse
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    provenance=ROOT/'analysis/multiplayer/integration/smg-prepared-effect-20261008/fixtures.json'
    pins=json.loads(provenance.read_text())['source_pins']
    for path,expected in pins.items():assert sha(ROOT/path)==expected,path
    source=ROOT/'analysis/multiplayer/map-pack/smg-constructor-equipped-source-fixture.json'
    original=json.loads(source.read_text());lines=['# name tick count; complete ordered tag previous a b failed-result tuples.']
    count=ticks=0;reports=[]
    for case in original['cases']:
        path=ROOT/'analysis/multiplayer/map-pack/smg-constructor-equipped-suite'/(case['name']+'.json')
        assert sha(path)==case['source_sha256']
        report=json.loads(path.read_text());reports.append(dict(path=str(path.relative_to(ROOT)),sha256=sha(path)))
        for row in case['rows']:
            calls=[event for event in report['weapon_events'] if event.get('tick')==row['tick'] and event.get('service')=='00107b50']
            words=[row['tick'],len(calls)]
            for event in calls:
                words.extend(int(v,16) for v in (event['registers']['eax'],*event['stack_arguments'],event['return_value']))
            lines.append(' '.join([case['name'],*(f'{v:08x}' for v in words)]));count+=len(calls);ticks+=1
    assert ticks==599
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    result=dict(scope=__doc__,ticks=ticks,effect_calls=count,reports=reports,source_pins=pins,
        source_provenance_sha256=sha(provenance),source_fixture_sha256=sha(source),
        fixture_sha256=sha(args.fixture),tool_sha256=sha(Path(__file__)))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(ticks=ticks,effect_calls=count)),flush=True)


if __name__=='__main__':main()

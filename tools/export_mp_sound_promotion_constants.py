#!/usr/bin/env python3
"""Import only paired immutable promotion arithmetic words from owned images."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS, Xbe, sha, save, encoded

PROVIDERS=ROOT/'analysis/multiplayer/integration/sound-promotion-native/paired/providers.json'
PROVIDERS_PIN='3b96cff25fd62c0a462dbbc22b9c9c5e7409acc1211cb64c9bbfe78328ff2fe0'
DEST=ROOT/'analysis/multiplayer/integration/sound-promotion-constants-provider-20261009/measured'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=DEST)
    output=parser.parse_args().output
    assert sha(PROVIDERS)==PROVIDERS_PIN
    providers=json.loads(PROVIDERS.read_text())
    rows=[];images=[]
    for index,(side,path)in enumerate([('old',OLD),('new',NEW)],1):
        assert sha(path)==PINS[str(path)];xbe=Xbe(path)
        words=[];operands=[]
        for name,entry in zip(providers['order'],providers['words'],strict=True):
            address=int(entry[side+'_va'],16);raw=xbe.read(address,4)
            assert raw.hex()==entry[side+'_hex']
            words.append(f'{int.from_bytes(raw,"little"):08x}')
            operands.append(dict(name=name,address=f'{address:08x}',raw_hex=raw.hex(),
                                 source_read_site=entry[side+'_site']))
        rows.append('\t'.join(['C',str(index),*words]))
        images.append(dict(source_path=str(path),source_sha256=sha(path),operands=operands))
    fixture=output/'constants.tsv';save(fixture,('\n'.join(rows)+'\n').encode())
    report=dict(schema='owned-sound-promotion-constants-v1',exporter_sha256=sha(__file__),
                providers_sha256=PROVIDERS_PIN,images=images,immutable_bytes=24,
                fixture_sha256=sha(fixture),mutable_source_state_imported=False,
                limits=['Base-image mapping remains unverified and absent.',
                        'Clock origin, counters, cooldown, options and sound admission retain separate actual owners.'])
    save(output/'report.json',encoded(report))
    print(json.dumps(dict(fixture_sha256=sha(fixture),report_sha256=sha(output/'report.json'),immutable_bytes=24)))

if __name__=='__main__':main()

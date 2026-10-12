#!/usr/bin/env python3
"""Export actual immutable listener and submit table operands from owned updates."""
import json
from pathlib import Path
from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS, Xbe, sha, save, encoded

PAIR = ROOT/'analysis/multiplayer/integration/sound-driver-remaining-native/paired'
SUBMIT = ROOT/'analysis/multiplayer/integration/sound-submit-native/paired-owners/providers.json'
PAIR_PIN = '9f738821e325385a50691d330dfda8bbb1ee7a830763bcea91f1164b89b4c8d4'
SUBMIT_PIN = 'ad7f1c944a1f2832c4e1b980467d18fc91c52828275be5947dcfcd60c0dc7220'
DEST = ROOT/'analysis/multiplayer/integration/sound-driver-owner-operands-provider-20261009/measured'


def main():
    assert sha(PAIR/'report.json') == PAIR_PIN and sha(SUBMIT) == SUBMIT_PIN
    pair=json.loads((PAIR/'report.json').read_text());submit=json.loads(SUBMIT.read_text())
    listener=next(q for q in pair['owners'] if q['old']=='002228a0')
    epsilon=[next(q for q in listener['memory']if q['old_va']==address)for address in ['0046508c','0046504c']]
    lines=[(PAIR/'instructions'/f'002228a0-{side}.asm').read_text().splitlines()for side in ['old','u5']]
    default_index=next(i for i,row in enumerate(lines[0])if row.startswith('00222b17 '))
    rows=[];evidence=[]
    for index,(tag,path)in enumerate([('old',OLD),('u5',NEW)]):
        assert sha(path)==PINS[str(path)];xbe=Xbe(path);side='old'if tag=='old'else'new'
        thresholds=[xbe.read(int(q[side+'_va'],16),4)for q in epsilon]
        for raw,q in zip(thresholds,epsilon):assert raw.hex()==q[side+'_hex']
        default_address=int(lines[index][default_index].split()[-1],16)
        default=xbe.read(default_address,72)
        callback=next(q for q in submit['effect_callback_tables']if q['image']==tag)
        callback_address=int(callback['address'],16);table=xbe.read(callback_address,32)
        assert table.hex()==callback['raw32_hex']
        rows.append('\t'.join(['C',str(index+1),*[f'{int.from_bytes(raw,"little"):08x}'for raw in thresholds],
                               default.hex(),f'{callback_address:08x}',table.hex()]))
        evidence.append(dict(image=tag,source_path=str(path),source_sha256=sha(path),
                             epsilon_addresses=[q[side+'_va']for q in epsilon],
                             default_address=f'{default_address:08x}',callback_address=f'{callback_address:08x}'))
    fixture=DEST/'operands.tsv';save(fixture,('\n'.join(rows)+'\n').encode())
    report=dict(schema='sound-driver-owner-operands-v1',exporter_sha256=sha(__file__),
                paired_report_sha256=PAIR_PIN,submit_provider_sha256=SUBMIT_PIN,images=evidence,
                immutable_bytes=232,fixture_sha256=sha(fixture),source_platform_returns=[],
                limits=['Only two thresholds, full72-byte default environment and full32-byte callback table/identity per image.',
                        'Mutable driver, device, active map, listener channels, effects, admission and original callbacks remain actual owners.'])
    save(DEST/'report.json',encoded(report));print(json.dumps(dict(fixture_sha256=sha(fixture),
        report_sha256=sha(DEST/'report.json'),exact_immutable_bytes=232)))


if __name__=='__main__':main()

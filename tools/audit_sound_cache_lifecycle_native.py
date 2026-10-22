#!/usr/bin/env python3
"""Audit original instruction authority, independent repeats and older freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
PRIOR=ROOT/'analysis/multiplayer/integration/sound-cache-request-native/final-manifest.json'
PRIOR_PIN='2568f5ded360ee5e8d7fd9967045f7c12a11fbb462b23f0e1beaf7d69b9c07dc'
def prior_pins():
    assert sha(PRIOR)==PRIOR_PIN
    r=json.loads(PRIOR.read_text());pins=dict(r['prior_pins']);pins[str(PRIOR.relative_to(ROOT))]=PRIOR_PIN
    for field in('owned_current_files','evidence_files','original_inputs'):
        for rel,q in r[field].items():
            assert rel not in pins or pins[rel]==q['sha256'];pins[rel]=q['sha256']
    return pins
def audit(d):
    pins=prior_pins();mismatches=[]
    for rel,pin in pins.items():
        path=Path(rel)if Path(rel).is_absolute()else ROOT/rel
        if sha(path)!=pin:mismatches.append(dict(path=rel,expected=pin,actual=sha(path)))
    assert not mismatches,mismatches
    equal=[]
    for left,right in(('paired','paired-repeat'),('measured','repeated')):
        for p in(d/left).rglob('*'):
            if not p.is_file()or p.name=='clock-values.json':continue
            q=d/right/p.relative_to(d/left);assert q.is_file()and p.read_bytes()==q.read_bytes(),(p,q)
            equal.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
    actual=json.loads((d/'measured/clock-values.json').read_text());replay=json.loads((d/'repeated/clock-values.json').read_text())
    assert [(q['key'],q['value'])for q in actual]==[(q['key'],q['value'])for q in replay]
    assert all(q['provider']=='actual CLOCK_BOOTTIME'for q in actual)
    assert all(q['provider']=='exact replay of separately retained actual CLOCK_BOOTTIME'for q in replay)
    assert (d/'measured/clock-values.json').read_bytes()==(d/'repeated/clock-replay.json').read_bytes()
    images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0
    for p in(d/'paired/instructions').glob('*.asm'):
        image=images[p.stem.rsplit('-',1)[1]]
        for line in p.read_text().splitlines():a,b,*_=line.split();raw=bytes.fromhex(b);assert image.read(int(a,16),len(raw))==raw;rows+=1
    for line in(d/'measured/instructions/executed-old.asm').read_text().splitlines():
        a,b,*_=line.split();raw=bytes.fromhex(b);assert images['old'].read(int(a,16),len(raw))==raw;rows+=1
    for rel in('include/halo2/sound_cache_lifecycle.hpp','src/audio/sound_cache_lifecycle.cpp','tests/sound_cache_lifecycle_tests.cpp'):
        for name in('validation-v2','validation-v2-repeat'):assert(ROOT/rel).read_bytes()==(d/name/'native/source'/rel).read_bytes(),rel
    for name in('normal.stdout','normal.stderr','sanitizer.stdout','sanitizer.stderr'):
        a=d/'validation-v2/native'/name;b=d/'validation-v2-repeat/native'/name
        assert a.read_bytes()==b.read_bytes();equal.append([str(a.relative_to(ROOT)),str(b.relative_to(ROOT)),sha(a)])
    assert (d/'fixtures.tsv').read_bytes()==(d/'export-repeat/fixtures.tsv').read_bytes()
    meta=json.loads((d/'fixtures.json').read_text());other=json.loads((d/'export-repeat/fixtures.json').read_text())
    assert meta['original_report']['sha256']==other['original_report']['sha256']
    other['original_report']['path']=meta['original_report']['path'];assert meta==other
    assert meta['cases']==12 and meta['stages']==18 and meta['stores']==864
    result=dict(schema='sound-cache-lifecycle-audit-v1',auditor_sha256=sha(__file__),prior_pins_verified=pins,exact_repeat_pairs=equal,original_instruction_rows_checked=rows,cases=12,phases=18,ordered_stores=864,alias_returns=768,actual_host_clock_reads=3,full_fixture_region_bytes=meta['full_region_bytes'],clock_repeat='Recorded actual OS input replayed only at unchanged GetTickCount data thunk; provider distinction retained.',fixture_metadata_repeat='Only original_report.path differs, pointing to independently identical report bytes.',mismatches=mismatches,active_audio_or_concurrent_claim=False)
    save(d/'audit.json',encoded(result));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(pins),equal_files=len(equal),instruction_rows=rows,mismatches=0)))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();audit(a.directory.resolve())

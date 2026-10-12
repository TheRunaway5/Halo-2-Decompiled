#!/usr/bin/env python3
"""Audit genuine driver stops, full paired bytes and protected prior freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
PRIOR=((ROOT/'analysis/multiplayer/integration/sound-cache-lifecycle-native/final-manifest.json','8fe991e53ef677ed8ec1806b67f70d2edc45f8ff88d8f93416d336f6d607e63b'),(ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/final-manifest.json','e75f3b1fabc796cf5ddf7608fbcc7786ce6504f97ff180e93994d322349ab228'))
def prior_pins():
    pins={}
    def put(rel,pin):
        assert rel not in pins or pins[rel]==pin,rel;pins[rel]=pin
    for path,pin in PRIOR:
        assert sha(path)==pin;put(str(path.relative_to(ROOT)),pin);r=json.loads(path.read_text())
        for rel,value in r['prior_pins'].items():put(rel,value)
        for field in('owned_current_files','evidence_files','original_inputs'):
            for rel,value in r[field].items():put(rel,value['sha256'])
    return pins
def audit(d):
    pins=prior_pins();mismatches=[]
    for rel,pin in pins.items():
        path=Path(rel)if Path(rel).is_absolute()else ROOT/rel
        if sha(path)!=pin:mismatches.append(dict(path=rel,expected=pin,actual=sha(path)))
    assert not mismatches,mismatches
    equal=[]
    for left,right in(('paired','paired-repeat'),('measured-v2','repeated-v2')):
        for p in(d/left).rglob('*'):
            if not p.is_file():continue
            q=d/right/p.relative_to(d/left);assert q.is_file()and p.read_bytes()==q.read_bytes(),(p,q);equal.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
    images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0
    for p in(d/'paired/instructions').glob('*.asm'):
        image=images[p.stem.rsplit('-',1)[1]]
        for line in p.read_text().splitlines():a,b,*_=line.split();raw=bytes.fromhex(b);assert image.read(int(a,16),len(raw))==raw;rows+=1
    for p in(d/'measured-v2/instructions').glob('*.asm'):
        image=images[p.stem.split('-')[0]]
        for line in p.read_text().splitlines():a,b,*_=line.split();raw=bytes.fromhex(b);assert image.read(int(a,16),len(raw))==raw;rows+=1
    for p in(d/'export-repeat').iterdir():q=d/p.name;assert p.read_bytes()==q.read_bytes();equal.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
    for rel in('include/halo2/sound_driver_update.hpp','src/sound_driver_update.cpp','tests/sound_driver_update_tests.cpp'):
        for name in('validation-v2','validation-v2-repeat','production-v2'):assert(ROOT/rel).read_bytes()==(d/name/('source'if name=='production-v2'else'native/source')/rel).read_bytes(),rel
    for name in('normal.stdout','sanitizer.stdout','normal.stderr','sanitizer.stderr'):
        a=d/'validation-v2/native'/name;b=d/'validation-v2-repeat/native'/name;assert a.read_bytes()==b.read_bytes();equal.append([str(a.relative_to(ROOT)),str(b.relative_to(ROOT)),sha(a)])
    meta=json.loads((d/'fixtures.json').read_text());report=json.loads((d/'measured-v2/report.json').read_text());assert meta['cases']==16 and meta['ordered_stores']==0
    assert all(not c['source_fields_assigned']and not c['whole_owner_returned']and c['stop']['result_supplied']is False for c in report['cases'])
    assert sum(c['events'][0]['service']=='required_DirectSoundDoWork'for c in report['cases'])==8
    production=json.loads((d/'production-v2/commands.json').read_text());assert all(q['exit_code']==0 for q in production['commands'])
    sanflags=(d/'production-v2/sanitizer/sound_driver_update_tests-flags.make').read_text();assert '-fsanitize=address,undefined'in sanflags
    for tag in('normal','sanitizer'):
        output=(d/'production-v2'/(tag+'-ctest.stdout')).read_text();assert '100% tests passed out of 2'in output and 'PASS 12 original lifecycle controls'in output and 'PASS 16 original1.1/u5 driver-prefix controls'in output
    registration=(d/'production-v2/source/CMakeLists.txt').read_text();assert 'src/sound_cache_lifecycle.cpp src/sound_driver_update.cpp'in registration and 'NAME original_sound_driver_update'in registration
    result=dict(schema='sound-driver-update-audit-v1',auditor_sha256=sha(__file__),prior_pins_verified=pins,exact_repeat_pairs=equal,original_instruction_rows_checked=rows,cases=16,ordered_persistent_stores=0,actual_unavailable_device_stops=8,actual_unavailable_section_stops=8,full_fixture_region_bytes=meta['full_region_bytes'],production_command_sha256=sha(d/'production-v2/commands.json'),production_source_drift=production['source_drift'],mismatches=mismatches,successful_active_update_or_status_supplied=False)
    save(d/'audit.json',encoded(result));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(pins),equal_files=len(equal),instruction_rows=rows,mismatches=0)))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();audit(a.directory.resolve())

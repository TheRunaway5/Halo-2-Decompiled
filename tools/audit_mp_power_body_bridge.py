#!/usr/bin/env python3
"""Audit immutable original body-state replay/export and literal native scope."""
import hashlib,json
from pathlib import Path
from oracle_mp_power_body_bridge_1_1 import ROOT,OUT,verify
from oracle_smg_firing_1_1 import Xbe,PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_power_body_bridge import REPORT_PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def files(p):return {q.name:sha(q)for q in p.iterdir()if q.is_file()}
def main():
    prior=verify();base=OUT/'final';a,b=base/'measured',base/'repeat'
    fa,fb=files(a),files(b);assert fa==fb and len(fa)==24
    assert sha(a/'report.json')==REPORT_PIN
    r=json.loads((a/'report.json').read_text());assert r['oracle_sha256']==sha(ROOT/'tools/oracle_mp_power_body_bridge_1_1.py')
    assert r['source_factory_outputs_and_full_prior_schedule_identical']
    assert not r['new_source_fields_or_results_supplied']and not r['instruction_patches']
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');sites=set();rows=0
    assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
    for q in r['instruction_listings']:
        p=a/q['file'];assert sha(p)==q['sha256'];lines=p.read_text().splitlines();assert len(lines)==q['rows'];rows+=len(lines)
        for line in lines:
            address,h,*_=line.split();address=int(address,16);raw=bytes.fromhex(h)
            assert image.read(address,len(raw))==raw;sites.add(address)
    for s in r['states']:
        body={k:v for k,v in s.items()if k not in ('id','sha256')}
        assert hashlib.sha256(encoded(body)).hexdigest()==s['sha256']
        for raw in s['raw_backing']:assert len(bytes.fromhex(raw['raw_hex']))==raw['size']
    assert len(r['calls'])==404 and len(r['states'])==328 and len(r['stores'])==3834 and len(r['reads'])==4900
    for c in r['calls']:assert c['order']<c['return_order']and c['return_esp_delta']>=4
    for e in r['stores']:
        assert e['size']in(1,2,4,8)
        assert len(bytes.fromhex(e['before_hex']))==len(bytes.fromhex(e['written_hex']))==e['size']
    e=base/'exported-owner';er=base/'exported-owner-repeat';fe,fr=files(e),files(er);assert fe==fr and len(fe)==2
    export=json.loads((e/'report.json').read_text());assert export['source_report_sha256']==REPORT_PIN
    assert export['exporter_sha256']==sha(ROOT/'tools/export_mp_power_body_bridge.py')
    assert export['fixture_sha256']==sha(e/'fixtures.tsv')and not export['native_expected_values_used']and not export['source_fields_or_results_supplied']
    assert len(export['cases'])==50 and sum(c['ordered_stores']for c in export['cases'])==1006
    # Existing exact refresh owner is a genuine nested source call. Its full
    # owning bytes are independently frozen and correlated by the exporter.
    for p in (ROOT/'analysis/multiplayer/sandbox-source/contact-lifecycle/final/measured').glob('*.asm'):
        for line in p.read_text().splitlines():
            address,h,*_=line.split();address=int(address,16);raw=bytes.fromhex(h)
            assert image.read(address,len(raw))==raw;sites.add(address)
    deliveries=0
    for line in (e/'fixtures.tsv').read_text().splitlines():
        if line.startswith('W '):assert int(line.split()[1],16)in sites
        deliveries+=line.startswith('D ')
    assert deliveries==1
    later=base/'later-complete/complete-report.json';lr=json.loads(later.read_text())
    assert len(lr['routines'])==23 and all(q['normalized_instruction_and_internal_branch_topology_equal']for q in lr['routines'])
    assert not lr['native_later_gameplay_execution']and not lr['native_gameplay_update_applied']
    result=dict(schema=1,auditor_sha256=sha(__file__),prior_lifecycle_files_verified=prior,
        independent_original_files=fa,independent_export_files=fe,original_instruction_rows=rows,
        original_calls=404,source_states=328,source_reads=4900,source_stores=3834,
        native_export_calls=50,native_export_stores=1006,exact_manager_deliveries=1,
        native_full_retained_bytes=sum(q['output_bytes']for q in export['cases']),
        source_later_body_correspondences=23,later_execution_claimed=False,
        factory_and_entire_prior_schedule_unchanged=True,source_fields_or_success_results_supplied=False,
        native_expected_values_used=False)
    frozen_write(base/'audit.json',encoded(result));print(json.dumps({k:v for k,v in result.items()if not isinstance(v,dict)}))
if __name__=='__main__':main()

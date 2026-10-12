#!/usr/bin/env python3
"""Verify original contact lifecycle freeze, source bytes and literal export."""
import hashlib,json
from pathlib import Path
from oracle_mp_physics_contact_lifecycle_1_1 import ROOT,OUT,verify
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,PIN
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    prior=verify();a=OUT/'final/measured';b=OUT/'final/repeat'
    fa={p.name:sha(p)for p in a.iterdir()if p.is_file()};fb={p.name:sha(p)for p in b.iterdir()if p.is_file()}
    assert fa==fb and len(fa)==16
    r=json.loads((a/'report.json').read_text());assert r['oracle_sha256']==sha(ROOT/'tools/oracle_mp_physics_contact_lifecycle_1_1.py')
    assert r['all_prior_phase_calls_stores_states_byte_identical']
    assert not r['new_supplied_game_or_Havok_fields']and not r['supplied_source_results']and not r['instruction_patches']
    imagepath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(imagepath)==PIN;image=Xbe(imagepath)
    sites=set();rows=0
    for l in r['original_listings']:
        p=a/l['file'];assert sha(p)==l['sha256']
        lines=p.read_text().splitlines();assert len(lines)==l['rows'];rows+=len(lines)
        for line in lines:
            address,h,*_=line.split();address=int(address,16);raw=bytes.fromhex(h)
            assert image.read(address,len(raw))==raw;sites.add(address)
    for s in r['states']:
        body={k:v for k,v in s.items()if k not in ('id','sha256')}
        assert hashlib.sha256(encoded(body)).hexdigest()==s['sha256']
        for name,n in (('raw_havok',0x106470),('raw_components',0x1409b)):
            assert s[name]['size']==n and len(bytes.fromhex(s[name]['raw_hex']))==n
    for c in r['calls']:assert c['order']<c['return_order']and c['return_esp_delta']>=4
    for e in r['stores']:
        # Constructor stores were already byte-audited by the immutable prior
        # constructor manifest; every other store is in this original listing.
        if int(e['instruction'],16)not in sites:assert 0x1d2df0<=int(e['instruction'],16)<0x1d2f10
        assert len(bytes.fromhex(e['before_hex']))==len(bytes.fromhex(e['written_hex']))==e['size']
    assert [x['consumed_hex']for x in r['removal_packet_consumption']]==['0000']
    assert next(c for c in r['calls']if c['entry']=='001d3fc0')['callback_consumed_index_hex']=='0000'
    assert all(m['x87_control_word']=='023f'and m['mxcsr']=='00001f80'for m in r['math'])
    assert [m['local18_hex']for m in r['math']if m['instruction']=='001d30b6']==['0a000000']*2
    exported=OUT/'final/exported-math';repeat=OUT/'final/exported-math-repeat'
    fe={p.name:sha(p)for p in exported.iterdir()if p.is_file()};fr={p.name:sha(p)for p in repeat.iterdir()if p.is_file()}
    assert fe==fr and len(fe)==2
    e=json.loads((exported/'report.json').read_text());assert e['source_report_sha256']==sha(a/'report.json')
    assert e['exporter_sha256']==sha(ROOT/'tools/export_mp_physics_contact_lifecycle.py')
    assert e['fixture_sha256']==sha(exported/'fixtures.tsv')and not e['native_expected_values_used']
    cases=e['cases'];assert len(cases)==11 and sum(c['ordered_stores']for c in cases)==90
    math=dispatch=0
    with (exported/'fixtures.tsv').open()as f:
        for line in f:
            math+=line.startswith('F ');dispatch+=line.startswith('V ')
    assert math==12 and dispatch==6
    result=dict(schema=1,auditor_sha256=sha(__file__),prior_artifacts_verified=prior,
        independently_repeated_source_files=fa,independently_repeated_export_files=fe,
        complete_original_instruction_rows=rows,source_states=len(r['states']),source_calls=len(r['calls']),
        source_reads=len(r['reads']),source_stores=len(r['stores']),source_math_snapshots=len(r['math']),
        exported_original_calls=11,exported_ordered_stores=90,exported_virtual_deliveries=6,
        exported_math_snapshots=12,exported_full_buffer_bytes=sum(c['source_output_bytes']for c in cases),
        unchanged_original_schedule=True,source_outputs_supplied=False,native_expected_values=False)
    frozen_write(OUT/'final/audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()

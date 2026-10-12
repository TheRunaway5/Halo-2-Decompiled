#!/usr/bin/env python3
"""Audit and freeze two independent original effect-owned grenade begin runs."""
import argparse
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, PIN, CACHE_PIN
from oracle_mp_grenade_effect_pool_1_1 import ROOT, OUT, motion


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve()
    assert not args.output.exists()
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert motion.digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
    assert motion.digest(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
    measured=OUT/'measured';repeat=OUT/'repeat'
    report=json.loads((measured/'report.json').read_text())
    assert (measured/'report.json').read_bytes()==(repeat/'report.json').read_bytes()
    assert report['oracle_sha256']==motion.digest(ROOT/'tools/oracle_mp_grenade_effect_pool_1_1.py')
    files={};checked=0
    for path,digest in report['source_pins'].items():assert motion.digest(ROOT/path)==digest,path
    assert report['prior_runtime_correlation']['byte_identical']
    assert not report['new_supplied_gameplay_fields'] and not report['new_supplied_source_returns'] and not report['instruction_patches']
    assert set(report['removed_inherited_services'])=={'00115650'}
    assert report['removed_inherited_services']['00115650']==['unit animation transition: supplied unavailable',0,16]
    assert len(report['constructor_stages'])==2
    assert [s['entry'] for s in report['constructor_stages']]==['001775d0','00177690']
    assert all(s['completion']=='complete_original_return' and s['invalid'] is None and not s['services'] for s in report['constructor_stages'])
    assert all(int(p['pointer'],16)==0 for p in report['before_pools'])
    assert all(s['clock']==report['states'][0]['clock'] and s['seed']==report['states'][0]['seed'] for s in report['states'])
    expected_pools=[('004f1f0c',256,0x190),('004f1f08',512,0x3c),('00518464',128,0x54),
                    ('00526514',1024,0x40),('00526518',256,0x4c),('0052651c',256,0x34)]
    for stage in report['constructor_stages']:
        assert [(p['global_address'],p['capacity'],p['stride']) for p in stage['complete_pools']]==expected_pools
        assert all(len(bytes.fromhex(p['complete_storage_hex']))==p['capacity']*p['stride'] for p in stage['complete_pools'])
    assert all(bytes.fromhex(p['header4c_hex'])[0x29]==1 for p in report['constructor_stages'][1]['complete_pools'])
    assert len(report['cases'])==1
    case=report['cases'][0]
    assert case['source_selector']==0 and case['actual_weapon_datum']=='e2700001'
    assert len(case['rows'])==2 and len(case['stages'])==25 and not case['unreturned_original_calls']
    assert all(s['completion']=='complete_original_return' and s['invalid'] is None for s in case['stages'])
    prior=json.loads((ROOT/'analysis/multiplayer/integration/grenade-bounded-phases/measured-complete/report.json').read_text())
    assert case['inputs']==prior['cases'][0]['inputs']
    assert {k:v for k,v in report['states'][0].items() if k not in {'id','sha256','effect_context'}}=={k:v for k,v in prior['states'][0].items() if k not in {'id','sha256'}}
    calls=report['calls']
    def one(entry):
        rows=[c for c in calls if c['entry']==entry]
        assert len(rows)==1,(entry,len(rows))
        return rows[0]
    def returned(entry,eax,delta):
        row=one(entry)
        assert row['return_cpu']['registers']['eax']==eax and row['return_esp_delta']==delta,entry
        return row
    returned('001775d0','00000000',4);returned('00175860','00000000',4);returned('00177690','00000000',4)
    begin=returned('000e95e0','00400001',12)
    created=returned('00179b90','e6650000',12)
    visible=returned('0017b1a0','00000001',4)
    retired=returned('00179080','00000001',4)
    effect=returned('001781f0','ffffffff',20)
    update=returned('000e99b0','00000001',12)
    assert created['return_order']<retired['order']<visible['return_order']<effect['return_order']<begin['return_order']<update['order']
    assert not any(c['entry'] in {'0018aa90','0018ad70','000e8d60','000b8a30'} for c in calls)
    # Begin's original cancellation dispatch calls the held-NONE E8F30 branch
    # before the new phase1. This is not release of an allocated held grenade.
    empty_cancel=one('000e8f30')
    assert empty_cancel['caller']=='000e9a81' and empty_cancel['stack24_words'][:4]==['e26f0000','00000001','00000000','00000000']
    assert 'return_cpu' in empty_cancel
    assert report['states'][empty_cancel['before_state']]['grenade']['held_object']=='ffffffff'
    assert report['states'][empty_cancel['after_state']]['grenade']['held_object']=='ffffffff'
    assert report['states'][empty_cancel['after_state']]['grenade']['phase']==3
    assert len([c for c in calls if c['entry']=='0011d620'])==1
    admitted=report['states'][begin['after_state']]['grenade']
    assert admitted['active_words']==['00400000','00000000'] and admitted['phase']==1 and admitted['timer']==0 and admitted['release_timer']==8
    assert admitted['counts']==[2,0] and admitted['held_object']=='ffffffff' and admitted['held_type']==-1
    final=case['final']['grenade']
    assert final==dict(admitted,timer=1,action44_hex=report['states'][update['after_state']]['grenade']['action44_hex'])
    stores=[s for s in report['stores'] if s['instruction']=='000e993b']
    assert len(stores)==1 and stores[0]['before_hex']=='00000000' and stores[0]['written_hex']=='00004000'
    assert stores[0]['address']==f'{int(admitted["action_pointer"],16)+4:08x}'
    assert all(not p['occupied'] and len(bytes.fromhex(p['complete_storage_hex']))==p['capacity']*p['stride'] for p in report['final_pools'])
    for metadata in report['source_metadata']['throw_begin_effects']:
        assert struct.unpack_from('<I',bytes.fromhex(metadata['root1c_hex']))[0]==2
        assert len(metadata['locations'])==len(metadata['events'])==1
        assert len(metadata['events'][0]['parts'])==1 and metadata['events'][0]['particle_system_count']==0
    # Source-produced pools and actual original call outputs are compared in full,
    # with every listing byte independently checked against the untouched image.
    for directory in (measured,repeat):
        assert (directory/'stderr.log').read_bytes()==b''
        logs=[json.loads(line) for line in (directory/'stdout.log').read_text().splitlines()]
        assert len(logs)==3 and logs[-1]['sha256']==motion.digest(directory/'report.json')
        for listing in report['instructions']:
            path=directory/listing['file']
            assert motion.digest(path)==listing['sha256']
            assert path.read_bytes()==(repeat/listing['file']).read_bytes()
            for line in path.read_text().splitlines():
                address,raw,_=line.split(' ',2);data=bytes.fromhex(raw)
                assert image.read(int(address,16),len(data))==data
                checked+=1
        for path in directory.rglob('*'):
            if path.is_file():files[str(path.relative_to(ROOT))]=motion.digest(path)
    previous_pins={}
    for name in ('analysis/multiplayer/integration/grenade-bounded-phases/manifest.json',
                 'analysis/multiplayer/integration/inventory-source/smg-effect-owner/manifest.json'):
        manifest=json.loads((ROOT/name).read_text());files[name]=motion.digest(ROOT/name)
        for key in ('source_files','verified_prior_pins','pins','prior_pins'):
            previous_pins.update(manifest.get(key,{}))
    for name,digest in previous_pins.items():assert motion.digest(ROOT/name)==digest,name
    for name in ('tools/oracle_mp_grenade_effect_pool_1_1.py','tools/audit_mp_grenade_effect_pool_1_1.py',
                 'docs/MP_GRENADE_EFFECT_POOL_BEGIN_RESEARCH.md'):
        files[name]=motion.digest(ROOT/name)
    value=dict(schema=1,source_files=files,verified_prior_pins=previous_pins,
        independently_identical_pair=['measured','repeat'],original_xbe_sha256=PIN,original_cache_sha256=CACHE_PIN,
        source_instruction_rows=checked,complete_original_calls=len(calls),complete_observed_states=len(report['states']),
        ordered_original_stores=len(report['stores']),begin=dict(entry='000e95e0',al=1,esp_delta=12,state=admitted,active_store=stores[0]),
        genuine_effect_result=dict(allocated_handle='e6650000',final_return='ffffffff',visibility='actual empty local PVS',retired=True,audio_producer_called=False),
        first_retained_update=dict(entry='000e99b0',al=1,esp_delta=12,state=final),
        empty_cancel=dict(entry='000e8f30',caller='000e9a81',held_object='ffffffff',phase_after=3),
        no_new_supplied_game_fields_or_returns=True,
        scope='Explicit source-owned effect-pool initialization plus bounded original grenade begin and first retained update on the unchanged allocated owner. Nonlocal negative visibility retires the actual effect. No naturally advancing world clock, local sound, grenade creation/release/fuse/explosion or live native action is claimed.')
    motion.frozen_write(args.output,motion.encoded(value))
    manifest=dict(value)
    manifest['source_files']={**files,str(args.output.relative_to(ROOT)):motion.digest(args.output)}
    manifest['native_gameplay_update_applied']=False
    path=OUT/'manifest.json';assert not path.exists()
    motion.frozen_write(path,motion.encoded(manifest))
    print(json.dumps(dict(audit_sha256=motion.digest(args.output),manifest_sha256=motion.digest(path),
        source_instruction_rows=checked,source_files=len(manifest['source_files']),prior_pins=len(previous_pins))),flush=True)


if __name__=='__main__':main()

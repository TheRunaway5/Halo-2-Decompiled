#!/usr/bin/env python3
"""Read-only audit/export of the separate frozen original FP sound evidence."""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,Cache,PIN,CACHE_PIN

BASE=ROOT/'analysis/multiplayer/map-pack/smg-fp-sound'
FROZEN=BASE/'frozen'
ORACLE_PIN='44777fe55191fe36ccdef4cca53afa3ab3b5720057bc18229ce8e9698111e4a6'
SUITE_PIN='f290a4fafe336efa899518e460ec7cbcaf6331e017e8c964067727924ae96641'
PRIOR_PINS={
    'tools/oracle_smg_fp_clock_1_1.py':'7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6',
    'tools/oracle_smg_fp_registration_1_1.py':'06c0ddfece364a4361b60219b4f2ab26e98acc693aacccbe16eb89f8d0535e92',
    'tools/oracle_smg_constructor_equipped_1_1.py':'1c6ce60d4576f37548f822bfbd5667f3e97d806926db593cd98cb9d7f6c9eb99',
    'tools/oracle_mp_weapon_attachment_1_1.py':'9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c',
    'tools/oracle_mp_inventory_equipped_1_1.py':'47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655',
    'analysis/multiplayer/map-pack/smg-fp-clock/source-fixture.json':'4f87bca8cda0b5b5652a0dadf9c48a5e957ae1c8e99caadd7db524624e48c2df',
    'analysis/multiplayer/map-pack/smg-fp-clock/frozen/suite.json':'d831472491423ff7a2fe9db7a20daabc2727e29a1a340a70739efed07e4ccebb',
    'analysis/multiplayer/map-pack/smg-fp-clock/ready-producer/report.json':'578faf3a028b8a5843b39491f6fd57539d5e317915b87e2e3d2321a3febe5a1d',
}
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def calls(r,entry):return [x for x in r['calls']if x['entry']==f'{entry:08x}']
def u(raw,offset,n=4):return int.from_bytes(raw[offset:offset+n],'little')

def main():
    assert digest(ROOT/'tools/oracle_smg_fp_sound_1_1.py')==ORACLE_PIN
    assert digest(FROZEN/'suite.json')==SUITE_PIN
    for p,pin in PRIOR_PINS.items():assert digest(ROOT/p)==pin,p
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    suite=json.loads((FROZEN/'suite.json').read_text());reports={};instruction_rows=0;listings=0
    for case in suite['cases']:
        p=ROOT/case['path'];q=ROOT/case['reexecution_path']
        assert digest(p)==case['sha256'] and p.read_bytes()==q.read_bytes()
        r=json.loads(p.read_text());reports[case['case']]=r
        assert r['oracle_sha256']==ORACLE_PIN and r['xbe_sha256']==PIN
        assert not r['platform_return_or_clock_supplied']
        assert not r['successful_audio_initialization'] and not r['successful_audio_playback']
        assert '0018ad70'not in r['supplied_services']
        ip=ROOT/r['instruction_inventory']['path'];assert digest(ip)==r['instruction_inventory']['sha256']
        inventory=json.loads(ip.read_text())
        for row in inventory['routines']:
            asm=ROOT/row['path'];assert digest(asm)==row['sha256']
            for line in asm.read_text().splitlines():
                address,hex_bytes,*rest=line.split();raw=bytes.fromhex(hex_bytes)
                assert image.read(int(address,16),len(raw))==raw
                instruction_rows+=1
            listings+=1
        for boundary in r['boundaries']:assert not boundary['platform_result_supplied']
    startup=reports['startup'];v=calls(startup,0x33785C)[0]
    assert v['stack'][:4]==['00000000','00023000','00001000','00000004']
    assert startup['boundaries'][0]['instruction']=='0033786f'
    assert startup['final_audio']['globals']['00517ff0']=='00000000'
    init=reports['cache_init'];v=calls(init,0x33785C)[0]
    assert v['stack'][:4]==['00000000','0000208c','00101000','00000004']
    assert all('return_eax'not in r for r in init['calls'])
    assert [b['dependency']for b in init['boundaries']]==['NtAllocateVirtualMemory','KeTickCount']
    state=init['final_audio'];assert state['initialized_78']==0 and state['hardware_available_79']==1 and state['enabled_7a']==1
    assert state['clock_origin_80']=='00000000'and state['clock_time_84']=='00000000'
    uninit=reports['uninitialized_ready'];assert uninit['boundaries'][0]['instruction']=='0021f366'
    assert uninit['boundaries'][0]['address']=='00000078'
    assert 'return_eax'not in calls(uninit,0x18AD70)[0]
    resources=reports['resources']['authored_sound'];metadata_records=0;chunk_records=0
    def audit_records(value):
        nonlocal metadata_records,chunk_records
        if isinstance(value,dict):
            if set(('file_offset','original_va','raw_hex'))<=value.keys():
                p=value['file_offset'];raw=bytes.fromhex(value['raw_hex'])
                assert value['original_va']==f'{p+cache.magic:08x}'
                assert bytes(cache.data[p:p+len(raw)])==raw;metadata_records+=1
            if 'raw_sha256'in value:
                path=ROOT/value['file'];size=value['size_from_original_21abd0_mask']
                with path.open('rb')as f:f.seek(value['file_offset']);raw=f.read(size)
                assert len(raw)==size and hashlib.sha256(raw).hexdigest()==value['raw_sha256'];chunk_records+=1
            for v in value.values():audit_records(v)
        elif isinstance(value,list):
            for v in value:audit_records(v)
    audit_records(resources)
    permutations=[p for s in resources['sounds']for rr in s['ranges']for p in rr['permutations']]
    assert len(resources['sounds'])==17 and len(permutations)==23 and chunk_records==84
    assert len(calls(reports['resources'],0x21ABD0))==23
    assert len(calls(reports['resources'],0x21AD70))==23
    for p in permutations:
        assert p['original_21ad70_word']==u(bytes.fromhex(p['record']['raw_hex']),6,2)
        assert p['original_21abd0_byte_total']==sum(x['size_from_original_21abd0_mask']for x in p['chunks'])
    previous=json.loads((ROOT/'analysis/multiplayer/map-pack/smg-fp-clock/source-fixture.json').read_text())
    fixture_cases=[];unchanged_ticks=0
    expectations={'ready':(1,0xE5AC0437,0x24),'reload':(47,0xE5AD0438,0x65),'empty_reload':(124,0xE5AD0438,0x64)}
    for name,(tick,datum,activity)in expectations.items():
        r=reports[name];event=calls(r,0x1687C2);assert len(event)==1
        event=event[0];event_bytes=bytes.fromhex(event['event_hex'])
        assert event['tick']==tick and u(event_bytes,0,2)==1 and u(event_bytes,4)==datum
        request=calls(r,0x18AD70);assert len(request)==1;request=request[0]
        assert request['stack'][:2]==[f'{datum:08x}','00000001']
        assert request['stack'][3:]==['3f800000','ffffffff','00000000']
        assert request['registers']['esi']=='00010002' and request['registers']['edi']=='0044b14c'
        assert request['return_eax']=='ffffffff'
        conversion=calls(r,0x18EF20)[0];assert conversion['type_before']==1 and conversion['type_after']==0
        callback=calls(r,0x18DDE0)[0];assert int(callback['return_eax'],16)&255==1
        builder=calls(r,0x18BA10)[0];raw=bytes.fromhex(builder['request_before_hex'])
        assert u(raw,0)==0 and u(raw,4)==0x3F800000
        assert u(raw,0x44)==0xFFFFFFFF and u(raw,0x48)==0x10002 and u(raw,0x4C)==0x44B14C
        assert u(raw,0x54)==0
        play=calls(r,0x21F220)[0];praw=bytes.fromhex(play['play_state_before_hex'])
        assert play['return_eax']=='ffffffff'and builder['return_eax']=='ffffffff'
        assert u(praw,0)==0x2D and u(praw,4)==1
        assert u(praw,0x50)==0x10002 and u(praw,0x58)==0x44B14C
        assert u(praw,0x8C,2)==40 and u(praw,0x98)==0xFFFFFFFF
        assert not calls(r,0x128380)and not calls(r,0x127750)
        pcase=next(x for x in previous['cases']if x['name']==('normal_wrapper_ready'if name=='ready'else name))
        prior=json.loads((ROOT/pcase['source_report_path']).read_text())
        for row,old in zip(r['tick_states'],prior['rows']):
            assert row['after_fp']==old['after_fp'],(name,row['tick']);unchanged_ticks+=1
        final=r['tick_states'][-1]['after_fp']['hands'][0]
        assert final['sound_identifier']=='ffffffff'and int(final['sound_activity'],16)==activity
        # Raw unused stack bytes are retained in original reports, but this
        # consumer contract exports only source-defined/admitted fields.
        fixture_cases.append({'case':name,'source_report':str((FROZEN/f'{name}.json').relative_to(ROOT)),
            'source_report_sha256':digest(FROZEN/f'{name}.json'),'tick':tick,
            'source_event_hex':event['event_hex'],'sound_datum':f'{datum:08x}',
            'owner_datum':'00010002','requested_audible':1,'resolved_audible':0,
            'source_request_scale_bits':'3f800000','callback_table':'0044b14c',
            'defined_location_30_hex':raw[:0x30].hex(),
            'marker_28_hex':builder['location_before_hex'],'play_state_flags':'0000002d',
            'play_state_priority':1,'play_state_source_data_size':40,
            'source_result':'ffffffff','retained_hand_sound_identifier':'ffffffff',
            'retained_hand_activity':f'{activity:04x}','audio_initialized':False,
            'successful_playback_or_permutation_selection_proven':False})
    fixture={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_sha256':ORACLE_PIN,
        'frozen_suite_sha256':SUITE_PIN,'cases':fixture_cases,'authored_sound':resources,
        'scope':'Original FP event/request construction with source-produced unavailable audio; no successful platform initialization/playback/scheduler/clock or permutation choice.',
        'native_gameplay_update_applied':False}
    fp=BASE/'source-fixture.json';fp.write_text(json.dumps(fixture,indent=2)+'\n')
    audit={'oracle_sha256':ORACLE_PIN,'frozen_suite_sha256':SUITE_PIN,
        'source_fixture_sha256':digest(fp),'independent_reexecutions':len(suite['cases']),
        'instruction_listings':listings,'original_instruction_rows':instruction_rows,
        'cache_metadata_records':metadata_records,'graph_sound_references':17,
        'source_metadata_permutation_queries':23,'retained_original_chunk_records':chunk_records,
        'completed_original_play_requests':3,'uninitialized_original_context_stop':1,
        'fp_after_states_equal_previous_unavailable_leaf':unchanged_ticks,
        'prior_dependency_pins_verified':PRIOR_PINS,'platform_result_or_clock_supplied':False,
        'successful_sound_scheduler_or_playback_proven':False,'native_gameplay_update_applied':False}
    (BASE/'audit.json').write_text(json.dumps(audit,indent=2)+'\n')
    print(json.dumps(audit,indent=2))

if __name__=='__main__':main()

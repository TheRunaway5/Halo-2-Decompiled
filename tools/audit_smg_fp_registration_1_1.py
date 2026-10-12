#!/usr/bin/env python3
"""Freeze/audit original FPS registration, with no native expected values."""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_constructor_equipped_1_1 import ROOT,PIN,Xbe,_Code

TOOL='06c0ddfece364a4361b60219b4f2ab26e98acc693aacccbe16eb89f8d0535e92'
REPORT='fe7c7ea7ef620b526ab6a9debda83068055a4e02ee700bdd1ed3dea9cb375ca2'
INVENTORY='e4fb2c631cbc68ee2799ad5226bba8a7f72abcb77044711c1bf04d1f618ddf82'
OUT=ROOT/'analysis/multiplayer/map-pack/smg-fp-registration'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    assert sha(ROOT/'tools/oracle_smg_fp_registration_1_1.py')==TOOL
    assert sha(OUT/'report.json')==sha(OUT/'report-reexecution.json')==REPORT
    assert sha(OUT/'instructions/inventory.json')==INVENTORY
    r=json.loads((OUT/'report.json').read_text());assert r['oracle_sha256']==TOOL and r['xbe_sha256']==PIN
    steps={s['name']:s for s in r['steps']}
    initial=steps['original_fps_global_initialization']['after']
    assert initial['globals_pointer']=='07600000' and initial['scratch_pointer']=='07608330'
    assert initial['allocator_used']==0x10340
    assert all(v['unit']==v['selector']=='ffffffff'and all(h['weapon']=='ffffffff'for h in v['hands'])for v in initial['views'])
    converted=steps['original_multiplayer_profile_conversion']['after'];assert converted['player_selector_88']==2
    profile_writes=[w for w in r['writes']if w.get('player_offset')=='088']
    assert len(profile_writes)==1 and profile_writes[0]['instruction']=='0014d685'and profile_writes[0]['after_hex']=='02000000'
    unchanged=steps['original_observer_after_profile_change_with_same_unit']['after']['views'][0]
    assert unchanged['selector']=='00000000'and unchanged['hands'][0]['weapon']=='ffffffff'
    registered=steps['original_equipped_view_weapon_registration']['after']['views'][0];h=registered['hands'][0]
    assert registered['unit']=='00010001'and registered['selector']=='00000002'
    assert h['weapon']=='00010002'and h['flags']=='00000006'
    assert h['weapon_node_count']==4 and h['arms_node_count']==37
    assert struct.unpack('<64i',bytes.fromhex(h['weapon_node_map_hex']))[:4]==(37,38,39,40)
    assert struct.unpack('<64i',bytes.fromhex(h['arms_node_map_hex']))[:37]==tuple(range(37))
    assert h['channel']=={'graph':'e5aa0435','animation_identifier':'0015ffff','flags_11':'00',
        'flags_12':'003f','rate_18_bits':'3f800000','frame_1c_bits':'00000000'}
    assert registered['hands'][1]['weapon']=='ffffffff'
    metadata=r['metadata'];graph=metadata['graph_nodes'];models={t['tag']['datum']:t for t in metadata['models']}
    for model_id,map_field,count_field in (('e5a80433','weapon_node_map_hex','weapon_node_count'),
                                          ('e9e90864','arms_node_map_hex','arms_node_count')):
        indices=struct.unpack('<64i',bytes.fromhex(h[map_field]))[:h[count_field]]
        assert len(models[model_id]['nodes'])==len(indices)
        for source,target in zip(models[model_id]['nodes'],indices):
            assert source['string_id']==graph[target]['string_id']
    assert metadata['selected_clips'][0]['index']==21 and metadata['selected_clips'][0]['name']=='first_person:idle'
    assert metadata['selected_clips'][0]['frames']==89
    assert steps['original_observer_after_supplied_requested_index_none']['after']['views'][0]['hands'][0]['weapon']=='00010002'
    for name in ('original_observer_after_supplied_current_index_none','original_view_release_handler'):
        assert steps[name]['after']['views'][0]['hands'][0]['weapon']=='ffffffff'
    assert steps['original_observer_after_supplied_current_index_restore']['after']['views'][0]['hands'][0]['weapon']=='00010002'
    assert steps['original_view_release_handler']['after']['views'][0]['unit']=='ffffffff'
    firing=steps['original_registered_two_firing_ticks']['hand_timer_phases']
    assert [x['after_weapon']for x in firing]==[[9,0],[8,0]]
    assert r['supplied_services']['00168866']['cleanup']==4 and '0016a356'in r['supplied_services']
    assert not any(c['entry']=='00168866'for c in r['calls'])
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(x.data).hexdigest()==PIN;code=_Code(x,'.text');checks=[]
    inv=json.loads((OUT/'instructions/inventory.json').read_text())
    for row in inv['routines']:
        p=ROOT/row['path'];assert sha(p)==row['sha256'];count=0
        for line in p.read_text().splitlines():
            address,data,*_=line.split();assert code.at(int(address,16)).bytes.hex()==data;count+=1
        checks.append({'entry':row['entry'],'sha256':row['sha256'],'instructions_checked':count})
    compact=OUT/'source-fixture.json'
    compact.write_text(json.dumps({'xbe_sha256':PIN,'source_report_sha256':REPORT,
        'oracle_sha256':TOOL,'profile_input':r['profile_input'],'external_inputs':r['external_inputs'],
        'metadata':r['metadata'],'steps':r['steps'],'scope':r['scope']},indent=2)+'\n')
    p=OUT/'audit.json';p.write_text(json.dumps({'xbe_sha256':PIN,'oracle_sha256':TOOL,
        'report_sha256':REPORT,'independent_reexecution_byte_identical':True,
        'instructions_inventory_sha256':INVENTORY,'instruction_listings':checks,
        'instructions_checked':sum(c['instructions_checked']for c in checks),
        'source_calls':len(r['calls']),'source_writes':len(r['writes']),
        'source_fixture_sha256':sha(compact),'original_node_name_mapping_pairs_verified':41,
        'first_person_clock_executed':False,'first_person_pose_executed':False,
        'native_gameplay_update_applied':False},indent=2)+'\n')
    print(json.dumps({'audit_sha256':sha(p),'source_fixture_sha256':sha(compact)}))

if __name__=='__main__':main()

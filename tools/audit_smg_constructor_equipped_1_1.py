#!/usr/bin/env python3
"""Audit the independently executed allocated/attached SMG evidence; export raw phases.

This reads frozen original output and original XBE instructions. It does not
import a native gameplay implementation or manufacture expected weapon state.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_constructor_equipped_1_1 import ROOT,PIN,Xbe,_Code

TOOL='1c6ce60d4576f37548f822bfbd5667f3e97d806926db593cd98cb9d7f6c9eb99'
DEFAULT='e071c76ab098696d88f9606026916f042dee169b848872a4eeb49d4acbaf50b1'
MANIFEST='54dad96620c0ee23b2e4be3086984a4f2d0696cdd2b07e1ec8ec1cb91119a764'
OUT=ROOT/'analysis/multiplayer/map-pack'
SUITE=OUT/'smg-constructor-equipped-suite'
REPEAT=OUT/'smg-constructor-equipped-reexecution'
PHASES=('before','after_unit','after_weapon','after_animation','after_postpass')

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def raw_state(s):
    b=bytes.fromhex(s['weapon_16c_260_hex']);assert len(b)==0xF4
    def h(off):return struct.unpack_from('<h',b,off-0x16C)[0]
    assert struct.unpack_from('<I',b,0x178-0x16C)[0]==s['weapon_state_u32']
    for field,off in (('blocking_ticks',0x17C),('weapon_animation_index',0x17E),
                      ('magazine_mode',0x224),('magazine_ticks',0x226),
                      ('reserve',0x22A),('loaded',0x22C),
                      ('reload_transfer_ticks',0x230),('magazine_232',0x232),
                      ('trigger_ticks',0x20E)):
        assert h(off)==s[field],(field,h(off),s[field])
    assert b[0x1A4-0x16C:0x1D8-0x16C].hex()==s['barrel_state_hex']
    assert b[0x20C-0x16C:0x218-0x16C].hex()==s['trigger_state_hex']
    assert b[0x224-0x16C:0x234-0x16C].hex()==s['magazine_state_hex']
    own=s['source_selected_weapon_own_animation'];assert own['index']==1 and own['frame_count']==1
    assert struct.unpack_from('<h',bytes.fromhex(own['raw_hex']),0x14)[0]==1
    return {k:s[k] for k in ('weapon_16c_260_hex','barrel_state_hex','trigger_state_hex','magazine_state_hex',
        'weapon_state_u32','blocking_ticks','weapon_animation_index','loaded','reserve','magazine_mode',
        'magazine_ticks','reload_transfer_ticks','magazine_232','weapon_flags','input_flags',
        'trigger_state','trigger_ticks','trigger_flags','trigger_effect_handle','barrel_effect_handle',
        'unit_hand_timers','unit_equipped_indices_hex','shared_seed','parent','parent_node','manager_hex',
        'weapon_own_primary_channel','source_selected_weapon_own_animation')}

def main():
    assert sha(ROOT/'tools/oracle_smg_constructor_equipped_1_1.py')==TOOL
    assert sha(OUT/'smg-constructor-equipped-1.1-oracle.json')==DEFAULT
    assert sha(REPEAT/'constructor.json')==DEFAULT
    assert sha(SUITE/'suite.json')==MANIFEST
    suite=json.loads((SUITE/'suite.json').read_text());repeat=json.loads((REPEAT/'suite.json').read_text())
    assert suite['oracle_tool_sha256']==repeat['oracle_tool_sha256']==TOOL
    assert suite['ticks']==repeat['ticks']==599 and suite['requests']==repeat['requests']==165
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(x.data).hexdigest()==PIN;code=_Code(x,'.text');listing_rows=[]
    for manifest in ('inventory.json','post-reload-inventory.json'):
        d=OUT/'smg-constructor-equipped-instructions';inv=json.loads((d/manifest).read_text())
        assert inv['xbe_sha256']==PIN
        for row in inv['routines']:
            p=ROOT/row['listing'];assert sha(p)==row['listing_sha256']
            count=0
            for line in p.read_text().splitlines():
                address,data,*_=line.split();inst=code.at(int(address,16));assert inst.bytes.hex()==data
                count+=1
            listing_rows.append({'entry':row['entry'],'instructions_checked':count,'sha256':sha(p)})
    original=json.loads((OUT/'smg-constructor-equipped-1.1-oracle.json').read_text())
    initial=original['actual_original_initial_loadout']['weapon'];assert initial['parent']=='00010001' and initial['parent_node']==20
    assert initial['weapon_state_u32']==9 and initial['blocking_ticks']==0 and initial['barrel_counter']==127
    assert [initial[k] for k in ('loaded','reserve','magazine_ticks','reload_transfer_ticks','magazine_232')]==[60,120,-1,-1,-1]
    assert original['explicit_timed_handler_case']['after']['blocking_ticks']==15
    predicates=original['explicit_reload_admission_operand_cases']['cases'];assert len(predicates)==12
    for row in predicates:
        admitted=all(row[k]==0 for k in ('supplied_weapon_state_178','supplied_trigger_20c','supplied_trigger_218'))
        assert int(row['return_eax'],16)==int(admitted)
    cases=[];verified=[]
    for case,other in zip(suite['cases'],repeat['cases']):
        assert case['name']==other['name'] and case['sha256']==other['sha256']
        p=ROOT/case['path'];assert sha(p)==sha(ROOT/other['path'])==case['sha256']
        r=json.loads(p.read_text());assert len(r['rows'])==case['ticks']
        assert r['oracle_tool_sha256']==TOOL and r['xbe_sha256']==PIN
        snapshots=0;rows=[]
        for row in r['rows']:
            phases={phase:raw_state(row[phase]['weapon']) for phase in PHASES};snapshots+=len(phases)
            assert all(s['parent']=='00010001' and s['parent_node']==20 for s in phases.values())
            requests=[{'descriptor_hex':q['request_descriptor_hex'],'source_service':q['service'],
                       'supplied_allocation_failed':True} for q in row['projectile_requests']]
            rows.append({'tick':row['tick'],'supplied_held':row['supplied_held'],
                'supplied_input_record_hex':row['control']['supplied_input_record_hex'],
                'source_converted_input_hex':row['control']['after_original_action_input_hex'],
                'phases':phases,'projectile_requests':requests})
        assert sum(len(row['projectile_requests']) for row in rows)==case['requests']
        for service in ('000fb560','001dc8f0','001dc940'):
            assert service not in r['supplied_services']
        if case['name']=='reload_rejected_45':
            gate=[e for e in r['source_calls'] if e['entry']=='00106030' and e['tick']==45]
            assert len(gate)==1 and gate[0]['admission_operands']['weapon_state_178']==1 and gate[0]['return_eax']=='00000000'
        if case['name']=='reload_accepted_46':
            assert [rows[46]['phases']['after_unit'][k] for k in ('weapon_state_u32','magazine_mode','magazine_ticks','reload_transfer_ticks')]==[5,1,50,50]
            assert [rows[46]['phases']['after_weapon'][k] for k in ('weapon_state_u32','magazine_ticks','reload_transfer_ticks')]==[0,49,49]
            assert [rows[95]['phases']['after_weapon'][k] for k in ('loaded','reserve','magazine_mode')]==[60,97,5]
            assert rows[96]['phases']['after_weapon']['weapon_state_u32']==3
            assert rows[97]['phases']['after_weapon']['weapon_state_u32']==0
        if case['name']=='held_empty':
            assert [rows[123]['phases']['after_weapon'][k] for k in ('weapon_state_u32','magazine_mode','magazine_ticks')]==[5,1,50]
            assert [rows[173]['phases']['after_weapon'][k] for k in ('loaded','reserve','magazine_mode')]==[60,60,5]
            assert rows[174]['phases']['after_weapon']['weapon_state_u32']==3
            assert len(rows[175]['projectile_requests'])==1
        cases.append({'name':case['name'],'source_sha256':case['sha256'],'initial':raw_state(r['initial']['weapon']),
                      'rows':rows,'summary':r['summary']})
        verified.append({'name':case['name'],'sha256':case['sha256'],'ticks':case['ticks'],
                         'requests':case['requests'],'raw_phase_snapshots':snapshots,'reexecution_byte_identical':True})
    compact=OUT/'smg-constructor-equipped-source-fixture.json'
    compact.write_text(json.dumps({'xbe_sha256':PIN,'oracle_tool_sha256':TOOL,'source_suite_sha256':MANIFEST,
        'scope':'Raw extraction from independently repeated original attached/equipped source cases. No native expected values or first-person presentation are supplied.',
        'cases':cases},indent=2)+'\n')
    audit=OUT/'smg-constructor-equipped-audit.json'
    audit.write_text(json.dumps({'xbe_sha256':PIN,'oracle_tool_sha256':TOOL,'source_suite_sha256':MANIFEST,
        'constructor_report_reexecution_byte_identical':True,'cases':verified,
        'ticks':599,'requests':165,'raw_phase_snapshots':2995,'reload_admission_operands':12,
        'instruction_listings':listing_rows,'source_fixture_path':str(compact.relative_to(ROOT)),
        'source_fixture_sha256':sha(compact),'native_gameplay_update_applied':False},indent=2)+'\n')
    print(json.dumps({'audit':str(audit),'audit_sha256':sha(audit),'source_fixture':str(compact),'source_fixture_sha256':sha(compact)}))

if __name__=='__main__':main()

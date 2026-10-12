#!/usr/bin/env python3
"""Audit independent input controls, exact original writes/stops and old pins."""
import argparse,json
from oracle_mp_combat_postprocess_controls import ROOT,IMAGES,sha,digest,CACHE_PIN
from oracle_smg_firing_1_1 import Cache,Xbe,_Code
BASE=ROOT/'analysis/multiplayer/title-update-1.5/combat-postprocess-controls'
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--suite',default=BASE/'frozen/suite.json',type=type(BASE));a=ap.parse_args();s=json.loads(a.suite.read_text())
    for p,h in s['files'].items():assert sha(ROOT/p)==h,p
    prior=0
    for p,h in s['prior_manifests'].items():
        assert sha(ROOT/p)==h
        for q,v in json.loads((ROOT/p).read_text())['files'].items():assert sha(ROOT/q)==v,q;prior+=1
    r=json.loads((BASE/'measured/report.json').read_text());b=json.loads((BASE/'repeat/report.json').read_text());assert r==b
    assert r['tool_sha256']==sha(ROOT/'tools/oracle_mp_combat_postprocess_controls.py')and r['original_fixture_dependency_sha256']==sha(ROOT/'tools/oracle_mp_combat_postprocess_updates.py')
    assert r['original_cache_sha256']==CACHE_PIN and not r['native_gameplay_update_applied']and r['variants']==43 and len(r['cases'])==86
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN;images={q[0]:Xbe(q[1])for q in IMAGES};rows=0;stores=0
    try:
        for z in r['cases']:
            input_=bytearray(c.data)
            for v in z['explicit_control_operand_changes']:
                p=int(v['file_offset'],16);n=v['size'];assert bytes(input_[p:p+n]).hex()==v['authored_before_hex']and int(v['original_va'],16)==p+c.magic
                input_[p:p+n]=bytes.fromhex(v['explicit_control_after_hex'])
            assert digest(input_)==z['fixture_input_cache_sha256'];patch=bytearray(input_);image=images[z['image_label']];code=_Code(image,'.text')
            assert digest(image.data)==z['xbe_sha256']and z['original_callbacks_services_gameplay_results_supplied']==[]
            for v in z['ordered_metadata_stores']:
                p=int(v['file_offset'],16);n=v['size'];assert bytes(patch[p:p+n]).hex()==v['before_hex'];patch[p:p+n]=bytes.fromhex(v['after_hex']);stores+=1
                t=c.tags[int(v['tag_datum'],16)];assert t['name']==v['tag_name']and t['group']==v['tag_group']and p-t['offset']==int(v['tag_relative_offset'],16)
                assert code.at(int(v['instruction'],16)).mnemonic=='movss'
            assert digest(patch)==z['complete_cache_output_sha256']
            if z['returned']:assert z['stack_cleanup']==4 and z['final_eip']=='03000000'and not z['unprovided_invalid_access_stop']
            else:
                assert len(z['unprovided_invalid_access_stop'])==1;stop=z['unprovided_invalid_access_stop'][0];assert stop['size']==4 and stop['address']in('00000028','0000000c')and stop['instruction']==z['final_eip']
            if z['case'].startswith('scenario_type_'):assert z['returned']and z['metadata_store_count']==0
        assert sum(z['returned']for z in r['cases'])==82
        for label,image in images.items():
            code=_Code(image,'.text')
            for run in('measured','repeat'):
                for p in(BASE/run/label).glob('*.asm'):
                    for line in p.read_text().splitlines():
                        at,h,mn,op=(line.split(' ',3)+[''])[:4];ins=code.at(int(at,16));raw=bytes.fromhex(h);assert ins.bytes==raw==image.read(int(at,16),len(raw))and ins.mnemonic==mn and ins.op_str==op;rows+=1
        for left,right in zip(r['cases'][::2],r['cases'][1::2]):
            assert left['case']==right['case']and left['complete_cache_output_sha256']==right['complete_cache_output_sha256']
            assert [{k:v for k,v in z.items()if k!='instruction'}for z in left['ordered_metadata_stores']]==[{k:v for k,v in z.items()if k!='instruction'}for z in right['ordered_metadata_stores']]
        result={'suite_sha256':sha(a.suite),'files':len(s['files']),'prior_frozen_members_unchanged':prior,'explicit_operand_variants':43,'original_runs':86,'returned_runs':82,'actual_access_stops':4,
                'original_instruction_rows_verified':rows,'source_ordered_metadata_stores_verified':stores,'independent_whole_repeat_identical':True,'native_gameplay_update_applied':False}
        (a.suite.parent/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
    finally:c.data.close();c.stream.close()
if __name__=='__main__':main()

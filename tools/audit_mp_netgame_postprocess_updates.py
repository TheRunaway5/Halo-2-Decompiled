#!/usr/bin/env python3
"""Verify six independent original netgame postprocess cases and old freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_netgame_postprocess_updates import ROOT,IMAGES,CACHE_PATHS,COMPAT,COMPAT_SOURCE_PIN,COMPAT_LIBRARY_PIN,sha,digest
from oracle_smg_firing_1_1 import Cache,Xbe,_Code
BASE=ROOT/'analysis/multiplayer/title-update-1.5/netgame-postprocess'
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--suite',type=Path,default=BASE/'frozen/suite.json');a=ap.parse_args();s=json.loads(a.suite.read_text())
    for p,h in s['files'].items():assert sha(ROOT/p)==h,p
    prior=0
    for p,h in s['prior_manifests'].items():
        assert sha(ROOT/p)==h;pairs=json.loads((ROOT/p).read_text())['files'].items()
        for q,v in pairs:assert sha(ROOT/q)==v,q;prior+=1
    for p,h in s['external_dependencies'].items():assert sha(p)==h,p
    r=json.loads((BASE/'measured/report.json').read_text());b=json.loads((BASE/'repeat/report.json').read_text());assert r==b
    assert r['tool_sha256']==sha(ROOT/'tools/oracle_mp_netgame_postprocess_updates.py')and not r['native_gameplay_update_applied']
    assert r['compatibility_source_sha256']==COMPAT_SOURCE_PIN and r['compatibility_library_sha256']==COMPAT_LIBRARY_PIN
    assert len(r['cases'])==6;instructions=0;services=0;stores=0
    for k,q in enumerate(r['cases']):
        path=CACHE_PATHS[k//2];label,xp,pin,entry,slot,wrapper=IMAGES[k%2];c=Cache(path);x=Xbe(xp);code=_Code(x,'.text')
        try:
            assert sha(path)==q['cache_sha256']and sha(xp)==pin==q['xbe_sha256'];assert q['entry']==f'{entry:08x}'and q['source_scenario_type']==1
            assert q['stack_cleanup']==4 and q['original_gameplay_or_activation_results_supplied']==[]
            start=int(q['source_netgame_file_offset'],16);n=q['source_netgame_count']*32;patch=bytearray(c.data)
            assert bytes(patch[start:start+n]).hex()==q['raw_netgame_hex']
            for z in q['ordered_netgame_stores']:
                p=int(z['file_offset'],16);size=z['size'];assert z['netgame_flag_index']==(p-start)//32 and int(z['member_offset'],16)==(p-start)%32
                assert bytes(patch[p:p+size]).hex()==z['before_hex'];patch[p:p+size]=bytes.fromhex(z['after_hex']);stores+=1
            assert bytes(patch[start:start+n]).hex()==q['processed_netgame_hex']and digest(patch)==q['complete_cache_output_sha256']
            directory=path.stem+'-'+label
            for run in('measured','repeat'):
                assert(BASE/run/directory/'netgame-flags-raw.bin').read_bytes().hex()==q['raw_netgame_hex']
                assert(BASE/run/directory/'netgame-flags-processed.bin').read_bytes().hex()==q['processed_netgame_hex']
                for p in(BASE/run/directory).glob('*.asm'):
                    for line in p.read_text().splitlines():
                        at,h,mn,op=(line.split(' ',3)+[''])[:4];ins=code.at(int(at,16));raw=bytes.fromhex(h)
                        assert raw==x.read(int(at,16),len(raw))==ins.bytes and ins.mnemonic==mn and ins.op_str==op;instructions+=1
            for z in q['access_services']:
                assert z['ordinal']==182 and z['void_no_result_supplied']and z['actual_platform_operation_completed']
                assert z['gpr_values_before']==z['gpr_values_after']and z['rounded_contents_before_sha256']==z['rounded_contents_after_sha256'];services+=1
        finally:c.data.close();c.stream.close()
    assert [q['source_map_id']for q in r['cases']]==[100,100,50,50,1200,1200]
    assert [q['actual_changed_stores']for q in r['cases']]==[2,2,0,0,0,2]
    assert [len(q['access_services'])for q in r['cases']]==[2,2,0,0,0,2]
    assert r['cases'][0]['processed_netgame_hex']==r['cases'][1]['processed_netgame_hex']
    assert r['cases'][4]['raw_netgame_hex']==r['cases'][4]['processed_netgame_hex']and r['cases'][5]['raw_netgame_hex']!=r['cases'][5]['processed_netgame_hex']
    result={'suite_sha256':sha(a.suite),'files':len(s['files']),'prior_frozen_members_unchanged':prior,'whole_original_cases':6,'independent_repeat_report_identical':True,
            'original_instruction_rows_verified':instructions,'measured_source_netgame_stores':stores,'actual_access_operations':services,
            'new_1_5_relic_changes':2,'native_gameplay_update_applied':False,'complete_Xbox_MM_or_map_activation_claimed':False}
    (a.suite.parent/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()

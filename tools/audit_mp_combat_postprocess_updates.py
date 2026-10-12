#!/usr/bin/env python3
"""Audit unchanged source bytes, repeat outputs and the independent freeze."""
import argparse,hashlib,json
from pathlib import Path
from oracle_mp_combat_postprocess_updates import ROOT,IMAGES,CACHE_PIN,sha,SCHEMAS,digest
from oracle_smg_firing_1_1 import Cache,Xbe,_Code

BASE=ROOT/'analysis/multiplayer/title-update-1.5/combat-postprocess'
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--suite',type=Path,default=BASE/'frozen/suite.json');a=ap.parse_args();s=json.loads(a.suite.read_text())
    for p,h in s['files'].items():assert sha(ROOT/p)==h,p
    prior_members=0
    for p,h in s['prior_manifests'].items():
        assert sha(ROOT/p)==h,p;m=json.loads((ROOT/p).read_text())
        if isinstance(m.get('files'),dict):members=m['files'].items()
        else:members=((z['path'],z['sha256'])for z in m['artifacts'])
        for q,v in members:assert sha(ROOT/q)==v,q;prior_members+=1
    r=json.loads((BASE/'measured/report.json').read_text());repeat=json.loads((BASE/'repeat/report.json').read_text());assert r==repeat
    assert r['tool_sha256']==sha(ROOT/'tools/oracle_mp_combat_postprocess_updates.py')
    assert r['cache_sha256']==CACHE_PIN and not r['native_gameplay_update_applied']
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
    try:
        assert r['cache_magic']==f'{c.magic:08x}'and r['ordered_metadata_stores_equal']and r['complete_cache_outputs_equal']
        for q,h in r['schema_sha256'].items():assert sha(SCHEMAS/(q+'.xml'))==h
        listings=0;total_stores=0
        for row,config in zip(r['images'],IMAGES):
            label,path,pin,entry,caller,glob,inventory=config;assert sha(path)==pin==row['xbe_sha256'];x=Xbe(path);code=_Code(x,'.text')
            assert row['entry']==f'{entry:08x}'and row['owning_caller']==f'{caller:08x}'and row['whole_original_instruction_count']==262
            assert row['source_scenario_gate_i16_at_10']==1==c.h(c.scenario+0x10)
            assert row['original_calls_or_platform_services_supplied']==[]and row['stack_cleanup']==4
            patched=bytearray(c.data);stores=[z for z in row['ordered_stores']if z['region']=='original_cache_metadata']
            assert len(stores)==row['metadata_store_count']==41
            assert sum(z['before_hex']!=z['after_hex']for z in stores)==row['metadata_actual_value_change_count']==38
            for z in stores:
                off=int(z['file_offset'],16);n=z['size'];assert int(z['address'],16)==c.magic+off
                assert bytes(patched[off:off+n]).hex()==z['before_hex'];patched[off:off+n]=bytes.fromhex(z['after_hex'])
                t=c.tags[int(z['tag']['datum'],16)];assert all(t[k]==z['tag'][k]for k in('datum','group','name','offset','size'))
                assert off-t['offset']==int(z['tag_relative_offset'],16)
                assert code.at(int(z['instruction'],16)).mnemonic=='movss';total_stores+=1
            assert digest(patched)==row['complete_cache_output_sha256']
            assert len(row['changed_tags'])==13
            for z in row['changed_tags']:
                t=z['tag'];off,n=t['offset'],t['size'];raw=bytes(c.data[off:off+n]);processed=bytes(patched[off:off+n])
                assert raw.hex()==z['raw_hex']and processed.hex()==z['processed_hex']
                assert digest(raw)==z['raw_sha256']and digest(processed)==z['processed_sha256']
                stem=t['datum']+'-'+t['group'].replace('!','_')
                for run in('measured','repeat'):
                    for suffix,data in(('raw',raw),('processed',processed)):assert(BASE/run/label/'tag-buffers'/(stem+'-'+suffix+'.bin')).read_bytes()==data
            for run in('measured','repeat'):
                for p in(BASE/run/label).glob('*.asm'):
                    for line in p.read_text().splitlines():
                        site,h,mn,op=(line.split(' ',3)+[''])[:4];ins=code.at(int(site,16));b=bytes.fromhex(h)
                        assert ins.bytes==b==x.read(int(site,16),len(b))and ins.mnemonic==mn and ins.op_str==op;listings+=1
        x,y=[Xbe(q[1])for q in IMAGES]
        for z in r['paired_image_memory_operands']:
            assert x.read(int(z['disc_address'],16),z['size']).hex()==z['disc_hex']
            assert y.read(int(z['later_address'],16),z['size']).hex()==z['later_hex']
            assert z['disc_hex']==z['later_hex']and z['bytes_equal']
        result={'suite_sha256':sha(a.suite),'frozen_files':len(s['files']),'prior_frozen_members_unchanged':prior_members,
                'source_instruction_rows_verified':listings,'complete_original_executions':4,'whole_report_repeat_identical':True,
                'original_metadata_writes_per_image':41,'changed_writes_per_image':38,'changed_original_tags_per_image':13,
                'complete_cache_outputs_equal':True,'native_gameplay_update_applied':False,'new_1_5_only_balance_change_claimed':False}
        (a.suite.parent/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
    finally:c.data.close();c.stream.close()
if __name__=='__main__':main()

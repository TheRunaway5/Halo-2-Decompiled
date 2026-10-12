#!/usr/bin/env python3
"""Verify repeated original resets on actual authored scenario/player objects."""
from collections import Counter
import hashlib
import json
from pathlib import Path

from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import Xbe,_Code
from oracle_mp_inventory_1_1 import Cache
from oracle_round_lifecycle import variants
from oracle_round_reset_scenario import PINS
from verify_round_reset import expanded

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/round-reset-scenario'


def sha(path):
    path=Path(path);before=path.stat();value=hashlib.sha256(path.read_bytes()).hexdigest();after=path.stat()
    assert (before.st_ino,before.st_size,before.st_mtime_ns)==(after.st_ino,after.st_size,after.st_mtime_ns),path
    return value


def main():
    report_path=OUT/'report.json';data=json.loads(report_path.read_text());r=expanded(data,data)
    assert report_path.read_bytes()==(OUT/'reproduced/report.json').read_bytes()
    assert sha(ROOT/'tools/oracle_round_reset_scenario.py')==data['oracle_sha256']
    assert data['prior_pins']==PINS
    for path,pin in(data['source_dependencies']|PINS).items():assert sha(ROOT/path)==pin,path
    for key,raw in data['raw_blobs'].items():assert hashlib.sha256(bytes.fromhex(raw)).hexdigest()==key
    spec=SPECS['tu11'];assert data['source_sha256']==sha(spec['path'])==spec['pin']
    assert data['cache_sha256']==sha(ROOT/'assets/disc/maps/lockout.map')
    assert data['generator_sha256']==sha(ROOT/'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/generator.exe')
    image=Xbe(spec['path']);code=_Code(image,'.text')
    listing=data['executed_instruction_listing'];path=OUT/'instructions'/listing['file']
    assert sha(path)==listing['sha256']
    assert path.read_bytes()==(OUT/'reproduced/instructions'/listing['file']).read_bytes()
    rows=path.read_text().splitlines();assert len(rows)==listing['rows']
    for row in rows:
        address,raw,assembly=row.split(' ',2);address=int(address,16);raw=bytes.fromhex(raw)
        instruction=code.at(address)
        assert instruction.bytes==image.read(address,len(raw))==raw
        assert (instruction.mnemonic+' '+instruction.op_str).strip()==assembly.strip()
    assert r['prepared']and r['whole_guest_text_unchanged']
    assert r['instruction_patches']==r['new_supplied_gameplay_returns']==[]
    assert r['variant']==json.loads(json.dumps(variants()['7']))and r['source_player']=='ec700000'
    cache=Cache(ROOT/'assets/disc/maps/lockout.map');index=cache.u(16)
    delivery=r['tag_index_delivery'];header=bytes(cache.data[index:index+32]).hex()
    assert delivery['file_offset']==f'{index:08x}'and delivery['raw32_hex']==header
    assert delivery['actual_mmap_succeeded']and delivery['actual_errno']==0 and delivery['capacity']==4096
    assert delivery['inherited_header_hex']!=header
    offset=delivery['inherited_header_matching_file_offset'];assert offset>=0 and offset!=index
    assert bytes(cache.data[offset:offset+32]).hex()==delivery['inherited_header_hex']
    post=r['tag_postprocess'];assert post['entry']=='0011de10'and post['complete_original_return']
    assert post['stop']is None and post['services']==[]and post['tag_index_writes']==[]
    assert post['file_header_hex']==post['actual_header_before_hex']==post['actual_header_after_hex']==header
    assert post['cache_tag_count']==len(cache.tags)==4512 and post['original_iterator_index']==0
    assert 0<post['observed_instructions']<post['instruction_budget']
    relevant=[t for t in cache.tags.values()if t['group']in('bipd','crea','coll','phmo','vehi')]
    assert len(post['before'])==len(post['after'])==len(relevant)==158
    changed=0
    for before,after,tag in zip(post['before'],post['after'],relevant):
        assert before['tag']==after['tag']==tag
        assert before['raw_hex']==bytes(cache.data[tag['offset']:tag['offset']+tag['size']]).hex()
        assert len(after['raw_hex'])==len(before['raw_hex'])
        changed+=before['raw_hex']!=after['raw_hex']
    assert changed==69
    sound=r['sound_initializer'];assert sound['completion']=='stopped_original_instruction'
    assert sound['stop']['instruction']=='800000b6'and sound['stop']['return_site']=='0021a7a3'
    assert sound['stop']['arguments']==[0x82a47000,0x300000,4]and sound['available']==0
    assert sound['independent_later_reset_not_a_resumption']and sound['services']==[]
    device=r['independent_device_constructor'];assert device['completion']=='stopped_original_instruction'
    assert device['arguments']==[0x451220]and device['original_argument_producer']=='00126c8b'
    assert device['original_argument_hex']==image.read(0x451220,12).hex()
    assert device['stop']['instruction']=='00383dd7'and device['stop']['return_site']=='0022062b'
    assert device['stop']['arguments']==[0,int(device['after']['mixer'],16)+0x2ab0,0]
    assert device['independent_later_reset_not_a_resumption']and device['services']==[]
    assert set(bytes.fromhex(device['after']['mixer2ad8_hex']))=={0}
    expected_preparation=[('original_sound_class_storage_initializer',0x223960,43,0),
        ('original_object_looping_sound_storage_initializer',0x18a200,46,0),
        ('original_sound_class_map_reset',0x2239b0,43,8),
        ('original_object_looping_sound_map_reset',0x18a2a0,46,8)]
    assert len(r['new_preparation'])==4
    for stage,(name,entry,row,field)in zip(r['new_preparation'],expected_preparation):
        assert stage['name']==name and stage['entry']==f'{entry:08x}'
        assert int.from_bytes(image.read(0x447428+row*36+field,4),'little')==entry
        assert stage['completion']=='complete_return'and stage['services']==[]and stage['invalid']is None
    # Read placement/palette offsets and strides from the original type table.
    authored=Counter()
    for kind in(10,11):
        descriptor=int.from_bytes(image.read(0x46fad0+kind*4,4),'little')
        block,palette,stride=(int.from_bytes(image.read(descriptor+off,2),'little',signed=True)for off in(10,12,14))
        assert block>=0 and palette>=0 and stride>0
        palette_rows=cache.block(cache.scenario+palette,0x28)
        for row in cache.block(cache.scenario+block,stride):
            authored[f'{cache.u(palette_rows[cache.h(row)]+4):08x}']+=1
    assert sum(authored.values())==19
    assert r['requested_cycles']==len(r['cycles'])==2
    assert len(r['storage_release_records'])==23
    assert all('after'in row and row['writes']for row in r['storage_release_records'])
    rng_events=0
    assert code.at(0x16f5d4).op_str=='ecx, ecx, 0x19660d'
    assert code.at(0x16f5da).op_str=='ecx, 0x3c6ef35f'
    for i,cycle in enumerate(r['cycles']):
        reset=cycle['reset'];assert cycle['index']==i and reset['completion']=='complete_original_return'
        assert reset['guarded']and reset['entry']=='0015b460'and reset['stop']is None
        assert cycle['storage_releases']==(2 if i==0 else 21)
        assert 0<cycle['observed_instructions']<cycle['instruction_budget']
        assert reset['before']['variant_hex']==reset['after']['variant_hex']==r['variant']['raw_hex']
        assert reset['before']['world']['object_count']==(2 if i==0 else 21)
        for key,value in(('object_count',19),('object_high_water',19),('component_count',15),('physics_counter',15)):
            assert reset['after']['world'][key]==value
        assert reset['after']['sound_available']==0
        players=reset['after']['player']['records'];assert len(players)==1
        player=players[0];assert player['datum']==r['source_player']
        assert player['current']==player['previous']=='ffffffff'
        raw=bytes.fromhex(player['raw_hex']);assert raw[0x174:0x178]==b'\xff'*4
        assert int.from_bytes(raw[0x218:0x21a],'little')==i+1
        objects=cycle['constructed_world_after']['objects']
        assert Counter(obj['definition']for obj in objects)==authored
        assert sum(obj['component']!='ffffffff'for obj in objects)==15
        assert sum(len(obj['bodies'])for obj in objects)==15
        h=bytes.fromhex(cycle['audio_after']['looping_header_hex'])
        assert int.from_bytes(h[0x20:0x24],'little')==1024
        assert int.from_bytes(h[0x24:0x28],'little')==24
        assert int.from_bytes(h[0x38:0x3c],'little')==int.from_bytes(h[0x3c:0x40],'little')==6
        entries=[c['entry']for c in reset['calls']]
        for entry in('0015b460','0017ef90','00127b50','000d6b60','000c1870','000d6c40'):
            assert entries.count(entry)==1,entry
        assert entries.count('000bd1f0')==cycle['storage_releases']
        assert len(reset['services'])==14
        for event in reset['services']:
            assert set(event)=={'rng_write_instruction','before','after','tick','phase'}
            assert event['rng_write_instruction']=='0016f5e0'
            assert int(event['after'],16)==(int(event['before'],16)*1664525+1013904223)&0xffffffff
            rng_events+=1
    paths=set(data['source_dependencies'])|set(PINS)
    paths.update((str(spec['path']),spec['tsv'],'assets/disc/maps/lockout.map','assets/disc/maps/mainmenu.map',
        'assets/disc/maps/shared.map','tools/verify_round_reset_scenario.py','docs/MP_ROUND_RESET_SCENARIO_RESEARCH.md',
        'docs/GAME_MODE_PROGRESS.md','analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/generator.exe'))
    paths.update(str(p.relative_to(ROOT))for p in(report_path,OUT/'reproduced/report.json',path,
        OUT/'reproduced/instructions'/listing['file']))
    metrics=dict(complete_original_full_resets=2,original_object_deletions=23,
        rebuilt_objects_per_reset=19,physics_components_per_reset=15,looping_records_per_reset=6,
        relevant_tags=158,postprocessed_tags_changed=changed,literal_instruction_rows=len(rows),
        original_rng_stores=rng_events,supplied_reset_gameplay_returns=0,
        complete_audio_startups=0,normal_matches_proven=0)
    result=dict(schema=1,metrics=metrics,artifact_pins={str(p):sha(ROOT/p)for p in sorted(paths)})
    output=OUT/'verification.json';output.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(verification=str(output),sha256=sha(output),pins=len(result['artifact_pins']),metrics=metrics)))


if __name__=='__main__':main()

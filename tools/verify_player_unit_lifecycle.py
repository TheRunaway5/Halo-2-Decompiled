#!/usr/bin/env python3
"""Verify original ownership/deletion evidence, native records and exact repeats."""
import csv
import hashlib
import json
from pathlib import Path
import re

from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import Xbe,_Code,literal,complete_tokens,references
from oracle_player_unit import OWNERS,NAMES
from oracle_mp_spawn_live_body_world import CONTEXT_OUT

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/player-unit'
OWNED=ROOT/'analysis/multiplayer/game-variants/owned-object-deletion'
FIXTURE=ROOT/'tests/fixtures/player_unit_original.json'


def sha(path):
    path=Path(path);before=path.stat();value=hashlib.sha256(path.read_bytes()).hexdigest();after=path.stat()
    assert (before.st_ino,before.st_size,before.st_mtime_ns)==(after.st_ino,after.st_size,after.st_mtime_ns),path
    return value


def expanded(data,value):
    if isinstance(value,dict):
        if set(value)=={'raw_blob_sha256'}:return data['raw_blobs'][value['raw_blob_sha256']]
        return {key:expanded(data,item)for key,item in value.items()}
    if isinstance(value,list):return [expanded(data,item)for item in value]
    return value


def main():
    for process in Path('/proc').iterdir():
        if not process.name.isdigit():continue
        try:
            command=(process/'cmdline').read_bytes().split(b'\0')
            executable=Path(command[0].decode()).name;cwd=(process/'cwd').resolve()
        except (OSError,IndexError,UnicodeError):continue
        assert not(cwd==ROOT/'build'and executable in('ninja','make','gmake','cmake')),command
    report=json.loads((OUT/'report.json').read_text())
    fixture=json.loads(FIXTURE.read_text());owned=json.loads((OWNED/'report.json').read_text())
    assert owned['source_sha256']==SPECS['tu11']['pin']
    assert owned['cache_sha256']==sha(ROOT/'assets/disc/maps/lockout.map')
    assert owned['generator_sha256']==sha(CONTEXT_OUT/'generator.exe')
    for directory in(OUT,OWNED):
        assert (directory/'report.json').read_bytes()==(directory/'reproduced/report.json').read_bytes()
    assert FIXTURE.read_bytes()==(OUT/'reproduced/fixture.json').read_bytes()
    assert sha(FIXTURE)==report['fixture_sha256']
    for data in(report,fixture,owned):
        for key,raw in data['raw_blobs'].items():assert hashlib.sha256(bytes.fromhex(raw)).hexdigest()==key
    for r,tool in((report,'oracle_player_unit.py'),(owned,'oracle_owned_object_deletion.py')):
        assert sha(ROOT/'tools'/tool)==r['oracle_sha256']
        for path,pin in(r['source_dependencies']|r.get('prior_pins',{})).items():assert sha(ROOT/path)==pin,path
        assert r['instruction_patches']==[]
    images={name:Xbe(spec['path'])for name,spec in SPECS.items()}
    codes={name:_Code(image,'.text')for name,image in images.items()}
    starts={name:{int(row['address'],16)for row in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
        for name,spec in SPECS.items()}
    for spec in SPECS.values():assert sha(spec['path'])==spec['pin']
    listings=[(OUT,r,r['file'].split('-')[0])for r in report['instruction_listings']]
    listings.append((OWNED,owned['executed_instruction_listing'],'tu11'))
    literal_rows=0
    for directory,record,name in listings:
        path=directory/'instructions'/record['file'];assert sha(path)==record['sha256']
        assert path.read_bytes()==(directory/'reproduced/instructions'/record['file']).read_bytes()
        lines=path.read_text().splitlines();assert len(lines)==record['rows']
        for line in lines:
            address,raw,assembly=line.split(' ',2);address=int(address,16);raw=bytes.fromhex(raw)
            assert images[name].read(address,len(raw))==raw
            instruction=codes[name].at(address)
            assert instruction.bytes==raw and(instruction.mnemonic+' '+instruction.op_str).strip()==assembly.strip()
        literal_rows+=len(lines)
    for pair in report['complete_source_pairs']:
        name,key=pair['earlier'],pair['owner']
        a,b=OWNERS[name][key],OWNERS['u5'][key]
        left=literal(codes[name],a,starts[name]|{a})[1]
        right=literal(codes['u5'],b,starts['u5']|{b})[1]
        equal=complete_tokens(left,images[name])==complete_tokens(right,images['u5'])
        assert equal==pair['complete_shape_equal']
        assert equal or(name=='retail'and key=='bind')
        if equal:assert references(left,right,images[name],images['u5'])==expanded(report,pair['relocated_operands'])
    for name,b in OWNERS.items():
        calls=set()
        for key in('bind','release','status'):
            rr=literal(codes[name],b[key],starts[name]|{b[key]})[1]
            calls.update(i.operands[0].imm for i in rr if i.mnemonic=='call'and i.operands[0].type==2)
        assert calls==set(b['services'])|{b['bind'],b['status']}
    assert report['summary']==dict(records=339,partial_records=24)
    records=[r for v in fixture['versions'].values()for r in v['records']]
    assert len(records)==339 and sum(not r['complete']for r in records)==24
    for name,version in fixture['versions'].items():
        assert version['whole_guest_text_unchanged']
        assert {r['omit']for r in version['records']if not r['complete']}==set(NAMES)
    assert owned['summary']==dict(joined_players=6,constructed_owned_bipeds=12,
        completed_object_deletions=24,reconstructed_flows=6)
    assert owned['supplied_teardown_services']==[]
    assert {(c['activation'],c['slot'])for c in owned['cases']}=={
        (activation,slot)for activation in('inactive','motion')for slot in(0,3,15)}
    for case in owned['cases']:
        assert case['whole_guest_text_unchanged']and len(case['creations'])==2
        assert len({c['unit']for c in case['creations']})==2
        assert len(case['storage_release_records'])==4
        for creation in case['creations']:
            assert creation['world']['pool_count']==2 and creation['world']['player_count']==1
            assert len(creation['world']['objects'])==2
        for stage in case['stages']:
            assert stage['completion']=='complete_original_return'and stage['stop']is None
            assert stage['services']==[]
            player=next(r for r in stage['player_after']['records']if r['index']==case['slot'])
            assert player['datum']==case['player']and player['current']==player['previous']=='ffffffff'
            raw=bytes.fromhex(expanded(owned,player['raw_hex']))
            assert raw[0x174:0x178]==b'\xff'*4
            assert int.from_bytes(raw[0x218:0x21a],'little')==stage['cycle']+1
            if stage['name']=='owned_unit_deactivate':
                entries=[r['entry']for r in stage['calls']]
                assert entries.count('00154440')==2 and entries.count('0014e240')==1
                assert entries.count('00153a90')==2 and entries.count('00189490')==2
            else:
                after=stage['after']
                assert after['object_count']==after['object_high_water']==after['component_count']==after['physics_counter']==0
        if case['activation']=='motion':
            phases=('original_current_cluster_masks_and_callbacks','original_object_prephysics',
                'original_world_physics','original_object_postphysics','original_object_newflags')
            for phase in phases:
                stages=[s for s in case['player_stages']if s['name']==phase]
                assert len(stages)==2 and all(s['completion']=='complete_original_return'and s['services']==[]for s in stages)
    ctest=(OUT/'ctest.log').read_text()
    assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 1',ctest)
    assert re.search(r'original_player_unit\s*\.+\s*Passed',ctest)
    assert 'Skipped'not in ctest and 'Failed'not in ctest
    binary=ROOT/'build/player_unit_tests';assert binary.stat().st_mtime_ns<=(OUT/'ctest.log').stat().st_mtime_ns
    test_sources=('src/gameplay/units/source_player_unit.cpp','include/halo2/source_player_unit.hpp','tests/player_unit_tests.cpp')
    assert all((ROOT/p).stat().st_mtime_ns<=binary.stat().st_mtime_ns for p in test_sources)
    sanitizer=OUT/'player_unit_tests_asan';log=(OUT/'sanitizer.log').read_text()
    assert 'PASS 339 complete/partial original player-unit records'in log and '24 absent-service checks'in log
    assert 'AddressSanitizer'not in log and 'runtime error:'not in log
    assert all((ROOT/p).stat().st_mtime_ns<=sanitizer.stat().st_mtime_ns for p in test_sources)
    assert sanitizer.stat().st_mtime_ns<=(OUT/'sanitizer.log').stat().st_mtime_ns
    system_object=ROOT/'build/CMakeFiles/halo2_systems.dir/src/source_player_unit.cpp.o'
    assert system_object.stat().st_mtime_ns<=(OUT/'systems-build.log').stat().st_mtime_ns
    assert all((ROOT/p).stat().st_mtime_ns<=system_object.stat().st_mtime_ns for p in test_sources[:2])
    paths=set(report['source_dependencies'])|set(owned['source_dependencies'])|set(owned['prior_pins'])
    paths.update(spec['path']for spec in SPECS.values());paths.update(spec['tsv']for spec in SPECS.values())
    paths.update(('CMakeLists.txt',*test_sources,'tests/fixtures/player_unit_original.json',
        'tools/verify_player_unit_lifecycle.py','docs/MP_PLAYER_UNIT_LIFECYCLE.md','docs/GAME_MODE_PROGRESS.md',
        'assets/disc/maps/lockout.map','assets/disc/maps/shared.map','build/player_unit_tests',
        'build/CMakeFiles/halo2_systems.dir/src/source_player_unit.cpp.o'))
    paths.add(str((CONTEXT_OUT/'generator.exe').relative_to(ROOT)))
    for directory in(OUT,OWNED):
        paths.update(str(p.relative_to(ROOT))for p in(directory/'report.json',directory/'reproduced/report.json'))
    paths.update(str(p.relative_to(ROOT))for p in(OUT/'reproduced/fixture.json',OUT/'build.log',OUT/'systems-build.log',
        OUT/'ctest.log',sanitizer,OUT/'sanitizer.log'))
    for directory,record,_ in listings:
        paths.update(str(p.relative_to(ROOT))for p in(directory/'instructions'/record['file'],directory/'reproduced/instructions'/record['file']))
    pins={str(p):sha(ROOT/p)for p in sorted(paths)}
    metrics=dict(records=len(records),partial_records=24,absent_service_checks=24,
        bytes_compared=sum(len(fixture['raw_blobs'][r['raw_hex']['raw_blob_sha256']])//2 for record in records for r in record['after']),
        ordered_writes=sum(len(r['writes'])for r in records),service_calls=sum(len(r['events'])for r in records),
        literal_instruction_rows=literal_rows,
        complete_equal_source_pairs=sum(p['complete_shape_equal']for p in report['complete_source_pairs']),
        explicit_retail_assignment_difference=True,**owned['summary'])
    result=dict(schema=1,metrics=metrics,artifact_pins=pins)
    output=OUT/'verification.json';output.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(verification=str(output),sha256=sha(output),pins=len(pins),metrics=metrics)))


if __name__=='__main__':main()

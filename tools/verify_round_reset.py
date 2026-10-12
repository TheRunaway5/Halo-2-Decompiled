#!/usr/bin/env python3
"""Verify shared reset CPU contracts and retained original live-world stops."""
import csv
import hashlib
import json
from pathlib import Path
import re

from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import Xbe,_Code,literal,complete_tokens,references
from oracle_round_reset import SLOTS,NAMES
from oracle_round_lifecycle import variants
from oracle_mp_spawn_live_body_world import CONTEXT_OUT

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/round-reset-native'
OWNED=ROOT/'analysis/multiplayer/game-variants/owned-round-reset'
FIXTURE=ROOT/'tests/fixtures/round_reset_original.json'


def sha(path):
    path=Path(path);before=path.stat();value=hashlib.sha256(path.read_bytes()).hexdigest();after=path.stat()
    assert (before.st_ino,before.st_size,before.st_mtime_ns)==(after.st_ino,after.st_size,after.st_mtime_ns),path
    return value


def expanded(data,value):
    if isinstance(value,dict):
        if set(value)=={'raw_blob_sha256'}:return data['raw_blobs'][value['raw_blob_sha256']]
        return {k:expanded(data,v)for k,v in value.items()}
    if isinstance(value,list):return [expanded(data,v)for v in value]
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
    for data,tool in((report,'oracle_round_reset.py'),(owned,'oracle_owned_round_reset.py')):
        assert sha(ROOT/'tools'/tool)==data['oracle_sha256']
        for path,pin in(data['source_dependencies']|data.get('prior_pins',{})).items():assert sha(ROOT/path)==pin,path
        assert data['instruction_patches']==[]
    assert owned['new_supplied_gameplay_returns']==[]
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
    owners={}
    for name,b in SLOTS.items():
        rows=literal(codes[name],b['reset'],starts[name]|{b['reset']})[1]
        calls=[i.operands[0].imm for i in rows if i.mnemonic=='call'and i.operands[0].type==2]
        owners[name]=dict(reset=b['reset'],unit_iterator=calls[0],object_iterator=calls[4])
    assert len(report['complete_source_pairs'])==6
    for pair in report['complete_source_pairs']:
        name,key=pair['earlier'],pair['owner'];a,b=owners[name][key],owners['u5'][key]
        left=literal(codes[name],a,starts[name]|{a})[1]
        right=literal(codes['u5'],b,starts['u5']|{b})[1]
        assert pair['complete_shape_equal']
        assert complete_tokens(left,images[name])==complete_tokens(right,images['u5'])
        assert references(left,right,images[name],images['u5'])==expanded(report,pair['relocated_operands'])
    assert report['summary']==dict(records=228,partial_records=33)
    records=[r for v in fixture['versions'].values()for r in v['records']]
    assert len(records)==228 and sum(not r['complete']for r in records)==33
    for name,version in fixture['versions'].items():
        assert version['whole_guest_text_unchanged']and len(version['records'])==76
        assert version['bindings']==report['bindings'][name]
        assert {r['omit']for r in version['records']if not r['complete']}==set(NAMES)
        assert sum(r['relocate_mask']for r in version['records'])==1
    assert len(owned['cases'])==2
    assert {c['sound_prefix_included']for c in owned['cases']}=={False,True}
    authored=json.loads(json.dumps(variants()['7']))
    for raw in owned['cases']:
        case=expanded(owned,raw);reset=case['reset'];sound=case['sound_prefix_included']
        assert case['variant']==authored and case['whole_guest_text_unchanged']
        assert case['source_player']=='ec700000'
        assert reset['entry']=='0015b460'and reset['guarded']
        assert reset['completion']=='stopped_original_instruction'and reset['services']==[]
        assert reset['before']['variant_hex']==reset['after']['variant_hex']==authored['raw_hex']
        assert int.from_bytes(bytes.fromhex(authored['raw_hex'])[0x48:0x4c],'little')&8
        assert reset['before']['world']['object_count']==2
        assert reset['before']['world']['component_count']==reset['before']['world']['physics_counter']==1
        assert reset['after']['world']['component_count']==reset['after']['world']['physics_counter']==0
        assert reset['after']['world']['object_count']==int(sound)
        assert reset['after']['mask4201_hex']=='ff'*0x4201
        player=reset['after']['player']['records'];assert len(player)==1
        assert player[0]['datum']==case['source_player']and player[0]['current']==player[0]['previous']=='ffffffff'
        assert bytes.fromhex(player[0]['raw_hex'])[0x174:0x178]==b'\xff'*4
        assert len(case['storage_release_records'])==2
        assert all('after'in r and r['writes']for r in case['storage_release_records'])
        assert reset['source_storage_after']
        entries=[r['entry']for r in reset['calls']]
        for entry in('0015b460','0017ef90','00127b50','00154440','0014e240','00153a90','00189490'):
            assert entry in entries,entry
        assert entries.count('000bd1f0')==2
        assert '000c1870'not in entries and '000d6c40'not in entries
        assert len(case['raw_vm'])==(4 if sound else 1)
        for vm in case['raw_vm']:
            assert vm['actual_mmap_succeeded']and vm['actual_errno']==0
            assert vm['arguments']==[0,vm['original_requested_bytes'],vm['arguments'][2],4]
            assert vm['actual_mmap_capacity']==(vm['original_requested_bytes']+4095)&~4095
        if sound:
            prefix=case['sound_initializer'];stop=prefix['stop']
            assert prefix['completion']=='stopped_original_instruction'and prefix['available']==0
            assert prefix['independent_later_reset_not_a_resumption']and prefix['services']==[]
            assert stop['instruction']=='800000b6'and stop['return_site']=='0021a7a3'
            assert stop['arguments']==[0x82a47000,0x300000,4]
            assert reset['before']['sound_available']==reset['after']['sound_available']==0
            assert reset['stop']['instruction']=='0016d2c3'and reset['stop']['address']=='00000034'
            assert '000d6b60'in entries and '0018a9c0'in entries
        else:
            assert case['sound_initializer']is None and reset['before']['sound_system']=='00000000'
            assert reset['stop']['instruction']=='00127b55'and reset['stop']['address']=='00000078'
            assert '000d6b60'not in entries
    image=images['tu11']
    for index,row in expanded(owned,owned['original_storage_rows']).items():
        address=0x447428+int(index)*36;raw=image.read(address,36)
        assert row['table_address']==f'{address:08x}'and row['record_hex']==raw.hex()
        assert row['initialize']==f'{int.from_bytes(raw[:4],"little"):08x}'
        assert row['map_reset']==f'{int.from_bytes(raw[8:12],"little"):08x}'
    next_owner=expanded(owned,owned['unresolved_object_looping_sound_pool'])
    assert next_owner['table_index']==46 and next_owner['global_slot']=='004f485c'
    assert next_owner['initializer']==owned['original_storage_rows']['46']['initialize']=='0018a200'
    assert next_owner['map_reset']==owned['original_storage_rows']['46']['map_reset']=='0018a2a0'
    assert image.read(int(next_owner['source_name_address'],16),22).hex()==next_owner['source_name_hex']
    assert bytes.fromhex(next_owner['source_name_hex'])==b'object looping sounds\0'
    next_rows={key:literal(codes['tu11'],int(next_owner[key],16),
        starts['tu11']|{int(next_owner[key],16)})[1]for key in('allocate','initializer','map_reset')}
    assert any(i.mnemonic=='mov'and i.op_str=='edx, dword ptr [0x4f485c]'for i in next_rows['allocate'])
    assert any(i.mnemonic=='call'and i.op_str=='0x16d2c0'for i in next_rows['allocate'])
    init_ops={(i.mnemonic,i.op_str)for i in next_rows['initializer']}
    assert {('push','0x60cc'),('push','0x45ad5c'),('mov','edx, 0x18'),('mov','ebx, 0x400'),
        ('mov','dword ptr [0x4f485c], esi')}<=init_ops
    assert any(i.mnemonic=='call'and i.op_str=='0x2230e0'for i in next_rows['map_reset'])
    ctest=(OUT/'ctest.log').read_text();assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 1',ctest)
    assert re.search(r'original_round_reset\s*\.+\s*Passed',ctest)
    assert 'Skipped'not in ctest and 'Failed'not in ctest
    test_sources=('src/source_round_reset.cpp','include/halo2/source_round_reset.hpp','tests/round_reset_tests.cpp')
    binary=ROOT/'build/round_reset_tests';assert binary.stat().st_mtime_ns<=(OUT/'ctest.log').stat().st_mtime_ns
    assert all((ROOT/p).stat().st_mtime_ns<=binary.stat().st_mtime_ns for p in test_sources)
    sanitizer=OUT/'round_reset_tests_asan';log=(OUT/'sanitizer.log').read_text()
    assert 'PASS 228 complete/partial original reset records'in log and '33 absent-service checks'in log
    assert 'AddressSanitizer'not in log and 'runtime error:'not in log
    assert all((ROOT/p).stat().st_mtime_ns<=sanitizer.stat().st_mtime_ns for p in test_sources)
    assert sanitizer.stat().st_mtime_ns<=(OUT/'sanitizer.log').stat().st_mtime_ns
    system_object=ROOT/'build/CMakeFiles/halo2_systems.dir/src/source_round_reset.cpp.o'
    assert system_object.stat().st_mtime_ns<=(OUT/'build.log').stat().st_mtime_ns
    assert all((ROOT/p).stat().st_mtime_ns<=system_object.stat().st_mtime_ns for p in test_sources[:2])
    paths=set(report['source_dependencies'])|set(owned['source_dependencies'])|set(owned['prior_pins'])
    paths.update(spec['path']for spec in SPECS.values());paths.update(spec['tsv']for spec in SPECS.values())
    paths.update(('CMakeLists.txt',*test_sources,'tests/fixtures/round_reset_original.json',
        'tools/verify_round_reset.py','docs/MP_ROUND_RESET_NATIVE.md','docs/GAME_MODE_PROGRESS.md',
        'assets/disc/maps/mainmenu.map','assets/disc/maps/lockout.map','assets/disc/maps/shared.map',
        'build/round_reset_tests','build/CMakeFiles/halo2_systems.dir/src/source_round_reset.cpp.o'))
    paths.add(str((CONTEXT_OUT/'generator.exe').relative_to(ROOT)))
    for directory in(OUT,OWNED):
        paths.update(str(p.relative_to(ROOT))for p in(directory/'report.json',directory/'reproduced/report.json'))
    paths.update(str(p.relative_to(ROOT))for p in(OUT/'reproduced/fixture.json',OUT/'build.log',OUT/'ctest.log',
        OUT/'sanitizer-build.log',sanitizer,OUT/'sanitizer.log'))
    for directory,record,_ in listings:
        paths.update(str(p.relative_to(ROOT))for p in(directory/'instructions'/record['file'],directory/'reproduced/instructions'/record['file']))
    pins={str(p):sha(ROOT/p)for p in sorted(paths)}
    metrics=dict(records=len(records),partial_records=33,absent_service_checks=33,
        bytes_compared=sum(len(fixture['raw_blobs'][r['raw_hex']['raw_blob_sha256']])//2 for record in records for r in record['after']),
        ordered_writes=sum(len(r['writes'])for r in records),service_calls=sum(len(r['events'])for r in records),
        literal_instruction_rows=literal_rows,complete_equal_source_pairs=6,
        source_joined_reset_probes=2,completed_object_deletions=4,complete_live_world_resets=0,
        new_supplied_reset_gameplay_returns=0)
    result=dict(schema=1,metrics=metrics,artifact_pins=pins)
    output=OUT/'verification.json';output.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(verification=str(output),sha256=sha(output),pins=len(pins),metrics=metrics)))


if __name__=='__main__':main()

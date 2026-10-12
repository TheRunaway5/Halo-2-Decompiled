#!/usr/bin/env python3
"""Verify retained original teardown/native storage evidence and artifact pins."""
import csv
import hashlib
import json
from pathlib import Path
import re
import sys

from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import Xbe, _Code, literal, complete_tokens, references

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/constructed-object-deletion'
FIXTURE=ROOT/'tests/fixtures/object_storage_release_original.json'


def sha(path):
    path=Path(path);before=path.stat();h=hashlib.sha256()
    with path.open('rb')as stream:
        for block in iter(lambda:stream.read(1<<20),b''):h.update(block)
    after=path.stat()
    assert (before.st_ino,before.st_size,before.st_mtime_ns)==(after.st_ino,after.st_size,after.st_mtime_ns),path
    return h.hexdigest()


def main():
    # Archives/executables are read only after authoritative process inspection.
    for process in Path('/proc').iterdir():
        if not process.name.isdigit():continue
        try:
            command=(process/'cmdline').read_bytes().split(b'\0')
            executable=Path(command[0].decode()).name
            cwd=(process/'cwd').resolve()
        except (OSError,IndexError,UnicodeError):continue
        assert not(cwd==ROOT/'build'and executable in('ninja','make','gmake','cmake')),command
    report=json.loads((OUT/'report.json').read_text())
    fixture=json.loads(FIXTURE.read_text())
    assert (OUT/'report.json').read_bytes()==(OUT/'reproduced/report.json').read_bytes()
    assert FIXTURE.read_bytes()==(OUT/'reproduced/fixture.json').read_bytes()
    assert sha(FIXTURE)==report['fixture_sha256']==sha(OUT/'header-only-checked2.json')
    assert sha(ROOT/'tools/oracle_constructed_object_deletion.py')==report['oracle_sha256']
    for data in(report,fixture):
        for key,raw in data['raw_blobs'].items():assert hashlib.sha256(bytes.fromhex(raw)).hexdigest()==key
    for path,pin in(report['prior_pins']|report['source_dependencies']).items():assert sha(ROOT/path)==pin,path
    images={name:Xbe(spec['path'])for name,spec in SPECS.items()}
    codes={name:_Code(image,'.text')for name,image in images.items()}
    starts={name:{int(row['address'],16)for row in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
        for name,spec in SPECS.items()}
    for name,spec in SPECS.items():assert sha(spec['path'])==spec['pin']
    literal_rows=0
    for record in report['instruction_listings']:
        file=OUT/'instructions'/record['file'];assert sha(file)==record['sha256']
        assert file.read_bytes()==(OUT/'reproduced/instructions'/record['file']).read_bytes()
        lines=file.read_text().splitlines();assert len(lines)==record['rows']
        name=record['file'].split('-')[0]
        for line in lines:
            address,raw,assembly=line.split(' ',2);address=int(address,16);raw=bytes.fromhex(raw)
            assert images[name].read(address,len(raw))==raw
            instruction=codes[name].at(address)
            assert instruction.bytes==raw and(instruction.mnemonic+' '+instruction.op_str).strip()==assembly.strip()
        literal_rows+=len(lines)
    for pair in report['complete_source_pairs']:
        name=pair['earlier'];a=int(pair['earlier_entry'],16);b=int(pair['later_entry'],16)
        left=literal(codes[name],a,starts[name]|{a})[1]
        right=literal(codes['u5'],b,starts['u5']|{b})[1]
        assert complete_tokens(left,images[name])==complete_tokens(right,images['u5'])
        def expand(value):
            if isinstance(value,dict):
                if set(value)=={'raw_blob_sha256'}:return report['raw_blobs'][value['raw_blob_sha256']]
                return {key:expand(item)for key,item in value.items()}
            if isinstance(value,list):return [expand(item)for item in value]
            return value
        assert references(left,right,images[name],images['u5'])==expand(pair['relocated_operands'])
    assert report['summary']==dict(constructed_bipeds=12,completed_deletions=10,storage_api_records=66)
    assert report['instruction_patches']==report['supplied_teardown_services']==[]
    for case in report['cases']:
        assert case['whole_guest_text_unchanged']
        if case['contexts']=='storage':
            for stage in case['stages']:
                assert stage['completion']=='complete_original_return'
                assert not any('service'in event for event in stage['services'])
            after=case['stages'][-1]['after']
            assert after['object_count']==after['component_count']==after['physics_counter']==after['object_high_water']==0
    ctest=(OUT/'ctest.log').read_text()
    assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 4',ctest)
    assert 'Skipped'not in ctest and 'Failed'not in ctest
    for name in('original_object_storage_release','original_object_construction','original_physics_body_disposal','original_round_lifecycle'):
        assert re.search(re.escape(name)+r'\s*\.+\s*Passed',ctest),name
    binaries=['object_storage_release_tests','object_construction_tests','physics_body_disposal_tests','round_lifecycle_tests']
    assert all((ROOT/'build'/name).stat().st_mtime_ns<=(OUT/'ctest.log').stat().st_mtime_ns for name in binaries)
    sanitizer=OUT/'sanitizers/object_storage_release_tests'
    sanitizer_log=(OUT/'sanitizers/run.log').read_text()
    assert 'PASS 66 complete original object-storage records and ordered writes'in sanitizer_log
    assert 'AddressSanitizer'not in sanitizer_log and 'runtime error:'not in sanitizer_log
    assert sanitizer.stat().st_mtime_ns<=(OUT/'sanitizers/run.log').stat().st_mtime_ns
    assert all((ROOT/path).stat().st_mtime_ns<=sanitizer.stat().st_mtime_ns for path in
        ('src/object_storage_release.cpp','tests/object_storage_release_tests.cpp','include/halo2/object_storage_release.hpp'))
    production=(OUT/'production.log').read_text()
    assert 'Multiplayer ticks=90 players=4 finished=0'in production
    assert (ROOT/'build/halo2_native').stat().st_mtime_ns<=(OUT/'production.log').stat().st_mtime_ns
    paths=set(report['prior_pins'])|set(report['source_dependencies'])
    paths.update(spec['path']for spec in SPECS.values())
    paths.update(spec['tsv']for spec in SPECS.values())
    paths.update(('CMakeLists.txt','include/halo2/object_storage_release.hpp','src/object_storage_release.cpp',
        'tests/object_storage_release_tests.cpp','tests/fixtures/object_storage_release_original.json',
        'tools/verify_constructed_object_deletion.py','docs/MP_CONSTRUCTED_OBJECT_DELETION.md',
        'docs/GAME_MODE_PROGRESS.md','docs/MP_ROUND_RESET_STORAGE_RESEARCH.md',
        'assets/disc/maps/lockout.map','assets/disc/maps/shared.map',
        'build/halo2_native','build/libhalo2_systems.a',*(f'build/{name}'for name in binaries)))
    paths.update(str(path.relative_to(ROOT))for path in(
        OUT/'report.json',OUT/'reproduced/report.json',OUT/'reproduced/fixture.json',OUT/'ctest.log',
        OUT/'production.log',OUT/'header-only-checked2.json',sanitizer,OUT/'sanitizers/run.log'))
    paths.update(str(path.relative_to(ROOT))for directory in(OUT/'instructions',OUT/'reproduced/instructions')for path in directory.glob('*.asm'))
    pins={str(path):sha(ROOT/path)for path in sorted(paths)}
    records=[record for version in fixture['versions'].values()for record in version['records']]
    metrics=dict(records=len(records),header_only=sum(record.get('header_only',False)for record in records),
        bytes_compared=sum(len(fixture['raw_blobs'][region['raw_hex']['raw_blob_sha256']])//2 for record in records for region in record['after']),
        ordered_writes=sum(len(record['writes'])for record in records),complete_source_pairs=len(report['complete_source_pairs']),
        literal_instruction_rows=literal_rows)
    result=dict(schema=1,pins=pins,metrics=metrics,complete_original_report_and_fixture_repeat=True,
        ctest_passes=4,sanitizers='Both new translation units compiled with ASan/UBSan;66 cases pass, leaks enabled',
        production='Four-player90-tick Slayer startup only; new storage release is unbound to normal teardown',
        boundary='Observed source listings include stopped entries/platform hooks. Complete native teardown, full authored map reset, owned players/active contact and complete normal variant matches remain unverified.')
    output=OUT/'verification.json';output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(verification=str(output),sha256=sha(output),pins=len(pins),metrics=metrics)))


if __name__=='__main__':main()

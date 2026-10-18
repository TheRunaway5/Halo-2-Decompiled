#!/usr/bin/env python3
"""Audit literal sound-chunk sector reads across the full owned weapon map scope.

This checks source metadata and the explicit native file namespace. It does not
select sounds, establish which chunks a weapon activates, construct runtime
cache records, or certify native/Xbox scheduling, decoding or playback.
"""
import argparse
import collections
import datetime
import json
import os
from pathlib import Path
import shutil
from inspect_multiplayer import Cache
from original_xbox_pose_case import ROOT,BASE,sha


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);output=args.output.absolute();output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch),tool_sha256=sha(Path(__file__))),indent=2)+'\n')
    scope_path=BASE/'authored-scope.json';scope=json.loads(scope_path.read_text());maps=scope['maps']
    assert len(maps)==38
    shared=ROOT/'assets/disc/maps';source_files={};results=[];unique=set();issues=[];totals=collections.Counter()
    for item in maps:
        path=Path(item['path']);assert sha(path)==item['sha256'],path
        cache=Cache(path)
        try:
            gestalts=[tag for tag in cache.tags.values()if tag['group']=='ugh!'and tag['offset']is not None]
            assert len(gestalts)==1
            gestalt=gestalts[0]['offset'];chunks=cache.block(gestalt+0x40,12)
            counts=collections.Counter();locations=collections.Counter()
            for index,at in enumerate(chunks):
                encoded,size_flags=cache.u(at),cache.u(at+4);location=encoded>>30;offset=encoded&0x3fffffff
                size=size_flags&0x3fffffff;rounded=(size+511)&~511
                counts['chunks']+=1;locations[location]+=1
                if not size:
                    counts['zero_size_rows']+=1
                    continue
                source=path if location==0 else shared/('', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[location]
                if source not in source_files:
                    source_files[source]=dict(path=str(source),exists=source.is_file())
                    if source.is_file():source_files[source].update(bytes=source.stat().st_size,sha256=sha(source))
                file=source_files[source];key=(str(source),offset,size,rounded);unique.add(key)
                raw_ok=file['exists']and offset+size<=file['bytes']
                rounded_ok=file['exists']and offset+rounded<=file['bytes']
                counts['nonempty_chunks']+=1;counts['raw_ranges_available']+=raw_ok
                counts['rounded_ranges_available']+=rounded_ok
                counts['sector_unaligned_offsets']+=bool(offset&511)
                counts['requests_above_native_size_limit']+=rounded>128*1024*1024
                if not raw_ok or not rounded_ok:
                    counts['unavailable_raw_ranges']+=not raw_ok;counts['unavailable_rounded_ranges']+=not rounded_ok
                    if len(issues)<256:issues.append(dict(map=str(path),chunk_index=index,chunk_file_offset=at,
                        raw_descriptor_hex=bytes(cache.data[at:at+12]).hex(),source_file=str(source),
                        encoded=encoded,size_flags=size_flags,offset=offset,bytes=size,rounded_bytes=rounded,
                        file_bytes=file.get('bytes'),raw_available=raw_ok,rounded_available=rounded_ok))
            results.append(dict(path=str(path),sha256=item['sha256'],gestalt_datum=gestalts[0]['datum'],
                chunk_table_count=len(chunks),counts=dict(counts),resource_locations=dict(locations)))
            totals.update(counts)
            print(json.dumps(dict(map=path.stem,chunks=len(chunks),zero=counts['zero_size_rows'],
                raw_unavailable=counts['unavailable_raw_ranges'],rounded_unavailable=counts['unavailable_rounded_ranges'])),flush=True)
        finally:cache.data.close();cache.stream.close()
    for path,record in source_files.items():
        if record['exists']:assert sha(path)==record['sha256']
    result=dict(audit_completed=True,timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),scope=__doc__,
        weapon_map_scope_sha256=sha(scope_path),maps=results,map_count=len(results),counts=dict(totals),
        unique_nonempty_resource_ranges=len(unique),source_files=list(source_files.values()),
        all_nonempty_raw_ranges_available=not totals['unavailable_raw_ranges'],
        all_nonempty_rounded_ranges_available=not totals['unavailable_rounded_ranges'],
        issues=issues,issue_examples_bounded=256,scope_rows_count_preserved=1110,weapon_paths_count_preserved=63,
        original_size_and_alignment_owner='TU1.5 sound-cache21F420: lower30 size bits, round upward to512',
        original_request_or_permutation_selection_executed=False,zero_size_runtime_admission_verified=False,
        actual_weapon_sound_dependency_closure_verified=False,full_weapon_parity=False,physical_xbox_verified=False,
        runner_scratch=str(scratch),large_artifacts_retained=[],tool_sha256=sha(Path(__file__)),
        cache_reader_sha256=sha(ROOT/'tools/inspect_multiplayer.py'))
    (output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    for source in (Path(__file__),ROOT/'tools/inspect_multiplayer.py'):shutil.copyfile(source,output/source.name)
    print(json.dumps(dict(audit_completed=True,maps=len(results),counts=dict(totals),unique_ranges=len(unique),
        new_large_reference_inputs=False,runner_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()

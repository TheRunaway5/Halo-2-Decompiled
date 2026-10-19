#!/usr/bin/env python3
"""Audit authored sound permutation counts over the complete owned map scope.

Counts cover resident metadata, including sounds unrelated to weapons. This
does not prove weapon dependency closure, selection, admission or playback.
"""
import argparse
import collections
import json
import os
from pathlib import Path
import shutil
from inspect_multiplayer import Cache
from original_xbox_pose_case import BASE, ROOT, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute();output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch)))+'\n')
    scope=BASE/'authored-scope.json';maps=json.loads(scope.read_text())['maps']
    assert len(maps)==38
    rows=[];issues=[];all_counts=collections.Counter();references=collections.Counter()
    for item in maps:
        path=Path(item['path']);assert sha(path)==item['sha256']
        cache=Cache(path)
        try:
            gestalt=next(tag for tag in cache.tags.values()if tag['group']=='ugh!'and tag['offset']is not None)
            ranges=cache.block(gestalt['offset']+0x20,12)
            counts=collections.Counter(cache.h(at+10)for at in ranges)
            all_counts.update(counts)
            classes=[tag for tag in cache.tags.values()if tag['group']=='sncl'and tag['offset']is not None]
            assert len(classes)==1
            class_rows=cache.block(classes[0]['offset'],92)
            sounds=[tag for tag in cache.tags.values()if tag['group']=='snd!'and tag['offset']is not None]
            referenced=collections.Counter();over32=[];at32=[]
            for sound in sounds:
                at=sound['offset'];start=cache.h(at+8);n=cache.data[at+10]
                cls=int.from_bytes(cache.data[at+2:at+3],'little',signed=True)
                if n>127 or start<0 or start+n>len(ranges) or cls<0 or cls>=len(class_rows):
                    issues.append(dict(map=str(path),sound=sound['datum'],start=start,ranges=n,sound_class=cls))
                    continue
                persistent=not(cache.data[class_rows[cls]+10]&0x20)
                for i in range(n):
                    record=ranges[start+i];count=cache.h(record+10)
                    referenced[count]+=1;references[count]+=1
                    entry=dict(sound=sound['datum'],range_index=i,range_file_offset=record,
                        count=count,persistent_cache=persistent)
                    if count>32:over32.append(entry)
                    if count==32:at32.append(entry)
            rows.append(dict(path=str(path),sha256=item['sha256'],pitch_ranges=len(ranges),
                resident_range_permutation_counts=dict(counts),sound_definitions=len(sounds),
                referenced_range_counts=dict(referenced),referenced_count32_examples=at32[:64],
                referenced_above32_examples=over32[:64],referenced_above32_count=len(over32)))
        finally:
            cache.data.close();cache.stream.close()
    result=dict(audit_completed=True,map_count=len(rows),maps=rows,
        resident_range_permutation_counts=dict(all_counts),referenced_range_permutation_counts=dict(references),
        maximum_resident_count=max(all_counts),maximum_referenced_count=max(references),
        invalid_sound_range_or_class_references=issues,scope_sha256=sha(scope),
        original_permutation_selection_executed=False,weapon_sound_dependency_closure_verified=False,
        full_weapon_parity=False,physical_xbox_verified=False,runner_scratch=str(scratch),
        large_artifacts_retained=[],tool_sha256=sha(Path(__file__)),
        cache_reader_sha256=sha(ROOT/'tools/inspect_multiplayer.py'))
    (output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    shutil.copyfile(Path(__file__),output/Path(__file__).name)
    print(json.dumps(dict(audit_completed=True,maps=len(rows),maximum_count=result['maximum_referenced_count'],
        count32=references[32],above32=sum(v for k,v in references.items()if k>32),
        invalid_references=len(issues),runner_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Read-only comparison of named PC/MCC symbols in selected v13 caches.

Use tools/run_test.py. No cache, identifier profile or admission rule changes.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from mcc_content import decode_cache

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache',action='append',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    profile=ROOT/'src/ui/mcc_pc_frontend_string_ids.inc'
    rows=[]
    for line in profile.read_text().splitlines():
        row=re.fullmatch(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,("(?:[^"\\]|\\.)*")\},',line)
        assert row,line
        rows.append((int(row[1],16),int(row[2],16),json.loads(row[3])))
    assert len(rows)==7669
    results=[]
    for path in args.cache:
        raw=path.read_bytes();data=decode_cache(raw)
        count,strings,size,indices=struct.unpack_from('<4I',data,48)
        assert count<=1000000 and indices+count*4<=len(data) and strings+size<=len(data)
        names=[];by_name=defaultdict(list)
        for index in range(count):
            offset=struct.unpack_from('<I',data,indices+index*4)[0]
            name=None
            if offset<size:
                end=data.find(b'\0',strings+offset,strings+size)
                assert end>=0
                name=data[strings+offset:end].decode('utf-8')
                if name:by_name[name].append(index)
            names.append(name)
        mismatches=[];missing=[];duplicates=[];remapped=[]
        for original,expected,name in rows:
            index=expected&0xffffff
            actual=names[index] if index<count else None
            exact=by_name.get(name,[])
            if actual!=name:
                mismatches.append(dict(pc_index=original,expected_id=f'{expected:08x}',expected_name=name,
                    actual_name=actual,unique_actual_index=exact[0] if len(exact)==1 else None,actual_matches=exact[:8],actual_match_count=len(exact)))
            if not exact:missing.append(name)
            elif len(exact)>1:duplicates.append(dict(name=name,indices=exact[:8],count=len(exact)))
            elif exact[0]!=index:remapped.append(dict(name=name,index=exact[0],expected_index=index))
        results.append(dict(path=str(path),sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw),decoded_bytes=len(data),
            identifiers=count,nonempty_unique_names=len(by_name),profile_rows=len(rows),matching_profile_rows=len(rows)-len(mismatches),
            mismatch_count=len(mismatches),first_mismatches=mismatches[:12],
            missing_count=len(missing),missing_examples=missing[:16],duplicate_count=len(duplicates),duplicate_examples=duplicates[:8],
            unique_remap_count=len(remapped),unique_remap_examples=remapped[:12]))
    report=dict(passed=True,scope=__doc__,profile_sha256=hashlib.sha256(profile.read_bytes()).hexdigest(),caches=results,
        original_inputs_modified=False,raw_banks_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()

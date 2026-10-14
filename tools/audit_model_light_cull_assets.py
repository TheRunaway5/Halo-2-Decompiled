#!/usr/bin/env python3
"""Inventory original Xbox model light-cull resources without inventing bounds.

Literal map metadata and authored resource bytes determine whether a section
can reach the linear, posed-node branch. This is asset coverage, not live
admission, rendered parity, or a physical NV2A precision claim.
"""
import argparse
import collections
import hashlib
import json
import struct
from pathlib import Path
from audit_rendering import Cache, file_sha


def audit(path, shared):
    cache=Cache(path)
    rows=[]
    try:
        for tag in cache.tags.values():
            if tag['group']!='mode':
                continue
            _, model=cache.tag(tag['datum'],'mode')
            for ordinal, section in enumerate(cache.block(model+0x24,92)):
                if not cache.u16(section+4):
                    continue
                resources=cache.block(section+0x48,16)
                assert resources, (tag['name'],ordinal,'missing resources')
                pointer,size,tail=(cache.u32(section+x) for x in (0x38,0x3c,0x44))
                source=path if pointer>>30==0 else path.parent/('', 'mainmenu.map','shared.map','single_player_shared.map')[pointer>>30]
                if not source.exists():
                    source=shared/source.name
                with source.open('rb') as stream:
                    stream.seek(pointer&0x3fffffff)
                    raw=stream.read(size)
                assert len(raw)==size and tail+12<=size
                base=size-tail-4
                assert base>=8+0x34
                subparts,bounds,accelerator=(struct.unpack_from('<i',raw,8+x)[0] for x in (8,16,48))
                assert min(subparts,bounds,accelerator)>=0
                def resource_bytes(kind,stride,count):
                    selected=[r for r in resources if cache.u16(r+4)==kind and cache.u16(r+6)==stride]
                    if count==0:
                        assert not selected or all(cache.u32(r+8)==0 for r in selected)
                        return b''
                    assert len(selected)==1
                    at=selected[0];first=base+cache.u32(at+12);length=cache.u32(at+8)
                    assert length==count*stride and first>=0 and first+length<=len(raw)
                    return raw[first:first+length]
                subpart_data=resource_bytes(8,8,subparts)
                bound_data=resource_bytes(16,20,bounds)
                bound_indices=[struct.unpack_from('<H',subpart_data,n*8+4)[0] for n in range(subparts)]
                used=[n for n in bound_indices if n!=65535]
                invalid=sum(n>=bounds for n in used)
                nodes=sorted({bound_data[n*20+16] for n in used if n<bounds})
                rows.append(dict(model=tag['datum'],name=tag['name'],section=ordinal,
                    geometry_class=cache.u16(section),vertices=cache.u16(section+4),
                    subparts=subparts,bounds=bounds,accelerator_count=accelerator,
                    bounded_subparts=len(used)-invalid,unresolved_bound_indices=invalid,bound_nodes=nodes,
                    posed_branch_possible=accelerator<=0 and bool(nodes),
                    resource_file=str(source.resolve()),resource_offset=pointer&0x3fffffff,
                    resource_bytes=size,resource_sha256=hashlib.sha256(raw).hexdigest()))
        counts=collections.Counter()
        for row in rows:
            counts['sections']+=1;counts['subparts']+=row['subparts'];counts['bounds']+=row['bounds']
            counts['bounded_subparts']+=row['bounded_subparts'];counts['posed_sections']+=row['posed_branch_possible']
            counts['accelerated_sections']+=row['accelerator_count']>0
            counts['unresolved_bound_indices']+=row['unresolved_bound_indices']
        return dict(map=str(path.resolve()),sha256=file_sha(path),counts=dict(counts),sections=rows)
    finally:
        cache.close()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map',type=Path,action='append',required=True)
    parser.add_argument('--shared-directory',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert not args.output.exists()
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),maps=[])
    for path in args.map:
        result=audit(path,args.shared_directory);report['maps'].append(result)
        print(path.stem,result['counts'],flush=True)
    report['posed_models']=[dict(map=row['map'],**section) for row in report['maps'] for section in row['sections'] if section['posed_branch_possible']]
    args.output.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':
    main()

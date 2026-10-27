#!/usr/bin/env python3
"""Read-only admission checks for selected compressed or decoded MCC UI caches.

Run through tools/run_test.py. This checks actual raw widget definitions,
finite bitmap descriptors, both named identifier tables and the imported
logical-layout sidecar. It is not native Cache admission or GPU execution.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from export_matchmaking_ui_tu15 import Definition
from import_mcc_hud import MccCache
from mcc_content import decode_cache

ROOT=Path(__file__).resolve().parents[1]


def cache(path):
    # Reuse the existing v13 metadata reader after the separately bounded
    # decoder. Imported native caches can already be decoded (chunk count0).
    out=object.__new__(MccCache)
    out.path=path.resolve();raw=out.path.read_bytes();out.data=decode_cache(raw);out.mcc=True
    assert out.u32(4)==13 and out.u32(8)==len(out.data)
    out.index=out.u32(16);out.magic=out.u32(720)-out.index
    table=out.u32(out.index+8)-(out.u32(out.index)-out.index-32)
    names,offsets,count=out.u32(36),out.u32(44),out.u32(out.index+24)
    assert 0<count<=65536
    out.require(table,count*16);out.require(offsets,count*4)
    out.tags={};out.by_index={}
    for index in range(count):
        at=table+index*16;datum,address,size=struct.unpack_from('<III',out.data,at+4)
        if datum==0xffffffff:continue
        start=names+out.u32(offsets+index*4);out.require(start,1)
        end=out.data.find(b'\0',start);assert end>=start
        offset=address-out.magic if address else None
        if offset is not None:out.require(offset,size)
        tag=dict(index=index,datum=datum,group=bytes(out.data[at:at+4][::-1]).decode('ascii'),
                 name=bytes(out.data[start:end]).decode('utf-8'),address=address,offset=offset,size=size)
        assert datum not in out.tags
        out.tags[datum]=tag;out.by_index[index]=tag
    return out,hashlib.sha256(raw).hexdigest()


def differences(a,b,path=''):
    if type(a)!=type(b):return [path+':type']
    if isinstance(a,dict):
        return [p for key in sorted(set(a)|set(b)) for p in
            ([path+'/'+key+':missing'] if key not in a or key not in b else differences(a[key],b[key],path+'/'+key))]
    if isinstance(a,list):
        return ([path+':length'] if len(a)!=len(b) else [])+[
            p for i,(x,y) in enumerate(zip(a,b)) for p in differences(x,y,path+'/'+str(i))]
    return [] if a==b else [path]


def symbols(c,filename):
    count,strings,size,indices=struct.unpack_from('<4I',c.data,48)
    assert count<=1000000;c.require(indices,count*4);c.require(strings,size)
    rows=[];mismatches=[]
    for line in (ROOT/'src/ui'/filename).read_text().splitlines():
        match=re.fullmatch(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,("(?:[^"\\]|\\.)*")\},',line)
        assert match,line
        original,compiled,name=int(match[1],16),int(match[2],16),json.loads(match[3])
        index=compiled&0xffffff;actual=None
        if index<count:
            offset=c.u32(indices+index*4)
            if offset<size:actual=c.string(strings+offset,size-offset)
        rows.append(original)
        if name!=actual:mismatches.append(dict(original_index=original,mcc_id=f'{compiled:08x}',expected=name,actual=actual))
    return dict(profile=filename,rows=len(rows),mismatch_count=len(mismatches),first_mismatches=mismatches[:12])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcc-root',type=Path,action='append',required=True)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    goal=json.loads((ROOT/'resources/frontend/tu15-matchmaking-layout.json').read_text())
    compiled=json.loads((ROOT/'resources/frontend/mcc-native-layout.json').read_text())['layout']
    results=[]
    for root in args.mcc_root:
        c,digest=cache(root/'halo2/h2_maps_win64_dx11/mainmenu.map')
        actual=Definition(c).export();delta=differences(actual,goal['raw'])
        # Runtime qualification compares consumed definitions, not equality of
        # the entire catalog. Native custom maps may add their own previews;
        # they cannot change any existing source-authenticated widget/bank.
        consumed={key:actual[key] for key in goal['raw'] if key!='map_images'}
        expected={key:value for key,value in goal['raw'].items() if key!='map_images'}
        consumed_delta=differences(consumed,expected)
        for scenario,descriptor in goal['raw']['map_images'].items():
            consumed_delta+=differences(actual['map_images'].get(scenario),descriptor,'/map_images/'+scenario)
        side=root/'presentation/ui/native-layout.json'
        imported=json.loads(side.read_text()) if side.exists() else None
        namespace=[symbols(c,name) for name in ('mcc_frontend_string_ids.inc','mcc_pc_frontend_string_ids.inc')]
        results.append(dict(root=str(root),mainmenu_sha256=digest,raw_goal_definition_equal=not delta,
            raw_goal_difference_count=len(delta),first_raw_goal_differences=delta[:24],
            consumed_goal_definition_equal=not consumed_delta,
            first_consumed_goal_differences=consumed_delta[:24],
            extra_catalog_scenarios=sorted(set(actual['map_images'])-set(goal['raw']['map_images'])),
            imported_layout_present=bool(imported),imported_layout_equal_current=not imported or imported.get('layout')==compiled,
            namespaces=namespace))
    report=dict(schema='H2_SELECTED_MCC_FRONTEND_ADMISSION_AUDIT_V1',results=results,
        passed=all(r['consumed_goal_definition_equal'] and r['imported_layout_equal_current'] and
            all(not n['mismatch_count'] for n in r['namespaces']) for r in results),
        scope=__doc__,scratch=str(scratch),native_admission_or_gpu_executed=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()

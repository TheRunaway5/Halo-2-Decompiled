"""Strict authoring gate for the reproduced origin-spanning collision defect."""
import json
import hashlib
import os
from pathlib import Path
import sys
import tempfile


def deleted_vertex_references(data):
    bad=[]
    for di,d in enumerate(data['instanced geometries definitions']):
        for si,surf in enumerate(d['surfaces']):
            if surf['flags']&16:continue
            first=surf['first edge'];edge=first;seen=set()
            while True:
                assert edge not in seen;seen.add(edge)
                row=d['edges'][edge];left=row['left surface']==si
                assert left or row['right surface']==si
                v=row['start vertex'] if left else row['end vertex']
                if d['vertices'][v]['first edge']==-1:bad.append([di,si,v])
                edge=row['forward edge'] if left else row['reverse edge']
                if edge==first:break
    return bad


def validate_classic(kit,interface,report):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    sys.path.insert(0,str(interface));import tag_interface as ti
    g,e,d,groups,ext,post,_=ti.get_version_args('halo2',0)
    with tempfile.TemporaryDirectory(prefix='collision-defs-',dir=scratch) as owned:
        defs=g.generate_defs(d,owned,groups,ext);tags=kit/'tags'
        data=ti.read_file(defs,str(tags),str(tags/'scenarios/multi/dust2_mombasa/dust2_mombasa.scenario_structure_bsp'),0,engine_tag=e)['Data']
        bad=deleted_vertex_references(data)
    result=dict(passed=not bad,deleted_vertex_references=len(bad),examples=bad[:50],
        scope='Every active Classic instance collision edge cycle must reference live vertices; rejects the reproduced origin-spanning ghost triangles before baking')
    report.write_text(json.dumps(result,indent=2)+'\n')
    assert result['passed'],result
    return result


def validate_cache(cache,report):
    bsp=next(t for t in cache.tags.values() if t['group']=='sbsp')
    banks=[(-1,at) for at in cache.block(bsp['offset']+0x14,64)]
    banks.extend((i,at+0x70) for i,at in enumerate(cache.block(bsp['offset']+312,200)))
    bad=[]
    for owner,at in banks:
        surfaces=cache.block(at+0x28,8);edges=cache.block(at+0x30,12);vertices=cache.block(at+0x38,16)
        for si,surf in enumerate(surfaces):
            if cache.data[surf+4]&16:continue
            first=cache.u16(surf+2);edge=first;seen=set()
            while True:
                assert edge not in seen;seen.add(edge)
                row=edges[edge];left=cache.u16(row+8)==si
                assert left or cache.u16(row+10)==si
                vertex=cache.u16(row+(0 if left else 2))
                if cache.u16(vertices[vertex]+12)==0xffff:bad.append([owner,si,vertex])
                edge=cache.u16(row+(4 if left else 6))
                if edge==first:break
    with cache.path.open('rb') as stream:cache_sha=hashlib.file_digest(stream,'sha256').hexdigest()
    result=dict(passed=not bad,deleted_vertex_references=len(bad),examples=bad[:50],cache_sha256=cache_sha,
        scope='Final compiled primary and instance collision banks; all active edge cycles reference live vertices')
    report.write_text(json.dumps(result,indent=2)+'\n')
    assert result['passed'],result
    return result

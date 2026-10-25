#!/usr/bin/env python3
"""Compare every triangle corner after Halo BSP ownership/batch changes.

Run through tools/run_test.py. Temporary sorted triangle fingerprints are owned
and removed. The explicit report records exact corner/material fidelity while
allowing triangles to move between primary and instanced geometry.
"""
from __future__ import annotations

import argparse
import hashlib
import mmap
import os
from pathlib import Path
import struct
import tempfile

import numpy as np

from build_dust2_mombasa import save_json, sha
from prepare_dust2_h2ek import IDENTITY, source_meshes, closed_architecture, collision_proxy, mixed_geometry


def fingerprints(path,target,offset=0,ordered=None,exclude_materials=None,exclude_objects=None):
    source=source_meshes(path);materials=next(source)
    sky={i for i,(name,_) in enumerate(materials) if name.startswith('"+sky')}
    count=0;skipped=0
    for source_object,(vertices,triangles) in enumerate(source):
        if exclude_objects and source_object in exclude_objects:continue
        assert (triangles[:,1:]>=0).all() and triangles[:,1:].max()<len(vertices)
        for m,a,b,c in triangles:
            if int(m) in sky:skipped+=1;continue
            if exclude_materials and int(m) in exclude_materials:continue
            data=struct.pack('<I',int(m)+offset)+vertices[int(a)]+vertices[int(b)]+vertices[int(c)]
            if ordered is not None:ordered.update(data)
            target.write(hashlib.sha256(data).digest());count+=1
    return count,skipped,len(materials)


def sorted_digest(path,count):
    values=np.memmap(path,dtype='V32',mode='r+',shape=(count,))
    try:
        values.sort()
        return hashlib.sha256(values.view('u1')).hexdigest()
    finally:
        values.flush();del values


def validate(package,assembled,report):
    partition=closed_architecture(package)
    proxy=collision_proxy(package)
    mixed=mixed_geometry(package,proxy)
    assert not (partition and proxy)
    with tempfile.TemporaryDirectory(prefix='dust2-corners-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as owned:
        root=Path(owned);expected=root/'expected.bin';actual=root/'actual.bin'
        ordered=hashlib.sha256()
        with expected.open('wb') as out:
            count,_,materials=fingerprints(package/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass',out,ordered=ordered,
                exclude_materials=partition['material_ids'] if partition else proxy['material_ids'] if mixed else None,
                exclude_objects=set(mixed['props']['proof']['source_render_object_ids']+
                    mixed['props']['proof'].get('excluded_scenery_render_object_ids',[])) if mixed else None)
            if partition:
                architecture,_,_=fingerprints(partition['path'],out,ordered=ordered)
                count+=architecture
            if mixed:
                offset=materials
                for role in ('architecture','props'):
                    added,_,palette_size=fingerprints(mixed[role]['path'],out,offset=offset,ordered=ordered)
                    count+=added;offset+=palette_size
            else:
                collision,_,collision_materials=fingerprints(package/'dust2_collision.ass',out,offset=materials,ordered=ordered)
                count+=collision
            if proxy and not mixed:
                proxy_count,_,_=fingerprints(proxy['path'],out,offset=materials+collision_materials,ordered=ordered)
                count+=proxy_count
        with actual.open('wb') as out:observed,sky,_=fingerprints(assembled,out)
        assert count==observed,(count,observed)
        before=sorted_digest(expected,count);after=sorted_digest(actual,observed)
        assert before==after,'Triangle corner/material multiset changed'
        with assembled.open('rb') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as raw:
            at=raw.find(b';### INSTANCES ###');assert at>0
            tail=raw[at:].decode();lines=tail.splitlines();instances=int(lines[1])
            assert tail.count(IDENTITY)==instances,'Nonidentity output instance transform'
    result=dict(schema='H2_DUST2_ASSEMBLY_CORNER_VALIDATION_V2',passed=True,
        assembled_ass_sha256=sha(assembled),source_triangles=count,assembled_triangles=observed,
        source_ordered_corner_sha256=ordered.hexdigest(),triangle_multiset_sha256=before,
        positions_normals_uvs_material_indices_and_winding_preserved=not bool(mixed),
        authored_compiler_input_corner_multiset_preserved=True,
        preservation_scope=('Exact mixed compiler-input corner multiset; original architecture surface and interpolated attributes certified by plane-clipping authoring proof; retained palm corners and hulls exact; excluded wreck supplied by separately qualified static scenery source'
            if mixed and mixed['props']['proof'].get('static_wreck_omitted') else 'Exact mixed compiler-input corner multiset; original architecture surface and interpolated attributes certified by plane-clipping authoring proof; all retail prop corners and hulls exact'
            if mixed else 'Exact compiler-input corners: nonarchitecture plus source-bound closed pieces; exterior attributes/volume/routes certified by partition manifest'
            if partition else ('All visible source and prop collision corners plus exact separately certified architecture collision input'
                if proxy else 'All original source compiler-input triangle corners')),
        closed_architecture_manifest_sha256=sha(partition['manifest']) if partition else None,
        collision_proxy_manifest_sha256=sha(proxy['manifest']) if proxy else None,
        mixed_geometry_manifest_sha256=sha(mixed['manifest']) if mixed else None,
        output_instance_transforms_identity=True,sky_shell_triangles=sky,
        geometry_decimated=False,collision_simplified_separately=bool(proxy),temporary_fingerprint_files_removed=not root.exists())
    save_json(report,result);print(result,flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',required=True,type=Path)
    parser.add_argument('--assembled-ass',required=True,type=Path)
    parser.add_argument('--report-output',required=True,type=Path)
    a=parser.parse_args();validate(a.package,a.assembled_ass,a.report_output)


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Select native Halo lighting modes without changing BSP mesh/collision bytes.

Uses the verified H2EK instance fieldset v2/88-byte layout: pathfinding policy
is at84 and lightmapping policy at86. Header CRC is the SDK CRC32 without the
final xor. Large faces keep per-pixel lighting; dense detail uses per-vertex.
Run under tools/run_test.py; the backup and explicit report are caller-owned.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import mmap
from pathlib import Path
import shutil
import struct
import tempfile
import zlib

import numpy as np

from build_dust2_mombasa import METERS_PER_ASS_UNIT, save_json, sha
from prepare_dust2_h2ek import source_meshes


def checksum(raw):
    value=0
    for at in range(64,len(raw),8*1024*1024):value=zlib.crc32(raw[at:at+8*1024*1024],value)
    return value^0xffffffff


def configure(assembled,assembly,bsp,backup,report,threshold):
    count=next(r['instances'] for r in assembly['batches'] if r['role']=='render')
    instances=sum(r['instances'] for r in assembly['batches'])
    source=source_meshes(assembled);next(source);rows=[]
    try:
        for i in range(count):
            records,triangles=next(source)
            p=np.fromstring(b'\n'.join(v.splitlines()[0] for v in records).decode(),dtype=float,sep=' ').reshape(-1,3)
            t=p[triangles[:,1:]]
            areas=np.linalg.norm(np.cross(t[:,1]-t[:,0],t[:,2]-t[:,0]),axis=1)*(.5*METERS_PER_ASS_UNIT**2)
            largest=float(areas.max())
            rows.append(dict(index=i,triangles=len(triangles),maximum_triangle_area_m2=largest,
                             policy=0 if largest>=threshold else 1))
    finally:source.close()
    if not backup.exists():shutil.copy2(bsp,backup)
    original_sha=sha(bsp)
    with tempfile.TemporaryDirectory(prefix='dust2-policy-',dir=bsp.parent) as owned:
        stage=Path(owned)/bsp.name;shutil.copy2(bsp,stage)
        changed=[];allowed=set(range(40,44))
        with stage.open('r+b') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_WRITE) as raw:
            header=struct.pack('<4s3i',b'dfbt',2,instances,88)
            at=raw.find(header)
            assert at>=0 and raw.find(header,at+1)<0,'Ambiguous/unsupported instance fieldset'
            assert struct.unpack_from('<I',raw,40)[0]==checksum(raw),'Input BSP CRC mismatch'
            for i in range(instances):
                base=at+16+i*88
                assert struct.unpack_from('<13f',raw,base)==(1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.)
                assert struct.unpack_from('<H',raw,base+52)[0]==i,'Instance definition order changed'
                old=struct.unpack_from('<H',raw,base+86)[0]
                assert old in (0,1)
                desired=rows[i]['policy'] if i<count else old
                if desired!=old:
                    struct.pack_into('<H',raw,base+86,desired);changed.append(i);allowed.update([base+86,base+87])
            struct.pack_into('<I',raw,40,checksum(raw));raw.flush()
        observed=[]
        with bsp.open('rb') as a,stage.open('rb') as b:
            offset=0
            while True:
                before=a.read(8*1024*1024);after=b.read(8*1024*1024)
                assert len(before)==len(after)
                if not before:break
                if before!=after:observed.extend(offset+i for i,(x,y) in enumerate(zip(before,after)) if x!=y)
                offset+=len(before)
        assert set(observed)<=allowed,'Unexpected BSP payload change'
        after_sha=sha(stage);stage.replace(bsp)
    result=dict(schema='H2_DUST2_NATIVE_LIGHTING_POLICY_V1',passed=True,source_ass_sha256=sha(assembled),
        bsp_before_sha256=original_sha,bsp_after_sha256=after_sha,backup=str(backup),
        large_face_threshold_m2=threshold,per_pixel_instances=sum(r['policy']==0 for r in rows),
        per_vertex_instances=sum(r['policy']==1 for r in rows),changed_instances=changed,
        changed_byte_count=len(observed),only_lighting_policy_and_header_crc_changed=True,
        mesh_collision_and_placement_bytes_unchanged=True,instance_fieldset_offset=at,
        instance_fieldset_version=2,instance_fieldset_bytes=88,instances=rows)
    save_json(report,result)
    print({k:v for k,v in result.items() if k not in ['instances','changed_instances']},flush=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--assembled-ass',required=True,type=Path)
    p.add_argument('--assembly-report',required=True,type=Path)
    p.add_argument('--bsp',required=True,type=Path)
    p.add_argument('--backup',required=True,type=Path)
    p.add_argument('--report-output',required=True,type=Path)
    p.add_argument('--large-face-area',type=float,default=4.)
    a=p.parse_args();configure(a.assembled_ass,json.loads(a.assembly_report.read_text()),a.bsp,a.backup,a.report_output,a.large_face_area)


if __name__=='__main__':main()

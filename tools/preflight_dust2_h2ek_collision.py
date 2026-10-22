#!/usr/bin/env python3
"""Compile each assembled collision piece at the real Classic Tool boundary.

Run inside tools/run_test.py with a private extracted kit. Temporary ASS/BSP
probes are removed; the explicit JSON report retains bounded diagnostics and
an adaptive split plan for prepare_dust2_h2ek.py. Original surfaces are intact.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import mmap
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

import numpy as np

from build_dust2_mombasa import save_json, sha
from prepare_dust2_h2ek import IDENTITY


def mesh_body(body,parts):
    lines=[s for s in body.splitlines(keepends=True) if s.strip() and not s.startswith(b';')]
    count=int(lines[3]);at=4+count*5
    vertices=[b''.join(lines[4+i*5:9+i*5]) for i in range(count)]
    triangles=np.fromstring(b''.join(lines[at+1:]).decode(),dtype=np.int64,sep=' ').reshape(-1,4)
    assert len(triangles)==int(lines[at])
    for part in np.array_split(triangles,parts):
        used,inverse=np.unique(part[:,1:],return_inverse=True)
        result=f'"MESH"\n""\n""\n{len(used)}\n'.encode()+b''.join(vertices[i] for i in used)
        result+=f'{len(part)}\n'.encode()
        result+=b''.join(f'{m}\n{a}\n{b}\n{c}\n'.encode() for m,(a,b,c) in zip(part[:,0],inverse.reshape(-1,3)))
        yield result


def preflight(kit,source,assembly,report,jobs,resume):
    kit=kit.resolve();source=source.resolve()
    rendered=next(r['instances'] for r in assembly['batches'] if r['role']=='render')
    collided=next(r['instances'] for r in assembly['batches'] if r['role']=='collision')
    reused=[];resume_sha=None
    if resume:
        previous=json.loads(resume.read_text());resume_sha=sha(resume)
        assert previous['source_ass_sha256']==sha(source)
        reused=[r for r in previous['results'] if r['passed']]
    with source.open('rb') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as raw:
        markers=[m.start() for m in re.finditer(b'"MESH"\n""\n""\n',raw)]
        assert len(markers)==rendered+collided+1
        end=raw.find(b';### INSTANCES ###',markers[-1])
        header=raw[:raw.find(b';### OBJECTS ###')]+b'2\n'
        sky=raw[markers[-1]:end]
        bodies=[raw[markers[i]:markers[i+1]] for i in range(rendered,rendered+collided)]
    parent=kit/'data/scenarios/multi'
    with tempfile.TemporaryDirectory(prefix='d2c_preflight_',dir=parent) as owned:
        data_root=Path(owned)
        tag_root=kit/'tags/scenarios/multi'/data_root.name
        assert not tag_root.exists()
        env=os.environ.copy();env['WINEDEBUG']='-all'
        def check(index):
            attempts=[]
            for parts in (1,2,4,8):
                passed=True
                for part,body in enumerate(mesh_body(bodies[index],parts)):
                    leaf=f'p{index:03d}_{parts}_{part}'
                    folder=data_root/leaf/'structure';folder.mkdir(parents=True)
                    instances='3\n-1\n"Scene Root"\n-1\n-1\n0\n'+IDENTITY+'0\n"%collision"\n1\n0\n0\n'+IDENTITY+'1\n"level"\n2\n0\n0\n'+IDENTITY
                    path=folder/(leaf+'.ass')
                    path.write_bytes(header+body+sky+instances.encode())
                    relative=str(path.relative_to(kit/'data')).replace('/','\\')
                    try:
                        process=subprocess.run(['wine','tool_fast.exe','-batch','structure-new-from-ass',relative,'no'],cwd=kit,env=env,capture_output=True,timeout=180)
                        log=(process.stdout+process.stderr).decode(errors='replace').replace('\r','\n')
                    except subprocess.TimeoutExpired:
                        subprocess.run(['wineserver','-k'],env=env,check=False)
                        return dict(index=index,passed=False,error='owned compiler timed out',attempts=attempts)
                    errors=[l.strip() for l in log.splitlines() if '### ERROR' in l or '### FATAL' in l or 'ASSERTION FAILED' in l]
                    bsp=tag_root/leaf/(leaf+'.scenario_structure_bsp')
                    ok=not errors and process.returncode==0 and bsp.is_file() and 'saving imported structure' in log
                    entry=dict(parts=parts,part=part,passed=ok,returncode=process.returncode,errors=errors[:8],source_sha256=sha(path))
                    if ok:entry.update(bsp_sha256=sha(bsp),bsp_bytes=bsp.stat().st_size)
                    attempts.append(entry)
                    if not ok:
                        passed=False
                        # Only subdivision-related errors justify another split.
                        known=('bsp2d is too large','plane builder unable to converge','too many strip indices')
                        specific=[e for e in errors if 'structure import pipeline failed' not in e]
                        if not specific or any(not any(k in e for k in known) for e in specific):
                            return dict(index=index,passed=False,attempts=attempts)
                        break
                if passed:return dict(index=index,passed=True,parts=parts,attempts=attempts)
            return dict(index=index,passed=False,attempts=attempts)
        results=list(reused)
        try:
            with ThreadPoolExecutor(max_workers=jobs) as pool:
                already={r['index'] for r in reused}
                pending=[pool.submit(check,i) for i in range(collided) if i not in already]
                for future in as_completed(pending):
                    row=future.result();results.append(row)
                    print(json.dumps({k:row[k] for k in ('index','passed','parts') if k in row}),flush=True)
        finally:
            if tag_root.exists():shutil.rmtree(tag_root)
        results.sort(key=lambda r:r['index'])
    passed=all(r['passed'] for r in results) and len(results)==collided
    plan={str(r['index']):r['parts'] for r in results if r['passed'] and r['parts']>1}
    save_json(report,dict(schema='H2_DUST2_COLLISION_PREFLIGHT_V1',passed=passed,
        source_ass_sha256=sha(source),pieces=collided,split_plan=plan,
        reused_verified_cases=len(reused),resume_report_sha256=resume_sha,
        definitions_after_splitting=rendered+sum(r.get('parts',1) for r in results),
        temporary_probe_data_removed=not data_root.exists(),temporary_probe_tags_removed=not tag_root.exists(),results=results))
    if not passed:raise RuntimeError('Collision preflight failed; inspect the report')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit-root',required=True,type=Path)
    parser.add_argument('--source-ass',required=True,type=Path)
    parser.add_argument('--assembly-report',required=True,type=Path)
    parser.add_argument('--report-output',required=True,type=Path)
    parser.add_argument('--jobs',type=int,default=4)
    parser.add_argument('--resume-report',type=Path)
    a=parser.parse_args()
    preflight(a.kit_root,a.source_ass,json.loads(a.assembly_report.read_text()),a.report_output,a.jobs,a.resume_report)


if __name__=='__main__':main()

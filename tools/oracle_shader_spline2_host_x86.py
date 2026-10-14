#!/usr/bin/env python3
"""Unchanged original spline2 execution on the host x86 CPU, all three images.

The test-only i386 harness maps the pinned original image bytes without edits
and calls its complete scalar function/actual CRT. This retains the host's
RSQRTSS estimate explicitly; it does not establish Xbox hardware precision.
Raw authored emitter/sprite curves and declared adjacent branch controls are
inputs. No shader, particle, game service or instruction result is substituted.
"""
import argparse,csv,hashlib,json,random,struct,subprocess
from pathlib import Path
from weapon_reference_versions import images
from inspect_multiplayer import Cache
from functions import _Code,_trace
from research_mp_inventory_switch_u5 import prefix,raw_prefix_candidates,norm
ROOT=Path(__file__).resolve().parents[1]
def f32(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--map',type=Path,required=True)
    p.add_argument('--harness',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
    c=Cache(a.map);records=[]
    def add(tag,block,source):
        n=c.u(block);address=c.u(block+4)-c.magic
        if n and c.data[address]==10:
            records.append(dict(tag=tag['name'],group=tag['group'],source=source,size=n,
                file_offset=address,raw_hex=bytes(c.data[address:address+n]).hex()))
    for tag in c.tags.values():
        if tag['offset']is None:continue
        if tag['group']=='effe':
            for event in c.block(tag['offset']+0x14,0x38):
                for system in c.block(event+0x30,0x38):
                    for emitter in c.block(system+0x30,0xb8):
                        for off in (8,0x18,0x28,0x38,0x48,0x58,0x68,0x7c,0x8c):
                            add(tag,emitter+off+8,f'emitter+{off:02x}')
        elif tag['group']=='prt3':
            for n in range(5):add(tag,tag['offset']+0x24+n*0x10,f'sprite property{n}')
    curves=sorted(set(r['raw_hex']for r in records));authored_count=len(curves)
    assert curves,'No original spline2 curves found'
    boundary=[]
    for label,points in [('zero terminal distance',(0,0,1,0,0,0,0,0)),
                         ('zero initial distance',(0,0,0,0,0,0,1,0)),
                         ('equal distances',(0,0,1,0,0,0,1,0)),
                         ('near epsilon',(0,0,1,0,0,0,.01,0))]:
        raw=bytearray.fromhex(curves[0]);struct.pack_into('<8f',raw,20,*points)
        boundary.append(dict(label=label,raw_hex=raw.hex()));curves.append(raw.hex())
    original_records=images();base=next(image for label,image,_ in original_records if label=='retail')
    code=_Code(base,'.text');body=_trace(code,0x13b390,{0x13b390})
    ins=[code.at(at)for at in sorted(body.code)];positions={i.address:n for n,i in enumerate(ins)}
    expected=[norm(i,base,positions)for i in ins];owners=[]
    for label,image,pin in original_records:
        if label=='retail':entry=0x13b390
        else:
            candidate_code=_Code(image,'.text')
            path=ROOT/('analysis/map-pack-update/functions.tsv'if label=='TU1.1'else'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')
            starts={int(r['address'],16)for r in csv.DictReader(path.open(),delimiter='\t')}
            candidates=[e for e in starts if prefix(candidate_code,image,e)==prefix(code,base,0x13b390)]
            candidates=sorted(set(candidates+raw_prefix_candidates(code,base,candidate_code,0x13b390)))
            matches=[]
            for e in candidates:
                f=_trace(candidate_code,e,starts);sequence=[candidate_code.at(at)for at in sorted(f.code)]
                pos={i.address:n for n,i in enumerate(sequence)}
                if [norm(i,image,pos)for i in sequence]==expected:matches.append(e)
            assert len(matches)==1,(label,matches);entry=matches[0]
        owners.append(dict(label=label,path=str(image.path),sha256=pin,entry=f'{entry:08x}'))
    rng=random.Random(0x13b8b5);rows=[]
    for curve in curves:
        for value in [n/32 for n in range(33)]+[rng.random()for _ in range(32)]:
            for range_ in (0.,.25,.5,1.):rows.append(f'{curve} {f32(value):08x} {f32(range_):08x}')
    results=[]
    for owner in owners:
        run=subprocess.run([str(a.harness),owner['path'],owner['entry']],input='\n'.join(rows)+'\n',
            text=True,capture_output=True,check=True)
        outputs=[int(line,16)for line in run.stdout.splitlines()];assert len(outputs)==len(rows)
        results.append(outputs);print(owner['label'],owner['entry'],len(outputs),flush=True)
    assert results[0]==results[1]==results[2],'Original versions differ on this host'
    a.fixture.parent.mkdir(parents=True,exist_ok=True)
    a.fixture.write_text('# Unchanged original scalar owner on host x86; Xbox RSQRTSS estimate precision unverified.\n'+
        '\n'.join(f'{row} {result:08x}'for row,result in zip(rows,results[0]))+'\n')
    report=dict(scope=__doc__,cases=len(rows),authored_curves=authored_count,records=records,boundary_controls=boundary,
        owners=owners,normalized_complete_bodies_equal=True,services=[],instruction_patches=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        harness_source_sha256=hashlib.sha256((ROOT/'tools/original_shader_spline2_host_x86.cpp').read_bytes()).hexdigest(),
        harness_sha256=hashlib.sha256(a.harness.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        map_sha256=hashlib.sha256(a.map.read_bytes()).hexdigest())
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(rows),authored_curves=authored_count)),flush=True)
if __name__=='__main__':main()

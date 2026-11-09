#!/usr/bin/env python3
"""Qualify private shared16C000 DATA against the unchanged original composition."""
import hashlib
import inspect
import json
import os
from pathlib import Path
import sys

import validate_native_camera_instanced_query_tu15 as original
from audit_native_execution_ownership import unique_function
ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_camera_instance_query_kernel.hpp','include/halo2/native_tu15_camera_instance_query_value.hpp','src/physics/native_tu15_camera_instanced_query.cpp','tools/validate_native_camera_instance_query_data_tu15.py','tools/validate_native_camera_instanced_query_tu15.py')

def hashes():return {n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in FILES}

def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);out=scratch/'shared-query-source';out.mkdir()
    for path in ROOT.iterdir():
        if path.name!='src' and path.name not in ('.git','.local','build','build-perf-4k60'):
            (out/path.name).symlink_to(path,target_is_directory=path.is_dir())
    (out/'src').mkdir();(out/'src/physics').mkdir()
    for path in (ROOT/'src').iterdir():
        if path.name!='physics':(out/'src'/path.name).symlink_to(path,target_is_directory=path.is_dir())
    for path in (ROOT/'src/physics').iterdir():
        if path.name!='native_tu15_camera_instanced_query.cpp':(out/'src/physics'/path.name).symlink_to(path,target_is_directory=path.is_dir())
    path=ROOT/'src/physics/native_tu15_camera_instanced_query.cpp';source=path.read_text();before=hashes()
    old=unique_function(source,'native_tu15_query_camera_instance')
    new='''bool native_tu15_query_camera_instance(SourcePhysicsContactMemory&memory,
 const NativeTu15CameraInstanceQuery&q,NativeTu15CameraInstanceQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CameraInstanceQueryStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 Memory m(memory,observe);
 const NativeTu15CameraInstanceQuery input{q.instance,q.mask,q.bsp_filter,q.origin,q.displacement,q.result};
 native_camera_instance_query_value_detail::View<Memory,Environment> view{memory,m,input,scratch,fp,depth,observe,stats};
 native_camera_instance_query_detail::Progress<__m128> data;return native_camera_instance_query_detail::query(view,data);
#else
 if(stats)++stats->calls;
 (void)memory;(void)q;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Native camera instance query backend unavailable");
#endif
}'''
    source=source.replace(old,new,1);source='#include "halo2/native_tu15_camera_instance_query_value.hpp"\n'+source
    (out/'src/physics/native_tu15_camera_instanced_query.cpp').write_text(source)
    # The installed child algorithms remain the genuine current owners. Only
    # private facade inputs are corrupted for two additional real controls;
    # the existing definition-filter child control remains unchanged.
    text=inspect.getsource(original.main)
    text=text.replace("('instance_stride','q.instance*0x58u','q.instance*0x54u')","('instance_identity','input{q.instance,','input{q.instance+1u,')")
    text=text.replace("('origin_translation','inverse+40u','inverse+44u')","('origin_identity','q.origin,q.displacement,q.result','q.origin+4u,q.displacement,q.result')")
    original.ROOT=out;original.base.ROOT=out
    env=dict(original.__dict__,__file__=str(out/'tools/validate_native_camera_instanced_query_tu15.py'));exec(text,env)
    report_arg=next((Path(sys.argv[n+1])/'report.json'for n,v in enumerate(sys.argv[:-1])if v=='--output'),None)
    try:result=env['main']()
    finally:
        if report_arg and report_arg.exists():
            report=json.loads(report_arg.read_text());report.update(shared_data_private=True,production_binding=False,shared_source_before=before,shared_source_after=hashes(),original_public_query_sha256=hashlib.sha256(old.encode()).hexdigest())
            if before!=hashes():report.update(passed=False,shared_source_drift=True)
            report_arg.write_text(json.dumps(report,indent=2)+'\n')
    return result if before==hashes() else 1
if __name__=='__main__':raise SystemExit(main())

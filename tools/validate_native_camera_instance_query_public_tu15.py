#!/usr/bin/env python3
"""Qualify installed shared16C000 DATA against unchanged original composition."""
import hashlib
import inspect
import json
import os
from pathlib import Path
import sys

import validate_native_camera_instanced_query_tu15 as original
from audit_native_execution_ownership import unique_function
ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_camera_instance_query_kernel.hpp','include/halo2/native_tu15_camera_instance_query_value.hpp','src/physics/native_tu15_camera_instanced_query.cpp','tools/validate_native_camera_instance_query_public_tu15.py','tools/validate_native_camera_instanced_query_tu15.py')

def hashes():return {n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in FILES}

def main():
    import native_camera_instance_query_replacements as binding
    before=hashes();source=(ROOT/'src/physics/native_tu15_camera_instanced_query.cpp').read_text()
    assert binding.replace_public(source)==source,'Installed public16C000 DATA facade missing'
    text=inspect.getsource(original.main)
    text=text.replace("('instance_stride','q.instance*0x58u','q.instance*0x54u')","('instance_identity','input{q.instance,','input{q.instance+1u,')")
    text=text.replace("('origin_translation','inverse+40u','inverse+44u')","('origin_identity','q.origin,q.displacement,q.result','q.origin+4u,q.displacement,q.result')")
    env=dict(original.__dict__,__file__=__file__);exec(text,env)
    report_arg=next((Path(sys.argv[n+1])/'report.json'for n,v in enumerate(sys.argv[:-1])if v=='--output'),None)
    try:result=env['main']()
    finally:
        if report_arg and report_arg.exists():
            report=json.loads(report_arg.read_text());report.update(shared_data_installed=True,production_binding=True,shared_source_before=before,shared_source_after=hashes(),installed_public_query_sha256=binding.digest(binding.PUBLIC_BODY))
            if before!=hashes():report.update(passed=False,shared_source_drift=True)
            report_arg.write_text(json.dumps(report,indent=2)+'\n')
    return result if before==hashes()else 1
if __name__=='__main__':raise SystemExit(main())

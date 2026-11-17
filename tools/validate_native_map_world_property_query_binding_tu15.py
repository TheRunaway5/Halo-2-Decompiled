#!/usr/bin/env python3
"""Authenticate and compile the exact paired MapWorld property binding."""
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

import native_map_world_property_query_replacements as bind
from specialize_native_operands import specialize
from research_mp_sandbox_update_functions import NEW_DEFAULT

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/runtime/source_map_world_support_tu15_generated.cpp'
GENERATOR='tools/generate_map_world_support_tu15.py'

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def reject(function,*args):
    try:function(*args)
    except ValueError:return
    raise ValueError('Changed MapWorld property owner was accepted')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='c++')
    p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if not NEW_DEFAULT.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,GENERATOR,'tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py',
           'tools/native_map_world_property_query_replacements.py','tools/validate_native_map_world_property_query_binding_tu15.py')
    pins={name:digest(ROOT/name)for name in names}
    headers={path.resolve():digest(path)for tree in ('include','src')for path in(ROOT/tree).rglob('*')if path.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins,contexts=[])
    try:
        fresh=scratch/'fresh.cpp'
        command=[sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--manifest',str(scratch/'generation.json')]
        run=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=600)
        report['generation']=dict(command=command,exit_code=run.returncode)
        if run.returncode:raise ValueError('Actual MapWorld generation failed: '+run.stderr[-7000:])
        current=(ROOT/UNIT).read_text();generated=fresh.read_text()
        report['production_bound']=bool(bind.authenticate(current)[3][bind.ENTRIES[0]])
        if a.require_binding:
            bind.installed_entries(current);bind.installed_entries(generated)
        from native_device_property_query_replacements import remove_device_property_query
        raw={name:bind.remove_map_world_property_queries(remove_device_property_query(text))
             for name,text in (('current',current),('fresh',generated))}
        for entry in bind.ENTRIES:
            if re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',bind.body(raw['current'],entry))!=bind.body(raw['fresh'],entry):
                raise ValueError('Fresh MapWorld property body has an additional source change '+entry)
        report['exact_raw_identity_projection']=True
        for origin,text in raw.items():
            prepared=specialize(text)[0];installed=bind.replace_map_world_property_queries(text)
            retagged,receipt=specialize(installed)
            official=receipt.get('native_map_world_property_query_entries')==list(bind.ENTRIES)
            if not official:retagged=bind.retag_specialized(installed,retagged)
            for literal,base in ((False,text),(True,prepared)):
                candidate=bind.replace_map_world_property_queries(base)
                if bind.remove_map_world_property_queries(candidate)!=base or bind.replace_map_world_property_queries(candidate)!=candidate:
                    raise ValueError('MapWorld property binding is not exactly reversible')
                if literal and retagged!=candidate:raise ValueError('Prepared MapWorld property round trip changed')
                leaf=bind.body(candidate,'1db800');parent=bind.body(candidate,'11e490')
                prefix=bind.prefix('11e490',literal);leaf_prefix=bind.prefix('1db800',literal)
                controls=(
                    candidate.replace(bind.INCLUDE,'',1),bind.INCLUDE+candidate.replace(bind.INCLUDE,'',1),bind.INCLUDE+candidate,
                    candidate.replace(prefix,bind.prefix('11e490',not literal),1),
                    candidate.replace(leaf_prefix,bind.prefix('1db800',not literal),1),
                    candidate.replace(leaf_prefix,'',1),
                    candidate.replace(parent,parent.replace(prefix,'',1)[:-2]+prefix+'\n}',1),
                    candidate.replace(leaf,leaf.replace(bind.ORDERED_MUL,bind.OLD_MUL,1),1),
                    candidate.replace(parent,parent.replace('run_1db800(child)','run_11e490(child)',1),1),
                    candidate.replace('case 0x11e490u:run_11e490(s);break;','case 0x11e490u:break;',1),
                    candidate.replace('static void dispatch(WorldState &s,','static void dispatch(WorldState &s, ',1),
                    candidate+'\nnamespace unexpected = halo2::'+bind.OWNER_NAMESPACE+';\n',
                    candidate+'\nusing namespace halo2::'+bind.CORE_NAMESPACE+';\n',
                    candidate+'\n#include <gameplay/objects/'+bind.INCLUDE_BASENAME+'>\n',
                )
                for bad in controls:
                    if bad==candidate:raise ValueError('Missing MapWorld property defect seam')
                    reject(bind.installed_entries,bad)
                if not literal and official:reject(specialize,controls[3])
                name=origin+('-prepared'if literal else'-raw')
                output=scratch/(name+'.cpp');output.write_text(candidate);dep=output.with_suffix('.d')
                command=[a.compiler,'-std=c++20','-fsyntax-only','-MD','-MF',str(dep),
                         '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(output)]
                result=subprocess.run(command,capture_output=True,text=True,timeout=600)
                if result.returncode:raise ValueError('Whole MapWorld property candidate syntax failed '+name+': '+result.stderr[-7000:])
                for token in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                    path=Path(token).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
                report['contexts'].append(dict(name=name,cpp_sha256=digest(output),syntax=True,
                    strict_refusals=len(controls)+int(not literal and official),actual_retag=official,command=command))
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:digest(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())

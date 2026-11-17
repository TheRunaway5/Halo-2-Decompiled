#!/usr/bin/env python3
"""Compare the actual linked MapWorld API with its explicitly retained control."""
import argparse
import filecmp
import hashlib
from itertools import zip_longest
import json
import os
import re
import subprocess
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe
from validate_native_map_world_property_query_frame_tu15 import PIN, corpus as leaves
from validate_native_map_world_property_caller_frame_tu15 import corpus as callers
from validate_native_device_property_query_frame_tu15 import corpus as devices
from validate_native_device_property_query_source_tu15 import SYMBOL_ROWS as DEVICE_SYMBOL_ROWS

ROOT = Path(__file__).resolve().parents[1]
OWNER = 'src/runtime/source_map_world_support_tu15_generated.cpp'
SYMBOLS = 'src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
JOIN = 'src/rendering/effects/source_object_function_symbols_tu15.inc'
CALLER_CONTROL = 'HALO2_REFERENCE_MAP_WORLD_PROPERTY_CALLER'
LEAF_CONTROL = 'HALO2_REFERENCE_MAP_WORLD_PROPERTY_QUERY'
DEVICE_CONTROL = 'HALO2_REFERENCE_DEVICE_PROPERTY_QUERY'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def fixture(xbe,include_device=False):
    symbols = re.findall(r'\bSymbol\{(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/SYMBOLS).read_text())
    joined = re.findall(r'ObjectFunctionSymbol\{0x[0-9a-f]+u,0x[0-9a-f]+u,0x[0-9a-f]+u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/JOIN).read_text())
    names = {}
    for key,name in [(mcc,name) for _,mcc,name in symbols]+joined:
        if names.setdefault(str(int(key,16)),name) != name:
            raise ValueError('Conflicting MapWorld symbol join')
    if len(symbols) != 10 or len(names) != 108:
        raise ValueError('MapWorld symbol join changed')
    join=(ROOT/JOIN).read_text()
    if join.count('ObjectFunctionSymbol{0x1db800u,0xa000693u,0xa00069fu,0xa00069fu,"phantom_on"}') != 1:
        raise ValueError('MapWorld property compiled symbol join changed')
    keys={str(key):key+12 for key in (0xa000693,0xd000691,0xd000692)}
    image=Xbe(xbe)
    rows=leaves(image)+callers(image)
    if include_device:
        for site,key,mcc,name in DEVICE_SYMBOL_ROWS:
            if join.count(f'ObjectFunctionSymbol{{0x{site:x}u,0x{key:x}u,0x{mcc:x}u,0x{mcc:x}u,"{name}"}}')!=1:
                raise ValueError('Device property compiled symbol join changed')
            if keys.setdefault(str(key),mcc)!=mcc:raise ValueError('Conflicting MapWorld public key')
        rows+=devices(image)
    return dict(rows=rows,names=names,keys=keys)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--include-device',action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    if not a.xbe.is_file():return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    report = dict(passed=False,production_bound=True,device_included=a.include_device,
                  owned_scratch=str(scratch),retained_large_artifacts=[])
    names=(OWNER,SYMBOLS,JOIN, 'src/gameplay/objects/native_tu15_map_world_property_query_source_frame.hpp',
           'src/gameplay/objects/native_tu15_map_world_property_caller_source_frame.hpp',
           'src/gameplay/objects/native_tu15_map_world_property_owners.hpp',
           'src/gameplay/objects/native_tu15_device_property_query_source_owner.hpp',
           'src/gameplay/objects/native_tu15_device_property_query_source_frame.hpp',
           'include/halo2/native_tu15_device_property_query_kernel.hpp',
           'tools/validate_native_device_property_query_frame_tu15.py',
           'tools/validate_native_device_property_query_source_tu15.py',
           'tools/validate_native_map_world_property_queries_public_tu15.py',
           'tests/native_tu15_map_world_property_queries_public_tests.cpp')
    before={name:sha(ROOT/name) for name in names}
    try:
        if sha(a.xbe) != PIN:raise ValueError('Wrong original TU1.5 image')
        report.update(source_before=before,linked_binary_sha256=sha(a.native),reference_xbe_sha256=PIN)
        data=scratch/'public-inputs.json';data.write_text(json.dumps(fixture(a.xbe,a.include_device),separators=(',',':')))
        report['input_sha256']=sha(data)
        env={k:v for k,v in os.environ.items() if not k.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        outputs={};hosts={};runs={}
        controls=(('default',{}),('caller-reference',{CALLER_CONTROL:'1'}),
                  ('leaf-reference',{LEAF_CONTROL:'1'}),('both-reference',{CALLER_CONTROL:'1',LEAF_CONTROL:'1'}),
                  ('unrelated-reference',{'HALO2_REFERENCE_PROJECTILE_FUNCTION_QUERY':'1'}))
        if a.include_device:
            controls+=(('device-reference',{DEVICE_CONTROL:'1'}),
                       ('all-three-reference',{CALLER_CONTROL:'1',LEAF_CONTROL:'1',DEVICE_CONTROL:'1'}))
        for label,override in controls:
            stream=scratch/(label+'.stream');host=scratch/(label+'.host.jsonl')
            command=[str(a.native),str(data),str(stream),str(host)]
            tested=subprocess.run(command,env={**env,**override},capture_output=True,text=True,timeout=1200)
            if tested.returncode:
                raise ValueError('Actual MapWorld API '+label+' failed: '+(tested.stdout+tested.stderr)[-6000:])
            counts=json.loads(tested.stdout)
            expected_normal=1152+(1536 if a.include_device else 0)
            devices_expected=counts['device_invocations']>1536 if a.include_device else counts['device_invocations']==0
            if counts['normal']!=expected_normal or not counts['prefixes'] or counts['caller_invocations']<=576 or counts['direct_leaf_invocations']<=576 or not devices_expected:
                raise ValueError('Actual MapWorld API did not exercise expected inputs: '+str(counts))
            outputs[label]=stream
            hosts[label]=host
            runs[label]=dict(command=command,controls=override,result=counts,stream_sha256=sha(stream),
                host_stream_sha256=sha(host),stream_bytes=stream.stat().st_size)
        first=runs['default']['result']
        if first['native_callers']!=first['caller_invocations'] or first['native_leaves']<=first['direct_leaf_invocations']:
            raise ValueError('Default MapWorld API did not enter both native owners with genuine children')
        if first['native_devices']!=first['device_invocations']:
            raise ValueError('Default MapWorld API missed the device owner')
        for label,override in controls:
            count=runs[label]['result']
            expected=(0 if CALLER_CONTROL in override else first['native_callers'],
                      0 if LEAF_CONTROL in override else first['native_leaves'],
                      0 if DEVICE_CONTROL in override else first['native_devices'])
            if (count['native_callers'],count['native_leaves'],count['native_devices'])!=expected:
                raise ValueError('MapWorld API reference scope changed '+label+': '+str(count))
        report['runs']=runs
        report['guest_streams_equal']=all(filecmp.cmp(outputs['default'],value,shallow=False)
            for name,value in outputs.items() if name!='default')
        report['host_streams_equal']=all(filecmp.cmp(hosts['default'],value,shallow=False)
            for name,value in hosts.items() if name!='default')
        if not report['host_streams_equal']:
            for label,path in hosts.items():
                if label=='default':continue
                with hosts['default'].open() as default, path.open() as other:
                    mismatch=next(((index,a,b) for index,(a,b) in enumerate(zip_longest(default,other)) if a!=b),None)
                if mismatch:
                    index,expected,actual=mismatch
                    report['first_host_mismatch']=dict(control=label,index=index,default=json.loads(expected or 'null'),
                                                       controlled=json.loads(actual or 'null'))
                    break
        if not report['guest_streams_equal'] or not report['host_streams_equal']:
            raise ValueError('Actual MapWorld public ABI/provider/observer/bank streams differ')
        report.update(passed=True,complete_streams_equal=True)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name) for name in names}
        if report['source_after']!=before:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())

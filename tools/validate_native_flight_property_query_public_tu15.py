#!/usr/bin/env python3
"""Compare an actual linked E0900 API with its explicitly retained control."""
import argparse
import filecmp
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe
from validate_native_flight_property_query_frame_tu15 import PIN, corpus

ROOT = Path(__file__).resolve().parents[1]
OWNER = 'src/physics/physics_live_support_tu15_generated.cpp'
DEATH_OWNER = 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
SYMBOLS = 'src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
JOIN = 'src/rendering/effects/source_object_function_symbols_tu15.inc'
CONTROL = 'HALO2_REFERENCE_FLIGHT_PROPERTY_QUERY'
DEATH_CONTROL = 'HALO2_REFERENCE_DEATH_CYCLE_FLIGHT_PROPERTY_QUERY'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def fixture(xbe):
    symbols = re.findall(r'\bSymbol\{(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/SYMBOLS).read_text())
    joined = re.findall(r'ObjectFunctionSymbol\{0x[0-9a-f]+u,0x[0-9a-f]+u,0x[0-9a-f]+u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/JOIN).read_text())
    names = {}
    for key,name in [(mcc,name) for _,mcc,name in symbols]+joined:
        if names.setdefault(str(int(key,16)),name) != name:
            raise ValueError('Conflicting Primary E0900 symbol join')
    if len(symbols) != 10 or len(names) != 108:
        raise ValueError('Primary E0900 symbol join changed')
    keys = {}
    for site,original,mcc,name in (
        (0xe093f,0x6000542,0x600054e,'flight'),
        (0xe0952,0xc0005a4,0xc0005b0,'flying_speed'),
    ):
        expected=f'ObjectFunctionSymbol{{0x{site:x}u,0x{original:x}u,0x{mcc:x}u,0x{mcc:x}u,"{name}"}}'
        if (ROOT/JOIN).read_text().count(expected)!=1:
            raise ValueError('Exact E0900 symbol join changed')
        keys[str(original)]=mcc
    return dict(rows=corpus(Xbe(xbe)),names=names,keys=keys)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--death-cycle',action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    if not a.xbe.is_file():return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    owner='death-cycle' if a.death_cycle else 'primary'
    controls=(DEATH_CONTROL,CONTROL) if a.death_cycle else (CONTROL,DEATH_CONTROL)
    report = dict(passed=False,production_bound=True,source_owner=owner,owned_scratch=str(scratch),retained_large_artifacts=[])
    names=(OWNER,DEATH_OWNER,SYMBOLS,JOIN,'src/gameplay/objects/native_tu15_flight_property_query_source_frame.hpp',
           'src/gameplay/objects/native_tu15_flight_property_query_source_owner.hpp',
           'src/gameplay/objects/native_tu15_death_cycle_flight_query_source_owner.hpp',
           'tests/native_tu15_flight_property_query_public_tests.cpp')
    before={name:sha(ROOT/name) for name in names}
    try:
        if sha(a.xbe) != PIN:raise ValueError('Wrong original TU1.5 image')
        report.update(source_before=before,linked_binary_sha256=sha(a.native),reference_xbe_sha256=PIN)
        data=scratch/'public-inputs.json';data.write_text(json.dumps(fixture(a.xbe),separators=(',',':')))
        report['input_sha256']=sha(data)
        env={k:v for k,v in os.environ.items() if not k.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        outputs={};hosts={};runs={}
        for label,override in (('default',{}),('own-reference',{controls[0]:'1'}),
                               ('other-copy-reference',{controls[1]:'1'})):
            stream=scratch/(label+'.stream');host=scratch/(label+'.host.jsonl')
            command=[str(a.native),str(data),str(stream),owner,str(host)]
            tested=subprocess.run(command,env={**env,**override},capture_output=True,text=True,timeout=1200)
            if tested.returncode:
                raise ValueError('Actual E0900 API '+label+' failed: '+(tested.stdout+tested.stderr)[-6000:])
            counts=json.loads(tested.stdout)
            expected=0 if label=='own-reference' else counts['normal']+counts['prefixes']
            if counts['normal']!=576 or not counts['prefixes'] or counts['native_entries']!=expected or counts['other_native_entries']:
                raise ValueError('Actual E0900 API did not exercise its expected route: '+str(counts))
            outputs[label]=stream
            hosts[label]=host
            runs[label]=dict(command=command,controls=override,result=counts,stream_sha256=sha(stream),
                             stream_bytes=stream.stat().st_size,host_stream_sha256=sha(host))
        report['runs']=runs
        report['guest_streams_equal']=all(filecmp.cmp(outputs['default'],value,shallow=False)
            for name,value in outputs.items() if name!='default')
        report['host_streams_equal']=all(filecmp.cmp(hosts['default'],value,shallow=False)
            for name,value in hosts.items() if name!='default')
        if not report['guest_streams_equal'] or not report['host_streams_equal']:
            raise ValueError('Actual E0900 public guest or host floating-point streams differ')
        report.update(passed=True,complete_streams_equal=True)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name) for name in names}
        if report['source_after']!=before:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())

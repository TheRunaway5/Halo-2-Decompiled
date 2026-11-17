#!/usr/bin/env python3
"""Compare the actual linked CombatAction API with its explicitly retained control."""
import argparse
import filecmp
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe
from validate_native_projectile_function_query_frame_tu15 import PIN, corpus
from validate_native_object_definition_mode_query_frame_tu15 import corpus as definition_inputs

ROOT = Path(__file__).resolve().parents[1]
OWNER = 'src/gameplay/units/source_combat_action_tu15_generated.cpp'
SYMBOLS = 'src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
JOIN = 'src/rendering/effects/source_object_function_symbols_tu15.inc'
CONTROL = 'HALO2_REFERENCE_PROJECTILE_FUNCTION_QUERY'
DEFINITION_CONTROL = 'HALO2_REFERENCE_OBJECT_DEFINITION_MODE_QUERY'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def fixture(xbe,definition_mode=False):
    symbols = re.findall(r'\bSymbol\{(0x[0-9a-f]+)u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/SYMBOLS).read_text())
    joined = re.findall(r'ObjectFunctionSymbol\{0x[0-9a-f]+u,0x[0-9a-f]+u,0x[0-9a-f]+u,(0x[0-9a-f]+)u,"([^"]+)"\}',(ROOT/JOIN).read_text())
    names = {}
    for key,name in [(mcc,name) for _,mcc,name in symbols]+joined:
        if names.setdefault(str(int(key,16)),name) != name:
            raise ValueError('Conflicting CombatAction symbol join')
    if len(symbols) != 10 or len(names) != 108:
        raise ValueError('CombatAction symbol join changed')
    keys = {str(int(original,16)):int(mcc,16) for original,mcc,_ in symbols[5:]}
    image=Xbe(xbe)
    return dict(rows=(definition_inputs if definition_mode else corpus)(image),names=names,keys=keys,
                definition_mode=definition_mode)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--definition-mode',action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    if not a.xbe.is_file():return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    report = dict(passed=False,production_bound=True,owned_scratch=str(scratch),retained_large_artifacts=[],
                  definition_mode=a.definition_mode)
    names=(OWNER,SYMBOLS,JOIN,'src/gameplay/objects/native_tu15_projectile_function_query_source_frame.hpp',
           'src/gameplay/objects/native_tu15_projectile_function_query_source_owner.hpp',
           'src/gameplay/objects/native_tu15_object_definition_mode_query_source_owner.hpp',
           'src/gameplay/objects/native_tu15_object_definition_mode_query_source_frame.hpp',
           'include/halo2/native_tu15_object_definition_mode_query_kernel.hpp',
           'tools/validate_native_object_definition_mode_query_frame_tu15.py',
           'tests/native_tu15_projectile_function_query_public_tests.cpp')
    before={name:sha(ROOT/name) for name in names}
    try:
        if sha(a.xbe) != PIN:raise ValueError('Wrong original TU1.5 image')
        report.update(source_before=before,linked_binary_sha256=sha(a.native),reference_xbe_sha256=PIN)
        data=scratch/'public-inputs.json';data.write_text(json.dumps(fixture(a.xbe,a.definition_mode),separators=(',',':')))
        report['input_sha256']=sha(data)
        env={k:v for k,v in os.environ.items() if not k.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        outputs={};runs={}
        own=DEFINITION_CONTROL if a.definition_mode else CONTROL
        other=CONTROL if a.definition_mode else DEFINITION_CONTROL
        for label,override in (('default',{}),('own-reference',{own:'1'}),('unrelated-reference',{other:'1'})):
            stream=scratch/(label+'.stream');command=[str(a.native),str(data),str(stream)]
            tested=subprocess.run(command,env={**env,**override},capture_output=True,text=True,timeout=1200)
            if tested.returncode:
                raise ValueError('Actual CombatAction API '+label+' failed: '+(tested.stdout+tested.stderr)[-6000:])
            counts=json.loads(tested.stdout)
            expected=0 if label=='own-reference' else counts['normal']+counts['prefixes']
            if counts['normal']!=384 or not counts['prefixes'] or counts['native_entries']!=expected or counts['other_native_entries']:
                raise ValueError('Actual CombatAction API did not exercise expected routes: '+str(counts))
            outputs[label]=stream
            runs[label]=dict(command=command,controls=override,result=counts,stream_sha256=sha(stream),stream_bytes=stream.stat().st_size)
        report['runs']=runs
        if any(not filecmp.cmp(outputs['default'],value,shallow=False) for name,value in outputs.items() if name!='default'):
            raise ValueError('Actual CombatAction public ABI/provider/observer/bank streams differ')
        report.update(passed=True,complete_streams_equal=True)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name) for name in names}
        if report['source_after']!=before:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Qualify a private public-facade delegation to the geometry leaf DATA core.

The complete existing public CPP remains the baseline. Only the two pinned
function bodies are replaced in owned scratch; the compose owner, adapter,
publication phases and public FP environment remain byte-for-byte unchanged.
"""
import argparse,copy,hashlib,json,os,re,subprocess,sys
from pathlib import Path
import validate_native_geometry_tu15 as original

ROOT=Path(__file__).resolve().parents[1]
CPP='src/physics/native_tu15_geometry.cpp'
CPP_PIN='6ddbdb31c2f109ca8748bec3edc74556d0ff72bc95b4120a603a7ccac889dc79'
BODY_PINS={
 'native_tu15_transform_transposed_direction':'3ea8b5da0583d29b62de864a61bf7991f049aa14dc36753762dccab7b14293e8',
 'native_tu15_transform_geometry_position':'2968d58699df5c2ee240543ea55fdb0f159b6c43c8ed8100096f48e9eebd043f',
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def public_candidate(text):
    if digest(text)!=CPP_PIN:raise ValueError('Complete public geometry baseline drift')
    output=text
    for name,core in(('native_tu15_transform_transposed_direction','transposed_direction'),('native_tu15_transform_geometry_position','transformed_position')):
        match=re.search(r'void '+name+r'\([^\n]+\)\{.*?\n\}',output,re.S)
        if not match or digest(match[0])!=BODY_PINS[name]:raise ValueError('Complete public geometry body drift '+name)
        old=match[0];start=old.index('    const auto');end=old.index('\n#else',start)
        after=old[:start]+'    native_geometry_leaf_detail::'+core+'(s);'+old[end:]
        output=output[:match.start()]+after+output[match.end():]
    include='#include "halo2/native_tu15_geometry_leaf_kernel.hpp"\n'
    output=include+output
    restored=output[len(include):]
    for name in BODY_PINS:
        old=re.search(r'void '+name+r'\([^\n]+\)\{.*?\n\}',text,re.S)[0]
        current=re.search(r'void '+name+r'\([^\n]+\)\{.*?\n\}',restored,re.S)
        restored=restored[:current.start()]+old+restored[current.end():]
    if restored!=text:raise ValueError('Public geometry overlay changed unrelated bytes')
    return output

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--compiler',action='append');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[])
    names=(CPP,'include/halo2/native_tu15_geometry.hpp','include/halo2/native_tu15_geometry_leaf_kernel.hpp','tests/native_tu15_geometry_tests.cpp','tools/validate_native_geometry_leaf_value_tu15.py','tools/validate_native_geometry_tu15.py','tools/observe_native_geometry_tu15.cpp')
    pins={name:original.sha(ROOT/name)for name in names}
    try:
        source=(ROOT/CPP).read_text();candidate=public_candidate(source);private=scratch/'geometry-shared.cpp';private.write_text(candidate)
        if original.sha(original.NEW_DEFAULT)!=original.NEW_PIN:raise ValueError('Original TU15 image drift')
        image=original.Xbe(original.NEW_DEFAULT);metadata=original.metadata(image);owned=original.prepare(image,metadata,scratch)
        rows=original.physical(owned,original.inputs(),'complete');prefixes=[]
        for seed in(row for row in rows if row['name'].startswith(('fault_seed','alias_fault_seed'))):
            for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                for ordinal in range(len(seed[key])):
                    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefixes),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal),pure_typed=False);prefixes.append(row)
        rows+=original.physical(owned,prefixes,'prefix')
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=original.NEW_PIN,required_backend=original.BACKEND,body_sites=metadata['body_sites'],listing_sha256=metadata['listing_sha256'],cases=rows),separators=(',',':')))
        for module in tuple(sys.modules.values()):
            name=getattr(module,'__file__',None)
            if name:
                path=Path(name).resolve()
                if path.is_relative_to(ROOT/'tools')and path.is_file():pins[str(path.relative_to(ROOT))]=original.sha(path)
        results=[]
        for compiler in args.compiler or('g++','clang++'):
            pair=[]
            for label,cpp in(('original-public',ROOT/CPP),('shared-private',private)):
                binary=scratch/(Path(compiler).name+'-'+label)
                command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(ROOT/'tests/native_tu15_geometry_tests.cpp'),str(cpp),'-o',str(binary)]
                if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
                compiled=subprocess.run(command,capture_output=True,text=True)
                if compiled.returncode:raise ValueError('Actual public geometry CPP compile failed: '+compiled.stderr[-7000:])
                tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=180)
                pair.append(dict(label=label,command=command,exit_code=tested.returncode,stdout=tested.stdout,stderr=tested.stderr[-4000:]))
                if tested.returncode:raise ValueError('Public geometry original-backed comparison failed '+label+': '+(tested.stdout+tested.stderr)[-7000:])
            if pair[0]['stdout']!=pair[1]['stdout']:raise ValueError('Original/shared public geometry acceptance counters differ')
            results.append(dict(compiler=compiler,variants=pair))
        report.update(passed=True,physical_cases=len(rows),image_sha256=original.NEW_PIN,body_sites=metadata['body_sites'],listing_sha256=metadata['listing_sha256'],candidate_cpp_sha256=digest(candidate),original_public_body_pins=BODY_PINS,compilers=results,
            scope='Existing complete public geometry CPP and two-body private shared-DATA facade each pass the genuine original physical value, ordered read/store, FP, alias, alignment, exception-prefix and host-x87 tests. The unchanged compose owner is exercised as a preservation control. No public/canonical/default source is edited, and hostile GP/FP provider mutation remains outside this physical readonly-provider value proof.')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_before']=pins;report['source_after']={name:original.sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

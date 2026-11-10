#!/usr/bin/env python3
"""Compare actual public queue APIs with their complete pinned predecessors."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import native_bsp_queue_find_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
UNITS={'instance':'src/rendering/world/source_bsp_admission.cpp','object':'src/gameplay/objects/source_object_admission.cpp'}
FILES=(*UNITS.values(),'tools/native_bsp_queue_find_replacements.py','include/halo2/native_tu15_id_search_kernel.hpp','tests/native_tu15_bsp_queue_find_value_tests.cpp','tests/native_tu15_bsp_queue_find_value_reference.inc','tools/validate_native_bsp_queue_find_values.py')
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:binding.digest((ROOT/name).read_text())for name in FILES};reference=(ROOT/'tests/native_tu15_bsp_queue_find_value_reference.inc').read_text();copies=[]
    for name,unit in UNITS.items():
        old=binding.public_body(reference,name)
        if binding.digest(old)!=binding.PUBLIC[name][3]:raise ValueError('Complete public lookup predecessor drift')
        owner,arg,rows,_=binding.PUBLIC[name]
        signature='const SourceBspAdmissionRecord *'+owner+'::'+name+'('+arg+' id) const'
        reference=reference.replace(signature,'const SourceBspAdmissionRecord *reference_'+name+'(const std::vector<SourceBspAdmissionRecord>&'+rows+','+arg+' id)',1)
        source=(ROOT/unit).read_text();bound=binding.replace_public(source,name)
        if args.require_binding and bound!=source:raise ValueError('Actual public lookup default is not installed')
        if binding.replace_public(bound,name)!=bound:raise ValueError('Public lookup binder is not idempotent')
        target=scratch/(name+'.cpp');target.write_text(bound);copies.append(str(target))
    (scratch/'bsp_queue_find_value_reference.inc').write_text(reference)
    binary=scratch/'values';command=[args.compiler,'-std=c++20','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_bsp_queue_find_value_tests.cpp'),*copies,'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Public queue lookup compile failed: '+compiled.stderr[-7000:])
    tested=subprocess.run([str(binary)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    if pins!={name:binding.digest((ROOT/name).read_text())for name in FILES}:raise ValueError('Public lookup source changed during proof')
    report=dict(passed=tested.returncode==0,owned_scratch=str(scratch),source_sha256=pins,compiler=args.compiler,sanitizers=args.sanitizers,scope='Actual public source APIs and complete pinned predecessor value regression; raw ABI independently compared with physical i386',retained_large_artifacts=[],diagnostic=(tested.stdout+tested.stderr)[-6000:])
    if tested.returncode==0:report['values']=json.loads(tested.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return tested.returncode
if __name__=='__main__':raise SystemExit(main())

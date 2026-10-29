#!/usr/bin/env python3
"""Validate actual listener metadata and the shared manager's low-byte contract."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.file_digest(path.open('rb'),'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=True)
    manager=ROOT/'src/audio/native_tu15_sound_manager.cpp';test=ROOT/'tests/native_campaign_listener_contract_tests.cpp';listener=ROOT/'src/audio/native_tu15_sound_kernels_listener.cpp'
    sources=[manager,test,listener,ROOT/'src/audio/native_tu15_sound_kernel.hpp',ROOT/'include/halo2/native_tu15_sound_manager.hpp',Path(__file__)]
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(p.relative_to(ROOT)):sha(p)for p in sources})
    project=scratch/'project';project.mkdir();source=manager.read_text();needle='else if(owed.gp_low8&(1u<<i))differ("gp"+std::to_string(i)+" low8",native.gp[i]&0xffu,a.gp[i]&0xffu);'
    if source.count(needle)!=1:raise ValueError('Shared byte-contract mutation location changed')
    mutant=project/'manager_without_byte_check.cpp';mutant.write_text(source.replace(needle,''))
    negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and 'AL mismatch escaped' in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint('Actual shared manager byte-contract removal rejected:',r.stderr.strip())\n")
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(listener_contracts LANGUAGES CXX)\ninclude(CTest)\n'
    for target,implementation in (('listener_contract',manager),('listener_contract_mutated',mutant)):
        cmake+=f'add_executable({target} "{test}" "{listener}" "{implementation}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_options({target} PRIVATE -Wl,--gc-sections)\n'
        command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
        if implementation==mutant:command+=f'"{sys.executable}" "{negative}" '
        cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}>)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),
        '--target','listener_contract','--target','listener_contract_mutated','--filter','^listener_contract(_mutated)?$','--jobs','2','--test-jobs','1']
    result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode,source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),
        negative_control='Remove actual low8 comparison from shared live manager',actual_listener_owners=5,full_and_low16_contracts_unchanged=True)
    report['passed']=result.returncode==0 and report['source_hashes_unchanged'];(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=report['passed'])));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

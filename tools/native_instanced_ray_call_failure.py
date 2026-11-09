"""Retained-provider memCALL receipt; distinct from hardware instruction stops."""
import hashlib
import json
import re
import subprocess
import sys
from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN

BODY_PIN='051c773a4b00e5564355f1a5d2fb2bbd070f642d2984ac5b83d2c4518217f35f'

def qualify(scratch,output,rows,io):
    source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
    body=re.search(r'static void run_24def0\(WorldState &s\) \{(.*?)\n\}\n\nstatic void run_24df20',source,re.S)
    assert body and hashlib.sha256(body[1].encode()).hexdigest()==BODY_PIN,'Complete24DEF0 retained source drift'
    project=scratch/'retained-call-failure';project.mkdir()
    (project/'instanced_callback_forward_reference.inc').write_text('static void canonical_forward(WorldState&s){'+body[1]+'\n}\n')
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
    cmake=f'''cmake_minimum_required(VERSION 3.20)
project(retained_call_failure LANGUAGES CXX)
include(CTest)
add_executable(retained_call_failure "{ROOT/'tests/native_tu15_instanced_ray_call_failure_tests.cpp'}")
target_include_directories(retained_call_failure PRIVATE "{project}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(retained_call_failure PRIVATE cxx_std_20)
target_compile_options(retained_call_failure PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(retained_call_failure PRIVATE -fsanitize=address,undefined)
add_test(NAME retained_call_failure COMMAND retained_call_failure "{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
             '--output',str(output/'retained-call-failure'),'--target','retained_call_failure','--filter','^retained_call_failure$']
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert result.returncode==0,'Retained memCALL policy receipt failed: '+result.stdout[-7000:]
    return dict(passed=True,whole_retained_forward_body_sha256=BODY_PIN,source_scope='Exact retained24DEF0 with only fixture symbol renamed; actual target read followed by provider stack-write failure',
                raw_gp4_policy='Commit after provider success',canonical_gp4_policy='Existing State.push predecrements before provider',
                target_read_and_all_other_abi_memory_preserved=True,full_exception_parity_claim=False,physical_post_target_provider_failure_claim=False,supplied_child_outcomes=False)

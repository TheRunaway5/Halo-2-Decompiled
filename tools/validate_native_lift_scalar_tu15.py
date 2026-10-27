#!/usr/bin/env python3
"""Compare the typed TU1.5 lift scalar with untouched original i386 execution.

Run inside tools/run_test.py. Unicorn discovers only the sparse input footprint;
the untouched original body on the physical i386 backend supplies every result.
All executable images, raw banks, fixtures and builds are owned scratch. Only
bounded hashes, commands and numerical coverage reports survive. Positive
exponents prove the actual missing power boundary; they do not inject a result.
"""
import argparse
import copy
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_vehicle_motion_math_20261011 import capture
from oracle_vehicle_motion_math_hardware_20261011 import execute as hardware
from export_mp_physics_contact_lifecycle import segments

BASE=0x1000000
STACK=BASE+0x10000
STOP=BASE+0xf000
OUT=BASE+0x1000
FIELDS=('mass','target','acceleration','velocity','velocity_limit','position','exponent','time_step')
SOURCES=('include/halo2/source_lift_scalar.hpp','include/halo2/source_lift_scalar_storage_tu15.hpp','src/physics/source_lift_scalar.cpp','tests/source_lift_scalar_tu15_tests.cpp')

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def bits(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]

def inputs(quick):
    base=dict(zip(FIELDS,map(bits,(2.,2.,3.,0.5,2.,0.25,0.,1./30.))))
    base.update(allow_reverse=False,scale_by_mass=False,force_acceleration=False)
    patterns=[]
    def add(**values):
        row=dict(base);row.update(values);patterns.append(row)
    # Reach acceleration, braking sqrt, behind-target, reverse, mass and forced branches.
    for reverse,scale,forced in itertools.product((False,True),repeat=3):
        add(allow_reverse=reverse,scale_by_mass=scale,force_acceleration=forced)
        add(allow_reverse=reverse,scale_by_mass=scale,force_acceleration=forced,target=bits(0.26))
        add(allow_reverse=reverse,scale_by_mass=scale,force_acceleration=forced,target=bits(-2.))
        add(allow_reverse=reverse,scale_by_mass=scale,force_acceleration=forced,acceleration=bits(-3.))
    # Raw IEEE words keep both zero signs, payloads and signaling NaNs intact.
    edges=(0,0x80000000,1,0x80000001,0x007fffff,0x807fffff,0x00800000,0x80800000,
           0x3f800000,0xbf800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,
           0x7fc12345,0xffc12345,0x7f812345,0xff812345)
    if quick:edges=(0,0x80000000,1,0x7f800000,0x7fc12345,0x7f812345)
    for field,word in itertools.product(FIELDS,edges):
        add(**{field:word,'scale_by_mass':True})
    epsilon=struct.unpack('<I',Xbe(NEW_DEFAULT).read(0x46a180,4))[0]
    for field in ('mass','acceleration','exponent'):
        for word in (epsilon-1,epsilon,epsilon+1):add(**{field:word,'scale_by_mass':True})
    for target,exponent in itertools.product((0,0x80000000,bits(-2.),0x7fc12345,0x7f812345),(bits(0.5),bits(2.),0x7f800000)):
        add(target=target,exponent=exponent)
    # Distinct payloads expose operand reordering, and x87 invalid sqrt is real.
    add(target=0x7fc12345,position=0x7fc54321,velocity=0xffc22222)
    add(acceleration=0x7fc12345,time_step=0x7fc54321,velocity=0xffc22222)
    add(target=bits(0.),position=bits(0.),velocity=bits(-1.))
    controls=list(itertools.product((0,2,3),range(4),(0,0x40,0x8000,0x8040)))
    if quick:controls=((2,0,0),(0,1,0x40),(3,2,0x8000),(3,3,0x8040))
    result=[]
    for n,(row,(precision,rounding,modes)) in enumerate(itertools.product(patterns,controls)):
        item=dict(row,x87_control_word=0x7f|(precision<<8)|(rounding<<10),
                  mxcsr=0x1f80|(rounding<<13)|modes|(n%64),x87_exception_flags=(n*7)%64,
                  initial_top=n%8)
        item['initial_condition_bits']=((n&7)<<8)|((n&8)<<11)
        result.append(item)
    return result

def original(rows):
    assert sha(NEW_DEFAULT)==NEW_PIN,'TU1.5 original hash changed'
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    starts={int(row.split('\t')[0],16) for row in (ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').read_text().splitlines()[1:]}
    trace=_trace(code,0x1da530,starts)
    assert not trace.tables and len(trace.code)==134
    thunk=code.at(0x37cde8)
    assert thunk.mnemonic=='jmp' and thunk.operands[0].imm==0x37cdea
    power_abi=_trace(code,0x37cdea,starts)
    assert not power_abi.tables and any(code.at(p).mnemonic=='call' and code.at(p).operands[0].imm==0x37ce0c for p in power_abi.code)
    sites=set(trace.code)|set(power_abi.code)|{thunk.address}
    paired=dict(entries={'001da530':0x1da530},address_map={},instruction_map={f'{p:08x}':p for p in sites})
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(BASE,0x120000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    constants=[struct.unpack('<I',image.read(p,4))[0] for p in (0x46a180,0x46a0d0,0x46a0cc,0x44aac8,0x45695c)]
    state={}
    def word(p,n):g.mem_write(p,struct.pack('<I',n))
    def instruction(_g,p,_n,_u):
        if p not in sites:state['exit_entry']=p;g.emu_stop()
        else:state['instructions'].append(p)
    def read(_g,_a,p,n,_v,_u):
        state['events'].append(dict(kind='R',instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,raw=bytes(g.mem_read(p,n)).hex()))
    def write(_g,_a,p,n,value,_u):
        state['events'].append(dict(kind='W',instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,
            before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    cases=[]
    for n,row in enumerate(rows):
        g.mem_write(STACK-0x1000,bytes(0x2000));word(0x51df54,BASE);word(BASE+4,row['time_step']);word(OUT,0xdeadbeef)
        params=(STOP,int(row['scale_by_mass']),*[row[k] for k in FIELDS[:-1]],int(row['force_acceleration']),OUT)
        for offset,value in enumerate(params):word(STACK+offset*4,value)
        for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI'):
            g.reg_write(getattr(R,'UC_X86_REG_'+name),STACK if name=='ESP' else int(row['allow_reverse']) if name=='EBX' else 0)
        for k in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{k}'),0)
        g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,row['x87_control_word']);g.reg_write(R.UC_X86_REG_MXCSR,row['mxcsr'])
        g.reg_write(R.UC_X86_REG_FPSW,(row['initial_top']<<11)|row['x87_exception_flags']|row.get('initial_condition_bits',0));g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        state.update(events=[],instructions=[],exit_entry=None);entry=capture(g)
        g.emu_start(0x1da530,0,count=1000)
        assert state['exit_entry'] in (STOP,0x37ce0c),(n,state['exit_entry'])
        first={};current={}
        for event in state['events']:
            p=event['address'];raw=bytes.fromhex(event['raw'] if event['kind']=='R' else event['before'])
            for k,value in enumerate(raw):
                if p+k not in current:first[p+k]=value;current[p+k]=value
                assert current[p+k]==value
            if event['kind']=='W':
                for k,value in enumerate(bytes.fromhex(event['written'])):current[p+k]=value
        cases.append(dict(id=n,entry=0x1da530,entry_abi=entry,exit=capture(g),exit_entry=state['exit_entry'],return_address=STOP,
            complete=state['exit_entry']==STOP,x87_defined_status_mask=0x383f,
            initial_banks=[dict(address=p,hex=b.hex()) for p,b in segments(first)],final_banks=[dict(address=p,hex=b.hex()) for p,b in segments(current)],
            instructions=state['instructions'],writes=[e for e in state['events'] if e['kind']=='W'],estimates=[],timestamps=[],packets=[],native_input=row))
    return dict(schema=1,image='1.5',cases=cases,scalar_constants=constants,native_expected_data_used=False),paired

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--quick',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,parity_target='English TU1.5 only',quick=args.quick,native_semantic_kernel=True,original_execution_offline_only=True,
                positive_exponent_native_power_proven=False,live_force_integration_proven=False,commands=[],source_sha256={p:sha(ROOT/p) for p in SOURCES},validator_sha256=sha(__file__),original_sha256=NEW_PIN)
    def save():
        report['retained_large_artifacts']=[]
        (args.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    save()
    try:
        fixture,paired=original(inputs(args.quick));seen={p for c in fixture['cases'] for p in c['instructions']}
        observed=copy.deepcopy(paired);observed['instruction_map']={p:q for p,q in paired['instruction_map'].items() if q in seen}
        physical=hardware(fixture,observed,NEW_DEFAULT,scratch/'hardware')
        hardware_cases={c['id']:c for c in physical['cases']}
        for case in fixture['cases']:
            for key in ('exit','final_banks','instructions','writes'):case[key]=hardware_cases[case['id']][key]
        fixture['oracle_backend']='original_i386_hardware_single_step'
        oracle=scratch/'scalar-original.json';oracle.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(cases=len(fixture['cases']),complete_original_calls=sum(c['complete'] for c in fixture['cases']),required_power_prefixes=sum(not c['complete'] for c in fixture['cases']),
            original_body_sites=134,covered_original_body_sites=len({p for p in seen if 0x1da530<=p<=0x1da700}),
            sqrt_original_calls=sum(0x1da641 in c['instructions'] for c in fixture['cases']),
            scalar_constants=[dict(address=p,raw_uint32=value) for p,value in zip((0x46a180,0x46a0d0,0x46a0cc,0x44aac8,0x45695c),fixture['scalar_constants'])],
            physical_instructions=physical['single_stepped_original_instructions'],cpu_model=physical['cpu_model'],oracle_sha256=sha(oracle),
            hardware_tool_sha256=physical['tool_sha256'],trace_observer_sha256=physical['trace_observer_sha256'],paired_instruction_bytes_changed=physical['paired_instruction_bytes_changed'],
            rounding_modes=4,x87_precision_bits=[24,53,64],sse_denormals_are_zero_and_flush_to_zero=True,numeric_status_checked=True,
            exact_return_bits_checked=True,original_x87_stack_domain='empty stack, all eight initial TOP positions',masked_exceptions_only=True,
            host_environment_restored_on_return_and_dependency_throw=True,required_native_power_entry=0x37ce0c)
        source=scratch/'source';shutil.copytree(ROOT/'include',source/'include')
        for name in SOURCES:
            target=source/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/name).read_bytes());assert sha(target)==report['source_sha256'][name]
        (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(native_lift_scalar_tu15 LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json REQUIRED)
add_executable(native_lift_scalar_tu15_tests src/physics/source_lift_scalar.cpp tests/source_lift_scalar_tu15_tests.cpp)
target_compile_features(native_lift_scalar_tu15_tests PRIVATE cxx_std_20)
target_include_directories(native_lift_scalar_tu15_tests PRIVATE include)
target_compile_options(native_lift_scalar_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_lift_scalar_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_lift_scalar_tu15 COMMAND native_lift_scalar_tu15_tests "'''+str(oracle)+'")\n')
        def validate(label):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--output',str(args.output/label),'--scratch-root',str(scratch/'builds'),'--target','native_lift_scalar_tu15_tests','--filter','^native_lift_scalar_tu15$','--jobs','2','--test-jobs','1']
            report['commands'].append(command);save();return subprocess.run(command,check=False).returncode
        assert validate('native')==0,'Native scalar differs from untouched original; inspect native/tests.log'
        # A private arithmetic error must be rejected by the independent oracle.
        native=source/'src/physics/source_lift_scalar.cpp';text=native.read_text();assert '"addss %1,%0"' in text
        native.write_text(text.replace('"addss %1,%0"','"subss %1,%0"',1))
        negative=validate('negative-control');log=(args.output/'negative-control/tests.log').read_text()
        assert negative!=0 and 'TU1.5 lift scalar result mismatch' in log,'Independent result oracle failed to reject the arithmetic mutation'
        report.update(passed=True,negative_control_rejected=True,negative_control='private addss-to-subss semantic arithmetic mutation',scratch_cleanup_owner=str(scratch))
    except BaseException as error:
        report['error']=str(error);raise
    finally:save()
    print(json.dumps(report,sort_keys=True))

if __name__=='__main__':main()

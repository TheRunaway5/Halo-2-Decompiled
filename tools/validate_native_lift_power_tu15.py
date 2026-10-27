#!/usr/bin/env python3
"""Qualify native finite/zero TU1.5 lift power against untouched i386 bodies.

NaN/infinite argument and CRT range-reporting owners remain required. Original
execution is offline only. All original images, banks, fixtures and native test
builds live under the enclosing run_test.py owner's disposable scratch.
"""
import argparse
import copy
import itertools
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_vehicle_motion_math_20261011 import capture
from oracle_vehicle_motion_math_hardware_20261011 import execute as hardware
import oracle_vehicle_motion_math_hardware_20261011 as hardware_module
from export_mp_physics_contact_lifecycle import segments
from validate_native_lift_scalar_tu15 import original as scalar_prefixes,sha,bits,BASE,STACK,STOP,OUT,FIELDS

SOURCES=('include/halo2/source_lift_scalar.hpp','src/physics/source_lift_scalar.cpp',
 'include/halo2/source_lift_scalar_storage_tu15.hpp',
 'include/halo2/native_lift_scalar_consumer_tu15.hpp','src/physics/native_lift_scalar_consumer_tu15.cpp','tests/native_lift_scalar_consumer_tu15_tests.cpp',
 'include/halo2/source_power_special_tu15.hpp','src/physics/source_power_special_tu15.cpp',
 'include/halo2/source_math_error_tu15.hpp','src/physics/source_math_error_tu15.cpp',
 'include/halo2/source_lift_power_tu15.hpp','src/physics/source_lift_power_tu15.cpp','tests/source_lift_power_tu15_tests.cpp')
OWNERS=(0x1da530,0x37cde8,0x37cdea,0x37ce0c,0x37cfd1,0x332084,0x332099,0x3320c9,0x332122,0x33216d,0x332030,0x37d948,
 0x3334f0,0x32e7b5,0x32ed0d,0x32ece9,0x32e9a2,0x32f1e5,0x330179,0x32ec1e,0x32eb99,0x33d1e8)
FS=BASE+0x2000;THREAD=BASE+0x3000;TLS_TABLE=BASE+0x4000;TLS_SLOT=BASE+0x5000;THREAD_DATA=BASE+0x6000;ERRNO=THREAD_DATA+8;GDT=BASE+0x100000

def inputs(quick):
    bases=(0,0x80000000,1,0x80000001,0x007fffff,0x00800000,bits(0.125),bits(0.5),0x3f7fffff,bits(1.),0x3f800001,
       bits(2.),bits(3.),bits(4.),bits(10.),bits(1.e20),0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc12345,0x7f812345,0xff812345)
    exponents=(bits(0.5),bits(1.),bits(1.5),bits(2.),bits(3.),bits(1./3.),bits(8.),bits(8.5),bits(32.),bits(128.),bits(1023.),bits(1074.),bits(1075.),0x7f7fffff,0x7f800000)
    if quick:bases=(0,0x80000000,1,bits(0.5),bits(1.),bits(2.),bits(3.),0x7f7fffff,0x7f800000,0x7fc12345);exponents=(bits(0.5),bits(1.),bits(3.),bits(128.),bits(1074.),0x7f800000)
    accelerations=(bits(3.),bits(-3.),0,0x80000000,1,0x7f800000,0x7fc54321,0x7f854321)
    controls=list(itertools.product((0,2,3),range(4),(0,0x40,0x8000,0x8040)))
    if quick:controls=((2,0,0),(0,1,0x40),(3,2,0x8000),(3,3,0x8040))
    rows=[]
    for pattern,(base,exponent) in enumerate(itertools.product(bases,exponents)):
        for precision,rounding,modes in controls:
            n=len(rows);row=dict(zip(FIELDS,map(bits,(2.,2.,3.,0.5,2.,0.25,0.,1./30.))))
            row.update(target=base,exponent=exponent,acceleration=accelerations[pattern%len(accelerations)],allow_reverse=bool(pattern&1),scale_by_mass=bool(pattern&2),force_acceleration=bool(pattern&4),
                x87_control_word=0x7f|(precision<<8)|(rounding<<10),mxcsr=0x1f80|(rounding<<13)|modes|(n%64),x87_exception_flags=(n*7)%64,initial_top=n%8)
            row.update(initial_errno=0x12340000+n,thread_local=bool(n%2),tls_index=n%3)
            row['initial_condition_bits']=((n&7)<<8)|((n&8)<<11)
            rows.append(row)
    return rows

def original(rows):
    fixture,_=scalar_prefixes(rows)
    assert all(not c['complete'] and c['exit_entry']==0x37ce0c for c in fixture['cases'])
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    starts={int(line.split('\t')[0],16) for line in (ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').read_text().splitlines()[1:]}
    sites=set();bodies=[]
    for entry in OWNERS:
        trace=_trace(code,entry,starts);assert not trace.tables
        sites.update(trace.code);bodies.append(dict(entry=entry,instructions=len(trace.code),instruction_sha256=__import__('hashlib').sha256(b''.join(code.at(p).bytes for p in sorted(trace.code))).hexdigest()))
    assert image.read(0x48d4f4,4)==bytes(4),'Original CRT checking mode changed'
    paired=dict(entries={f'{p:08x}':p for p in OWNERS},address_map={},instruction_map={f'{p:08x}':p for p in sites})
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(BASE,0x120000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    def segment(base,access):return struct.pack('<II',0xffff|((base&0xffff)<<16),((base>>16)&255)|(access<<8)|0xcf0000|(base&0xff000000))
    g.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93));g.reg_write(R.UC_X86_REG_GDTR,(0,GDT,0xfff,0))
    for register,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):g.reg_write(register,value)
    assert g.reg_read(R.UC_X86_REG_FS_BASE)==FS
    state={}
    def on_code(_g,p,_n,_u):
        if p not in sites:state['exit_entry']=p;g.emu_stop()
        else:state['instructions'].append(p)
    def on_read(_g,_a,p,n,_v,_u):state['events'].append(dict(kind='R',instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,raw=bytes(g.mem_read(p,n)).hex()))
    def on_write(_g,_a,p,n,value,_u):state['events'].append(dict(kind='W',instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    g.hook_add(UC_HOOK_CODE,on_code);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write)
    for case in fixture['cases']:
        g.mem_write(STACK-0x1000,bytes(0x2000));g.mem_write(OUT,bytes(4));
        row=case['native_input'];params=(STOP,int(row['scale_by_mass']),*[row[k] for k in FIELDS[:-1]],int(row['force_acceleration']),OUT)
        for offset,value in enumerate(params):g.mem_write(STACK+offset*4,struct.pack('<I',value))
        for bank in case['initial_banks']:g.mem_write(bank['address'],bytes.fromhex(bank['hex']))
        g.mem_write(0x51df54,struct.pack('<I',BASE));g.mem_write(BASE+4,struct.pack('<I',case['native_input']['time_step']))
        records=((FS+0x24,0,1),(FS+0x28,THREAD,4),(THREAD+0x28,int(row['thread_local']),4),(0x48c920,row['tls_index'],4),
            (FS+4,TLS_TABLE,4),(TLS_TABLE+4*row['tls_index'],TLS_SLOT,4),(TLS_SLOT+0xc,THREAD_DATA,4),(0x573840,THREAD_DATA,4),(ERRNO,row['initial_errno'],4))
        for p,value,size in records:g.mem_write(p,value.to_bytes(size,'little'))
        entry=case['entry_abi'];entry['fs_base']=FS
        for n,name in enumerate(('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')):g.reg_write(getattr(R,'UC_X86_REG_'+name),entry['gp'][n])
        for n in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),int(entry['xmm'][n],16))
        g.reg_write(R.UC_X86_REG_EFLAGS,entry['eflags']);g.reg_write(R.UC_X86_REG_FPCW,entry['x87_control_word']);g.reg_write(R.UC_X86_REG_MXCSR,entry['mxcsr']);g.reg_write(R.UC_X86_REG_FPSW,entry['x87_status']);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        state.update(events=[],instructions=[],exit_entry=None)
        try:g.emu_start(0x1da530,0,count=5000)
        except Exception as error:raise RuntimeError(dict(case=case['id'],input=case['native_input'],eip=hex(g.reg_read(R.UC_X86_REG_EIP)),last=state['events'][-5:])) from error
        assert state['exit_entry']==STOP,(case['id'],hex(state['exit_entry']),state['instructions'][-10:])
        first={};current={}
        for event in state['events']:
            p=event['address'];raw=bytes.fromhex(event['raw'] if event['kind']=='R' else event['before'])
            for n,value in enumerate(raw):
                if p+n not in current:first[p+n]=value;current[p+n]=value
                assert current[p+n]==value,(case['id'],event)
            if event['kind']=='W':
                for n,value in enumerate(bytes.fromhex(event['written'])):current[p+n]=value
        # Physical execution can legitimately take a different range branch
        # from Unicorn's transcendental approximation. Supply every declared
        # scalar input even when the emulator stops before its first read.
        declared={}
        for offset,value in enumerate(params):
            for n,b in enumerate(struct.pack('<I',value)):declared[STACK+offset*4+n]=b
        for p,raw in ((0x51df54,struct.pack('<I',BASE)),(BASE+4,struct.pack('<I',row['time_step'])),(OUT,bytes(4))):
            for n,b in enumerate(raw):declared[p+n]=b
        for p,value,size in records:
            for n,b in enumerate(value.to_bytes(size,'little')):declared[p+n]=b
        for p,value in declared.items():
            if p in first:assert first[p]==value
            else:first[p]=value
        case.update(exit=capture(g),exit_entry=state['exit_entry'],complete=state['exit_entry']==STOP,
            initial_banks=[dict(address=p,hex=b.hex()) for p,b in segments(first)],final_banks=[dict(address=p,hex=b.hex()) for p,b in segments(current)],
            instructions=state['instructions'],writes=[e for e in state['events'] if e['kind']=='W'])
    return fixture,paired,bodies

def original_hardware(fixture,paired,directory):
    # The shared harness assumes emulator and hardware stop owners agree. That
    # assertion is not a numerical oracle: recover the already captured physical
    # results when the real processor discovers a different admitted exit.
    assert all(c['entry_abi']['fs_base']==FS for c in fixture['cases'])
    # Install an actual Linux LDT selector for the declared FS base. The
    # original TLS instructions are unchanged; this is test-harness routing.
    original_bridge=hardware_module.ASM
    assert original_bridge.count('setup:\n')==1
    hardware_module.ASM=original_bridge.replace('setup:\n',f'''setup:
 mov $123,%eax
 mov $1,%ebx
 mov $fs_descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js failure
 mov $7,%eax
 mov %ax,%fs
''')+f'\n.align 4\nfs_descriptor: .long 0,{FS},0xfffff,0x51\n'
    mismatched=False
    try:return hardware(fixture,paired,NEW_DEFAULT,directory)
    except AssertionError as error:
        detail=error.args[0] if error.args else None
        assert isinstance(detail,tuple) and len(detail)==3 and detail[1] in (STOP,0x3334f0,0x37d948),error
        assert (directory/'output.bin').is_file() and (directory/'trace.json').is_file(),error
        mismatched=True
    raw=(directory/'output.bin').read_bytes();tracing=json.loads((directory/'trace.json').read_text());traces={c['id']:c for c in tracing['cases']}
    results=[];offset=0;different=[]
    for case in fixture['cases']:
        assert offset+280<=len(raw);record=raw[offset:offset+280];offset+=280
        exit=copy.deepcopy(case['exit']);exit['gp']=list(struct.unpack_from('<8I',record));exit['eflags'],exit['mxcsr']=struct.unpack_from('<II',record,32)
        env=record[40:148];exit['x87_control_word']=struct.unpack_from('<H',env)[0];exit['x87_status']=struct.unpack_from('<H',env,4)[0];exit['x87_tag']=struct.unpack_from('<H',env,8)[0]
        top=(exit['x87_status']>>11)&7;exit['x87_values']=[env[28+n*10:38+n*10].hex() for n in range(8) if (exit['x87_tag']>>(2*((top+n)&7)))&3!=3]
        exit['xmm']=[f'{int.from_bytes(record[148+n*16:164+n*16],"little"):032x}' for n in range(8)]
        stop=struct.unpack_from('<I',record,276)[0];assert stop in (STOP,0x3334f0,0x37d948)
        if stop!=case['exit_entry']:different.append(dict(id=case['id'],emulator=case['exit_entry'],physical=stop))
        banks=[]
        for bank in case['initial_banks']:
            size=len(bytes.fromhex(bank['hex']));banks.append(dict(address=bank['address'],hex=raw[offset:offset+size].hex()));offset+=size
        results.append(dict(id=case['id'],exit=exit,final_banks=banks,exit_entry=stop,instructions=traces[case['id']]['instructions'],writes=traces[case['id']]['writes']))
    assert mismatched and offset==len(raw) and different
    return dict(cases=results,tool_sha256=sha(ROOT/'tools/oracle_vehicle_motion_math_hardware_20261011.py'),trace_observer_sha256=sha(ROOT/'tools/vehicle_motion_math_hardware_trace_20261011.cpp'),
        single_stepped_original_instructions=tracing['single_stepped_original_instructions'],paired_instruction_bytes_changed=False,
        cpu_model=next(line.split(':',1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')),
        emulator_physical_exit_disagreements=different)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--quick',action='store_true');parser.add_argument('--consumer',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,parity_target='English TU1.5 only',quick=args.quick,native_semantic_kernel=True,full_power_parity_proven=False,live_force_integration_proven=False,
        original_execution_offline_only=True,required_native_services=['thread errno cell'],commands=[],source_sha256={p:sha(ROOT/p) for p in SOURCES},validator_sha256=sha(__file__),original_sha256=NEW_PIN)
    def save():(args.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    save()
    try:
        fixture,paired,bodies=original(inputs(args.quick));seen={p for c in fixture['cases'] for p in c['instructions']}
        physical=original_hardware(fixture,paired,scratch/'hardware');physical_cases={c['id']:c for c in physical['cases']}
        for case in fixture['cases']:
            for key in ('exit','final_banks','instructions','writes'):case[key]=physical_cases[case['id']][key]
            case['emulator_exit_entry']=case['exit_entry'];case['exit_entry']=physical_cases[case['id']]['exit_entry'];case['complete']=case['exit_entry']==STOP
        # The original ELF maps the authenticated readonly constants even if
        # Unicorn took another arithmetic branch while discovering its sparse
        # footprint. Add only scalar absolute operands actually loaded by the
        # physical original. This is original program data, never a callee result.
        code=_Code(Xbe(NEW_DEFAULT),'.text');image=Xbe(NEW_DEFAULT);supplements=[]
        scalar_loads={0x1da537:0x46a180,0x1da58e:0x46a180,0x1da5c2:0x46a180,0x1da6b5:0x46a180,
            0x1da602:0x46a0d0,0x1da615:0x46a0cc,0x1da64f:0x44aac8,0x1da685:0x45695c}
        for case in fixture['cases']:
            observed=set(case['instructions'])
            for site,address in scalar_loads.items():
                if site not in observed:continue
                instruction=code.at(site);assert any(o.type==3 and not o.mem.base and not o.mem.index and o.mem.disp==address and o.size==4 for o in instruction.operands)
                present={b['address']+n:v for b in case['initial_banks'] for n,v in enumerate(bytes.fromhex(b['hex'])) if address<=b['address']+n<address+4}
                if len(present)==4:continue
                raw=image.read(address,4);assert all(v==raw[p-address] for p,v in present.items())
                for key in ('initial_banks','final_banks'):case[key].append(dict(address=address,hex=raw.hex()))
                supplements.append(dict(case=case['id'],physical_load_site=site,address=address,original_hex=raw.hex()))
        fixture['oracle_backend']='original_i386_hardware_single_step';oracle=scratch/'original-power.json';oracle.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(cases=len(fixture['cases']),complete_original_calls=sum(c['complete'] for c in fixture['cases']),required_infinite_argument_prefixes=sum(c['exit_entry']==0x37d948 for c in fixture['cases']),required_crt_error_prefixes=sum(c['exit_entry']==0x3334f0 for c in fixture['cases']),
            crt_error_original_calls=sum(0x3334f0 in c['instructions'] for c in fixture['cases']),thread_errno_original_accesses=sum(0x330179 in c['instructions'] for c in fixture['cases']),
            zero_power_branch_calls=sum(0x37cf55 in c['instructions'] for c in fixture['cases']),negative_base_parity_calls=sum(0x37cfac in c['instructions'] for c in fixture['cases']),
            original_bodies=bodies,physical_instructions=physical['single_stepped_original_instructions'],cpu_model=physical['cpu_model'],oracle_sha256=sha(oracle),hardware_tool_sha256=physical['tool_sha256'],trace_observer_sha256=physical['trace_observer_sha256'],
            paired_instruction_bytes_changed=physical['paired_instruction_bytes_changed'],native_power_result_bits_checked=True,composed_scalar_result_bits_checked=True,composed_full_mxcsr_and_x87_exception_flags_checked=True,
            emulator_physical_exit_disagreements=physical.get('emulator_physical_exit_disagreements',[]),pentium_iii_transcendental_approximation_bits_proven=False,full_original_machine_postimage_proven=False,
            native_crt_errno_side_effects_checked=True,native_crt_binary64_intermediates_checked=True,missing_native_errno_service_checked=True,explicit_and_current_environment_seams_checked=True,
            native_callback_control_word_preserved=True,thread_storage_inputs='declared initialized TLS and fallback records, no callee outputs',
            complete_x87_status_and_control_checked=True,native_consumer_adapter_checked=args.consumer,
            hardware_bridge_sha256=sha(scratch/'hardware/bridge.s'),
            physically_observed_readonly_supplements=supplements,
            x87_precision_bits=[24,53,64],rounding_modes=4,daz_ftz_modes=4,masked_exceptions_only=True,scratch_cleanup_owner=str(scratch),retained_large_artifacts=[])
        source=scratch/'source';shutil.copytree(ROOT/'include',source/'include')
        for path in SOURCES:
            target=source/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/path).read_bytes());assert sha(target)==report['source_sha256'][path]
        (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(native_lift_power_tu15 LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json REQUIRED)
add_executable(native_lift_power_tu15_tests src/physics/source_lift_scalar.cpp src/physics/source_lift_power_tu15.cpp src/physics/source_power_special_tu15.cpp src/physics/source_math_error_tu15.cpp tests/source_lift_power_tu15_tests.cpp)
target_compile_features(native_lift_power_tu15_tests PRIVATE cxx_std_20)
target_include_directories(native_lift_power_tu15_tests PRIVATE include)
target_compile_options(native_lift_power_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_lift_power_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_lift_power_tu15 COMMAND native_lift_power_tu15_tests "'''+str(oracle)+'")\n')
        if args.consumer:
            with (source/'CMakeLists.txt').open('a') as cmake:
                cmake.write('''add_executable(native_lift_consumer_tu15_tests src/physics/source_lift_scalar.cpp src/physics/source_lift_power_tu15.cpp src/physics/source_power_special_tu15.cpp src/physics/source_math_error_tu15.cpp src/physics/native_lift_scalar_consumer_tu15.cpp tests/native_lift_scalar_consumer_tu15_tests.cpp)
target_compile_features(native_lift_consumer_tu15_tests PRIVATE cxx_std_20)
target_include_directories(native_lift_consumer_tu15_tests PRIVATE include)
target_compile_options(native_lift_consumer_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_lift_consumer_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_lift_consumer_tu15 COMMAND native_lift_consumer_tu15_tests "'''+str(oracle)+'")\n')
        def validate(label):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--output',str(args.output/label),'--scratch-root',str(scratch/'builds'),'--target','native_lift_power_tu15_tests']
            if args.consumer:command+=['--target','native_lift_consumer_tu15_tests']
            command+=['--filter','^native_lift_(power|consumer)_tu15$','--jobs','2','--test-jobs','1'];report['commands'].append(command);save();return subprocess.run(command,check=False).returncode
        assert validate('native')==0,'Native power differs from original; inspect native/tests.log'
        native=source/'src/physics/source_lift_power_tu15.cpp';text=native.read_text();assert 'fyl2x;' in text;native.write_text(text.replace('fyl2x;','fyl2xp1;',1))
        result=validate('negative-logarithm');log=(args.output/'negative-logarithm/tests.log').read_text()
        assert result!=0 and 'TU1.5 native power result mismatch' in log,'Independent original result failed to reject the logarithm mutation'
        native.write_text(text)
        crt=source/'src/physics/source_math_error_tu15.cpp';policy=crt.read_text();assert 'kind==1?33:34' in policy
        crt.write_text(policy.replace('kind==1?33:34','kind==1?34:33',1))
        result=validate('negative-errno');log=(args.output/'negative-errno/tests.log').read_text()
        assert result!=0 and 'TU1.5 native pow errno side effect mismatch' in log,'Original CRT side effects failed to reject the errno mutation'
        crt.write_text(policy.replace('exponent-=1536;','exponent-=1535;',1))
        result=validate('negative-underflow');log=(args.output/'negative-underflow/tests.log').read_text()
        assert result!=0 and 'TU1.5 native CRT binary64 result mismatch' in log,'Original CRT intermediates failed to reject the underflow adjustment mutation'
        report.update(passed=True,negative_controls_rejected=True,negative_controls=['private FYL2X-to-FYL2XP1 arithmetic mutation','private domain/overflow errno swap','private underflow binary64 exponent adjustment'])
    except BaseException as error:report['error']=str(error);raise
    finally:save()
    print(json.dumps(report,sort_keys=True))

if __name__=='__main__':main()

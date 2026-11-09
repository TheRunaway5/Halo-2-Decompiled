#!/usr/bin/env python3
"""Typed promotion with real mutexes and unchanged original CPU segments."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import validate_native_async_promote_tu15 as retained

ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_async_promotion_kernel.hpp','src/platform/native_tu15_async_promotion_abi.hpp',
    'src/platform/native_tu15_async_queue_abi.hpp','include/halo2/native_tu15_async_queue_kernel.hpp',
    'src/physics/runtime/native_tu15_raw_x87_scope.hpp','tests/native_tu15_async_promote_abi_tests.cpp',
    'tests/native_tu15_async_promotion_data_abi_tests.cpp',
    'tools/validate_native_async_promote_tu15.py','tools/validate_native_tu15_async_promotion_data.py',
    'tests/reference/native_tu15_async_promotion_reference.hpp','tools/native_async_promotion_data_replacements.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--leak-sanitizer',action='store_true');parser.add_argument('--read-prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--mutations',action='store_true');parser.add_argument('--source-routes',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);pins={name:sha(ROOT/name)for name in FILES};report=dict(passed=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    original_run=subprocess.run;original_argv=sys.argv[:];original_physical=retained.physical_original
    try:
        fixture=(ROOT/'tests/native_tu15_async_promotion_data_abi_tests.cpp').read_text()
        if fixture.count('HALO2_PROMOTION_DATA_OWNER::promote(')!=2:raise ValueError('Protected promotion fixture shape changed')
        if args.require_binding:
            from native_async_promotion_data_replacements import replace_abi,replace_public
            for name,install in(('src/platform/native_tu15_async_promote.hpp',replace_abi),('src/platform/source_async_jobs.cpp',replace_public)):
                source=(ROOT/name).read_text()
                if install(source)!=source:raise ValueError('Typed promotion default delegation required '+name)
                pins[name]=sha(ROOT/name)
        caller_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
        fp_code='''
namespace promotion_caller_fp {
thread_local unsigned ordinal{},count{},seeded{};
constexpr std::array<unsigned,4> depths{0,1,4,8};
constexpr std::array<std::array<unsigned char,10>,8> values{{VALUES}};
void seed(SourcePhysicsWorldRuntimeAbi&input){
    ++seeded;count=depths[(ordinal/128)%4];input.x87_depth=count;
    input.x87_status=std::uint16_t((input.x87_status&~0x3800u)|((ordinal++%8)<<11));
    for(unsigned n=0;n<count;++n)std::memcpy(&input.x87[n],values[n].data(),10);
}
void verify(const SourcePhysicsWorldRuntimeAbi&value){
    if(!seeded||value.x87_depth!=count)throw std::runtime_error("Promotion caller x87 input was not seeded or its depth changed");
    for(unsigned n=0;n<count;++n)if(std::memcmp(&value.x87[n],values[n].data(),10))
        throw std::runtime_error("Promotion changed a caller80 value at a real child/return");
}
}
'''.replace('VALUES',','.join('{{'+','.join(str(byte)for byte in bytes.fromhex(value))+'}}'for value in caller_values))
        fixture=fixture.replace('static void record_promote_child(std::uint32_t,State&);',fp_code+'\nstatic void record_promote_child(std::uint32_t,State&);',1)
        fixture=fixture.replace('const auto abi=[&](const auto& value){','const auto abi=[&](const auto& value){promotion_caller_fp::verify(value);',1)
        entry='State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);'
        if fixture.count(entry)!=1:raise ValueError('Protected actual promotion caller acquisition changed')
        fixture=fixture.replace(entry,entry.replace('state.load(input);','promotion_caller_fp::seed(input);state.load(input);'),1)
        fixture=fixture.replace('const auto word=[](){','promotion_caller_fp::ordinal=caller_index;\n    const auto word=[](){',1)
        candidate=scratch/'promotion-data.cpp';candidate.write_text(fixture)
        def physical(image,rows,directory,**keywords):
            values=copy.deepcopy(rows)
            for row in values:
                ordinal=int(row['name'].split('-',1)[0]);depth=(0,1,4,8)[(ordinal//128)%4]
                row['live_x87']=list(caller_values[:depth]);row['before_fp'][1]=(row['before_fp'][1]&~0x3800)|((ordinal%8)<<11)
            keywords.update(trace_extended=True,preserve_x87_prefix=True)
            result,proof=original_physical(image,values,directory,**keywords)
            for row,cpu in zip(values,result['cases'],strict=True):
                if not cpu['live_x87_unchanged']:raise ValueError('Original promotion segment changed caller80')
                for call in cpu['calls']:
                    if call['live_x87']!=row['live_x87']:raise ValueError('Original promotion child changed caller80')
            report['physical_caller80_and_real_entry_packets_exact']=True
            return result,proof
        retained.physical_original=physical
        commands=[];mutation_include=None
        def run(command,*positional,**keywords):
            if isinstance(command,list)and str(ROOT/'tests/native_tu15_async_promote_abi_tests.cpp')in command:
                command=list(command);command[0]=args.compiler;command[command.index(str(ROOT/'tests/native_tu15_async_promote_abi_tests.cpp'))]=str(candidate)
                if args.require_binding:command[1:1]=['-DHALO2_NATIVE_ASYNC_PROMOTION_BOUND_ROUTE=1']
                if mutation_include:command[1:1]=['-I'+str(mutation_include)]
                commands.append(command)
            # All original instruction images, metadata and service results
            # remain untouched. Only the candidate native compilation selects
            # its new typed adapter; the authenticated retained oracle stays.
            keywords.setdefault('timeout',180)
            return original_run(command,*positional,**keywords)
        subprocess.run=run
        if args.sanitizers and args.leak_sanitizer:
            # The retained driver intentionally sets detect_leaks=0. Change
            # only that child runtime environment for the separately permitted
            # leak-inspection run; no fixture or service input is affected.
            def leak_run(command,*positional,**keywords):
                env=keywords.get('env')
                if env is not None and Path(str(command[0])).name=='native-async-promote':
                    keywords['env']=dict(env,ASAN_OPTIONS=env.get('ASAN_OPTIONS','').replace('detect_leaks=0','detect_leaks=1'))
                return run(command,*positional,**keywords)
            subprocess.run=leak_run
        inner=scratch/'retained-segmented-proof.json'
        sys.argv=[str(ROOT/'tools/validate_native_async_promote_tu15.py'),'--xbe',str(args.xbe),'--report',str(inner),'--require-binding']+(['--sanitizers']if args.sanitizers else[])
        code=retained.main()
        if code:raise SystemExit(code)
        proof=json.loads(inner.read_text());report['segmented_original_and_actual_mutex_proof']=proof
        binary=scratch/'native-async-promote';env=dict(os.environ);env.pop('HALO2_REFERENCE_ASYNC_QUEUE',None)
        if args.sanitizers:env['ASAN_OPTIONS']='detect_leaks=1'if args.leak_sanitizer else'detect_leaks=0'
        guards=original_run([str(binary),'--guard-tests'],capture_output=True,text=True,env=env,timeout=30)
        if guards.returncode:raise ValueError('Promotion admission controls failed '+guards.stderr[-1500:])
        report['admission_guards']=json.loads(guards.stdout)
        if args.read_prefixes:
            inputs=(scratch/'promotion.in').read_bytes();stride=retained.ABI_BYTES+sum(size for _,size in retained.REGIONS)
            if len(inputs)!=512*stride:raise ValueError('Protected promotion input stride changed')
            env=dict(os.environ);env.pop('HALO2_REFERENCE_ASYNC_QUEUE',None)
            if args.sanitizers:env['ASAN_OPTIONS']='detect_leaks=1'if args.leak_sanitizer else'detect_leaks=0'
            receipts=[]
            for choice,index in enumerate((0,8,40,72,104,77,141,205)):
                context=(choice%4)*128+index%8;data=inputs[index*stride:(index+1)*stride]
                common=['--no-observer','--caller-index',str(context)]
                normal=original_run([str(binary),*common,'--accesses'],input=data,capture_output=True,env=env,timeout=30)
                if normal.returncode:raise ValueError('Promotion prefix baseline failed '+normal.stderr[-1500:].decode())
                accesses=list(struct.iter_unpack('<4I',normal.stdout[4:]));count=struct.unpack_from('<I',normal.stdout)[0]
                if count!=len(accesses):raise ValueError('Protected promotion access protocol changed')
                reads=sum(not access[2]for access in accesses)
                for ordinal in range(reads):
                    outputs=[]
                    for mode in((),('--reference',)):
                        result=original_run([str(binary),*common,'--fault-read',str(ordinal),*mode],input=data,capture_output=True,env=env,timeout=30)
                        if result.returncode:raise ValueError('Promotion actual mutex/read-prefix failed '+result.stderr[-2000:].decode())
                        if struct.unpack_from('<I',result.stdout)[0]!=1:raise ValueError('Promotion failed to stop at declared real read')
                        outputs.append(result.stdout)
                    if outputs[0]!=outputs[1]:
                        first=next((n for n,(a,b)in enumerate(zip(*outputs))if a!=b),min(map(len,outputs)))
                        raise ValueError(f'Promotion read-prefix ABI differs input{index}/read{ordinal} byte{first}: '+outputs[0][first:first+16].hex()+' vs '+outputs[1][first:first+16].hex())
                receipts.append(dict(input_case=index,caller_index=context,read_prefixes=reads,passed=True))
            report.update(retained_read_prefixes=receipts,retained_read_prefix_count=sum(row['read_prefixes']for row in receipts),
                fault_source_snapshot_precedes_test_host_mutex_cleanup=True,observer_preimage_faults_qualified=False,physical_page_fault_atomicity_qualified=False)
        if args.source_routes:
            if not args.require_binding:raise ValueError('Source-prefix routing requires qualified installed API')
            def source_run(arguments,force=False):
                environment=dict(env)
                if force:environment['HALO2_REFERENCE_ASYNC_QUEUE']='1'
                with(scratch/'promotion.in').open('rb')as stream:
                    result=original_run([str(binary),'--installed-source',*arguments],stdin=stream,capture_output=True,env=environment,timeout=90)
                if result.returncode:raise ValueError('Installed promotion source route failed '+result.stderr[-2000:].decode())
                return result.stdout
            baselines=[source_run(['--reference']),source_run(['--reference','--accesses'])]
            from native_async_promote_replacements import authenticate
            match,body=authenticate((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())
            include=scratch/'async_promote_reference.inc';text=include.read_text()
            if text.count(body)!=1 or match[0]==body:raise ValueError('Complete installed promotion prefix not found')
            include.write_text(text.replace(body,match[0],1));original_run(commands[0],check=True)
            source_results=[]
            for force in(False,True):
                arguments=['--reference']if force else[]
                streams=[source_run(arguments,force),source_run([*arguments,'--accesses'],force)]
                if streams!=baselines:raise ValueError('Installed promotion prefix/environment fallback differs')
                source_results.append(dict(forced_reference=force,cases=512,value_bytes=len(streams[0]),value_sha256=hashlib.sha256(streams[0]).hexdigest(),ordered_access_cursor_sha256=hashlib.sha256(streams[1]).hexdigest()))
            report['actual_installed_prefix_and_environment_routes']=source_results
        if args.mutations:
            text=(ROOT/'include/halo2/native_tu15_async_promotion_kernel.hpp').read_text();rejections=[]
            for label,old,new in(('generation-ownership','if(d.owner==d.job){','if(d.owner!=d.job){'),('priority-publication','access.write(W::priority,d,d.priority);','access.write(W::priority,d,d.priority+1u);')):
                if text.count(old)!=1:raise ValueError('Missing DATA policy mutation witness')
                mutation_include=scratch/('mutant-'+label);(mutation_include/'halo2').mkdir(parents=True)
                (mutation_include/'halo2/native_tu15_async_promotion_kernel.hpp').write_text(text.replace(old,new,1))
                try:retained.main()
                except ValueError as error:
                    if not str(error).startswith(('Native/canonical promotion differs','Promotion failed')):raise
                    rejections.append(dict(mutation=label,compiled=True,rejected=True,reason=str(error)[:1500]))
                else:raise ValueError('DATA policy mutation escaped unchanged original/canonical controls '+label)
            mutation_include=None;report['compiled_policy_mutation_rejections']=rejections
        if any(sha(ROOT/name)!=pin for name,pin in pins.items()):raise ValueError('Promotion DATA proof sources changed')
        report.update(passed=True,compile_commands=commands,address_undefined_sanitizers=args.sanitizers,
            leak_detection_qualified=args.sanitizers and args.leak_sanitizer,production_data_default_installed=args.require_binding,
            actual_native_mutex_wait_release=True,old_instruction_shaped_controller_used_as_native=False,
            caller_live80_depths=[0,1,4,8],all_top_positions=True,
            original_instruction_or_service_outcomes_modified=False,whole_xbox_mutex_or_live_world_pending_qualified=False)
    except BaseException as error:report.update(passed=False,error=repr(error));raise
    finally:
        subprocess.run=original_run;sys.argv=original_argv;retained.physical_original=original_physical
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,cases=len(proof['cases']),original_segments=proof['segmented_physical_cases'])));return 0
if __name__=='__main__':raise SystemExit(main())

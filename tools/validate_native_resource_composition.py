#!/usr/bin/env python3
"""Private source-backed resource controller composition with actual file I/O.

Canonical CPU controls and native owners compose the shared synchronous World
policy. This is not a continuous Xbox scheduling or live World epoch proof.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import shlex
import subprocess
import sys
from native_resource_composition_inputs import ROOT,CONTROLLERS,freeze_resource_registry,freeze_controller_sources,composition_rows
from validate_native_tu15_prt_helpers import PIN,physical_original

SOURCES=('tests/native_tu15_resource_composition_tests.cpp','tests/native_tu15_resource_composition_backend.hpp',
    'tools/native_resource_composition_inputs.py','tools/validate_native_resource_composition.py',
    'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp','src/assets/cache.cpp',
    'src/rendering/source_bitmap_async_read.cpp','src/platform/source_async_jobs.cpp',
    'src/platform/native_tu15_rng_startup.cpp','src/runtime/source_execution_inputs.cpp',
    'src/physics/native_tu15_physical_epoch.cpp','src/runtime/source_runtime_memory.cpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def physical_segments(image,rows,scratch,binary,fixture,file,env):
    """Unchanged CPU on each side of the actual synchronous native service.

    Only original instructions execute in either CPU segment. Real file and
    registry owners produce the service state; the original import is neither
    patched nor resumed with a supplied page/job outcome.
    """
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    raw_metadata=common.metadata
    def metadata(image):
        value=raw_metadata(image);code=common._Code(image,'.text')
        for site in value['sites']:
            instruction=code.at(site['instruction'])
            if instruction.mnemonic in('pop','ret'):
                site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            # Capstone omits CS_AC_READ for several TEST r/m,reg encodings
            # (including genuine16EA85=851C8E). TEST memory is read-only;
            # the authenticated original opcode/operand owns that access.
            if instruction.mnemonic=='test':
                for operand in site['operands']:operand.update(read=True,write=False)
            expected={0x16e6c5:'852c8b',0x1401a2:'851c88',0x32a672:'8500',0x32a689:'8501',0x32a696:'8501'}.get(site['instruction'])
            if expected:
                if site['bytes']!=expected:raise ValueError('Composition original TEST metadata drift')
                for operand in site['operands']:operand.update(read=True,write=False)
            if site['instruction']==0x140071:
                if site['bytes']!='0f184c0464':raise ValueError('Composition original PREFETCH metadata drift')
                for operand in site['operands']:operand.update(read=False,write=False)
        return value
    entries=tuple(int(entry,16)for entry,_,_ in CONTROLLERS)+(0x1232c0,0x1ab330,0x12f040,0x12f0c0,0x13fd10)
    regions=tuple(sorted({(bank['address'],len(bank['before'])//2)for row in rows for bank in row['banks']}))
    common.metadata=metadata
    try:
        before=copy.deepcopy(rows)
        for row in before:row.update(stop_before_entries=[0x1a3b70],max_steps=100000)
        directory=scratch/'before-service';directory.mkdir();pre,preproof=physical_original(image,before,directory,entries=entries,regions=regions,preserve_x87_prefix=True)
        service=[]
        for row,result in zip(rows,pre['cases'],strict=True):
            # The observer's completed bit also denotes a declared entry stop;
            # the actual stopped PC owns this boundary classification.
            if result['stopped_at']!=0x1a3b70:raise ValueError(row['name']+': physical caller failed to reach genuine I/O entry; stopped='+hex(result['stopped_at']))
            value=copy.deepcopy(row);value.update(gp=result['boundary_gp'],eflags=result['boundary_eflags'],xmm=result['boundary_xmm'],before_fp=result['after_fp'])
            for bank,actual in zip(value['banks'],result['banks'],strict=True):bank['before']=actual['after']
            service.append(value)
        service_fixture=scratch/'service.json';service_fixture.write_text(json.dumps(dict(cases=service),separators=(',',':')))
        execution=subprocess.run([str(binary),'--service',str(service_fixture),str(file)],capture_output=True,text=True,env=env,timeout=60)
        if execution.returncode:raise ValueError('Actual resource service failed after physical CPU: '+execution.stderr[-4000:])
        services=json.loads(execution.stdout);after=[]
        for row,value in zip(rows,services,strict=True):
            if value['retained_jobs']!=1:raise ValueError('Actual job owner failed physical composition')
            post=copy.deepcopy(row);abi=value['abi'];post.update(entry=0x12613e,gp=abi['gp'],eflags=abi['eflags'],xmm=abi['xmm'],before_fp=abi['fp'],banks=value['banks'],stop_before_entries=[],max_steps=100000)
            def actual_stack_word(address):
                return next(int.from_bytes(bytes.fromhex(bank['before'])[address-bank['address']:address-bank['address']+4],'little')
                    for bank in post['banks']if bank['address']<=address<=bank['address']+len(bank['before'])//2-4)
            # The common observer seeds these three caller words. A resumed
            # interior entry owns a live private frame, so preserve its actual
            # post-service bytes rather than the outer caller's old arguments.
            frame=abi['gp'][4];post.update(return_pc=actual_stack_word(frame),matrix=actual_stack_word(frame+4),source=actual_stack_word(frame+8))
            after.append(post)
        directory=scratch/'after-service';directory.mkdir();post,postproof=physical_original(image,after,directory,entries=entries,regions=regions,preserve_x87_prefix=True)
        execution=subprocess.run([str(binary),'--export',str(fixture),str(file)],capture_output=True,text=True,env=env,timeout=60)
        if execution.returncode:raise ValueError('Native resource export failed: '+execution.stderr[-4000:])
        actual=json.loads(execution.stdout);total_reads=total_stores=0
        for row,first,service,last,native in zip(rows,pre['cases'],services,post['cases'],actual,strict=True):
            name=row['name']
            if not last['completed']:raise ValueError(name+': physical continuation failed to return')
            abi=native['abi']
            if abi['gp']!=last['boundary_gp']or abi['eflags']!=last['boundary_eflags']or abi['xmm']!=last['boundary_xmm']or abi['fp']!=last['after_fp']or abi['live_x87']!=row['live_x87']:
                raise ValueError(name+': independent physical complete ABI differs')
            expected_banks=[[bank['address'],bank['after']]for bank in last['banks']]
            if native['banks']!=expected_banks:raise ValueError(name+': independent physical complete banks differ')
            reads=[[event['address'],event['bytes']]for event in first['reads']]+service['reads']+[[event['address'],event['bytes']]for event in last['reads']]
            stores=[[event['address'],event['before'],event['written']]for event in first['writes']]+service['writes']+[[event['address'],event['before'],event['written']]for event in last['writes']]
            for label,expected in(('reads',reads),('writes',stores)):
                if native[label]!=expected:
                    actual=native[label];index=next((n for n,(a,b)in enumerate(zip(actual,expected))if a!=b),min(len(actual),len(expected)))
                    instructions=[event['instruction']for event in first[label]]+[0x1a3b70]*len(service[label])+[event['instruction']for event in last[label]]
                    context=[dict(event=event,pc=hex(pc))for event,pc in zip(expected[max(0,index-5):index+5],instructions[max(0,index-5):index+5])]
                    raise ValueError(name+f': physical ordered {label} differ at{index}: native={actual[max(0,index-5):index+5]} original={context}')
            total_reads+=len(reads);total_stores+=len(stores)
        return dict(passed=True,cases=len(rows),before_service=preproof,after_service=postproof,ordered_reads=total_reads,ordered_stores=total_stores,
            real_file_service_instances=len(services),original_instructions_patched=False,supplied_original_outcomes=False,
            scope='Independent unchanged i386 segments before genuine I/O entry and after the actual native file/registry service. No continuous Xbox import/scheduling or live World epoch claim.')
    finally:common.metadata=raw_metadata
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--leak-sanitizer',action='store_true');parser.add_argument('--physical',action='store_true');args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686'):
        print('SKIP original resource composition requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 executable');return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if sha(args.xbe)!=PIN:raise ValueError('Composition requires authenticated original TU1.5 image')
    pins={name:sha(ROOT/name)for name in SOURCES};report=dict(passed=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        from oracle_smg_firing_1_1 import Xbe
        image=Xbe(args.xbe);registry=freeze_resource_registry(scratch);controllers=freeze_controller_sources(scratch);rows,inputs=composition_rows(image)
        fixture=scratch/'composition.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        file=scratch/'actual-cache-sectors.bin';file.write_bytes(random.Random(0x126200).randbytes(32768));binary=scratch/'resource-composition'
        command=[args.compiler,'-std=c++20','-O1'if args.sanitizers else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-pthread','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/gameplay/weapons'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),
            str(ROOT/SOURCES[0]),*[str(ROOT/name)for name in SOURCES[4:]],'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report.update(registry=registry,controllers=controllers,input_provenance=inputs,compile_command=command,cases=len(rows))
        dependencies=subprocess.run(command[:-2]+['-MM','-MT','resource_dependency'],capture_output=True,text=True,check=True)
        paths=set()
        for line in dependencies.stdout.replace('\\\n',' ').splitlines():
            for token in shlex.split(line.partition(':')[2]):
                path=Path(token).resolve()
                if path.is_file()and path.is_relative_to(ROOT):paths.add(path)
        dependency_pins={str(path.relative_to(ROOT)):sha(path)for path in paths}
        report['compiler_dependency_sha256']=dependency_pins
        report['fixture_body_pose_publication_active']=False
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Resource composition compile failed: '+result.stderr[-12000:])
        env=dict(os.environ)
        if args.sanitizers:env['ASAN_OPTIONS']=env.get('ASAN_OPTIONS','')+(':detect_leaks=1'if args.leak_sanitizer else':detect_leaks=0')
        result=subprocess.run([str(binary),str(fixture),str(file)],capture_output=True,text=True,env=env,timeout=90)
        if result.returncode:raise ValueError('Resource composition failed: '+(result.stdout+result.stderr)[-10000:])
        report['canonical_result']=json.loads(result.stdout)
        if args.physical:report['physical_segments']=physical_segments(image,rows,scratch,binary,fixture,file,env)
        if any(sha(ROOT/name)!=value for name,value in pins.items()):raise ValueError('Composition source drift')
        if any(sha(ROOT/name)!=value for name,value in dependency_pins.items()):raise ValueError('Composition compiler dependency drift')
        report.update(passed=True,result=json.loads(result.stdout),leak_detection_qualified=args.sanitizers and args.leak_sanitizer,
            shared_production_synchronous_policy=True,live_world_epoch_or_restore_qualified=False,pending_worker_policy_qualified=False,
            continuous_original_cpu_or_xbox_scheduling_qualified=False,production_default_changed=False)
    except Exception as error:
        report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report['result'],sort_keys=True));return 0
if __name__=='__main__':raise SystemExit(main())

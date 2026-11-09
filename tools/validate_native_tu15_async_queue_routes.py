#!/usr/bin/env python3
"""Execute authenticated installed Arena/Shadow prefixes and their env fallback."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from native_async_queue_data_inputs import corpus
from native_async_order_replacements import authenticate as order_owner
from native_async_insert_replacements import authenticate as insert_owner

ROOT=Path(__file__).resolve().parents[1]
FILES=('src/physics/physics_game_arena_runtime_generated.cpp','src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',
    'src/platform/native_tu15_async_order.hpp','src/platform/native_tu15_async_queue_abi.hpp',
    'include/halo2/native_tu15_async_queue_kernel.hpp','tests/native_tu15_async_order_abi_tests.cpp',
    'tests/native_tu15_async_insert_abi_tests.cpp','tools/native_async_order_replacements.py',
    'tools/native_async_insert_replacements.py','tools/native_async_queue_data_inputs.py',
    'tools/validate_native_tu15_async_queue_routes.py','src/physics/runtime/native_tu15_raw_x87_scope.hpp')
def digest(value):return hashlib.sha256(value).hexdigest()
def once(source,old,new):
    if source.count(old)!=1:raise ValueError('Protected diagnostic fixture shape changed '+old[:100])
    return source.replace(old,new,1)
def relocate(text,first,last,delta):
    return re.sub(r'0x([0-9a-f]+)u',lambda m:f'0x{int(m[1],16)+delta:x}u'if first<=int(m[1],16)<=last else m[0],text)
def namespace(name,resolver,body):
    return 'namespace '+name+'{\nusing namespace halo2::physics_world_runtime_detail;\n'+resolver+'\n'+body+'\n}\n'
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--leak-sanitizer',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);pins={name:digest((ROOT/name).read_bytes())for name in FILES}
    report=dict(passed=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        arena=(ROOT/FILES[0]).read_text();shadow=(ROOT/FILES[1]).read_text()
        cmatch,child=order_owner(arena);pmatch,parent=insert_owner(arena,'122020');smatch,shadow_parent=insert_owner(shadow,'1232c0')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',arena,re.S)[0]
        shadow_resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',shadow,re.S)[0]
        # authenticate() above checks the entire real resolver, not a selected
        # subset of mappings. No inferred PC/global/callback identity is used.
        if cmatch[0]==child or pmatch[0]==parent or smatch[0]==shadow_parent:raise ValueError('Installed original owner prefixes required')
        environment=dict(os.environ);environment.pop('HALO2_REFERENCE_ASYNC_QUEUE',None)
        if args.sanitizers:environment['ASAN_OPTIONS']=environment.get('ASAN_OPTIONS','')+(':detect_leaks=1'if args.leak_sanitizer else':detect_leaks=0')
        results=[]
        for kind in('order','insert'):
            directory=scratch/kind;directory.mkdir();rows,recipe=corpus(kind,directory)
            # Legacy binary caller inputs deliberately have an empty x87
            # stack. The separate full ABI gate owns live80/prefix coverage.
            fixture=(ROOT/('tests/native_tu15_async_'+kind+'_abi_tests.cpp')).read_text()
            if kind=='order':
                original=namespace('async_reference',resolver,relocate(child,0x1a5080,0x1a5109,0x62b0))+namespace('async_canonical',resolver,child)
                installed=namespace('async_reference',resolver,relocate(cmatch[0],0x1a5080,0x1a5109,0x62b0))+namespace('async_canonical',resolver,cmatch[0])
                execution='''if(reference){if(canonical_sites)async_canonical::run_1a5080(s);else async_reference::run_1a5080(s);}
        else if(!native_async_order_detail::compare(s,canonical_sites?0x1a5080u:0x1ab330u))throw std::runtime_error("Native async ordering declined");'''
                routed=once(fixture,execution,'if(canonical_sites)async_canonical::run_1a5080(s);else async_reference::run_1a5080(s);')
                routes=[('arena-comparator',['--canonical-sites','--cursor'],['--canonical-sites','--cursor','--reference'])]
            else:
                def arena_namespace(name,body,leaf):
                    # Diagnostic wrapping records the genuine child entry; it
                    # supplies no GP, flag, memory or comparator result.
                    code=leaf.replace('run_1a5080(','retained_compare(',1)+'\nstatic void run_1a5080(WorldState&s){::record_async_child(s);retained_compare(s);}\n'+body
                    return namespace(name,resolver,code)
                def shadow_namespace(body,leaf):
                    # Arena's authenticated resolver maps the four uses of
                    # this callback to actual1AAA80; Shadow is identity-only.
                    # resolver. Preserve the real imported child's operand.
                    code=relocate(leaf,0x1a5080,0x1a5109,0x62b0)
                    if code.count('selected(0x1a47d0u,s.image)')!=4:raise ValueError('Protected imported callback uses changed')
                    code=code.replace('selected(0x1a47d0u,s.image)','0x1aaa80u').replace('run_1a5080(','retained_compare(',1)
                    # The genuine imported comparator is represented by its
                    # authenticated CPU body here; no outcome is fabricated.
                    code+='''\nstatic void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){
                        if(target!=0x1ab330u||site!=0x123339u||tail)throw std::runtime_error("Unexpected source insert child");
                        ::record_async_child(s);retained_compare(s);}\n'''+body
                    return namespace('insert_shadow_reference',shadow_resolver,code)
                actual_parent=relocate(parent,0x122020,0x1220d7,0x12a0)
                actual_child=relocate(child,0x1a5080,0x1a5109,0x62b0)
                common=arena_namespace('insert_reference',actual_parent,actual_child)
                original=common+arena_namespace('insert_canonical',parent,child)+shadow_namespace(shadow_parent,child)
                installed=common+arena_namespace('insert_canonical',pmatch[0],cmatch[0])+shadow_namespace(smatch[0],cmatch[0])
                # Source-native Shadow directly invokes the comparator and
                # has no diagnostic child wrapper. Exclude this diagnostic
                # stream in both builds; the full ABI oracle checks 480 real
                # comparator packets independently.
                fixture=once(fixture,'emit(calls.size());for(auto&call:calls)abi(call);','')
                start=fixture.index('        if(reference){if(shadow)');end=fixture.index('        if((reference&&!callbacks)',start)
                routed=fixture[:start]+'''        if(shadow)insert_shadow_reference::run_1232c0(s);
        else if(canonical)insert_canonical::run_122020(s);else insert_reference::run_122020(s);
'''+fixture[end:]
                routed=once(routed,'else if(arg=="--shadow-reference"){reference=true;shadow=true;}',
                    'else if(arg=="--shadow-reference"){reference=true;shadow=true;}else if(arg=="--installed-shadow")shadow=true;')
                routes=[('arena-insertion',['--canonical-sites'],['--canonical-sites','--reference']),
                    ('shadow-insertion',['--installed-shadow'],['--shadow-reference'])]
            binary=directory/'probe';source=directory/'probe.cpp';include=directory/('async_'+kind+'_reference.inc')
            command=[args.compiler,'-std=c++20','-O1'if args.sanitizers else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(directory),str(source),'-o',str(binary)]
            if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            def run(arguments,force=False):
                env=dict(environment)
                if force:env['HALO2_REFERENCE_ASYNC_QUEUE']='1'
                with(directory/'inputs').open('rb')as stream:
                    result=subprocess.run([str(binary),*arguments],stdin=stream,capture_output=True,env=env,timeout=90)
                if result.returncode:raise ValueError('Installed source route failed '+result.stderr[-3000:].decode(errors='replace'))
                return result.stdout
            include.write_text(original);source.write_text(fixture);subprocess.run(command,check=True)
            baselines={}
            for label,native_args,reference_args in routes:
                baselines[label]=(run(reference_args),run([*reference_args,'--accesses']))
            include.write_text(installed);source.write_text(routed);subprocess.run(command,check=True)
            for label,native_args,reference_args in routes:
                for force,arguments in((False,native_args),(True,reference_args)):
                    actual=run(arguments,force);accesses=run([*arguments,'--accesses'],force)
                    if(actual,accesses)!=baselines[label]:raise ValueError('Installed source prefix/refcontrol differs '+label+str(force))
                    results.append(dict(owner=label,forced_reference=force,cases=len(rows),value_bytes=len(actual),value_sha256=digest(actual),
                        ordered_access_cursor_bytes=len(accesses),ordered_access_cursor_sha256=digest(accesses),passed=True))
        if any(digest((ROOT/name).read_bytes())!=pin for name,pin in pins.items()):raise ValueError('Installed source route inputs changed')
        report.update(passed=True,routes=results,original_full_body_and_resolver_authenticated=True,
            actual_installed_source_prefix_and_environment_fallback=True,source_namespaces_preserved=True,
            original_cpu_rerun=False,scope='Installed source route equivalence to the separately physical-qualified retained body on identical deterministic inputs.',
            descendant_callee_packets_qualified_by_separate_full_abi_gate=True,import_dispatcher_qualified=False,
            live80_and_fault_prefixes_qualified_by_this_gate=False,legacy_map_pack_qualified=False,
            address_undefined_sanitizers=args.sanitizers,leak_detection_qualified=args.sanitizers and args.leak_sanitizer)
    except BaseException as error:report.update(passed=False,error=repr(error));raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,routes=len(results),cases_per_route=512)));return 0
if __name__=='__main__':raise SystemExit(main())

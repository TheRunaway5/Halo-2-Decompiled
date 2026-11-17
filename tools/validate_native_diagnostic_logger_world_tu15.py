#!/usr/bin/env python3
"""Exact World getter route, real fallback edges and whole frozen CPP syntax."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_diagnostic_logger_world_replacements as binding
WORLD='src/physics/physics_live_world_tu15.cpp'
TEST='tests/native_tu15_diagnostic_logger_owner_tests.cpp'

def routed_source(fragment):
    source=(ROOT/TEST).read_text()
    insertion='''struct LegacyFallback{};
struct WorldRoute {
    Memory&bank;const SourcePhysicsWorldRuntimeServices&provided;
    unsigned factory_calls{},legacy_fallbacks{};
    const SourcePhysicsWorldRuntimeServices&services(){++factory_calls;return provided;}
    void required_entry_original(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o){
'''+fragment+'''
        ++legacy_fallbacks;throw LegacyFallback{};
    }
};
bool routed(Memory&m,const SourcePhysicsContactRuntimeCall&call,SourcePhysicsWorldRuntimeAbi&abi,
    const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe){
    WorldRoute world{m,services};bool handled=true;
    try{world.required_entry_original(m,call,abi,observe);}catch(const LegacyFallback&){handled=false;}
    check(world.factory_calls==(call.target==owner::entry?1u:0u),"World getter services factory lost target short-circuit");
    check(world.legacy_fallbacks==(handled?0u:1u),"World getter fallback edge differs");return handled;
}
void wrong_bank(){
    reset_host();Memory bank,other;SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x62001000;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};
    other.abi=&abi;other.banks.emplace(0x62000000u,std::vector<std::byte>(8192));
    const auto original=packet(abi);WorldRoute world{bank,services};bool refused{};
    try{world.required_entry_original(other,{0x30ae24u,owner::entry,false},abi,{});}
    catch(const std::runtime_error&error){refused=std::string_view(error.what())=="TU1.5 live world bank owner differs";}
    check(refused&&!world.factory_calls&&!world.legacy_fallbacks&&!other.attempts&&packet(abi)==original,"Original World bank gate did not precede getter route");
}
'''
    # reset_host is declared below this insertion in the original harness.
    insertion='void reset_host();\n'+insertion
    anchor='unsigned guards(){'
    if source.count(anchor)!=1 or source.count('owner::run(')!=2:raise ValueError('Complete getter routing test seam drift')
    source=source.replace('owner::run(','routed(',2).replace(anchor,insertion+anchor+'\n    wrong_bank();',1)
    return source

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],production_binding_required=args.require_binding,routes=[],syntax=[])
    try:
        source=(ROOT/WORLD).read_text();bound=binding.replace_diagnostic_logger_world(source)
        if args.require_binding and bound!=source:raise ValueError('World diagnostic getter default route is not installed')
        if binding.replace_diagnostic_logger_world(bound)!=bound:raise ValueError('World getter route is not idempotent')
        body=binding.function(bound,binding.METHOD);start=body.index('{')+1;end=body.index(binding.ANCHOR+binding.PREFIX)+len(binding.ANCHOR+binding.PREFIX)
        fragment=body[start:end];test=routed_source(fragment);test_path=scratch/'routed.cpp';test_path.write_text(test)
        api_report=scratch/'api-proof.json';compiler=(args.compiler or['g++'])[0]
        command=[sys.executable,'-B',str(ROOT/'tools/validate_native_diagnostic_logger_owner_tu15.py'),'--xbe',str(args.xbe),'--report',str(api_report),'--compiler',compiler]
        if args.sanitizers:command.append('--sanitizers')
        baseline=subprocess.run(command,capture_output=True,text=True)
        report['api_preparation']=json.loads(api_report.read_text())
        if baseline.returncode:raise ValueError('Getter routing authentic inputs failed '+baseline.stdout[-4000:]+baseline.stderr[-2000:])
        fixture=scratch/'fixture.json'
        for compiler in args.compiler or['g++']:
            binary=scratch/('route-'+Path(compiler).name)
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(test_path),'-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            compiled=subprocess.run(command,capture_output=True,text=True)
            if compiled.returncode:raise ValueError('Actual getter route compilation failed '+compiled.stderr[-9000:])
            for name,arguments,reference in(('native',[str(fixture)],False),('reference',[str(fixture)],True),('fallbacks',['--guards'],False),('reference_fallbacks',['--guards'],True)):
                env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');env.pop('HALO2_REFERENCE_DIAGNOSTIC_LOGGER',None)
                if reference:env['HALO2_REFERENCE_DIAGNOSTIC_LOGGER']='1'
                tested=subprocess.run([str(binary),*arguments],capture_output=True,text=True,env=env)
                row=dict(compiler=compiler,route=name,passed=tested.returncode==0,diagnostic=(tested.stdout+tested.stderr)[-8000:]);report['routes'].append(row)
                if tested.returncode:raise ValueError('Actual getter World route failed '+row['diagnostic'])
                row['result']=json.loads(tested.stdout)
            if args.semantic_mutants:
                definitions=(('wrong-target',binding.PREFIX,binding.PREFIX.replace('c.target==0x2e16d0u','c.target==0x2e16d1u'),False),
                    ('dropped-adapter',binding.PREFIX,'\n        if(c.target==0x2e16d0u)return;',False),
                    ('eager-services',binding.PREFIX,'\n        if(native_diagnostic_logger_owner_detail::run(m,c,a,services(),o) && c.target==0x2e16d0u)return;',True),
                    ('removed-bank-gate',binding.ANCHOR,'        ;',True))
                for name,old,new,guard in definitions:
                    if test.count(old)!=1:raise ValueError('Missing exact World getter mutant '+name)
                    altered=scratch/(Path(compiler).name+'-'+name+'.cpp');altered.write_text(test.replace(old,new,1));mutant=scratch/(Path(compiler).name+'-'+name)
                    mutation=list(command);mutation[mutation.index(str(test_path))]=str(altered);mutation[mutation.index('-o')+1]=str(mutant)
                    compiled=subprocess.run(mutation,capture_output=True,text=True)
                    if compiled.returncode:raise ValueError('Getter route mutant did not compile '+name+': '+compiled.stderr[-3000:])
                    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');env.pop('HALO2_REFERENCE_DIAGNOSTIC_LOGGER',None)
                    tested=subprocess.run([str(mutant),'--guards'if guard else str(fixture)],capture_output=True,text=True,env=env)
                    if tested.returncode==0:raise ValueError('Compiled World getter mutant survived '+name)
                    report.setdefault('semantic_mutants',[]).append(dict(compiler=compiler,name=name,compiled=True,rejected=True,diagnostic=(tested.stdout+tested.stderr)[-1800:]))
        # Freeze the actual whole World CPP and every local quoted dependency;
        # only this CPP's authenticated getter prefix/include differs.
        frozen=scratch/'world';pending=[WORLD];pins={}
        while pending:
            name=pending.pop()
            if name in pins:continue
            original=ROOT/name;data=bound.encode()if name==WORLD else original.read_bytes();pins[name]=hashlib.sha256(original.read_bytes()).hexdigest()
            target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
            for include in re.findall(r'^\s*#\s*include\s+"([^"]+)"',data.decode(errors='replace'),re.M):
                for base in(original.parent,ROOT/'include',ROOT/'src'):
                    candidate=base/include
                    if candidate.is_file():pending.append(str(candidate.relative_to(ROOT)));break
        sdl=shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True))
        for compiler in args.compiler or['g++']:
            command=[compiler,'-std=c++20','-fsyntax-only','-I'+str(frozen/'include'),'-I'+str(frozen/'src'),'-I'+str(frozen/'src/gameplay/weapons'),*sdl,'-MD','-MF',str(scratch/(Path(compiler).name+'-world.d')),str(frozen/WORLD)]
            tested=subprocess.run(command,capture_output=True,text=True);report['syntax'].append(dict(compiler=compiler,command=command,passed=tested.returncode==0,diagnostic=tested.stderr[-9000:]))
            if tested.returncode:raise ValueError('Actual whole World getter CPP failed syntax '+tested.stderr[-9000:])
        for name,pin in pins.items():
            if name!=WORLD and hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin:raise ValueError('World getter quoted input changed '+name)
        binding.authenticate((ROOT/WORLD).read_text())
        report.update(passed=True,original_world_method_pin=binding.METHOD_PIN,frozen_world_sha256=hashlib.sha256(bound.encode()).hexdigest(),quoted_source_sha256=pins,
            exact_prefix_source=binding.PREFIX,default_route_installed=bound==source,actual_world_integration='Exact authenticated prefix executes in a disposable minimal owner; actual whole frozen World CPP syntax compiles. No complete World instance is executed.',
            production_reference_control='HALO2_REFERENCE_DIAGNOSTIC_LOGGER',unqualified_domains=['Initializer or platform sink execution','Older-image getter mapping','Actual signal/MMU fault envelope'])
    except Exception as error:report.update(passed=False,error=str(error)[-14000:])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=report['passed'],error=report.get('error'),report=str(args.report))));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

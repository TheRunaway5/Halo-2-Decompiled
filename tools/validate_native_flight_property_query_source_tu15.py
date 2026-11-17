#!/usr/bin/env python3
"""Compare the private E0900 source frame with four genuine Primary profiles."""
import argparse
from dataclasses import dataclass
import hashlib
import json
import os
import platform
import re
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Callable

from oracle_smg_firing_1_1 import Xbe
import native_flight_property_query_replacements as binding
from specialize_native_operands import specialize
from validate_native_flight_property_query_frame_tu15 import corpus, PIN

ROOT = Path(__file__).resolve().parents[1]
UNIT = 'src/physics/physics_live_support_tu15_generated.cpp'
GEN = 'tools/generate_physics_live_support_tu15.py'
DEATH_UNIT = 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
DEATH_GEN = 'tools/generate_death_cycle_runtime_tu15.py'
SYMBOLS = 'src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
JOIN = 'src/rendering/effects/source_object_function_symbols_tu15.inc'
FRAME = 'src/gameplay/objects/native_tu15_flight_property_query_source_frame.hpp'
TEST = 'tests/native_tu15_flight_property_query_source_tests.cpp'
BODY = re.compile(r'^static void run_e0900\(WorldState &s\) \{.*?^\}', re.M|re.S)
STATE = re.compile(r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',re.M|re.S)
HELPER = re.compile(r'^std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?^\}',re.M|re.S)
BODY_PINS = ('8439613527b310d55ffdb2a7ce3e6f7abc0379f0ff8a0ed945343a23662f44c7',
             '63a4d0a9996e2c8671d16f9bbbe24d2527b1a1c06d52b91e4231ba17f14a790f',
             'afec8f038faa58df44caf71ad42dbf0a656ba01e63eba8cd8f2b10f784171f7d',
             '79c00ea11e6d50eaf04a8f858cac23de28ac41bd48b26e37803b21857f328d91')
DEATH_BODY_PINS=('7f6f154bf4e8adb4a0e08b489b0e92e12f0b6c102b1c97eb631cd6e0942d6e6e',BODY_PINS[1],
                 'e0b5ea5e4dfffe5558fef8823e3749fc8aa793f976e7a5f223bd62d74dc68dca',BODY_PINS[3])
DEATH_DISPATCH_PINS=('2ddcb4ca932da7abcdbbbf8050bead73a3a18e4a4153021913f0555281b8aa20',
                     '1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45',
                     '03799301083f53afa5eb43c5043509a12d3518626043f8cca11c6ce5e0e68ccc',
                     '1940da2e3890172390375a282ec9b1827d5a001fc02f5b85d0367efb9666ac45')
DEATH_CURRENT_RESOLVER='cc3118fc8d131fc3d8beb8e314827a9412e84288f99b72509be63ff966688b61'
STATE_PINS = ('086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
              '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575')
HELPER_PIN = 'e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1'
DISPATCH = re.compile(r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]+\}$',re.M)

@dataclass(frozen=True)
class SourceProfile:
    unit:str
    generator:str
    frame:str
    driver:str
    prepare:Callable
    inputs:Callable
    label:str
    controls:tuple[tuple[str,str,str],...]
    extra_pins:tuple[str,...]=()


def digest(data):
    if isinstance(data,str):
        data=data.encode()
    return hashlib.sha256(data).hexdigest()


def one(pattern,text,label):
    found=pattern.findall(text)
    if len(found)!=1:
        raise ValueError('Missing or ambiguous exact '+label)
    return found[0]


def prepare(scratch,require_binding=False,death=False):
    current=(ROOT/(DEATH_UNIT if death else UNIT)).read_text()
    fresh=scratch/'fresh.cpp'
    manifest=scratch/'generation.json'
    cmd=[sys.executable,'-B',str(ROOT/(DEATH_GEN if death else GEN)),'--output',str(fresh)]
    if death:
        manifest=scratch/'generation/generation.json';cmd+=['--directory',str(manifest.parent)]
    else:cmd+=['--manifest',str(manifest)]
    made=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True,timeout=600)
    if made.returncode:
        raise ValueError('Complete E0900 owning generation failed: '+made.stderr[-8000:])
    generated=fresh.read_text()
    if death:
        import native_death_cycle_flight_query_replacements as death_binding
        if require_binding:
            death_binding.installed_entries(current);death_binding.installed_entries(generated)
        raw=[death_binding.remove_death_cycle_flight_query(text)for text in (current,generated)]
    else:
        from native_object_record_ratio_replacements import remove_object_record_ratio
        if require_binding:
            binding.installed_entries(current);binding.installed_entries(generated)
        raw=[binding.remove_flight_property_query(remove_object_record_ratio(text))for text in (current,generated)]
    variants=raw+[specialize(text)[0] for text in raw]
    bodies=[one(BODY,text,'Primary E0900 body')for text in variants]
    body_pins=DEATH_BODY_PINS if death else BODY_PINS
    if tuple(map(digest,bodies))!=body_pins:
        raise ValueError('Complete E0900 source profile changed')
    states=[one(STATE,text,'Primary LiveState')for text in raw]
    if tuple(map(digest,states))!=STATE_PINS:
        raise ValueError('Complete Primary LiveState changed')
    helpers=[one(HELPER,current,'current DeathCycle E0900 spill helper') if death else '',
             one(HELPER,generated,'fresh E0900 spill helper')]
    if any(digest(helper)!=HELPER_PIN for helper in helpers if helper):
        raise ValueError('Complete fresh spill helper changed')
    dispatcher_hashes=tuple(digest(one(DISPATCH,text,'E0900 dispatch'))for text in variants)
    resolver=''
    if death:
        if dispatcher_hashes!=DEATH_DISPATCH_PINS:
            raise ValueError('Complete DeathCycle dispatch profiles changed')
        resolver=one(re.compile(r'^std::uint32_t selected\([^\n]+',re.M),current,'DeathCycle selected resolver')
        constants=set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',bodies[0]))
        table=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolver))
        if digest(resolver)!=DEATH_CURRENT_RESOLVER or len(constants)!=22 or any(table.get(value)!=value for value in constants):
            raise ValueError('Current DeathCycle E0900 identity projection changed')
    raw_sites=[re.findall(r's\.services\.instruction\((0x[0-9a-f]+u)\)',b) for b in bodies[:2]]
    prepared_sites=[re.findall(r's\.instruction_literal\((0x[0-9a-f]+u)\)',b) for b in bodies[2:]]
    native_sites=set(re.findall(r'0xe0[9a][0-9a-f]{2}u',(ROOT/FRAME).read_text()))
    if len(set(raw_sites[0]))!=76 or raw_sites!=prepared_sites or raw_sites[0]!=raw_sites[1] or native_sites!=set(raw_sites[0]):
        raise ValueError('E0900 source transport instruction boundaries changed')
    joins=(ROOT/JOIN).read_text()
    required=('ObjectFunctionSymbol{0xe093fu,0x6000542u,0x600054eu,0x600054eu,"flight"}',
              'ObjectFunctionSymbol{0xe0952u,0xc0005a4u,0xc0005b0u,0xc0005b0u,"flying_speed"}')
    if any(joins.count(row)!=1 for row in required):
        raise ValueError('Exact E0900 symbol joins changed')
    names={}
    pairs=re.findall(r'\bSymbol\{0x[0-9a-f]+u,0x([0-9a-f]+)u,"([^"]+)"\}',(ROOT/SYMBOLS).read_text())
    pairs+=re.findall(r'ObjectFunctionSymbol\{0x[0-9a-f]+u,0x[0-9a-f]+u,0x[0-9a-f]+u,0x([0-9a-f]+)u,"([^"]+)"\}',joins)
    for key,name in pairs:
        if names.setdefault(int(key,16),name)!=name:
            raise ValueError('Conflicting complete symbol registry rows')
    if len(names)!=108:
        raise ValueError('Incomplete symbol registry fixture')
    chunks=['static std::string fixture_name(std::uint32_t p){switch(p){'+''.join(
        f'case 0x{k:x}u:return {json.dumps(v)};'for k,v in sorted(names.items()))+
        'default:throw std::runtime_error("Undeclared E0900 symbol fixture");}}',
        'static std::uint32_t fixture_key(std::uint32_t p){return (p==0x6000542u||p==0xc0005a4u)?p+12u:p;}']
    for variant,ns in enumerate(('current','fresh')):
        chunks += ['namespace '+ns+' {','using namespace halo2::physics_world_runtime_detail;',states[variant]]
        if helpers[variant]:chunks.append(helpers[variant])
        if death and not variant:chunks.append(resolver)
        for marker,body in (('raw',bodies[variant]),('literal',bodies[variant+2])):
            chunks.append(body.replace('run_e0900(',marker+'(',1))
        chunks.append('}')
        chunks.append(f'static void run_reference({ns}::WorldState&s,unsigned variant){{'
                      f'if(variant&2u){ns}::literal(s);else {ns}::raw(s);}}')
        fresh_flag='true' if variant else 'false'
        fixed='true' if death and not variant else 'false'
        spill=ns+'::native_single_spill' if helpers[variant] else 'nullptr'
        chunks.append(f'static void run_native({ns}::WorldState&s,unsigned variant){{'
                      'using namespace halo2::native_flight_property_query_detail;'
                      f'if(variant&2u)run_source_frame<true,{fresh_flag},{fixed}>(s,{spill});'
                      f'else run_source_frame<false,{fresh_flag},{fixed}>(s,{spill});}}')
    (scratch/'flight_property_query_reference.inc').write_text('\n'.join(chunks)+'\n')
    return dict(generation=cmd, generation_manifest_sha256=digest(manifest.read_bytes()),
                body_sha256=body_pins,state_sha256=STATE_PINS,spill_helper_sha256=HELPER_PIN,
                dispatch_sha256=dispatcher_hashes,death_cycle_current_identity_projection=death,
                raw_prepared_instruction_sites=76,retained_semantic_operations_changed=False,
                symbol_fixture_named_rows=len(names))


def main(profile=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--semantic-mutants',action='store_true')
    p.add_argument('--require-binding',action='store_true')
    p.add_argument('--death-cycle',action='store_true',help='Compare the separate private DeathCycle source copy')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if profile is not None and a.death_cycle:p.error('This profile has no DeathCycle variant')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    a.output.mkdir(parents=True,exist_ok=False)
    unit,generator,frame,driver=(profile.unit,profile.generator,profile.frame,profile.driver)if profile else(UNIT,GEN,FRAME,TEST)
    paths={unit,generator,SYMBOLS,JOIN,frame,driver,'tools/physics_live_support_tu15_entries.json',
           'tools/native_object_type_query_replacements.py','tools/native_object_record_ratio_replacements.py'}
    if profile:paths.update(profile.extra_pins)
    if a.death_cycle:paths.update((DEATH_UNIT,DEATH_GEN,'tools/native_death_cycle_flight_query_replacements.py'))
    for module in tuple(sys.modules.values()):
        file=getattr(module,'__file__',None)
        if file and (path:=Path(file).resolve()).is_relative_to(ROOT/'tools'):paths.add(str(path.relative_to(ROOT)))
    pins={name:digest((ROOT/name).read_bytes())for name in sorted(paths)}
    headers={path.resolve():digest(path.read_bytes())for tree in ('include','src','tests')
             for path in (ROOT/tree).rglob('*')if path.suffix in ('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],compilers=[])
    try:
        if digest(a.xbe.read_bytes())!=PIN:raise ValueError('Wrong original TU1.5 authority')
        report['authority']=profile.prepare(scratch,a.require_binding)if profile else prepare(scratch,a.require_binding,a.death_cycle)
        report['source_owner']=profile.label if profile else 'DeathCycle' if a.death_cycle else 'Primary'
        report['installed_context_authenticated']=a.require_binding
        fixture=scratch/'inputs.json'
        fixture.write_text(json.dumps((profile.inputs if profile else corpus)(Xbe(a.xbe)),separators=(',',':')))
        report['fixture_sha256']=digest(fixture.read_bytes())
        for compiler in a.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name
            base=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                  *('-I'+str(ROOT/tree)for tree in('include','src','tests')),'-I'+str(scratch)]
            if a.sanitizers:base+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            objects=[];commands=[]
            for index,name in enumerate((driver,SYMBOLS)):
                obj=binary.with_suffix(f'.{index}.o');dep=obj.with_suffix('.d');objects.append(str(obj))
                command=base+['-MD','-MF',str(dep),'-c',str(ROOT/name),'-o',str(obj)]
                built=subprocess.run(command,capture_output=True,text=True,timeout=600);commands.append(command)
                if built.returncode:raise ValueError('Flight source compile failed: '+built.stderr[-8000:])
                for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                    path=Path(item).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            link=base+objects+['-o',str(binary)]
            linked=subprocess.run(link,capture_output=True,text=True,timeout=600);commands.append(link)
            if linked.returncode:raise ValueError('Flight source link failed: '+linked.stderr[-8000:])
            ran=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200)
            if ran.returncode:raise ValueError('Flight source comparison failed: '+(ran.stdout+ran.stderr)[-10000:])
            mutants=[]
            if a.semantic_mutants:
                original=(ROOT/frame).read_text()
                controls=(
                    ('wrong-ret-cleanup','s.gp[4]+=16u;return','s.gp[4]+=12u;return'),
                    ('missing-register-pop-c1','step(0xe0993u);s.fp.st(3)=s.fp.st(0);clear_c1();','step(0xe0993u);s.fp.st(3)=s.fp.st(0);'),
                    ('stale-spill-profile','if constexpr(Fresh||DeathCurrent)w(s.memory','if constexpr(false)w(s.memory'),
                    ('early-observer-pop','step(0xe099fu);','step(0xe099fu);if(s.fp.count>1)s.fp.pop();'),
                    ('ignore-live-speed-reread','divide(s.fp.st(0),ext(f(r(s.memory,0x2dcu+s.gp[2],4))))',
                     'divide(s.fp.st(0),ext(_mm_cvtss_f32(s.xmm[0])))'),
                    ('skip-live-flight-branch','step(0xe09c8u);',''),
                )
                if a.death_cycle:
                    controls+=(('replace-fixed-selector-service','if constexpr(DeathCurrent)return original;',
                                'if constexpr(false)return original;'),)
                if profile:controls=profile.controls
                for label,old,new in controls:
                    if original.count(old)!=1:raise ValueError('Missing flight source defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);header=overlay/frame
                    header.parent.mkdir(parents=True);header.write_text(original.replace(old,new,1))
                    mutant=overlay/'bad';command=base[:1]+['-I'+str(overlay/'src')]+base[1:]+[str(ROOT/driver),objects[1],'-o',str(mutant)]
                    built=subprocess.run(command,capture_output=True,text=True,timeout=600)
                    if built.returncode:raise ValueError('Flight source defect failed to compile '+label+': '+built.stderr[-6000:])
                    tested=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=1200)
                    detail=(tested.stdout+tested.stderr)[-6000:]
                    failure=(profile.label if profile else 'Flight query')+' retained source differs'
                    if not tested.returncode or failure not in detail:
                        raise ValueError('Flight source defect survived/unrecognized '+label+': '+detail)
                    mutants.append(dict(name=label,rejected=True,diagnostic=detail))
            report['compilers'].append(dict(compiler=compiler,commands=commands,result=json.loads(ran.stdout),compiled_mutations=mutants))
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:digest((ROOT/name).read_bytes()) for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Compare private D3750 source view with exact VehicleUpdate raw/literal bodies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import subprocess
import sys

from native_vehicle_render_root_replacements import authenticate as authenticate_bbe70
from specialize_native_operands import INSTRUCTION, specialize
from validate_native_vehicle_effect_ratio_tu15 import corpus, original, NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
OWNER = 'src/physics/vehicles/physics_vehicle_update_generated.cpp'
PHANTOM = 'src/physics/physics_phantom_world_tu15_generated.cpp'
GENERATOR = 'tools/generate_vehicle_update_20261011.py'
CORRESPONDENCE = 'analysis/multiplayer/sandbox-source/vehicle-update-20261011/correspondence-verified/report.json'
CORRESPONDENCE_SHA = '4c6d1a4b5733144af7109feaa34d0f06a69b47f1eeabb6123db866734c6f488f'
FRAME = 'src/physics/vehicles/native_tu15_vehicle_effect_ratio_source_frame.hpp'
ADMISSION = 'src/physics/vehicles/native_tu15_vehicle_effect_ratio_source_owner.hpp'
TEST = 'tests/native_tu15_vehicle_effect_ratio_source_tests.cpp'
CONTEXT = tuple(re.compile(x,re.M|re.S) for x in (
    r'^static void run_d26c0\(VehicleState &s\) \{.*?^\}',
    r'^std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',
    r'^static void dispatch\(VehicleState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',
    r'^void source_run_physics_vehicle_update_runtime\(.*?^\}',
))
PINS = (
    ('7ce128976a92d65eb1cf072c32570fe48531f676995c7705b841a105d489fd88',
     '1ad94227d3a1fefcce67499652d13611b97df348eef749f2bb19c323da59196a',
     'd52a1a4589a2d4a0179531ddc498125f06e394e36cb8df7f4a671c6a49692bdf',
     'effa8b2c7a7ce8d6948977f95352a9bf88c026ad9e116c78189910aebfd6241f'),
    ('036d309f5dd4d39d87fa2b27c05ae473f5501a96e1d11fae3cc30fba56b12e1c',
     '1ad94227d3a1fefcce67499652d13611b97df348eef749f2bb19c323da59196a',
     'a8a9f8851187b20c7e0ce414b04fcf6a586f28848eebe8d645c7f2472776bb13',
     '20ef009651ce29cf4e3920f30007800c04f39b9a23845f1c0c450d9276e263cc'),
)


def digest(value):
    return hashlib.sha256(value.encode() if isinstance(value,str) else value).hexdigest()


def authenticate(source,literal):
    parts=[rx.findall(source) for rx in CONTEXT]
    if any(len(part)!=1 for part in parts):raise ValueError('Ambiguous VehicleUpdate ratio owner')
    selected=tuple(part[0] for part in parts)
    if tuple(map(digest,selected))!=PINS[literal]:
        raise ValueError('Complete VehicleUpdate ratio body/resolver/dispatcher/export drift')
    sibling=authenticate_bbe70(source)
    if sibling is None or sibling[-2:]!=(literal,True):
        raise ValueError('VehicleUpdate must preserve its exact installed BBE70 sibling')
    return selected


def binding_controls(unbound,bound,literal,binder):
    match,_,_,_=binder.authenticate(bound)
    prefix=binder.prefix(literal);body=match[0];clean=body.replace(prefix,'',1)
    def replace_body(value):return bound[:match.start()]+value+bound[match.end():]
    variants={
        'wrong-format':replace_body(body.replace(prefix,binder.prefix(not literal),1)),
        'body-end-prefix':replace_body(clean[:-2]+prefix+'\n}'),
        'before-owner-prefix-without-include':prefix+replace_body(clean).replace(binder.INCLUDE,'',1),
        'nested-prefix':replace_body(clean[:len(binder.SIGNATURE)]+f'\n    if(true){{{prefix}}}'+clean[len(binder.SIGNATURE):]),
        'duplicate-prefix-outside-owner':bound+prefix,
        'rogue-prefix-in-unbound-input':prefix+unbound,
        'wrong-owner-prefix':replace_body(clean).replace('static void run_e7e30(VehicleState &s) {',
            'static void run_e7e30(VehicleState &s) {'+prefix,1),
        'misplaced-include':bound.replace(binder.INCLUDE,'',1)+'\n'+binder.INCLUDE,
        'changed-export':bound.replace('case 0xd26c0u:run_d26c0(s);break;',
            'case 0xd26c0u:run_e7e30(s);break;',1),
        'changed-resolver':bound.replace('case 0x4e78d0u:return 0x4ecbd0u;',
            'case 0x4e78d0u:return 0x4ecbd4u;',1),
        'changed-dispatcher':bound.replace('run_d26c0(s);return;}','run_e7e30(s);return;}',1),
        'changed-body-read':replace_body(body.replace('0x334u+s.gp[1]','0x330u+s.gp[1]',1)),
        'missing-observed-shift':replace_body(body.replace(binder.OBSERVED_SHIFT,binder.MODELED_SHIFT,1)),
        'rogue-ordered-helper':unbound+'\n'+binder.OBSERVED_SHIFT,
    }
    for label,text in variants.items():
        if text in (bound,unbound):raise ValueError('Unapplied VehicleUpdate binding control '+label)
        for operation in (binder.authenticate,binder.install,binder.installed_entries):
            try:operation(text)
            except ValueError:pass
            else:raise ValueError('Invalid VehicleUpdate ratio binding accepted '+label+' '+operation.__name__)
    return sorted(variants)


def references(scratch,binding_candidate=False,require_installed=False):
    correspondence=ROOT/CORRESPONDENCE
    if digest(correspondence.read_bytes())!=CORRESPONDENCE_SHA:
        raise ValueError('VehicleUpdate original correspondence drift')
    fresh=scratch/'fresh.cpp';manifest=scratch/'fresh.json'
    command=[sys.executable,'-B',str(ROOT/GENERATOR),'--correspondence',str(correspondence),'--output',str(fresh),'--manifest',str(manifest)]
    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=90)
    if result.returncode:raise ValueError('Genuine VehicleUpdate generation failed: '+(result.stdout+result.stderr)[-6000:])
    authority={};definitions={};syntax_deps=[]
    if binding_candidate:
        import native_vehicle_effect_ratio_replacements as binder
    for label,path in (('current',ROOT/OWNER),('fresh',fresh)):
        raw=path.read_text();prepared,receipt=specialize(raw)
        if digest(prepared)!=receipt['specialized_sha256']:raise ValueError('VehicleUpdate preparation receipt drift')
        if require_installed:
            if (binder.installed_entries(raw)!=['d26c0'] or binder.installed_entries(prepared)!=['d26c0'] or
                receipt['native_vehicle_effect_ratio_entries']!=['d26c0']):
                raise ValueError('Actual VehicleUpdate ratio binding is required in current and fresh formats')
            bound_raw,bound_prepared=raw,prepared
            raw,prepared=binder.remove(raw),binder.remove(prepared)
            if specialize(raw)[0]!=prepared:
                raise ValueError('Installed VehicleUpdate removal does not recover the complete prepared reference')
        if binding_candidate:
            if not require_installed:bound_raw,bound_prepared=binder.install(raw),binder.install(prepared)
            if binder.rebind_prepared(specialize(bound_raw)[0])!=bound_prepared:
                raise ValueError('VehicleUpdate literal binding differs from exact late retag')
        for literal,text in ((False,raw),(True,prepared)):
            body,resolver,*_=authenticate(text,literal)
            format_name='prepared' if literal else 'raw'
            retained=binder.ordered_body(body) if binding_candidate else body
            row=f'namespace effect_ratio_{format_name} {{\n{resolver}\n{retained}\n}}\n'
            if label=='current':definitions[format_name]=row
            elif row!=definitions[format_name]:raise ValueError('Fresh actual VehicleUpdate projection differs')
            authority[label+'-'+format_name]=dict(unit_sha256=digest(text),body_sha256=digest(body),context_sha256=PINS[literal])
            if binding_candidate:
                bound=bound_prepared if literal else bound_raw
                if (binder.installed_entries(bound)!=['d26c0'] or binder.remove(bound)!=text or
                    binder.install(bound)!=bound or binder.installed_entries(text)):
                    raise ValueError('Exact VehicleUpdate binding cannot round trip the complete source unit')
                bound_body=binder.authenticate(bound)[0][0];overlay=binder.prefix(literal)
                marker=bound_body.index(overlay)+len(overlay)
                observed=bound_body[:marker]+'\n    effect_ratio_retained_entry();'+bound_body[marker:]
                row=f'namespace effect_ratio_bound_{format_name} {{\n{resolver}\n{observed}\n}}\n'
                if label=='current':definitions['bound-'+format_name]=row
                elif row!=definitions['bound-'+format_name]:raise ValueError('Fresh bound ratio projection differs')
                owned=scratch/(label+'-'+format_name+'.cpp');owned.write_text(bound)
                dep=owned.with_suffix('.d')
                syntax=['clang++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
                        '-MMD','-MF',str(dep),str(owned)]
                result=subprocess.run(syntax,cwd=ROOT,capture_output=True,text=True,timeout=120)
                if result.returncode:raise ValueError('Whole bound VehicleUpdate syntax failed: '+result.stderr[-6500:])
                syntax_deps.append(dep)
                authority[label+'-'+format_name].update(bound_unit_sha256=digest(bound),bound_body_sha256=digest(bound_body),
                    negative_controls=binding_controls(text,bound,literal,binder),whole_syntax=True)
    raw=CONTEXT[0].search((ROOT/OWNER).read_text())[0]
    sites=[int(match[1].removesuffix('u'),16) for match in INSTRUCTION.finditer(raw)]
    phantom=(ROOT/PHANTOM).read_text()
    tables=re.findall(r'^constexpr std::array<Binding,16402> vehicle_sites=\{\{\n.*?^\}\};',phantom,re.M|re.S)
    if len(tables)!=1:raise ValueError('Actual Phantom vehicle site table missing')
    table=tables[0]
    rows={int(a,16):int(b,16) for a,b in re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u\}',table)}
    if len(sites)!=34 or any(rows.get(pc)!=pc+0x1090 for pc in sites) or len(rows)!=16402:
        raise ValueError('Actual Phantom D3750 mapping drift')
    if ('result.instruction=PhysicsInstructionRemapTu15{&services,sites};' not in phantom or
        'source_run_physics_phantom_force_tu15_runtime(m,entry,abi,fp,bound(vehicle_sites),observer(vehicle_sites));' not in phantom):
        raise ValueError('Actual Phantom remap producer drift')
    table_definition=table.replace('std::array<Binding,16402> vehicle_sites',
        'std::array<halo2::PhysicsInstructionRemapTu15::Binding,16402> vehicle_effect_ratio_actual_sites',1)
    (scratch/'vehicle_effect_ratio_reference.inc').write_text(''.join(definitions.values())+table_definition+'\n')
    proof=dict(contexts=authority,phantom_table_sha256=digest(table),generation_command=command,
               generation_manifest_sha256=digest(manifest.read_bytes()),source_sites=len(sites))
    if binding_candidate:
        stub=scratch/'unsupported-backend.cpp';exe=scratch/'unsupported-backend'
        stub.write_text('''#include "physics/vehicles/native_tu15_vehicle_effect_ratio_ordered.hpp"
struct State {
  struct Flags {
    unsigned calls{};
    unsigned shift(unsigned value,unsigned count,unsigned width) {
      ++calls;return value+count+width;
    }
  } flags;
};
int main(){State s;
 const auto r=halo2::native_vehicle_effect_ratio_ordered_detail::shift_index(s,7u);
 return r==15u&&s.flags.calls==1u?0:1;
}
''')
        unsupported=['clang++','-std=c++20','-U__x86_64__','-U__i386__','-nostdinc++','-I'+str(ROOT/'src'),
                     str(stub),'-o',str(exe)]
        result=subprocess.run(unsupported,cwd=ROOT,capture_output=True,text=True,timeout=90)
        if result.returncode:raise ValueError('Ordered shift parsed unsupported backend internals: '+result.stderr[-4000:])
        subprocess.run([str(exe)],check=True,timeout=20)
        proof['unsupported_backend_fallback']=dict(command=unsupported,executed_modeled_shift=True)
    return proof,syntax_deps


def defects(binding_candidate):
    text=(ROOT/FRAME).read_text()
    seams={
        'raw-uses-literal-shortcut':('if constexpr(Literal)s_.instruction_literal(pc);','if constexpr(true)s_.instruction_literal(pc);'),
        'acquire-pool-operand-before-callback':('at(0xd26c6u);g[2]=word(g[1]+0x44u);',
            'const auto pool_rows=g[1]+0x44u;at(0xd26c6u);g[2]=word(pool_rows);'),
        'drop-late-baseline-acquisition':('s_.xmm[0]=_mm_sub_ss(s_.xmm[0],scalar(s_.gp[1]+0x338u));',
            's_.xmm[0]=_mm_sub_ss(s_.xmm[0],s_.xmm[1]);'),
        'drop-lahf-upper-bits':('s_.set(0,s_.flags.bits()|2u,1,8);','s_.gp[0]=s_.flags.bits()|2u;'),
    }
    for label,(old,new) in seams.items():
        if text.count(old)!=1:raise ValueError('Exact source defect seam drift '+label)
        yield label,FRAME,text.replace(old,new,1),False,'effect ratio retained route differs'
    if binding_candidate:
        text=(ROOT/ADMISSION).read_text()
        for label,old,new,forced,diagnostic in (
            ('use-modeled-shift-in-native-source','run_source_private<Literal,true>(s)',
             'run_source_private<Literal,false>(s)',False,'effect ratio retained route differs'),
            ('ignore-reference-control','||reference()||','||false||',True,'effect ratio actual route did not enter'),
            ('ignore-title-image','s.image!=SourcePhysicsContactImage::TitleUpdate15||','false||',False,
             'Effect ratio refusal did not reach untouched first original statement'),
        ):
            if text.count(old)!=1:raise ValueError('Exact admission defect seam drift '+label)
            yield label,ADMISSION,text.replace(old,new,1),forced,diagnostic


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    parser.add_argument('--mutant',action='append',help='With --mutations, run only this named compiled defect')
    parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--binding-candidate',action='store_true')
    parser.add_argument('--require-installed',action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:parser.error('Use tools/run_test.py')
    if args.mutant and not args.mutations:parser.error('--mutant needs --mutations')
    if args.require_installed and not args.binding_candidate:parser.error('--require-installed needs --binding-candidate')
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    if not NEW_DEFAULT.is_file():
        print('SKIP missing pinned English TU1.5 owning generator input')
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    before={p.resolve():digest(p.read_bytes()) for tree in ('src','include','tests','tools')for p in (ROOT/tree).rglob('*')
            if p.suffix in ('.hpp','.h','.cpp','.inc','.def','.py') and p.is_file()}
    deps={ROOT/path for path in (FRAME,TEST,PHANTOM,OWNER,GENERATOR,'tools/validate_native_vehicle_effect_ratio_source_tu15.py')}
    report=dict(passed=False,owned_scratch=str(scratch),private_unbound=not args.require_installed,
        installed_route_required=args.require_installed,compilers=[],retained_large_artifacts=[])
    try:
        if digest(NEW_DEFAULT.read_bytes())!=NEW_PIN:raise ValueError('Original TU1.5 image drift')
        report['authority'],syntax_deps=references(scratch,args.binding_candidate,args.require_installed)
        def record_dependencies(dep):
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path.is_relative_to(ROOT) and not path.is_relative_to(scratch):deps.add(path)
        for dep in syntax_deps:record_dependencies(dep)
        fixture=scratch/'fixture.json';rows=corpus(original.Xbe(NEW_DEFAULT),args.smoke)
        fixture.write_text(json.dumps(rows,separators=(',',':')))
        for module in tuple(sys.modules.values()):
            if name:=getattr(module,'__file__',None):
                path=Path(name).resolve()
                if path in before and path.is_relative_to(ROOT/'tools'):deps.add(path)
        mutants=list(defects(args.binding_candidate)) if args.mutations else []
        if args.mutant:
            selected=set(args.mutant)
            if not selected<={defect[0] for defect in mutants}:raise ValueError('Unknown effect ratio mutant')
            mutants=[defect for defect in mutants if defect[0] in selected]
        report['compiled_defect_selection']=[defect[0] for defect in mutants]
        environment={k:v for k,v in os.environ.items() if k!='HALO2_REFERENCE_VEHICLE_EFFECT_RATIO'}
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),
                str(ROOT/TEST),'-o',str(binary),'-MMD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            if args.binding_candidate:command+=['-DHALO2_EFFECT_RATIO_BOUND_TEST=1']
            built=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
            if built.returncode:raise ValueError('Source effect ratio compile failed: '+built.stderr[-7000:])
            record_dependencies(dep)
            result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=180,env=environment)
            if result.returncode:raise ValueError('Source effect ratio comparison failed: '+(result.stdout+result.stderr)[-7000:])
            row=dict(compiler=compiler,command=command,result=json.loads(result.stdout),compiled_defects=[])
            report['compilers'].append(row)
            if args.binding_candidate:
                forced=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=180,
                    env={**environment,'HALO2_REFERENCE_VEHICLE_EFFECT_RATIO':'1'})
                if forced.returncode:raise ValueError('VehicleUpdate real forced reference differs: '+(forced.stdout+forced.stderr)[-6000:])
                row['forced_reference']=json.loads(forced.stdout)
            for label,path,text,forced,expected_failure in mutants:
                overlay=scratch/('defect-'+label);target=overlay/path.removeprefix('src/')
                target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text)
                broken=scratch/(Path(compiler).name+'-'+label)
                compile_defect=[command[0],'-I'+str(overlay),*command[1:]]
                compile_defect[compile_defect.index('-o')+1]=str(broken)
                compile_defect[compile_defect.index('-MF')+1]=str(broken.with_suffix('.d'))
                built=subprocess.run(compile_defect,cwd=ROOT,capture_output=True,text=True,timeout=180)
                if built.returncode:raise ValueError('Source effect ratio defect compile failed: '+built.stderr[-4000:])
                defect_env={**environment,'HALO2_REFERENCE_VEHICLE_EFFECT_RATIO':'1'} if forced else environment
                rejected=subprocess.run([str(broken),str(fixture)],capture_output=True,text=True,timeout=180,env=defect_env)
                diagnostic=rejected.stdout+rejected.stderr
                if rejected.returncode!=1 or expected_failure not in diagnostic:
                    raise ValueError('Source effect ratio defect survived '+label+': '+diagnostic[-4000:])
                row['compiled_defects'].append(dict(name=label,sha256=digest(text),diagnostic=diagnostic[-1600:]))
        report.update(passed=True,scope=('Actual installed' if args.require_installed else 'Private candidate')+
            ' VehicleUpdate source-body extract, complete raw/prepared callbacks and genuine Phantom remap table. No full exported chain or parent admission claim in this test.')
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        report['inputs_before']={str(p.relative_to(ROOT)):before.get(p) for p in sorted(deps)}
        report['inputs_after']={str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in sorted(deps)}
        if report['inputs_before']!=report['inputs_after']:report.update(passed=False,input_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())

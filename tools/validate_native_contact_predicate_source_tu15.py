#!/usr/bin/env python3
"""Exact 303800 complete retained direct/literal and retired-I/O proof."""
import argparse
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import re
import runpy
import shlex
import subprocess
import sys

from specialize_native_operands import INSTRUCTION, MAPPING, specialize
import native_contact_predicate_replacements as binder

ROOT = Path(__file__).resolve().parents[1]
CPP = 'src/physics/physics_live_support_tu15_generated.cpp'
TEST = 'tests/native_tu15_contact_predicate_source_tests.cpp'
FRAME = 'src/physics/runtime/native_tu15_contact_predicate_source_frame.hpp'
KERNEL = 'include/halo2/native_tu15_contact_predicate_kernel.hpp'
GENERATOR = 'tools/generate_physics_live_support_tu15.py'
ENTRIES = 'tools/physics_live_support_tu15_entries.json'
PARTITION = 'tools/split_native_translation_unit.py'
SIGNATURE = 'static void run_303800(WorldState &s) {'
BODY = re.compile(re.escape(SIGNATURE) + r'.*?\n\}', re.S)
DISPATCH = re.compile(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}')
RAW_BODY = '79b2196359a4ff74201eb00415220048669183c05087f6ecd9a42325f852bc87'
PREPARED_BODY = '6856c5049553512dc212013bb7e791ed21948ea28e4143d9f255690a1b91b673'
RESOLVER = '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
RAW_DISPATCH = '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'
PREPARED_DISPATCH = '37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'
FRESH_RAW_BODY = '89d7da38867c4aa88439f76786039e85d134714924cf0b30932fe432a41515c6'
FRESH_PREPARED_BODY = '999d5b9290123fe306c099de877c47a219ad6288b6e39040bcb71b884b0fd804'
FRESH_RAW_DISPATCH = 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
FRESH_PREPARED_DISPATCH = '8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'
NEXT = tuple('3038'+x for x in (
    '01','0c','0f','10','16','24','28','2c','2f','53','57','5a',
    '64','67','8e','92','95','9f','a2','a3','a6','ac','af','b0','b3'))
ORDERED_EXPRESSIONS = (
    ('_mm_mul_ps(s.xmm[0],s.xmm[2])','product(s.xmm[0],s.xmm[2])'),
    ('_mm_add_ss(s.xmm[2],s.xmm[0])','sum_low(s.xmm[2],s.xmm[0])'),
    ('_mm_add_ss(s.xmm[3],s.xmm[2])','sum_low(s.xmm[3],s.xmm[2])'),
    ('_mm_sub_ps(s.xmm[2],s.xmm[0])','difference(s.xmm[2],s.xmm[0])'),
    ('_mm_mul_ps(s.xmm[0],s.xmm[1])','product(s.xmm[0],s.xmm[1])'),
    ('_mm_add_ss(s.xmm[1],s.xmm[0])','sum_low(s.xmm[1],s.xmm[0])'),
    ('_mm_add_ss(s.xmm[2],s.xmm[1])','sum_low(s.xmm[2],s.xmm[1])'),
)


def digest(value):
    return hashlib.sha256(value.encode() if isinstance(value,str) else value).hexdigest()


def pin(path):
    return digest(path.read_bytes())


def unique(pattern, text):
    found = pattern.findall(text)
    if len(found)!=1:
        raise ValueError('Expected one complete '+pattern.pattern[:100])
    return found[0]


def route(body, prepared):
    anchors = [f's.instruction_literal(0x{pc}u);' if prepared else
               f'if(s.services.instruction){{s.services.instruction(0x{pc}u);}}' for pc in NEXT]
    labels = [(anchor, 'NativeContactPredicateResume_'+pc+': ') for anchor,pc in zip(anchors,NEXT,strict=True)]
    suffix = body
    for anchor,label in labels:
        if suffix.count(anchor)!=1:
            raise ValueError('Continuation anchor drift '+anchor)
        suffix = suffix.replace(anchor,label+anchor,1)
    reconstructed = suffix
    for anchor,label in labels:
        if reconstructed.count(label+anchor)!=1:
            raise ValueError('Continuation is not immediately before its original callback')
        reconstructed = reconstructed.replace(label+anchor,anchor,1)
    if reconstructed!=body:
        raise ValueError('Retained continuation body cannot be reversed')
    switch = '\nswitch(resume){case 0u:break;'+''.join(
        f'case 0x{pc}u:goto NativeContactPredicateResume_{pc};' for pc in NEXT)
    switch += 'default:throw std::logic_error("Foreign predicate continuation");}'
    resume_signature = SIGNATURE.replace('run_303800','resume_303800').replace('&s)','&s, std::uint32_t resume)')
    return suffix.replace(SIGNATURE,resume_signature+switch,1)


def ordered_body(body):
    output=body
    for old,new in ORDERED_EXPRESSIONS:
        if output.count(old)!=1:
            raise ValueError('Exact ordered expression drift '+old)
        output=output.replace(old,new,1)
    reverted=output
    for old,new in ORDERED_EXPRESSIONS:
        if reverted.count(new)!=1:
            raise ValueError('Exact ordered expression cannot reverse '+new)
        reverted=reverted.replace(new,old,1)
    if reverted!=body:raise ValueError('Ordered overlay does not recover original source body')
    return output


def generate_fresh(scratch):
    dest=scratch/'fresh.cpp'
    command=[str(ROOT/GENERATOR),'--output',str(dest),'--manifest',str(scratch/'generation')]
    captured=io.StringIO()
    argv=sys.argv
    try:
        sys.argv=command
        with contextlib.redirect_stdout(captured):
            try:runpy.run_path(command[0],run_name='__main__')
            except SystemExit as result:
                if result.code:raise
    finally:sys.argv=argv
    return dest.read_text(),dict(command=command,output=captured.getvalue()[-3000:])


def negative_controls(unbound, bound, literal):
    match,_,_,_=binder.authenticate(bound)
    body=match[0]
    prefix=binder.prefix(literal)
    marker='halo2::'+binder.NAMESPACE+'::bridge'
    def altered(text):return bound[:match.start()]+text+bound[match.end():]
    def candidates():
        yield 'prefix-before-owner',bound[:match.start()]+prefix+altered(body.replace(prefix,'',1))[match.start():]
        yield 'prefix-at-body-end',altered(body.replace(prefix,'',1)[:-2]+prefix+'\n}')
        yield 'wrong-format',altered(body.replace(prefix,binder.prefix(not literal),1))
        yield 'prefix-in-unbound-earlier-helper-without-include',prefix+'\n'+unbound
        yield 'outside-namespace',f'\nvoid foreign(){{{marker}<false>(s);}}\n'+bound
        yield 'without-include',bound.replace(binder.INCLUDE,'',1)
        first=binder.ORDERED_EXPRESSIONS[0][1]
        yield 'ordered-destination-swapped',altered(body.replace(first,first.replace(
            's.xmm[0],s.xmm[2]','s.xmm[2],s.xmm[0]'),1))
        for n,pc in enumerate(NEXT):
            label=binder.LABEL+pc+': '
            destination=binder.callback(NEXT[(n+1)%len(NEXT)],literal)
            yield 'relocated-'+pc,altered(body.replace(label,'',1).replace(destination,label+destination,1))
    cases=[]
    for label,variant in candidates():
        if variant in (bound,unbound):raise ValueError('Source defect did not modify its subject '+label)
        for entry in (binder.authenticate,binder.install):
            try:entry(variant)
            except ValueError:pass
            else:raise ValueError('Invalid contact-predicate overlay accepted '+label+' '+entry.__name__)
        cases.append(label)
    return cases


def partition_syntax(scratch, name, text):
    cpp=scratch/(name+'-bound.cpp');cpp.write_text(text)
    destination=scratch/(name+'-partitions')
    command=[sys.executable,'-B',str(ROOT/PARTITION),'--source',str(cpp),'--output',str(destination),'--count','32']
    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
    if result.returncode:raise ValueError('Contact-predicate partitioning failed: '+result.stderr[-5000:])
    owner=[path for path in destination.glob('body_*.cpp') if 'run_303800(WorldState &s) {' in path.read_text()]
    if len(owner)!=1:raise ValueError('Expected one actual partitioned contact-predicate owner')
    commands=[];depfiles=[]
    for path in (destination/'router.cpp',owner[0]):
        dep=path.with_suffix('.d')
        build=['clang++','-std=c++20','-fsyntax-only','-fno-fast-math','-ffp-contract=off','-frounding-math',
               '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MMD','-MF',str(dep),str(path)]
        result=subprocess.run(build,cwd=ROOT,capture_output=True,text=True,timeout=180)
        if result.returncode:raise ValueError('Actual partitioned contact-predicate syntax failed: '+result.stderr[-6500:])
        commands.append(build)
        depfiles.append(str(dep))
    return dict(partition=command,owner=owner[0].name,syntax=commands,depfiles=depfiles)


def references(scratch, ordered, bound_candidate, fresh, structure, require_installed=False):
    raw, generation = generate_fresh(scratch) if fresh else ((ROOT/CPP).read_text(),None)
    prepared, receipt = specialize(raw)
    if digest(prepared)!=receipt['specialized_sha256']:
        raise ValueError('Specializer receipt does not identify returned bytes')
    installed=None
    if require_installed:
        if (binder.installed_entries(raw)!=['303800'] or
            binder.installed_entries(prepared)!=['303800'] or
            receipt['native_contact_predicate_entries']!=['303800']):
            raise ValueError('303800 must already be installed in actual raw and prepared owners')
        installed=(raw,prepared)
        raw,prepared=map(binder.remove,installed)
        if specialize(raw)[0]!=prepared:
            raise ValueError('Exact installed removal does not recover the prepared reference unit')
    outputs = {}
    bodies = {}
    rows=(('raw',raw,FRESH_RAW_BODY,FRESH_RAW_DISPATCH),
          ('prepared',prepared,FRESH_PREPARED_BODY,FRESH_PREPARED_DISPATCH)) if fresh else (
          ('raw',raw,RAW_BODY,RAW_DISPATCH),('prepared',prepared,PREPARED_BODY,PREPARED_DISPATCH))
    for label,text,body_pin,dispatch_pin in rows:
        body = unique(BODY,text)
        if digest(body)!=body_pin or digest(unique(DISPATCH,text))!=dispatch_pin:
            raise ValueError('Complete predicate body/dispatcher drift in '+label)
        resolver = MAPPING.search(text)
        if not resolver or digest(resolver[0])!=RESOLVER:
            raise ValueError('Complete predicate selected-operand resolver drift')
        bodies[label]=body
        binder.authenticate(text)
        outputs[label]=dict(unit=digest(text),body=body_pin,resolver=RESOLVER,dispatcher=dispatch_pin,
            **{key:digest(binder.only(pattern,text)[0]) for key,pattern in
               (('export',binder.EXPORT),('owns',binder.OWNS),('state',binder.STATE))})
    expected,count = INSTRUCTION.subn(lambda match:f's.instruction_literal({match[1]});',bodies['raw'])
    if count!=60 or expected!=bodies['prepared']:
        raise ValueError('Prepared predicate is not the exact all-60 literal spelling')
    for label,body in tuple(bodies.items()):
        bodies[label]=ordered_body(body) if ordered else body
        outputs[label]['compiled_body']=digest(bodies[label])
    definitions=''.join(
        f'namespace {label}_reference {{\n'+body+'\n'+route(body,label=='prepared')+'\n}\n'
        for label,body in bodies.items())
    if bound_candidate:
        bound_raw,bound_prepared=installed if installed is not None else (binder.install(raw),binder.install(prepared))
        if binder.rebind_prepared(specialize(bound_raw)[0])!=bound_prepared:
            raise ValueError('Authenticated raw owner does not prepare to identical literal binding')
        for label,text,source in (('raw',bound_raw,raw),('prepared',bound_prepared,prepared)):
            if binder.remove(text)!=source or binder.install(text)!=text:
                raise ValueError('Private bound body does not round trip its complete unit')
            body=binder.authenticate(text)[0][0]
            definitions+=f'namespace {label}_bound {{\n'+body+'\n}\n'
            outputs[label].update(bound_body=digest(body),bound_unit=digest(text))
            if structure:
                outputs[label]['auth_refusals']=negative_controls(source,text,label=='prepared')
                outputs[label]['partition']=partition_syntax(scratch,label,text)
    (scratch/'contact_predicate_source_reference.inc').write_text(definitions)
    return outputs,generation


def defects():
    source=(ROOT/FRAME).read_text()
    def change(old,new):
        if source.count(old)!=1:
            raise ValueError('Exact predicate source defect seam drift '+old)
        return source.replace(old,new,1)
    compare='    s_.fp.comparison(physics_step_detail::ext(physics_step_detail::f(physics_step_detail::r(s_.memory,address,4))),1);\n'
    checkpoint='        // The FCOMP memory access and logical pop retire before FNSTSW AX.\n        retired(next);\n'
    return {
        'skip-live-policy-check':change('if (!source_continuation_allowed<Format>(s_))','if (false)'),
        'stale-scratch-without-reread':change('load_float(read,compare,s_.gp[4]+12u);',
            's_.fp.push(physics_step_detail::ext(_mm_cvtss_f32(s_.xmm[value_reg])));retired(compare);'),
        'cached-normal-after-positions':change('__m128 separation_normal() const','__m128 unused_separation_normal() const'),
        'push-publication-before-source-esp':change('s_.push(pc,s_.gp[reg]);',
            'const auto dest=s_.gp[4]-4u;physics_step_detail::w(s_.memory,s_.observe,pc,dest,4,s_.gp[reg]);s_.gp[4]=dest;'),
        'checkpoint-before-x87-compare':change('    '+compare+checkpoint,'        retired(next);\n    '+compare),
    }


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--ordered-reference',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    parser.add_argument('--bound-candidate',action='store_true')
    parser.add_argument('--require-installed',action='store_true')
    parser.add_argument('--fresh',action='store_true')
    parser.add_argument('--structure',action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if args.require_installed and not args.bound_candidate:
        parser.error('--require-installed needs --bound-candidate')
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    if args.fresh:
        from research_mp_sandbox_update_functions import NEW_DEFAULT
        if not NEW_DEFAULT.is_file():
            print('SKIP missing pinned English TU1.5 generator input')
            return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    before={p.resolve():pin(p) for tree in ('src','include','tests','tools') for p in (ROOT/tree).rglob('*')
            if p.suffix in ('.hpp','.h','.cpp','.inc','.def','.py') and p.is_file()}
    if args.fresh:before[ROOT/ENTRIES]=pin(ROOT/ENTRIES)
    deps={ROOT/name for name in (CPP,TEST,FRAME,KERNEL)}
    if args.fresh:deps.update(ROOT/name for name in (GENERATOR,ENTRIES))
    if args.structure:deps.add(ROOT/PARTITION)
    for module in tuple(sys.modules.values()):
        if name:=getattr(module,'__file__',None):
            path=Path(name).resolve()
            if path in before and path.is_relative_to(ROOT/'tools'):deps.add(path)
    report=dict(passed=False,private_unbound=not args.require_installed,installed_route_required=args.require_installed,
        owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        report['source_authority'],report['generation']=references(scratch,args.ordered_reference,
            args.bound_candidate,args.fresh,args.structure,args.require_installed)
        def record_dependencies(dep):
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path.is_relative_to(ROOT) and not path.is_relative_to(scratch):deps.add(path)
        if args.structure:
            for row in report['source_authority'].values():
                for name in row['partition']['depfiles']:record_dependencies(Path(name))
        report['private_ordered_expressions']=list(ORDERED_EXPRESSIONS) if args.ordered_reference else []
        report['original_source_sites']=60
        report['next_pc_labels']=list(NEXT)
        mutation_inputs=defects() if args.mutations else {}
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name
            dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),
                str(ROOT/TEST),'-o',str(binary),'-MMD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            if args.bound_candidate:command+=['-DHALO2_CONTACT_PREDICATE_BOUND_TEST=1']
            result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
            if result.returncode:raise ValueError('Predicate retained proof did not compile: '+result.stderr[-7000:])
            record_dependencies(dep)
            result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120)
            observed=dict(compiler=compiler,command=command,exit_code=result.returncode,
                output=result.stdout.strip(),diagnostic=result.stderr[-6000:],compiled_defects=[])
            report['compilers'].append(observed)
            if result.returncode:
                raise ValueError('Predicate retained source differs: '+(result.stdout+result.stderr)[-7000:])
            if args.bound_candidate:
                ref=subprocess.run([str(binary)],capture_output=True,text=True,timeout=120,
                    env={**os.environ,'HALO2_REFERENCE_CONTACT_PREDICATE':'1'})
                observed['forced_reference']=dict(exit_code=ref.returncode,output=ref.stdout.strip(),diagnostic=ref.stderr[-4000:])
                if ref.returncode:
                    raise ValueError('Bound predicate forced-reference stream differs: '+(ref.stdout+ref.stderr)[-6000:])
            for label,text in mutation_inputs.items():
                overlay=scratch/('defect-'+label)
                target=overlay/FRAME.removeprefix('src/')
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_text(text)
                broken=scratch/(Path(compiler).name+'-'+label)
                control=[command[0],'-I'+str(overlay),*command[1:]]
                control[control.index('-o')+1]=str(broken)
                control[control.index('-MF')+1]=str(broken.with_suffix('.d'))
                built=subprocess.run(control,cwd=ROOT,capture_output=True,text=True,timeout=180)
                if built.returncode:raise ValueError('Predicate source defect failed to compile: '+built.stderr[-7000:])
                failed=subprocess.run([str(broken)],capture_output=True,text=True,timeout=120)
                if failed.returncode!=1 or 'predicate source ' not in failed.stderr:
                    raise ValueError('Predicate source defect did not produce a semantic failure '+label+': '+failed.stderr[-6000:])
                observed['compiled_defects'].append(dict(name=label,sha256=pin(target),command=control,
                    exit_code=failed.returncode,diagnostic=failed.stderr[-2000:]))
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        for module in tuple(sys.modules.values()):
            if name:=getattr(module,'__file__',None):
                path=Path(name).resolve()
                if path in before and path.is_relative_to(ROOT/'tools'):deps.add(path)
        report['inputs_before']={str(p.relative_to(ROOT)):before.get(p) for p in sorted(deps)}
        report['inputs_after']={str(p.relative_to(ROOT)):pin(p) for p in sorted(deps)}
        if report['inputs_before']!=report['inputs_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())

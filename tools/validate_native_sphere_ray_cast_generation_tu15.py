#!/usr/bin/env python3
"""Complete fresh/current Primary sphere ownership and whole-unit syntax."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='clang++');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    os.chdir(ROOT);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    import native_sphere_ray_cast_replacements as binding
    from native_sphere_ray_cast_inputs import UNIT
    from specialize_native_operands import specialize
    from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER
    source=(ROOT/UNIT).read_text();before=sha(ROOT/UNIT)
    tools={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'tools').rglob('*.py')}
    headers={p.resolve():sha(p)for tree in('include','src')for p in(ROOT/tree).rglob('*')if p.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],production_binding_required=args.require_binding,source_before=before)
    try:
        fresh=scratch/'fresh.cpp';metadata=scratch/'generator-tools.json'
        command=[str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'generator.json')]
        result=subprocess.run([sys.executable,'-B','-c',CHILD_WRAPPER,str(metadata),*command],capture_output=True,text=True,timeout=300)
        if result.returncode:raise ValueError('Complete sphere Primary generation failed '+result.stderr[-7000:])
        generated=fresh.read_text();current_match,current_body=binding.authenticate(source)
        untrusted=binding.BODY.search(generated)
        if untrusted:
            candidate=binding.canonical_body(untrusted[0])
            report['fresh_body_sha256']=binding.digest(candidate)
            if candidate!=current_body:
                report['fresh_body_diff']='\n'.join(list(difflib.unified_diff(current_body.splitlines(),candidate.splitlines(),fromfile='current authenticated sphere',tofile='fresh unqualified sphere'))[:180])[:30000]
        fresh_match,fresh_body=binding.authenticate(generated)
        if binding.original_projection(current_body)!=binding.original_projection(fresh_body):raise ValueError('Complete fresh/current sphere projection differs')
        current=binding.replace_sphere_ray_cast(source);bound=binding.replace_sphere_ray_cast(generated)
        if args.require_binding and(current!=source or bound!=generated):raise ValueError('Current/fresh sphere default missing')
        if binding.remove_sphere_ray_cast(current)!=source and binding.PREFIX not in current_match[0]:raise ValueError('Private sphere installer changed unrelated source')
        if binding.replace_sphere_ray_cast(current)!=current or binding.replace_sphere_ray_cast(bound)!=bound:raise ValueError('Sphere installer not idempotent')
        rejected=0
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',generated,re.S)[0]
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',generated)[0]
        mutations=(generated.replace(fresh_match[0],'',1),
            generated.replace(fresh_match[0],fresh_match[0].replace('0xcu','0x10u',1),1),
            generated.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
            generated.replace(dispatcher,dispatcher.replace('run_2e8dd0(s);','run_306220(s);',1),1),
            bound.replace(binding.PREFIX,binding.PREFIX.replace('::bridge','::wrong_bridge'),1),bound.replace(binding.INCLUDE,'',1))
        for changed in mutations:
            if changed in(generated,bound):raise ValueError('Missing fresh sphere mutation witness')
            try:binding.authenticate(changed);binding.replace_sphere_ray_cast(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Fresh sphere context mutation admitted')
        paths=[];receipts=[]
        for name,text in(('current',current),('fresh',bound)):
            prepared,receipt=specialize(text);receipts.append(dict(name=name,receipt_sha256=binding.digest(json.dumps(receipt,sort_keys=True))))
            if binding.PREFIX not in binding.BODY.search(prepared)[0]:raise ValueError('Prepared sphere native route lost')
            for suffix,value in(('raw',text),('prepared',prepared)):
                path=scratch/(name+'-'+suffix+'.cpp');path.write_text(value);paths.append(path)
        syntax=[];dependencies=set()
        for path in paths:
            dep=path.with_suffix('.d');cmd=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(path)]
            result=subprocess.run(cmd,capture_output=True,text=True,timeout=300)
            if result.returncode:raise ValueError('Whole Primary sphere syntax failed '+result.stderr[-7000:])
            dependencies.update(Path(p).resolve()for p in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]));syntax.append(dict(command=cmd,sha256=sha(path),passed=True))
        used={str(Path(p).relative_to(ROOT))for p in json.loads(metadata.read_text())if Path(p).is_relative_to(ROOT/'tools')}
        used.update(str(Path(m.__file__).resolve().relative_to(ROOT))for m in tuple(sys.modules.values())if getattr(m,'__file__',None)and Path(m.__file__).resolve().is_relative_to(ROOT/'tools'))
        pins={n:tools[n]for n in sorted(used)};header_pins={str(p.relative_to(ROOT)):headers[p]for p in dependencies if p in headers}
        if sha(ROOT/UNIT)!=before:raise ValueError('Sphere canonical owner source changed during generation')
        for name,pin in(*pins.items(),*header_pins.items()):
            if sha(ROOT/name)!=pin:raise ValueError('Sphere generation input changed '+name)
        report.update(passed=True,complete_body_sha256=binding.PIN,fresh_complete_body_sha256=binding.FRESH_PIN,
            fresh_delta_scope='Exact whole-body-pinned C1 clearing and native_single_spill emission; original projection byte-identical; neither retained body rewritten',
            generated_prefix_installed=binding.PREFIX in fresh_match[0],strict_authentication_mutations_rejected=rejected,whole_cpp_syntax=syntax,prepared_receipts=receipts,preparation_tools=pins,header_hashes=header_pins,whole_current_fresh_unit_equal=current==bound,scope='Exact sphere owner/context/default routing and four complete current/fresh raw/prepared syntax units; unrelated historical generator differences are not promoted to parity')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())

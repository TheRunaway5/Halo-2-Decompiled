#!/usr/bin/env python3
"""Replay the two genuine full owners of the private scalar codec candidate."""
import argparse,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import native_codec_scale_replacements as binding
from validate_native_codec_scale_source_tu15 import parts
from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER
from specialize_native_operands import specialize

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    tools={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'tools').rglob('*.py')}
    headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    used={'tools/audit_native_codec_scale_owners_tu15.py'};dependencies=set()
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],owners=[],source_contexts_before={})
    try:
        for owner,name,script in zip(('model','sandbox'),binding.UNITS,('generate_model_render_tu15.py','generate_sandbox_special_tu15.py')):
            canonical=(ROOT/name).read_text();binding.authenticate(canonical);baseline=parts(canonical)
            report['source_contexts_before'][name]=list(map(binding.inputs.digest,baseline))
            projection=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',baseline[0])
            generated=scratch/(owner+'-fresh-bound.cpp');imports=scratch/(owner+'-imports.json')
            command=[sys.executable,'-B',str(ROOT/'tools'/script),'--output',str(generated),'--directory',str(scratch/(owner+'-generation'))]
            done=subprocess.run([sys.executable,'-B','-c',CHILD_WRAPPER,str(imports),*command[2:]],capture_output=True,text=True,timeout=300)
            if done.returncode:raise ValueError('Genuine scale owner generation failed '+(done.stdout+done.stderr)[-7000:])
            used.update(str(Path(value).relative_to(ROOT))for value in json.loads(imports.read_text())if Path(value).is_relative_to(ROOT/'tools'))
            fresh=generated.read_text();fresh_parts=parts(fresh)
            expected_dispatch={'model':'4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604','sandbox':'d46a1635c618577534bfdd091f45ecea819b4886662ca1309400154271d5a987'}[owner]
            if fresh_parts[0]!=projection or fresh_parts[1]!=baseline[1] or fresh_parts[3]!=baseline[3]or binding.inputs.digest(fresh_parts[2])!=expected_dispatch:
                raise ValueError('Fresh complete scale subject/context differs from exact independent identity projection')
            if fresh_parts[2].count('case 0x296220u:run_296220(s);return;')!=1:raise ValueError('Fresh scale direct target missing')
            rows=[]
            for stage,source in(('current',canonical),('fresh',fresh)):
                prepared,receipt=specialize(source)
                for form,text in(('bound',source),('prepared',prepared)):
                    selected=parts(text);normalized=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',selected[0]);normalized=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',normalized)
                    if normalized!=projection:raise ValueError('Scale raw/prepared body changes beyond identity/literal spelling')
                    path=scratch/(owner+'-'+stage+'-'+form+'.cpp');path.write_text(text);dep=path.with_suffix('.d')
                    syntax=['clang++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(path)]
                    done=subprocess.run(syntax,capture_output=True,text=True,timeout=300)
                    if done.returncode:raise ValueError('Complete scale owner CPP syntax failed '+done.stderr[-7000:])
                    dependencies.update(Path(value).resolve()for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]))
                    rows.append(dict(stage=stage,form=form,context=list(map(binding.inputs.digest,selected)),whole_cpp_syntax=syntax,complete_body_identity_projection=True,numeric_source_route_qualified=False))
            report['owners'].append(dict(source=name,command=command,variants=rows))
        used.update(str(Path(module.__file__).resolve().relative_to(ROOT))for module in tuple(sys.modules.values())if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['tool_hashes']={name:tools[name]for name in used}
        report['header_hashes']={str(p.relative_to(ROOT)):headers[p]for p in dependencies if p in headers}
        for name,pin in{**report['tool_hashes'],**report['header_hashes']}.items():
            if sha(ROOT/name)!=pin:raise ValueError('Scale generation dependency changed '+name)
        report['source_contexts_after']={name:list(map(binding.inputs.digest,parts((ROOT/name).read_text())))for name in binding.UNITS}
        if report['source_contexts_after']!=report['source_contexts_before']:raise ValueError('Scale owning context changed during generation')
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Replay the genuine full ModelRender owner of the private scale interpolation candidate."""
import argparse,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import native_codec_scale_interpolation_replacements as binding
from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER
from specialize_native_operands import specialize
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe,_Code,_trace

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def parts(source):
    patterns=(r'static void run_296620\(WorldState &s\) \{.*?\n\}',r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;')
    found=tuple(re.search(pattern,source,re.S)[0]for pattern in patterns)
    return (binding.canonical_body(found[0]),*found[1:])
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    tools={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'tools').rglob('*.py')}
    headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    used={'tools/audit_native_codec_scale_interpolation_owners_tu15.py'};dependencies=set()
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],owners=[],source_contexts_before={})
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original interpolation code image drift')
        code=_Code(Xbe(NEW_DEFAULT),'.text');original=_trace(code,0x296620,{0x296620});sites=sorted(original.code)
        if len(sites)!=21:raise ValueError('Original complete interpolation instruction set changed')
        arithmetic={0x296661:('subss','xmm1,xmm0'),0x296665:('mulss','xmm1,dwordptr[0x510f4c]'),0x29666d:('addss','xmm1,xmm0')}
        for pc,expected in arithmetic.items():
            instruction=code.at(pc)
            if (instruction.mnemonic,instruction.op_str.replace(' ',''))!=expected:raise ValueError('Original ordered SSE opcode/operands changed '+hex(pc))
        report['original_sites']=[dict(instruction=hex(pc),bytes=bytes(code.at(pc).bytes).hex(),mnemonic=code.at(pc).mnemonic,operands=code.at(pc).op_str)for pc in sites]
        report['ordered_arithmetic_sites']=[hex(pc)for pc in arithmetic]
        report['original_code_sha256']=hashlib.sha256(b''.join(bytes(code.at(pc).bytes)for pc in sites)).hexdigest()
        for owner,name,script in zip(('model',),binding.UNITS,('generate_model_render_tu15.py',),strict=True):
            canonical=(ROOT/name).read_text();binding.authenticate(canonical);baseline=parts(canonical)
            report['source_contexts_before'][name]=list(map(binding.inputs.digest,baseline))
            projection=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',baseline[0])
            generated=scratch/(owner+'-fresh-bound.cpp');imports=scratch/(owner+'-imports.json')
            command=[sys.executable,'-B',str(ROOT/'tools'/script),'--output',str(generated),'--directory',str(scratch/(owner+'-generation'))]
            done=subprocess.run([sys.executable,'-B','-c',CHILD_WRAPPER,str(imports),*command[2:]],capture_output=True,text=True,timeout=300)
            if done.returncode:raise ValueError('Genuine scale interpolation owner generation failed '+(done.stdout+done.stderr)[-7000:])
            used.update(str(Path(value).relative_to(ROOT))for value in json.loads(imports.read_text())if Path(value).is_relative_to(ROOT/'tools'))
            fresh=generated.read_text();fresh_parts=parts(fresh)
            expected_dispatch='4af1a5ed426380ed605d661066155bd625fd9f3a1add625cedf00fdc17f15604'
            if fresh_parts[0]!=projection or fresh_parts[1]!=baseline[1] or binding.inputs.digest(fresh_parts[3])!='457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'or binding.inputs.digest(fresh_parts[2])!=expected_dispatch:
                raise ValueError('Fresh complete scale interpolation subject/context differs from exact independent identity projection')
            if fresh_parts[2].count('case 0x296620u:run_296620(s);return;')!=1:raise ValueError('Fresh scale interpolation direct target missing')
            rows=[]
            for stage,source in(('current',canonical),('fresh',fresh)):
                prepared,receipt=specialize(source)
                for form,text in(('bound',source),('prepared',prepared)):
                    binding.authenticate(text);selected=parts(text);normalized=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',selected[0]);normalized=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',normalized)
                    if normalized!=projection:raise ValueError('Scale raw/prepared body changes beyond identity/literal spelling')
                    calls=re.findall(r'(?:s\.services\.instruction|s\.instruction_literal)\((?:selected\()?0x([0-9a-f]+)u',selected[0])
                    if [int(value,16)for value in calls]!=sites:raise ValueError('Original interpolation site order changed')
                    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',selected[1])}
                    for operand in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',selected[0]):
                        if mapping.get(int(operand,16))!=int(operand,16):raise ValueError('Interpolation selected data is not exact identity')
                    path=scratch/(owner+'-'+stage+'-'+form+'.cpp');path.write_text(text);dep=path.with_suffix('.d')
                    syntax=['clang++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(path)]
                    done=subprocess.run(syntax,capture_output=True,text=True,timeout=300)
                    if done.returncode:raise ValueError('Complete scale interpolation owner CPP syntax failed '+done.stderr[-7000:])
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

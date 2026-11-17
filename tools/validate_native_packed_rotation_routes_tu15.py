#!/usr/bin/env python3
"""All genuine packed-rotation current/fresh raw/prepared defaults and fallbacks."""
import argparse,copy,hashlib,json,os,random,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import native_packed_rotation_replacements as binding
from native_packed_rotation_source_inputs import UNITS,HELPERS,function
from validate_native_packed_rotation_generation_tu15 import parts
from validate_native_packed_rotation_tu15 import corpus
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(scratch,report_path,require_binding):
    command=[sys.executable,'-B',str(ROOT/'tools/validate_native_packed_rotation_generation_tu15.py'),'--report',str(report_path),'--candidate-preparation']
    if require_binding:command.append('--require-binding')
    done=subprocess.run(command,capture_output=True,text=True)
    if done.returncode:raise ValueError('Genuine packed rotation generation/preparation failed '+(done.stdout+done.stderr)[-7000:])
    rows=[];blocks=[]
    for owner in('primary','ai','model'):
        for stage in('current','fresh'):
            for form in('bound','prepared'):
                path=scratch/(owner+'-'+stage+'-'+form+'.cpp');source=path.read_text();index=len(rows)
                if binding.installed_entries(source)!=['296180']:raise ValueError('Missing complete packed source binding')
                body,resolver,dispatcher,state=parts(source)
                retained=binding.canonical_body(body)
                if body==retained:raise ValueError('Packed source prefix missing')
                instrumented=body.replace(binding.PREFIX,binding.PREFIX+f'\n++rotation_literals[{index}];',1)
                for old in('if(s.services.instruction){','s.instruction_literal('):
                    instrumented=instrumented.replace(old,f'++rotation_statements[{index}];'+old)
                if instrumented.count(f'++rotation_statements[{index}];')!=31:raise ValueError('Packed source retained statement set changed')
                instrumented=instrumented.replace('run_296180(', 'bound(',1)
                reference=retained.replace('run_296180(','retained(',1)
                controls={}
                helper=function(source,'native_read64')
                mutations={'body':source.replace(body,body.replace('0x8u','0xcu',1),1),'resolver':source.replace(resolver,resolver.replace('switch(p)','p^=1u;switch(p)',1),1),'dispatcher':source.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1),'state':source.replace(state,state.replace('struct LiveState :','struct LiveState final :',1),1),'helper':source.replace(helper,helper.replace('{','{if(false)return 0;',1),1),'include':source.replace(binding.INCLUDE,'',1),'prefix':source.replace(binding.PREFIX,'',1),'duplicate_body':source+'\n'+body}
                for name,mutated in mutations.items():
                    if mutated==source:raise ValueError('Missing packed binding mutation seam '+name)
                    try:binding.installed_entries(mutated)
                    except ValueError as error:controls[name]=str(error)
                    else:raise ValueError('Packed complete binding mutation accepted '+name)
                blocks.append('namespace packed_rotation_route'+str(index)+' {using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'+state+'\n'+resolver+'\n'+'\n'.join(function(source,name)for name in HELPERS)+'\n'+instrumented+'\n'+reference+'\n}\n')
                rows.append(dict(owner=owner,stage=stage,form=form,source_sha256=sha(path),context_sha256=list(map(binding.inputs.digest,(body,resolver,dispatcher,state))),strict_authentication_mutants=controls))
    text='\n'.join(blocks)+'\n#define PACKED_ROTATION_OWNERS(X) '+ ' '.join('X('+str(n)+')'for n in range(12))+'\n';(scratch/'packed_rotation_routes.inc').write_text(text)
    return dict(generation_command=command,variants=rows,source_copy_sha256=binding.inputs.digest(text))
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--mmx',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    paths=(*UNITS,'tools/native_packed_rotation_replacements.py','tools/native_packed_rotation_source_inputs.py','tools/validate_native_packed_rotation_generation_tu15.py','tools/validate_native_packed_rotation_routes_tu15.py','tools/validate_native_packed_rotation_tu15.py','tests/native_tu15_packed_rotation_routes_tests.cpp')
    pins={name:sha(ROOT/name)for name in paths};headers={p.resolve():sha(p)for tree in('src','include','tests')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc','.cpp')}
    report=dict(passed=False,new_entry_default_qualified=False,installed=args.require_binding,earlier_memory_and_before_policy_readonly=True,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        report['preparation']=prepare(scratch,args.output/'generation.json',args.require_binding);rows=corpus(True)
        if args.mmx:
            rng=random.Random(0x296180a11)
            for mask in range(256):
                row=copy.deepcopy(rows[mask%16]);row.update(id=len(rows),name=f'packed-route-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800;row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
        fixture=scratch/'routes.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/(Path(compiler).name+'-routes');dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_packed_rotation_routes_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True)
            if done.returncode:raise ValueError('Packed source route compilation failed '+done.stderr[-10000:])
            for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            modes={};env=os.environ.copy();env.pop('HALO2_REFERENCE_PACKED_ROTATION',None)
            for name,mode in(('default',0),('own-reference',0),('traced',1),('rebound',2),('unknown-uninvoked-owner',3),('no-observer',4),('empty-services',5),('unrelated-controls',0)):
                environment=env.copy()
                if name=='own-reference':environment['HALO2_REFERENCE_PACKED_ROTATION']='1'
                if name=='unrelated-controls':
                    for control in('OBJECT_PARENT_MODE','MARKER_OWNER','MODEL_MARKER_LOOKUP','ZERO_SHAPE_SUPPORT','BOX_SHAPE_SUPPORT'):environment['HALO2_REFERENCE_'+control]='1'
                done=subprocess.run([str(binary),str(fixture),str(mode)],capture_output=True,text=True,env=environment)
                if done.returncode:raise ValueError('Packed source route failed '+name+': '+(done.stdout+done.stderr)[-7000:])
                modes[name]=json.loads(done.stdout)
            done=subprocess.run([str(binary),'--guards'],capture_output=True,text=True,env=env)
            if done.returncode:raise ValueError('Packed source live refusal failed '+(done.stdout+done.stderr)[-7000:])
            results.append(dict(compiler=compiler,command=command,modes=modes,live_refusals=json.loads(done.stdout)))
        report.update(passed=True,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

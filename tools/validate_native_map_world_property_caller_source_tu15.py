#!/usr/bin/env python3
"""Private 11E490 source transport with genuine retained and native 1DB800."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess,sys
from pathlib import Path
import native_map_world_property_query_replacements as binding
from oracle_smg_firing_1_1 import Xbe
from specialize_native_operands import specialize
from validate_native_map_world_property_caller_frame_tu15 import corpus,PIN
from validate_native_map_world_property_query_source_tu15 import prepare as prepare_child

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/runtime/source_map_world_support_tu15_generated.cpp'
SYMBOLS='src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
JOIN='src/rendering/effects/source_object_function_symbols_tu15.inc'
FRAME='src/gameplay/objects/native_tu15_map_world_property_caller_source_frame.hpp'
TEST='tests/native_tu15_map_world_property_caller_source_tests.cpp'
RAW_PIN='a95fc0a2a7f7fd9573754b371206b514e586fbc03156465fe93111f67991d65b'
PREPARED_PIN='038aa0e13b0067f9db55ae1cae455a5a61d1dedfc406dbbb90a09ac6fc2ef351'
RESOLVER_PIN='6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9'

def digest(data):return hashlib.sha256(data).hexdigest()

def prepare(scratch,require_binding=False):
    child=prepare_child(scratch,require_binding)
    text=(ROOT/UNIT).read_text()
    from native_device_property_query_replacements import remove_device_property_query
    text=binding.remove_map_world_property_queries(remove_device_property_query(text))
    def body(value):
        match,=re.findall(r'^static void run_11e490\(WorldState &s\) \{.*?^\}',value,re.M|re.S)
        return match
    raw=body(text);prepared=body(specialize(text)[0])
    if (digest(raw.encode()),digest(prepared.encode()))!=(RAW_PIN,PREPARED_PIN):
        raise ValueError('Complete MapWorld caller body changed')
    resolver,=re.findall(r'^std::uint32_t selected\([^\n]+$',text,re.M)
    literals=sorted(set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',raw)))
    table=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolver))
    if digest(resolver.encode())!=RESOLVER_PIN or len(literals)!=15 or any(table.get(v)!=v for v in literals):
        raise ValueError('Complete MapWorld caller resolver changed')
    actual=re.findall(r's\.services\.instruction\((0x[0-9a-f]+u)\)',raw)
    tagged=re.findall(r's\.instruction_literal\((0x[0-9a-f]+u)\)',prepared)
    native=set(re.findall(r'step\((0x11e4[0-9a-f]{2}u)',(ROOT/FRAME).read_text()))
    if len(set(actual))!=30 or actual!=tagged or set(actual)!=native:
        raise ValueError('MapWorld caller callback sites changed')
    # The child fixture has its own exact identity resolver. Add the caller
    # literals only, preserving the complete original operations of both.
    include=scratch/'map_world_property_query_reference.inc'
    child_source=include.read_text()
    marker='default:throw std::runtime_error("Undeclared MapWorld property literal");'
    if child_source.count(marker)!=1:raise ValueError('Missing exact private resolver seam')
    known=set(re.findall(r'case (0x[0-9a-f]+u):return \1;',child_source))
    add=''.join(f'case {v}:return {v};'for v in literals if v not in known)
    include.write_text(child_source.replace(marker,add+marker,1))
    result='#include "map_world_property_query_reference.inc"\n'
    for form,value in (('raw',raw),('literal',prepared)):
        if value.count('run_1db800(s);')!=1:raise ValueError('Missing genuine MapWorld child edge')
        result+=value.replace('run_11e490','retained_map_world_caller_'+form,1).replace(
            'run_1db800(s);','retained_map_world_query_'+form+'(s);',1)+'\n'
    (scratch/'map_world_property_caller_reference.inc').write_text(result)
    return dict(raw_body_sha256=RAW_PIN,prepared_body_sha256=PREPARED_PIN,resolver_sha256=RESOLVER_PIN,
                caller_sites=30,child_sites=34,identity_operands=15,genuine_child='1DB800',child_authority=child)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true')
    p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    paths={UNIT,SYMBOLS,JOIN,FRAME,TEST,'include/halo2/native_tu15_map_world_property_caller_kernel.hpp',
           'src/gameplay/objects/native_tu15_map_world_property_query_source_frame.hpp',
           'include/halo2/native_tu15_map_world_property_query_kernel.hpp'}
    for module in tuple(sys.modules.values()):
        file=getattr(module,'__file__',None)
        if file and (path:=Path(file).resolve()).is_relative_to(ROOT/'tools'):paths.add(str(path.relative_to(ROOT)))
    pins={name:digest((ROOT/name).read_bytes())for name in sorted(paths)}
    headers={path.resolve():digest(path.read_bytes())for tree in ('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],compilers=[])
    try:
        if digest(a.xbe.read_bytes())!=PIN:raise ValueError('Wrong original TU1.5 authority')
        image=Xbe(a.xbe)
        if image.read(0x11e4da,5).hex()!='e821d30b00':raise ValueError('Original MapWorld direct child edge changed')
        report['authority']=prepare(scratch,a.require_binding)
        report['installed_context_authenticated']=a.require_binding
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(corpus(image),separators=(',',':')));report['fixture_sha256']=digest(fixture.read_bytes())
        for compiler in a.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name
            base=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                  *('-I'+str(ROOT/tree)for tree in('include','src','tests')),'-I'+str(scratch)]
            if a.sanitizers:base+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            objects=[];commands=[]
            for index,name in enumerate((TEST,SYMBOLS)):
                obj=binary.with_suffix(f'.{index}.o');dep=obj.with_suffix('.d');objects.append(str(obj))
                command=base+['-MD','-MF',str(dep),'-c',str(ROOT/name),'-o',str(obj)]
                built=subprocess.run(command,capture_output=True,text=True,timeout=600);commands.append(command)
                if built.returncode:raise ValueError('MapWorld source compile failed: '+built.stderr[-8000:])
                for item in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                    path=Path(item).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            command=base+objects+['-o',str(binary)]
            linked=subprocess.run(command,capture_output=True,text=True,timeout=600);commands.append(command)
            if linked.returncode:raise ValueError('MapWorld source link failed: '+linked.stderr[-8000:])
            ran=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200)
            if ran.returncode:raise ValueError('MapWorld source comparison failed: '+(ran.stdout+ran.stderr)[-10000:])
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT/FRAME).read_text()
                for label,old,new in (
                    ('skip-live-branch','step(0x11e4e4u);',''),
                    ('wrong-ret-cleanup','s_.gp[4]+=16u;return','s_.gp[4]+=12u;return'),
                    ('wrong-child-return','s_.push(0x11e4dau,0x11e4dfu);','s_.push(0x11e4dau,0x11e4e1u);'),
                    ('late-push-esp','s_.push(0x11e4afu,s_.reg(3,4,0));',
                     'physics_world_runtime_detail::w(s_.memory,s_.observe,0x11e4afu,g[4]-4u,4,s_.reg(3,4,0));g[4]-=4u;'),
                ):
                    if original.count(old)!=1:raise ValueError('Missing MapWorld source defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);header=overlay/FRAME;header.parent.mkdir(parents=True);header.write_text(original.replace(old,new,1))
                    mutant=overlay/'bad';command=base[:1]+['-I'+str(overlay/'src')]+base[1:]+[str(ROOT/TEST),objects[1],'-o',str(mutant)]
                    built=subprocess.run(command,capture_output=True,text=True,timeout=600)
                    if built.returncode:raise ValueError('MapWorld source defect failed to compile '+label+': '+built.stderr[-6000:])
                    tested=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=1200)
                    detail=(tested.stdout+tested.stderr)[-6000:]
                    if not tested.returncode or 'MapWorld caller retained source differs'not in detail:
                        raise ValueError('MapWorld source defect survived/unrecognized '+label+': '+detail)
                    mutations.append(dict(name=label,rejected=True,diagnostic=detail))
            report['compilers'].append(dict(compiler=compiler,commands=commands,result=json.loads(ran.stdout),compiled_mutations=mutations))
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:digest((ROOT/name).read_bytes()) for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())

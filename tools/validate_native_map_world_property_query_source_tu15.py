#!/usr/bin/env python3
"""Private 1DB800 source transport against its exact complete MapWorld bodies."""
import argparse,hashlib,json,os,platform,re,shlex,subprocess,sys
from pathlib import Path
import native_map_world_property_query_replacements as binding
from oracle_smg_firing_1_1 import Xbe
from specialize_native_operands import specialize
from validate_native_map_world_property_query_frame_tu15 import corpus,PIN

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/runtime/source_map_world_support_tu15_generated.cpp'
SYMBOLS='src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
JOIN='src/rendering/effects/source_object_function_symbols_tu15.inc'
FRAME='src/gameplay/objects/native_tu15_map_world_property_query_source_frame.hpp'
TEST='tests/native_tu15_map_world_property_query_source_tests.cpp'
RAW_PIN='9aec69524b78ece77f6b8f3252b282cb4b9c3e42e2125f4fb41623b55c2ee819'
PREPARED_PIN='51dd3d8808ea27010d233e13719b94e1e390add295d300df44837586003567b4'
RESOLVER_PIN='6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9'

def digest(data):return hashlib.sha256(data).hexdigest()

def prepare(scratch,require_binding=False):
    text=(ROOT/UNIT).read_text()
    if require_binding:binding.installed_entries(text)
    from native_device_property_query_replacements import remove_device_property_query
    text=binding.remove_map_world_property_queries(remove_device_property_query(text))
    def body(value):
        match,=re.findall(r'^static void run_1db800\(WorldState &s\) \{.*?^\}',value,re.M|re.S)
        return match
    raw=body(text);prepared=body(specialize(text)[0])
    if (digest(raw.encode()),digest(prepared.encode()))!=(RAW_PIN,PREPARED_PIN):
        raise ValueError('Complete MapWorld property query body changed')
    resolver,=re.findall(r'^std::uint32_t selected\([^\n]+$',text,re.M)
    literals=sorted(set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',raw)))
    table=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolver))
    if digest(resolver.encode())!=RESOLVER_PIN or len(literals)!=10 or any(table.get(v)!=v for v in literals):
        raise ValueError('Complete MapWorld selected resolver changed')
    actual=re.findall(r's\.services\.instruction\((0x[0-9a-f]+u)\)',raw)
    tagged=re.findall(r's\.instruction_literal\((0x[0-9a-f]+u)\)',prepared)
    native=set(re.findall(r'0x1db8[0-9a-f]{2}u',(ROOT/FRAME).read_text()))
    if len(set(actual))!=34 or actual!=tagged or set(actual)!=native:
        raise ValueError('MapWorld property callback formats changed')
    source=(ROOT/SYMBOLS).read_text();join=(ROOT/JOIN).read_text()
    symbol='ObjectFunctionSymbol{0x1db800u,0xa000693u,0xa00069fu,0xa00069fu,"phantom_on"}'
    if join.count(symbol)!=1:raise ValueError('Exact 1DB800 symbol join changed')
    names={}
    pair=re.findall(r'\bSymbol\{0x[0-9a-f]+u,0x([0-9a-f]+)u,"([^"]+)"\}',source)
    pair+=re.findall(r'ObjectFunctionSymbol\{0x[0-9a-f]+u,0x[0-9a-f]+u,0x[0-9a-f]+u,0x([0-9a-f]+)u,"([^"]+)"\}',join)
    for key,name in pair:
        if names.setdefault(int(key,16),name)!=name:raise ValueError('Conflicting symbol joins')
    if len(names)!=108:raise ValueError('Incomplete authenticated symbol registry inputs')
    result='static std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){switch(p){'
    result+=''.join(f'case {v}:return {v};'for v in literals)+'default:throw std::runtime_error("Undeclared MapWorld property literal");}}\n'
    result+='static std::string map_world_query_fixture_name(std::uint32_t p){switch(p){'
    result+=''.join(f'case 0x{k:x}u:return {json.dumps(v)};'for k,v in sorted(names.items()))
    result+='default:throw std::runtime_error("Undeclared MapWorld symbol name");}}\n'
    # Only the first comparison calls the registry. The next two cases are
    # literal subtraction/DEC distances, so the same delta applies to inputs.
    result+='static std::uint32_t map_world_query_fixture_key(std::uint32_t p){return (p==0xa000693u||p==0xd000691u||p==0xd000692u)?p+12u:p;}\n'
    old='_mm_mul_ss(s.xmm[0],';ordered='halo2::native_map_world_property_query_detail::ordered_multiply(s.xmm[0],'
    for name,value in (('raw',raw),('literal',prepared)):
        if value.count(old)!=2:raise ValueError('Missing both authored MapWorld multiply sites')
        result+=value.replace(old,ordered,2).replace('run_1db800','retained_map_world_query_'+name,1)+'\n'
    (scratch/'map_world_property_query_reference.inc').write_text(result)
    return dict(raw_body_sha256=RAW_PIN,prepared_body_sha256=PREPARED_PIN,resolver_sha256=RESOLVER_PIN,
                callback_sites=34,identity_operands=10,compiled_symbol='phantom_on',
                only_reference_adjustments='Original destination-first MULSS at 1DB825 and 1DB837')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true')
    p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    paths={UNIT,SYMBOLS,JOIN,FRAME,TEST,'include/halo2/native_tu15_map_world_property_query_kernel.hpp'}
    for module in tuple(sys.modules.values()):
        file=getattr(module,'__file__',None)
        if file and (path:=Path(file).resolve()).is_relative_to(ROOT/'tools'):paths.add(str(path.relative_to(ROOT)))
    pins={name:digest((ROOT/name).read_bytes())for name in sorted(paths)}
    headers={path.resolve():digest(path.read_bytes())for tree in ('include','src','tests')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[],compilers=[])
    try:
        if digest(a.xbe.read_bytes())!=PIN:raise ValueError('Wrong original TU1.5 authority')
        image=Xbe(a.xbe)
        if any(image.read(pc,8).hex()!='f30f5905c4a04600'for pc in(0x1db825,0x1db837)):
            raise ValueError('Original MapWorld multiply encoding changed')
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
                    ('fixed-selector','source_object_function_compiled_symbol_tu15(s_.memory,0x1db800u,0xa000693u)','0xa000693u'),
                    ('skip-live-branch','step(0x1db85bu);',''),
                    ('commuted-multiply','ordered_multiply(s_.xmm[0],scalar(0x46a0c4u))','ordered_multiply(scalar(0x46a0c4u),s_.xmm[0])'),
                    ('clear-upper-lanes','_mm_cvtsi32_ss(s_.xmm[0],','_mm_cvtsi32_ss(_mm_setzero_ps(),'),
                ):
                    if original.count(old)!=1:raise ValueError('Missing MapWorld source defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);header=overlay/FRAME;header.parent.mkdir(parents=True);header.write_text(original.replace(old,new,1))
                    mutant=overlay/'bad';command=base[:1]+['-I'+str(overlay/'src')]+base[1:]+[str(ROOT/TEST),objects[1],'-o',str(mutant)]
                    built=subprocess.run(command,capture_output=True,text=True,timeout=600)
                    if built.returncode:raise ValueError('MapWorld source defect failed to compile '+label+': '+built.stderr[-6000:])
                    tested=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=1200)
                    detail=(tested.stdout+tested.stderr)[-6000:]
                    if not tested.returncode or 'MapWorld property retained source differs'not in detail:
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

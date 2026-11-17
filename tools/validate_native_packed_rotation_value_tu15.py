#!/usr/bin/env python3
"""Whole predecessor/candidate frontend grip policy with shared decoder DATA."""
import argparse,hashlib,json,os,re,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PUBLIC='src/ui/frontend_model_locals.cpp'
PUBLIC_PIN='9dcc5919a56879a106b79fbfa92da195e935803661b223e991af6306ffa92e7c'
PUBLIC_NEW_PIN='9dcbef09d5a9ec24abf8ba2ad035631e10b5a1643914a5fd65fe3d54c8bf5a13'
NORM_PIN='20d459429061725ac2202bbe0246141d57db8e3a0f90004f8a79d58ae5bceb5d'
SEED_PIN='a78e86c216224d27316fb0e581fb05c24bfc27211acca95cceab2905bd2cd446'
DEFAULT_PIN='cdc1d00185ebc43ea46aaf2b377391ed55ba5b1fd81c06d7efe993b60add421a'
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def predecessor(source):
    if digest(source)==PUBLIC_PIN:return source
    if digest(source)!=PUBLIC_NEW_PIN:raise ValueError('Complete installed frontend grip owner drift')
    new_norm='float grip_norm(std::array<std::int16_t,4> quantized) {\n  return native_packed_rotation_detail::norm_value(quantized);\n}'
    old_norm='float grip_norm(std::array<std::int16_t,4> quantized) {\n  const float x=quantized[0],y=quantized[1],z=quantized[2],w=quantized[3];\n  const float a=x*x+z*z,b=y*y+w*w;\n  return a+b;\n}'
    new_seed='std::array<float,4> frontend_grip_rotation(std::array<std::int16_t,4> input,\n                                          float inverse_sqrt_seed) {\n  return native_packed_rotation_detail::with_seed(input,inverse_sqrt_seed);\n}'
    old_seed='std::array<float,4> frontend_grip_rotation(std::array<std::int16_t,4> input,\n                                          float inverse_sqrt_seed) {\n  if(!(grip_norm(input)>0) || !std::isfinite(inverse_sqrt_seed) || inverse_sqrt_seed<=0)\n    throw FormatError("invalid original grip quaternion seed");\n  std::array<float,4> out;\n  for(unsigned i=0;i<out.size();++i)out[i]=float(input[i])*inverse_sqrt_seed;\n  return out;\n}'
    include='#include "halo2/native_tu15_packed_rotation_value.hpp"\n'
    if source.count(new_norm)!=1 or source.count(new_seed)!=1 or source.count(include)!=1:raise ValueError('Installed grip DATA facade shape changed')
    source=source.replace(new_norm,old_norm,1).replace(new_seed,old_seed,1).replace(include,'',1)
    if digest(source)!=PUBLIC_PIN:raise ValueError('Exact whole frontend predecessor restoration failed')
    return source
def candidate(source):
    if digest(source)!=PUBLIC_PIN:raise ValueError('Complete frontend grip predecessor drift')
    norm=re.search(r'float grip_norm\(.*?\n\}',source,re.S)
    seed=re.search(r'std::array<float,4> frontend_grip_rotation\(std::array<std::int16_t,4> input,\n.*?\n\}',source,re.S)
    if not norm or digest(norm[0])!=NORM_PIN or not seed or digest(seed[0])!=SEED_PIN:raise ValueError('Complete frontend grip helpers drift')
    source=source.replace(norm[0],'float grip_norm(std::array<std::int16_t,4> quantized) {\n  return native_packed_rotation_detail::norm_value(quantized);\n}',1)
    source=source.replace(seed[0],'std::array<float,4> frontend_grip_rotation(std::array<std::int16_t,4> input,\n                                          float inverse_sqrt_seed) {\n  return native_packed_rotation_detail::with_seed(input,inverse_sqrt_seed);\n}',1)
    return '#include "halo2/native_tu15_packed_rotation_value.hpp"\n'+source
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=(PUBLIC,'include/halo2/native_tu15_packed_rotation_kernel.hpp','include/halo2/native_tu15_packed_rotation_value.hpp','tests/native_tu15_packed_rotation_value_tests.cpp','tools/validate_native_packed_rotation_value_tu15.py')
    pins={name:sha(ROOT/name)for name in files};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_candidate_only=True,actual_public_source_unchanged=True,default_qualified=False,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        installed=(ROOT/PUBLIC).read_text();original=predecessor(installed);new=candidate(original);sources=[];whole=[]
        if args.require_binding and installed!=new:raise ValueError('Shared frontend grip DATA facade is not installed')
        default=re.search(r'std::array<float,4> frontend_grip_rotation\(std::array<std::int16_t,4> input\) \{.*?\n\}',original,re.S)
        if not default or digest(default[0])!=DEFAULT_PIN:raise ValueError('Complete default grip seed policy drift')
        for name,text in(('predecessor',original),('candidate',new)):
            if text.count('namespace halo2 {')!=1:raise ValueError('Frontend namespace ownership changed')
            full=scratch/(name+'-whole.cpp');full.write_text(text);whole.append(full)
            norm=re.search(r'float grip_norm\(.*?\n\}',text,re.S);seed=re.search(r'std::array<float,4> frontend_grip_rotation\(std::array<std::int16_t,4> input,\n.*?\n\}',text,re.S)
            if not norm or not seed:raise ValueError('Missing exact grip function fragment')
            includes='\n'.join(re.findall(r'^#include[^\n]*',text,re.M))
            path=scratch/(name+'.cpp');path.write_text(includes+'\nnamespace packed_rotation_'+name+' {\nusing namespace halo2;\nnamespace{\n'+norm[0]+'\n}\n'+seed[0]+'\n'+default[0]+'\n}\n');sources.append(path)
        results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_packed_rotation_value_tests.cpp'),*map(str,sources),'-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-8000:]
            for source in(ROOT/'tests/native_tu15_packed_rotation_value_tests.cpp',*sources,*whole):
                dep=scratch/(Path(compiler).name+'-'+source.name+'.d');syntax=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(source)]
                checked=subprocess.run(syntax,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-4000:]
                for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(value).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary)],capture_output=True,text=True);assert done.returncode==0,(done.stdout+done.stderr)[-8000:]
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout)))
        report.update(passed=True,compilers=results,installed_shared_public=args.require_binding,private_candidate_only=not args.require_binding,candidate_public_sha256=digest(new),predecessor_public_sha256=PUBLIC_PIN,numerical_scope='Exact complete grip helper/overload bodies; full preceding and candidate CPP syntax separately checked')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Genuine three-owner source continuation after numeric instruction-policy changes."""
import argparse,copy,hashlib,json,os,random,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from native_packed_rotation_source_inputs import prepare,UNITS
from validate_native_packed_rotation_tu15 import corpus
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--mmx',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    files=(*UNITS,'tools/native_packed_rotation_source_inputs.py','tools/validate_native_packed_rotation_policy_tu15.py','tools/validate_native_packed_rotation_tu15.py','tools/native_model_marker_lookup_replacements.py','tests/native_tu15_packed_rotation_policy_tests.cpp')
    pins={name:sha(ROOT/name)for name in files};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,private_unbound=True,earlier_memory_and_before_policy_readonly=True,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        ownership=prepare(ROOT,scratch);rows=corpus(True)
        if args.mmx:
            rng=random.Random(0x296180a11)
            for mask in range(256):
                row=copy.deepcopy(rows[mask%16]);row.update(id=len(rows),name=f'packed-source-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_packed_rotation_policy_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-8000:]
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert done.returncode==0,(done.stdout+done.stderr)[-8000:]
            mutations=[]
            if args.semantic_mutants:
                relative='src/physics/runtime/native_tu15_packed_rotation_frame.hpp';source=(ROOT/relative).read_text()
                definitions=(('cached-returned-policy','io([&]{s_.instruction_literal(pc);});','if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=pc;'),('late-returned-xmm0','s_.xmm[0]=inverse;returned_boundary(0x2961deu,false);','returned_boundary(0x2961deu,false);'),('cached-returned-multiply','return multiply(s_.xmm[1],s_.xmm[0]);','return multiply(quaternion,inverse);'),('cached-output-vector','std::memcpy(bytes.data(),&s_.xmm[1],16);','std::memcpy(bytes.data(),&value,16);'),('reread-half-output-base','const auto address=output_address+offset;','const auto address=s_.gp[0]+offset;'))
                for name,old,new in definitions:
                    if source.count(old)!=1:raise ValueError('Missing packed source-policy defect witness '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));mutant=overlay/'test'
                    mutated=list(command);mutated.insert(1,'-I'+str(overlay/'src'));mutated[mutated.index('-o')+1]=str(mutant)
                    checked=subprocess.run(mutated,capture_output=True,text=True);assert checked.returncode==0,checked.stderr[-5000:]
                    checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=(checked.stdout+checked.stderr)[-4000:]
                    if checked.returncode==0 or 'Packed rotation source policy continuation differs'not in diagnostic:raise ValueError('Packed source-policy defect survived/unrecognized '+name+': '+diagnostic)
                    mutations.append(dict(name=name,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_mutations=mutations))
        report.update(passed=True,source_ownership=ownership,compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if pins!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

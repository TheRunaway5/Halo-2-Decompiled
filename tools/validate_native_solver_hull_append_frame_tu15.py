#!/usr/bin/env python3
"""Private complete313290 append policy/raw frame with source atomic boundaries."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from native_solver_hull_append_replacements import canonical_body
ROOT=Path(__file__).resolve().parents[1]
PIN='46e4d78ccc0231ec938c131be02b5184f74acbd46a2926b6259f1d0d57fe10c9'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def owner_digest():
    body=next(b for sig,b in function_bodies((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())if sig.symbol=='313290')
    body=canonical_body(body)
    return hashlib.sha256(body.encode()).hexdigest()
def reference(scratch):
    body=next(b for sig,b in function_bodies((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())if sig.symbol=='313290')
    body=canonical_body(body)
    if hashlib.sha256(body.encode()).hexdigest()!=PIN:raise ValueError('Complete313290 body drift')
    owner=body.replace('run_313290(','owner(',1).replace('s.push(','atomic_push(s,')
    owner,count=re.subn(r's.flags.shift\(s.reg\(([0-7]),4,0\),0x4u,4\)',lambda m:f'native_mopp_aabb_abi_detail::shifted_immediate<4>(s,s.reg({m[1]},4,0))',owner)
    if count!=3:raise ValueError('Append immediate SHL count drift')
    old='w(s.memory,s.observe,0x3132e8u,(0x80u+s.gp[1]),4,s.flags.increment(r(s.memory,(0x80u+s.gp[1]),4),4,false));'
    if owner.count(old)!=1:raise ValueError('Append actual INC seam drift')
    owner=owner.replace(old,'increment_count(s);',1)
    primitive='''static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,address,{},{}};if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);s.memory.write(address,bytes);s.gp[4]=address;if(s.observe){event.written.assign(bytes.begin(),bytes.end());s.observe(event);}}
static void increment_count(WorldState&s){const auto address=s.gp[1]+0x80u;const auto current=r(s.memory,address,4);auto pending=s.flags;const auto value=pending.increment(current,4,false);SourcePhysicsContactRecordStore event{0x3132e8u,address,{},{}};if(s.observe){event.before.resize(4);s.memory.read(address,event.before);}std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);s.memory.write(address,bytes);s.flags=pending;if(s.observe){event.written.assign(bytes.begin(),bytes.end());s.observe(event);}}
'''
    sites=[]
    for line in body.splitlines():
        match=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if match and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line):sites.append(int(match[1],16))
    text=primitive+'static const std::set<std::uint32_t>io_sites{'+','.join(hex(pc)+'u'for pc in sites)+'};\n'+owner
    (scratch/'solver_hull_append_reference.inc').write_text(text);return dict(complete_canonical_body_sha256=PIN,reference_sha256=hashlib.sha256(text.encode()).hexdigest(),phase_reference='Explicit3 native SHL flags, CPU-atomic PUSH and INC flag publication after write before completed observer; independent original instruction proof still required'),body
def semantic_mutants(scratch,command):
    kernel='include/halo2/native_tu15_solver_hull_append_kernel.hpp';frame='src/physics/runtime/native_tu15_solver_hull_append_frame.hpp';result={}
    for name,path,relative,old,new in[
        ('omit_current_first_point',kernel,'halo2/native_tu15_solver_hull_append_kernel.hpp','data.point=memory.vector(P::first_point,data,data.pair);','data.point=data.first;'),
        ('omit_second_simplex_reload',kernel,'halo2/native_tu15_solver_hull_append_kernel.hpp','data.simplex=memory.simplex(P::second_point,data);','/* Reuse stale simplex. */'),
        ('wrong_second_array_bias',kernel,'halo2/native_tu15_solver_hull_append_kernel.hpp','data.simplex+((data.index+4u)<<4u)','data.simplex+((data.index+3u)<<4u)'),
        ('publish_INC_flags_before_provider',frame,'physics/runtime/native_tu15_solver_hull_append_frame.hpp','const auto value=retired.increment(current,4,false);','const auto value=retired.increment(current,4,false);s_.flags=retired;'),
    ]:
        text=(ROOT/path).read_text()
        if text.count(old)!=1:raise ValueError('Append semantic mutation seam drift: '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe');built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Append semantic mutant did not compile '+name+': '+built.stderr[-5000:])
        run=subprocess.run([str(overlay/'probe')],capture_output=True,text=True)
        if not run.returncode:raise ValueError('Append semantic mutant accepted: '+name)
        result[name]=dict(rejected=True,diagnostic=run.stderr[-5000:])
    return result
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['tools/native_solver_hull_append_replacements.py','include/halo2/native_tu15_solver_hull_append_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_append_frame.hpp','tests/native_tu15_solver_hull_append_tests.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            path=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if path is None:continue
            relative=str(path.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:digest(ROOT/name)for name in sorted(seen)};report=dict(passed=False,scratch_owner=str(scratch),production_bound=False,source_sha256=pins,retained_large_artifacts=[])
    try:
        report['reference'],_=reference(scratch);command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_append_tests.cpp'),'-o',str(scratch/'probe')]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Append compile failed: '+built.stderr[-8000:])
        run=subprocess.run([str(scratch/'probe')],capture_output=True,text=True)
        if run.returncode:raise ValueError('Append source phase failed: '+run.stderr[-10000:])
        report['source']=json.loads(run.stdout.strip().splitlines()[-1]);report['semantic_mutants']=semantic_mutants(scratch,command);report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(digest(ROOT/name)==pin for name,pin in pins.items())and owner_digest()==PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Append source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in('passed','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

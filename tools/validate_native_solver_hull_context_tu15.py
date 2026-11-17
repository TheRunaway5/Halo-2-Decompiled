#!/usr/bin/env python3
"""Private314710 canonical DATA/ABI segments; genuine313BB0 remains a boundary."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
ROOT=Path(__file__).resolve().parents[1]
PIN='0d8b7578de1f94bc8e3d8b9c905307c6c5cda881de39844aad279565952e08f2'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def owner_digest():
    body=next(b for sig,b in function_bodies((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text())if sig.symbol=='314710')
    return hashlib.sha256(body.encode()).hexdigest()
def reference(scratch):
    source=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';body=next(b for sig,b in function_bodies(source.read_text())if sig.symbol=='314710')
    if hashlib.sha256(body.encode()).hexdigest()!=PIN:raise ValueError('Complete314710 owner drift')
    if body.count('run_313bb0(s);')!=1:raise ValueError('Genuine hull child transport changed')
    owner=body.replace('run_314710(','owner(',1).replace('run_313bb0(s);','child(s);',1).replace('s.push(','atomic_push(s,')
    start='if(s.services.instruction){s.services.instruction(0x31479bu);}'
    if owner.count(start)!=1:raise ValueError('Exact post-child continuation changed')
    continuation='static void continuation(WorldState&s){\n'+owner[owner.index(start):]
    sites=[]
    for line in body.splitlines():
        match=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if match and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line or'timestamp_counter' in line):sites.append(int(match[1],16))
    primitive='''static std::function<void(WorldState&)>child;
static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore e{pc,address,{},{}};if(s.observe){e.before.resize(4);s.memory.read(address,e.before);}std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);s.memory.write(address,bytes);s.gp[4]=address;if(s.observe){e.written.assign(bytes.begin(),bytes.end());s.observe(e);}}
'''
    text=primitive+'static const std::set<std::uint32_t>io_sites{'+','.join(hex(pc)+'u'for pc in sites)+'};\n'+owner+'\n'+continuation
    (scratch/'solver_hull_context_reference.inc').write_text(text)
    return dict(complete_canonical_body_sha256=PIN,generated_reference_sha256=hashlib.sha256(text.encode()).hexdigest(),atomic_push_primitive_explicit=True,child_scope='Genuine313BB0 entry stop only; separate declared postchild inputs do not claim a child outcome or whole parent completion',clock_scope='Independently declared deterministic test clock inputs only; unchanged-original RDTSC replay and actual clock-owner qualification remain pending')
def semantic_mutants(scratch,command):
    source=(ROOT/'include/halo2/native_tu15_solver_hull_context_kernel.hpp').read_text();result={}
    for name,old,new in[
        ('omit_private_timestamp_reload','const auto low=memory.reload_timestamp(data,ending);','const auto low=std::uint32_t(timestamp);'),
        ('acquire_differences_before_aliasing_store','memory.store_word(P::first_shape,data,data.first);\n    data.differences=memory.differences(data);','data.differences=memory.differences(data);\n    memory.store_word(P::first_shape,data,data.first);'),
        ('fail_to_clear_child_context_state','memory.store_word(P::initial_state,data,0u);','memory.store_word(P::initial_state,data,1u);'),
    ]:
        if source.count(old)!=1:raise ValueError('Hull-context mutation seam changed: '+name)
        overlay=scratch/name;header=overlay/'halo2/native_tu15_solver_hull_context_kernel.hpp';header.parent.mkdir(parents=True,exist_ok=True);header.write_text(source.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe');built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Hull-context mutation compile failed '+name+': '+built.stderr[-5000:])
        probe=subprocess.run([str(overlay/'probe')],capture_output=True,text=True)
        if not probe.returncode:raise ValueError('Hull-context semantic mutant accepted: '+name)
        result[name]=dict(rejected=True,diagnostic=probe.stderr[-5000:])
    return result
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);paths=['include/halo2/native_tu15_solver_hull_context_kernel.hpp','src/physics/runtime/native_tu15_solver_hull_context_frame.hpp','tests/native_tu15_solver_hull_context_tests.cpp','tests/native_tu15_mopp_ray_frame_test_memory.hpp',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            path=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if path is None:continue
            relative=str(path.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:digest(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        report['reference']=reference(scratch);command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_context_tests.cpp'),'-o',str(scratch/'probe')]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Hull-context compile failed: '+result.stderr[-10000:])
        result=subprocess.run([str(scratch/'probe')],capture_output=True,text=True)
        if result.returncode:raise ValueError('Hull-context canonical seam failed: '+result.stderr[-10000:])
        report['segments']=json.loads(result.stdout.strip().splitlines()[-1]);report['semantic_mutants']=semantic_mutants(scratch,command);report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(digest(ROOT/name)==pin for name,pin in pins.items())and owner_digest()==PIN
    if not report['sources_unchanged']:report.update(passed=False,error='Hull-context source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

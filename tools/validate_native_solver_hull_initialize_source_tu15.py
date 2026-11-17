#!/usr/bin/env python3
"""Private retained313560 view, separate from physical x87/store publication."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from validate_native_solver_hull_initialize_frame_tu15 import body,inputs,BODY_PIN
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def reference(scratch):
    original=body();text=original.replace('run_313560(','owner(',1)
    for pc,target in ((0x3135dd,0x313390),(0x3135e9,0x313290)):
        needle=f's.push(0x{pc:x}u,0x{pc+5:x}u);run_{target:x}(s);'
        if text.count(needle)!=1:raise ValueError('Retained initialize actual-child statement changed')
        text=text.replace(needle,f's.push(0x{pc:x}u,0x{pc+5:x}u);child(s,0x{target:x}u,0x{pc:x}u);',1)
    # Preserve operand order already independently measured at the original
    # instructions by the physical module; no source FP/helper/store projection.
    for opcode,count,name in [('mul_ps',3,'mul'),('add_ss',6,'add_scalar')]:
        text,n=re.subn(r'_mm_'+opcode+r'\((s.xmm\[[0-7]\]),(s.xmm\[[0-7]\])\)',lambda m:name+'('+m[1]+','+m[2]+')',text)
        if n!=count:raise ValueError('Retained initialize ordered SIMD statement changed '+opcode)
    sites=[]
    for line in original.splitlines():
        pc=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if pc and(re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line):sites.append(int(pc[1],16))
    primitive='''static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
'''
    output=primitive+'static const std::set<std::uint32_t>io_sites{'+','.join(hex(pc)+'u'for pc in sites)+'};\n'+text
    (scratch/'solver_hull_initialize_reference.inc').write_text(output)
    return dict(original_body_sha256=BODY_PIN,retained_reference_sha256=hashlib.sha256(output.encode()).hexdigest(),ordered_SIMD_operations=9,retained_PUSH_and_FSTP_and_observer_helpers_unchanged=True,genuine_child_dispatch_only=True)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='g++');p.add_argument('--sanitizers',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['src/physics/runtime/native_tu15_solver_hull_initialize_source_frame.hpp','tests/native_tu15_solver_hull_initialize_source_tests.cpp','tools/validate_native_solver_hull_initialize_frame_tu15.py','tools/validate_native_solver_hull_edge_frame_tu15.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((q for q in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if q.is_file()),None)
            if target is not None:
                relative=str(target.relative_to(ROOT))
                if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        from research_mp_sandbox_update_functions import Xbe
        from validate_native_tu15_prt_helpers import PIN
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original TU15 image')
        report['reference']=reference(scratch);fixture=scratch/'inputs.json';fixture.write_text(json.dumps(inputs(Xbe(a.xbe)),separators=(',',':')));report['input_sha256']=sha(fixture)
        binary=scratch/'retained';command=[a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_initialize_source_tests.cpp'),'-o',str(binary)]
        if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True,timeout=600)
        if built.returncode:raise ValueError('Retained initialize compile failed: '+built.stderr[-10000:])
        for mode,args in [('normal',[]),('parent-prefixes',['--prefixes'])]:
            with fixture.open('rb')as stream:done=subprocess.run([str(binary)]+args,stdin=stream,capture_output=True,text=True,timeout=600)
            if done.returncode:raise ValueError('Retained initialize differs '+mode+': '+done.stderr[-10000:])
            report[mode]=json.loads(done.stdout)
        report.update(passed=True,scope='Private ordinary-backed retained GP/FP/store protocol with genuine computed children; no policy/default/full raw-source format or child-prefix claim')
    except Exception as error:report['error']=repr(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())and hashlib.sha256(body().encode()).hexdigest()==BODY_PIN
    if not report['sources_unchanged']:report.update(passed=False,source_drift=True)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

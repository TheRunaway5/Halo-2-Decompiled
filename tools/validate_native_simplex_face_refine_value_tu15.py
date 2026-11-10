#!/usr/bin/env python3
"""Qualify shared refinement DATA view against the frozen previous value owner."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from validate_native_simplex_face_refine_tu15 import ROOT,reference
from validate_native_simplex_direction_tu15 import sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--native-source',type=Path,default=ROOT/'tests/reference/native_tu15_simplex_face_selection_shared_candidate.cpp');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_changed=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    source=args.native_source.resolve();paths=[source,ROOT/'include/halo2/native_tu15_simplex_face_refine_value.hpp',ROOT/'include/halo2/native_tu15_simplex_face_refine_kernel.hpp',ROOT/'tests/native_tu15_simplex_face_refine_value_tests.cpp',ROOT/'tests/native_tu15_simplex_face_refine_tests.cpp',ROOT/'tests/reference/native_tu15_simplex_face_refine_value_before.cpp',Path(__file__)]
    pending=list(paths);seen=set(paths)
    while pending:
        current=pending.pop()
        for header in re.findall(r'^#include \"([^\"\n]+)\"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is not None and target not in seen:seen.add(target);pending.append(target)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(seen)}
    try:
        if pins['tests/reference/native_tu15_simplex_face_refine_value_before.cpp']!='6e0dec5b5b3b6fc1f3da2e376ff5c86b09c72a8390287690b0a7ade6ef4cd686':raise ValueError('Frozen previous refine DATA oracle drift')
        report['reference']=reference(scratch);binary=scratch/'value';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_face_refine_value_tests.cpp'),str(source),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Refine DATA compile failed:\n'+result.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        if result.returncode:raise ValueError('Refine DATA compatibility failed:\n'+result.stderr[-7000:])
        report['value']=json.loads(result.stderr.strip().splitlines()[-1])
        header=ROOT/'include/halo2/native_tu15_simplex_face_refine_value.hpp';text=header.read_text();mutants={
            'publish_before_all_dot_results':('if(slot!=3u)return;','{constexpr std::array pc{0x30d25eu,0x30d277u,0x30d28du,0x30d2abu},offset{0x14u,0x1cu,0x18u,0x10u};memory_.word(pc[slot],scratch_+offset[slot],bits);return;}'),
            'omit_live_cursor_reread':('void advance(const Data&){const auto cursor=memory_.word(scratch_+12u);','void advance(const Data&d){const auto cursor=d.vertices+d.index*16u;'),
        };report['semantic_mutants']={}
        for name,(old,new)in mutants.items():
            if text.count(old)!=1:raise ValueError('Refine value mutation seam drift: '+name)
            changed=text.replace(old,new);directory=scratch/'mutants'/name;target=directory/'halo2'/header.name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(changed)
            mutant=list(command);at=mutant.index('-I'+str(ROOT/'include'));mutant[at:at]=['-I'+str(directory)];mutant[-1]=str(directory/'probe');built=subprocess.run(mutant,capture_output=True,text=True)
            if built.returncode:raise ValueError('Refine value mutant did not compile: '+name+'\n'+built.stderr[-5000:])
            probe=subprocess.run([str(directory/'probe')],capture_output=True,text=True)
            if probe.returncode==0:raise ValueError('Refine value semantic mutant accepted: '+name)
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(changed.encode()).hexdigest(),diagnostic=probe.stderr[-2000:])
        report['source_sha256']=pins;report['sources_unchanged']=all(sha(ROOT/path)==pin for path,pin in pins.items())
        if not report['sources_unchanged']:raise ValueError('Refine DATA inputs drifted')
        report['passed']=True
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_changed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Private shared313300 value view versus the complete frozen public facade."""
import argparse,hashlib,json,os,platform,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FUNCTION=re.compile(r'std::uint32_t native_tu15_sample_solver_hull_direction\(.*?\n\}',re.S)
INCLUDE='#include "halo2/native_tu15_solver_hull_sample_value.hpp"\n'
OLD_PIN='8124345f6fa9f37b4522cda24670fb4bdd5670998210df33c258a3240e056bdc'
NEW_PIN='bd9596a4f01a8faa214387811e464d35c10375c520ae43fe147a4ccb9960b226'
REFERENCE_PIN='7727e60c8374ff4dcdaa7e03b21eb0f840798544e4acdf25bd133eeb24266332'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def public_overlay(source):
    matches=list(FUNCTION.finditer(source))
    if len(matches)!=1:raise ValueError('Missing or ambiguous complete public hull sample function')
    m=matches[0]
    pin=hashlib.sha256(m[0].encode()).hexdigest()
    if pin==NEW_PIN:
        if source.count(INCLUDE)!=1:raise ValueError('Complete shared public313300 include ownership drift')
        return source
    if pin!=OLD_PIN or INCLUDE in source:raise ValueError('Complete public313300 function/header drift')
    start=m[0].index('        Environment phase(fp,depth);')
    end=m[0].index('\n#else',start)
    replacement='        Environment phase(fp,depth);\n        native_solver_hull_sample_value_detail::View view{Memory{memory,observe},seed,lower,upper,output};\n        native_tu15_solver_hull_sample_kernel(view);\n        if(stats)++stats->sample_completed;return view.current;'
    new=m[0][:start]+replacement+m[0][end:]
    return INCLUDE+source[:m.start()]+new+source[m.end():]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mutations',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);names=['src/physics/native_tu15_solver_hull_polytope.cpp','include/halo2/native_tu15_solver_hull_sample_value.hpp','tests/reference/native_tu15_solver_hull_sample_value_before.inc','tests/native_tu15_solver_hull_sample_value_tests.cpp','tools/validate_native_solver_hull_sample_frame_tu15.py',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        from research_mp_sandbox_update_functions import Xbe
        from validate_native_tu15_prt_helpers import PIN
        from validate_native_solver_hull_sample_frame_tu15 import inputs
        if sha(a.xbe)!=PIN or sha(ROOT/'tests/reference/native_tu15_solver_hull_sample_value_before.inc')!=REFERENCE_PIN:raise ValueError('Public hull sample original/reference identity drift')
        rows=inputs(Xbe(a.xbe));path=scratch/'inputs.json';path.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(path)
        original=(ROOT/'src/physics/native_tu15_solver_hull_polytope.cpp').read_text();candidate=public_overlay(original)
        if a.require_binding and candidate!=original:raise ValueError('Shared public hull sample facade is not installed')
        source=scratch/'hull sample.cpp';source.write_text(candidate);report['public_candidate_sha256']=sha(source)
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/physics'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_solver_hull_sample_value_tests.cpp'),str(source),'-o',str(scratch/'value')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Shared public hull sample compile failed: '+built.stderr[-10000:])
        with path.open('rb')as inp:run=subprocess.run([str(scratch/'value')],stdin=inp,capture_output=True,text=True)
        if run.returncode:raise ValueError('Shared public hull sample differs: '+run.stderr[-10000:])
        report['value']=json.loads(run.stdout.strip().splitlines()[-1])
        if a.mutations:
            relative='halo2/native_tu15_solver_hull_sample_value.hpp';header=(ROOT/'include'/relative).read_text();report['semantic_mutants']={}
            for name,old,new in [
                ('wrong_seed_multiplier','current*native_solver_hull_sample_multiplier','current*(native_solver_hull_sample_multiplier+2u)'),
                ('inverted_lower_bound','fadds %0','fsubs %0'),
                ('omitted_scale_reacquisition','const auto scale=memory.word(0x41d780u);','const auto scale=0x30000000u;'),
            ]:
                if header.count(old)!=1:raise ValueError('Public hull sample mutation seam changed '+name)
                overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(header.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'value')
                built=subprocess.run(mutated,capture_output=True,text=True)
                if built.returncode:raise ValueError('Public hull sample mutant compile failed '+name+': '+built.stderr[-6000:])
                with path.open('rb')as inp:run=subprocess.run([str(overlay/'value')],stdin=inp,capture_output=True,text=True)
                if not run.returncode or 'complete public hull sample DATA/FP/provider phases differ'not in run.stderr:raise ValueError('Public hull sample mutant lacks expected mismatch '+name+': '+run.stderr[-6000:])
                report['semantic_mutants'][name]=dict(rejected=True,diagnostic=run.stderr[-7000:])
        report.update(passed=True,production_bound=a.require_binding,scope='Existing depth-zero public sampling DATA facade with full return, FP, provider order and alias compatibility. No generated source/default or raw ABI claim.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Public hull sample input drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Selected native30DE60 against actual-data unchanged-original composition.

The original2E prefix derives ordinary work/shape/relative DATA independently.
All captured retained banks, defined work publications, reads and FP are strict.
Four unprovided private work rows stay guarded, never native GP/control inputs.
"""
import hashlib,inspect,json,sys,subprocess
from pathlib import Path
import validate_native_solver_hull_query_actual_tu15 as hull
ROOT=Path(__file__).resolve().parents[1]
UNITS=('native_tu15_convex_simplex_single.cpp',)+hull.UNITS
def observer_source():
 text=(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()
 return text[:text.index('int main(int argc,char**argv)')]+(ROOT/'tools/observe_native_convex_simplex_single_actual_tu15.cpp').read_text()
def main():
 original_run=subprocess.run;compiled={};mutants=[]
 kernel=ROOT/'src/physics/native_tu15_convex_simplex_single.cpp'
 mutations={
  'wrong_difference':('subps %1,%0','addps %1,%0'),
  'wrong_coefficient':('0x3f800000u','0x00000000u'),
  'disabled_guard':('if(n)regions_.push_back({p,std::vector<bool>(n,false)});','(void)n;'),
 }
 def checked_run(command,*args,**kwargs):
  r=original_run(command,*args,**kwargs)
  if isinstance(command,list) and str(kernel) in command and r.returncode==0:
   compiled[command[command.index('-o')+1]]=list(command)
  if isinstance(command,list) and command and command[0] in compiled and r.returncode==0:
   compiler_command=compiled[command[0]];compiler=compiler_command[0];scratch=Path(command[0]).parent;output=Path(command[4]).parent
   for name,(old,new) in mutations.items():
    source=kernel.read_text();assert source.count(old)==1;(scratch/f'{name}.cpp').write_text(source.replace(old,new));target=scratch/(name+'-'+compiler)
    build=[str(scratch/f'{name}.cpp')if c==str(kernel)else str(target)if c==command[0]else c for c in compiler_command];build.insert(1,'-I'+str(ROOT/'src/physics'))
    b=original_run(build,capture_output=True,text=True);(output/f'{compiler}-{name}-build.log').write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr[-4000:]
    result=output/f'{compiler}-{name}.json';run=original_run([str(target),*command[1:4],str(result)],capture_output=True,text=True);(output/f'{compiler}-{name}.log').write_text(run.stdout+run.stderr)
    assert run.returncode!=0 and result.exists() and not json.loads(result.read_text())['passed'],name+' survived'
    mutants.append(dict(compiler=compiler,mutant=name,rejected=True,result=str(result),result_sha256=hashlib.sha256(result.read_bytes()).hexdigest()))
  return r
 subprocess.run=checked_run
 text=inspect.getsource(hull.main)
 text=text.replace('parent.OWNERS=(0x314710,','parent.OWNERS=(0x30de60,0x314710,')
 text=text.replace("counts[0]==53","counts[0]==725")
 text=text.replace("'include/halo2/native_tu15_solver_hull_query.hpp','tools/observe_native_solver_hull_query_actual_tu15.cpp','tools/validate_native_solver_hull_query_actual_tu15.py'", "'include/halo2/native_tu15_convex_simplex_single.hpp','tools/observe_native_convex_simplex_single_actual_tu15.cpp','tools/validate_native_convex_simplex_single_actual_tu15.py'")
 text=text.replace('halo2/native_tu15_solver_hull_query.hpp','halo2/native_tu15_convex_simplex_single.hpp')
 text=text.replace("r['qualified_child_owner']=0x314710;r['qualified_child_sites']=53", "r['qualified_child_owner']=0x30de60;r['qualified_child_sites']=725")
 env=dict(hull.__dict__,UNITS=UNITS,observer_source=observer_source,__doc__=__doc__)
 try:exec(text,env);rc=env['main']()
 finally:subprocess.run=original_run
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(p.read_text());r['retained_and_initialized_work_domain']=True;r['raw_private_copy_ledger_qualified']=False;r['shared_unprovided_memory_guard']=True;r['captured_child_invocation_claimed']=False;r['new_proof_tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();r['mutants']=mutants;r['native_full_725_site_domain']=False
 own={int(line.split()[0],16)for line in (p.parent/'0030de60.txt').read_text().splitlines()}
 r['executed_owner_sites']=len({pc for compiler in ['g++','clang++']for profile in json.loads((p.parent/f'{compiler}-actual.json').read_text())['profiles']for pc in profile['source_executed_sites']if pc in own})
 p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())

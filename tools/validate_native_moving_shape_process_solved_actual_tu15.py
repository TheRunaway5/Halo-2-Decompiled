#!/usr/bin/env python3
"""Genuine2E→native2F→native30E full-return, stopped at real30CD publication.

No result, private afterimage or unavailable Source work bytes feed the native
chain. Every kernel/provider shares the same initialized-DATA validity view.
"""
import hashlib,inspect,json,sys,subprocess
from pathlib import Path
import validate_native_moving_shape_process_actual_tu15 as actual
import probe_native_moving_shape_solver_original_tu15 as graph
ROOT=Path(__file__).resolve().parents[1]
UNITS=actual.UNITS+('native_tu15_convex_contact_process.cpp','native_tu15_convex_contact_query.cpp','native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp','native_tu15_convex_simplex_solver.cpp','native_tu15_convex_simplex_single.cpp','native_tu15_contact_features.cpp','native_tu15_solver_hull_query.cpp','native_tu15_solver_hull_single_vertex.cpp','native_tu15_solver_hull_support.cpp','native_tu15_contact_shape.cpp')
def observer_source():
 text=(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text();base=text[:text.index('int main(int argc,char**argv)')].replace('std::array<std::byte,4096>data','std::array<std::byte,16384>data').replace('<=4096;','<=16384;')
 return base+(ROOT/'tools/observe_native_moving_shape_process_solved_actual_tu15.cpp').read_text()
def main():
 original_run=subprocess.run;compiled={};mutants=[]
 process=ROOT/'src/physics/native_tu15_convex_contact_process.cpp';single=ROOT/'src/physics/native_tu15_convex_simplex_single.cpp'
 changes={'wrong_radius':(process,'minus(m.word(radius_shape+0xcu));','plus(m.word(radius_shape+0xcu));'),'wrong_second_point':(process,'transpose_direction(x,y,z,relative)','transpose_direction(z,y,x,relative)'),'disabled_guard':(single,'if(n)regions_.push_back({p,std::vector<bool>(n,false)});','(void)n;')}
 def checked_run(command,*args,**kwargs):
  r=original_run(command,*args,**kwargs)
  if isinstance(command,list)and str(process)in command and r.returncode==0:compiled[command[command.index('-o')+1]]=list(command)
  if isinstance(command,list)and command and command[0]in compiled and r.returncode==0:
   build=compiled[command[0]];compiler=build[0];scratch=Path(command[0]).parent;output=Path(command[4]).parent
   for name,(kernel,old,new)in changes.items():
    source=kernel.read_text();assert source.count(old)==1;(scratch/f'{name}.cpp').write_text(source.replace(old,new));target=scratch/(name+'-'+compiler)
    variant=[str(scratch/f'{name}.cpp')if c==str(kernel)else str(target)if c==command[0]else c for c in build];variant.insert(1,'-I'+str(ROOT/'src/physics'));b=original_run(variant,capture_output=True,text=True);(output/f'{compiler}-{name}-build.log').write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr[-4000:]
    p=output/f'{compiler}-{name}.json';run=original_run([str(target),*command[1:4],str(p)],capture_output=True,text=True);(output/f'{compiler}-{name}.log').write_text(run.stdout+run.stderr);assert run.returncode!=0 and p.exists()and not json.loads(p.read_text())['passed'],name+' survived';mutants.append(dict(compiler=compiler,mutant=name,rejected=True,report=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
  return r
 subprocess.run=checked_run
 parent=actual.parent;parent.OWNERS=(*parent.OWNERS,0x2f8b60,0x30e930,0x30de60,0x314710,0x313bb0,0x32a660,0x313560,0x313390,0x313290,0x2e5e20,0x3070f0,0x2e8d20,0x2e7d90,0x2e5db0,0x1f6610,0x2e4760,0x30d350)
 text=inspect.getsource(parent.metadata).replace('    assert not op.mem.segment;base,index=',"    if op.mem.segment:\n     assert ins.reg_name(op.mem.segment)=='fs';continue\n    base,index=");space=dict(parent.__dict__);exec(text,space);build=space['metadata']
 def metadata(image,output):
  r=build(image,output)
  for site in r['sites']:
   if site['instruction']in(0x313bf5,0x30ea30,0x30ea37):site['cpu_dispatch_fetch']=True
  r['cpu_dispatch_tables']=[dict(address=p,size=n,bytes=image.read(p,n).hex(),sha256=hashlib.sha256(image.read(p,n)).hexdigest())for p,n in [(0x3146f8,20),(0x30f6e0,44),(0x30f70c,25)]]
  return r
 parent.metadata=metadata
 text=inspect.getsource(actual.main).replace("(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()","observer_source()")
 text=text.replace("'tools/validate_native_moving_shape_process_actual_tu15.py'","'tools/validate_native_moving_shape_process_actual_tu15.py','tools/observe_native_moving_shape_process_solved_actual_tu15.cpp','tools/validate_native_moving_shape_process_solved_actual_tu15.py','include/halo2/native_tu15_convex_simplex_solver_binding.hpp'")
 text=text.replace('halo2/native_tu15_activation_services.hpp','halo2/native_tu15_activation_services.hpp'+chr(34)+chr(92)*2+'n#include '+chr(34)+'halo2/native_tu15_convex_contact_process.hpp'+chr(34)+chr(92)*2+'n#include '+chr(34)+'halo2/native_tu15_convex_simplex_solver_binding.hpp')
 text=text.replace("text=text.replace('production_binding=True'","text=text.replace('source=inspect.getsource(base.original);source=','source=original_source_with_fs(base.original);source=');text=text.replace('production_binding=True'")
 text=text.replace('environment=dict(common.__dict__,parent=parent,__doc__=__doc__)','environment=dict(common.__dict__,parent=parent,observer_source=observer_source,original_source_with_fs=original_source_with_fs,__doc__=__doc__)')
 text=text.replace('production_binding=False,first_actual_child_entry_only=True','production_binding=False,first_actual_child_entry_only=False,solved_prefix_to_publication_only=True')
 env=dict(actual.__dict__,UNITS=UNITS,observer_source=observer_source,original_source_with_fs=graph.original_source_with_fs,__doc__=__doc__)
 try:exec(text,env);rc=env['main']()
 finally:subprocess.run=original_run
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(p.read_text());r['next_actual_owner']=0x30cd00;r['whole_process_qualified']=False;r['same_guarded_memory']=True;r['raw_private_ledger_qualified']=False;r['mutants']=mutants;p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())

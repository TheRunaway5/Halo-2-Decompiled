#!/usr/bin/env python3
"""Selected whole30E930→native30DE/hull/features on genuine1479 DATA.

Every captured retained byte and Source-initialized work publication is strict.
The four unavailable dead-private rows stay guarded through the whole solver.
"""
import hashlib,inspect,json,sys,subprocess
from pathlib import Path
import validate_native_solver_hull_query_actual_tu15 as hull
ROOT=Path(__file__).resolve().parents[1]
UNITS=('native_tu15_convex_simplex_solver.cpp','native_tu15_convex_simplex_single.cpp','native_tu15_contact_features.cpp')+hull.UNITS
PRODUCTION_BINDING=False
OVERLAP_INPUTS=False
def observer_source():
 base=(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text();base=base[:base.index('int main(int argc,char**argv)')]
 text=(ROOT/'tools/observe_native_convex_simplex_single_actual_tu15.cpp').read_text()
 text=text.replace('#include <set>','#include <set>\n#include "halo2/native_tu15_contact_features.hpp"')
 text=text.replace('0x30de60u','0x30e930u').replace('std::array<std::uint32_t,4>args','std::array<std::uint32_t,5>args').replace('k<4;++k)args','k<5;++k)args')
 old='for(auto [p,n]:{std::pair{work,8u},std::pair{work+0x20u,16u},std::pair{work+0xa0u,16u},std::pair{work+0xe0u,16u},std::pair{args[2],64u}})inputs'
 new='for(auto [p,n]:{std::pair{work+0xcu,1u},std::pair{args[2],64u}})inputs'
 assert text.count(old)==1;text=text.replace(old,new)
 text=text.replace('return captured(p,n)||contains(inputs,p,n);','if(captured(p,n)||contains(inputs,p,n))return true;for(auto [a,z]:{std::pair{work,12u},std::pair{work+0x10u,4u},std::pair{work+0x20u,16u},std::pair{work+0xa0u,16u},std::pair{work+0xe0u,16u},std::pair{work+0x120u,16u}})if(p>=a&&std::uint64_t(p-a)+n<=z)return true;return false;')
 text=text.replace('std::pair{work,8u},std::pair{work+0x20u,16u},std::pair{work+0xe0u,16u},std::pair{work+0x120u,16u}', 'std::pair{work,12u},std::pair{work+0x10u,4u},std::pair{work+0x20u,16u},std::pair{work+0xa0u,16u},std::pair{work+0xe0u,16u},std::pair{work+0x120u,16u},std::pair{args[4],16u}')
 text=text.replace('std::pair{work,8u},std::pair{work+0x20u,16u},std::pair{work+0xa0u,16u},std::pair{work+0xe0u,16u},std::pair{work+0x120u,16u}', 'std::pair{work,12u},std::pair{work+0x10u,4u},std::pair{work+0x20u,16u},std::pair{work+0xa0u,16u},std::pair{work+0xe0u,16u},std::pair{work+0x120u,16u},std::pair{args[4],16u}')
 text=text.replace('sp+20u','sp+24u').replace('Source30DE RET16','Source30E RET20')
 check='check(pc==0x313bf5u&&p==0x3146fcu&&n==4&&values[0]==1u,"CPU dispatch domain differs");'
 replacement='''if(pc==0x30ea30u)check(values[0]<25&&p==0x30f70cu+values[0]&&n==1,"Source simplex case map differs");else if(pc==0x30ea37u)check(p>=0x30f6e0u&&p+4u<=0x30f70cu&&n==4,"Source simplex dispatch differs");else check(pc==0x313bf5u&&p==0x3146fcu&&n==4&&values[0]==1u,"Source hull dispatch differs");'''
 assert text.count(check)==1;text=text.replace(check,replacement)
 text=text.replace('J reads=J::array(),stores=J::array()', 'for(const auto&t:meta.at("simplex_dispatch_tables"))check(process_hex(original.read(t.at("address"),t.at("bytes").get<std::string>().size()/2u))==t.at("bytes").get<std::string>(),"Simplex CPU table authentication failed");J reads=J::array(),stores=J::array()')
 text=text.replace('NativeTu15ConvexSimplexSingleServices services;services.support=', 'NativeTu15ConvexSimplexSolverServices services;services.features=[&](auto&guarded,auto shape,auto owner,auto ids,auto count,auto output,auto&current){check(&guarded==&view,"Features bypassed shared DATA guard");native_tu15_contact_shape_features(guarded,shape,owner,ids,count,output,current,0);};services.single.support=')
 text=text.replace('NativeTu15ConvexSimplexSingleStats activity;','NativeTu15ConvexSimplexSolverStats activity;')
 old='''native_tu15_solve_single_vertex_convex_simplex(view,{work,{args[0]},{args[1]},args[2]},
       {p+0xa00u,p+0xb00u,p+0xb40u,{p+0x400u,{p+0x200u,p+0x204u,{p,p+0x10u,p+0x30u,p+0x60u,p+0x20u}}}},fp,0,services,{},&activity)'''
 new='''native_tu15_solve_convex_contact_simplex(view,{work,{args[0]},{args[1]},args[2],args[3],args[4]},
       {p+0xe00u,p+0xe04u,p+0xe08u,p+0xe10u,p+0xe20u,{p+0xa00u,p+0xb00u,p+0xb40u,{p+0x400u,{p+0x200u,p+0x204u,{p,p+0x10u,p+0x30u,p+0x60u,p+0x20u}}}}},fp,0,services,{},&activity)'''
 assert text.count(old)==1;text=text.replace(old,new)
 text=text.replace('activity.hull.hull.support.shape_support_calls','activity.single.hull.hull.support.shape_support_calls')
 if PRODUCTION_BINDING:
  text=text.replace('#include "halo2/native_tu15_contact_features.hpp"','#include "halo2/native_tu15_contact_features.hpp"\n#include "halo2/native_tu15_convex_simplex_solver_binding.hpp"')
  text=text.replace('NativeTu15ConvexSimplexSolverStats activity;','NativeTu15ConvexSimplexBindingActivity activity;')
  old='''const auto result=native_tu15_solve_convex_contact_simplex(view,{work,{args[0]},{args[1]},args[2],args[3],args[4]},
       {p+0xe00u,p+0xe04u,p+0xe08u,p+0xe10u,p+0xe20u,{p+0xa00u,p+0xb00u,p+0xb40u,{p+0x400u,{p+0x200u,p+0x204u,{p,p+0x10u,p+0x30u,p+0x60u,p+0x20u}}}}},fp,0,services,{},&activity)'''
  new='''auto bound=native_tu15_make_bound_convex_contact_services(view,
       {{p+0xe00u,p+0xe04u,p+0xe08u,p+0xe10u,p+0xe20u,{p+0xa00u,p+0xb00u,p+0xb40u,{p+0x400u,{p+0x200u,p+0x204u,{p,p+0x10u,p+0x30u,p+0x60u,p+0x20u}}}}},shape_scratch},0,{},&activity);
   const auto result=bound.solve_simplex(work,{args[0]},{args[1]},args[2],args[3],args[4],fp)'''
  assert text.count(old)==1;text=text.replace(old,new).replace('activity.single.hull.hull.support.shape_support_calls','activity.solver.single.hull.hull.support.shape_support_calls')
 text=text.replace('Native30DE','Native30E').replace('Source simplex','Source whole simplex')
 if OVERLAP_INPUTS:
  # Additional ordinary input: the real overlap-query parent initializes this
  # one byte to1. Apply that declared input independently before either owner
  # runs; no returned simplex/count/plane byte is supplied.
  text=text.replace('for(unsigned profile=0;profile<4;++profile)', 'for(unsigned profile=0;profile<8;++profile)')
  text=text.replace('(profile<<10u)', '((profile%4u)<<10u)').replace('(profile<<13u)', '((profile%4u)<<13u)')
  old='if(c.regs().rip==0x30e930u)return;'
  assert text.count(old)==1
  text=text.replace(old,'if(c.regs().rip==0x30e930u){if(profile>=4u){const std::array value{std::byte{1}};c.write(std::uint32_t(c.regs().rcx)+12u,value);}return;}')
  text=text.replace('{"declared_rounding_control",profile}', '{"declared_rounding_control",profile%4u},{"declared_overlap_flag",profile>=4u}')
 return base+text
def main():
 global PRODUCTION_BINDING,OVERLAP_INPUTS
 direction_header=ROOT/'include/halo2/native_tu15_simplex_direction_kernel.hpp';direction_pin=hashlib.sha256(direction_header.read_bytes()).hexdigest()
 candidate_header=ROOT/'include/halo2/native_tu15_simplex_candidate_kernel.hpp';candidate_pin=hashlib.sha256(candidate_header.read_bytes()).hexdigest()
 finalize_header=ROOT/'include/halo2/native_tu15_simplex_finalize_kernel.hpp';finalize_pin=hashlib.sha256(finalize_header.read_bytes()).hexdigest()
 feature_headers={ROOT/'include/halo2'/name:hashlib.sha256((ROOT/'include/halo2'/name).read_bytes()).hexdigest()for name in ('native_tu15_polygon_features_kernel.hpp','native_tu15_polygon_features_value.hpp','native_tu15_polygon_support_kernel.hpp','native_tu15_polygon_support_value.hpp')}
 original_argv=list(sys.argv);PRODUCTION_BINDING='--production-binding'in sys.argv
 if PRODUCTION_BINDING:sys.argv.remove('--production-binding')
 OVERLAP_INPUTS='--overlap-inputs'in sys.argv
 if OVERLAP_INPUTS:sys.argv.remove('--overlap-inputs')
 original_run=subprocess.run;compiled={};mutants=[];kernel=ROOT/'src/physics/native_tu15_convex_simplex_solver.cpp'
 mutations={
  'wrong_candidate':('subtract(opposite,first)','add(opposite,first)'),
  'early_second_owner':('const auto current_second=m.word(input.work+4u),current_first=m.word(input.work);\n        const auto second_owner=m.word(second_table+0x34u);','const auto second_owner=m.word(second_table+0x34u);\n        const auto current_second=m.word(input.work+4u),current_first=m.word(input.work);'),
 }
 header_mutations={'wrong_normal_coefficient':(finalize_header,'0x3f000000u','0x3f800000u')}
 if OVERLAP_INPUTS:mutations['wrong_overlap_result']=('result=3u;','result=2u;')
 def checked_run(command,*args,**kwargs):
  r=original_run(command,*args,**kwargs)
  if isinstance(command,list)and str(kernel)in command and r.returncode==0:compiled[command[command.index('-o')+1]]=list(command)
  if isinstance(command,list)and command and command[0]in compiled and r.returncode==0:
   build=compiled[command[0]];compiler=build[0];scratch=Path(command[0]).parent;output=Path(command[4]).parent
   for name,(source_path,old,new)in {**{name:(kernel,*pair)for name,pair in mutations.items()},**header_mutations}.items():
    source=source_path.read_text();assert source.count(old)==1;target=scratch/(name+'-'+compiler)
    if source_path==kernel:
     changed_source=scratch/f'{name}.cpp';changed_source.write_text(source.replace(old,new))
     mutant=[str(changed_source)if c==str(kernel)else str(target)if c==command[0]else c for c in build]
    else:
     directory=scratch/(name+'-include');(directory/'halo2').mkdir(parents=True,exist_ok=True);(directory/'halo2'/source_path.name).write_text(source.replace(old,new))
     mutant=[str(target)if c==command[0]else c for c in build];mutant.insert(1,'-I'+str(directory))
    mutant.insert(1,'-I'+str(ROOT/'src/physics'))
    b=original_run(mutant,capture_output=True,text=True);(output/f'{compiler}-{name}-build.log').write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr[-4000:]
    result=output/f'{compiler}-{name}.json';run=original_run([str(target),*command[1:4],str(result)],capture_output=True,text=True);(output/f'{compiler}-{name}.log').write_text(run.stdout+run.stderr);assert run.returncode!=0 and result.exists()and not json.loads(result.read_text())['passed'],name+' survived'
    mutants.append(dict(compiler=compiler,mutant=name,rejected=True,result=str(result),result_sha256=hashlib.sha256(result.read_bytes()).hexdigest()))
  return r
 subprocess.run=checked_run
 # The existing metadata builder authenticates the complete native closure.
 # Mark only the two architecture dispatch-table reads as nonsemantic; every
 # captured code bank is still compared unchanged by the ordinary bank proof.
 # Wrap the original builder after its53→1020 rewrite, so the two new markers
 # survive the hull wrapper's independent313BF5 table authentication.
 text=inspect.getsource(hull.main).replace('parent.OWNERS=(0x314710,','parent.OWNERS=(0x30e930,0x30de60,0x2e5e20,0x3070f0,0x314710,').replace('counts[0]==53','counts[0]==1020')
 text=text.replace('old_metadata=parent.metadata','old_metadata=decorate_metadata(parent.metadata)')
 text=text.replace("'include/halo2/native_tu15_solver_hull_query.hpp','tools/observe_native_solver_hull_query_actual_tu15.cpp','tools/validate_native_solver_hull_query_actual_tu15.py'", "'include/halo2/native_tu15_convex_simplex_solver.hpp','include/halo2/native_tu15_convex_simplex_single.hpp','tools/validate_native_convex_simplex_solver_actual_tu15.py','tools/observe_native_convex_simplex_single_actual_tu15.cpp'")
 text=text.replace('halo2/native_tu15_solver_hull_query.hpp','halo2/native_tu15_convex_simplex_solver.hpp').replace("r['qualified_child_owner']=0x314710;r['qualified_child_sites']=53","r['qualified_child_owner']=0x30e930;r['qualified_child_sites']=1020")
 def decorate_metadata(builder):
  def wrapped(image,output):
   result=builder(image,output)
   for site in result['sites']:
    if site['instruction']in(0x30ea30,0x30ea37):site['instruction_dispatch_code_fetch']=True
   result['simplex_dispatch_tables']=[dict(address=p,bytes=image.read(p,n).hex(),sha256=hashlib.sha256(image.read(p,n)).hexdigest())for p,n in [(0x30f6e0,44),(0x30f70c,25)]]
   return result
  return wrapped
 env=dict(hull.__dict__,UNITS=UNITS,observer_source=observer_source,decorate_metadata=decorate_metadata,__doc__=__doc__)
 try:exec(text,env);rc=env['main']()
 finally:subprocess.run=original_run;sys.argv[:]=original_argv
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(p.read_text());r['retained_and_initialized_work_domain']=True;r['raw_private_copy_ledger_qualified']=False;r['shared_unprovided_memory_guard']=True;r['native_full_1020_site_domain']=False;r['new_proof_tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();own={int(s.split()[0],16)for s in (p.parent/'0030e930.txt').read_text().splitlines()};r['executed_owner_sites']=len({pc for c in ['g++','clang++']for f in json.loads((p.parent/f'{c}-actual.json').read_text())['profiles']for pc in f['source_executed_sites']if pc in own});r['mutants']=mutants;r['production_binding']=PRODUCTION_BINDING
 if PRODUCTION_BINDING:
  for f in ['include/halo2/native_tu15_convex_simplex_solver_binding.hpp','include/halo2/native_tu15_convex_contact_query.hpp','include/halo2/native_tu15_contact_features.hpp']:r['source_hashes'][f]=hashlib.sha256((ROOT/f).read_bytes()).hexdigest()
 r['declared_overlap_input_profiles']=OVERLAP_INPUTS
 assert hashlib.sha256(direction_header.read_bytes()).hexdigest()==direction_pin,'Shared simplex direction changed during proof'
 r['source_hashes'][str(direction_header.relative_to(ROOT))]=direction_pin
 assert hashlib.sha256(candidate_header.read_bytes()).hexdigest()==candidate_pin,'Shared simplex candidate changed during proof'
 r['source_hashes'][str(candidate_header.relative_to(ROOT))]=candidate_pin
 assert hashlib.sha256(finalize_header.read_bytes()).hexdigest()==finalize_pin,'Shared simplex finalization changed during proof'
 r['source_hashes'][str(finalize_header.relative_to(ROOT))]=finalize_pin
 for header,pin in feature_headers.items():
  assert hashlib.sha256(header.read_bytes()).hexdigest()==pin,'Shared polygon feature/support changed during proof'
  r['source_hashes'][str(header.relative_to(ROOT))]=pin
 if OVERLAP_INPUTS:
  r['profiles_per_compiler']=len(json.loads((p.parent/'g++-actual.json').read_text())['profiles'])
  r['other_profiles']='Four rounding controls for each of the source-derived flag0 and separately declared ordinary overlap flag1 inputs; no live overlap invocation claimed'
 p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())

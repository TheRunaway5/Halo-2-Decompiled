#!/usr/bin/env python3
"""Native2E5320/fast2F8B60 profiling with genuine external hardware-clock inputs.

The unchanged original executes every RDTSC. Its recorded counter samples are
typed external platform inputs to the native timestamp service; they are not
physics/contact outcomes or a GP register postimage. Arithmetic/DATA remains
computed natively. Production must supply its actual platform cycle clock.
"""
import copy,inspect,struct
import validate_native_moving_shape_process_fast_tu15 as parent
ORIGINAL_CASES=parent.cases
ORIGINAL_DRIVER=parent.driver_source
def cases(image):
 rows=[]
 for seed in ORIGINAL_CASES(image):
  if seed['name'].startswith('fault_seed'):continue
  c=copy.deepcopy(seed);bank=next(b for b in c['banks']if b['address']==0x48c974);bank['before']=struct.pack('<II',parent.base.BASE+0xf000,parent.base.BASE+0xf080).hex();c['name']='profiled_'+c['name'];rows.append(c)
 for seed in(rows[0],rows[72],rows[-1]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=ORIGINAL_DRIVER();anchor='   services.classify_shape=';assert text.count(anchor)==1
 text=text.replace(anchor,'''   std::size_t timestamp_index{};
   services.timestamp_counter=[&]()->std::uint64_t{check(timestamp_index<c.at("timestamps").size(),"Native profile selected extra platform timestamp");return c.at("timestamps").at(timestamp_index++).at("value").get<std::uint64_t>();};
'''+anchor)
 text=text.replace('NativeTu15ConvexProcessServices convex;convex.geometry.solve_simplex','NativeTu15ConvexProcessServices convex;convex.geometry.timestamp_counter=services.timestamp_counter;convex.geometry.solve_simplex')
 text=text.replace('   check(failed==fault_case','   check(timestamp_index==c.at("timestamps").size(),"Native profile skipped original platform timestamp");\n   check(failed==fault_case')
 return text
def main():
 parent.cases=cases;parent.driver_source=driver_source
 source=inspect.getsource(parent.main).replace("scope=__doc__,remaining=","original_platform_counter_samples_used=True,supplied_physics_outcomes=False,scope=__doc__,remaining=")
 source=source.replace("('wrong_clear','shape+0x10u,_mm_setzero_ps()','shape+0x10u,broadcast(0x3f800000u)')):","('wrong_clear','shape+0x10u,_mm_setzero_ps()','shape+0x10u,broadcast(0x3f800000u)'),('wrong_profile_marker','ending?0x41c394u:0x41c398u','ending?0x41c398u:0x41c394u')):")
 environment=dict(parent.__dict__,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

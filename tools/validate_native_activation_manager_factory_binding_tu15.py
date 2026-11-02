#!/usr/bin/env python3
"""Same native-only manager service builder in genuine sleeping-body context."""
import json,os,sys
from pathlib import Path
import validate_native_object_motion_activation_tu15 as parent
def main():
 assert '--manager'not in sys.argv;sys.argv.append('--manager')
 write=Path.write_text;scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 def staged_write(path,text,*args,**kwargs):
  if path.is_relative_to(scratch)and path.name=='driver.cpp':
   text='#include "halo2/native_tu15_activation_manager_factory_binding.hpp"\n'+text
   text=text.replace('NativeTu15ActivationContactManagerStats manager_stats;','NativeTu15ActivationManagerFactoryBindingActivity manager_binding_activity;NativeTu15ActivationContactManagerStats manager_stats;')
   old='return native_tu15_create_activation_contact_manager(memory,factory,first,second,fp,manager_services,{},&manager_stats);'
   assert text.count(old)==1
   text=text.replace(old,'return native_tu15_create_bound_activation_contact_manager(memory,factory,owner,first,second,fp,allocation,{},&manager_binding_activity);')
   text=text.replace('manager_stats.','manager_binding_activity.managers.')
   anchor='\n std::size_t different{},compared{};';assert text.count(anchor)==1
   text=text.replace(anchor,'\n check(manager_binding_activity.refusals==0&&manager_binding_activity.unqualified_factories==0&&manager_binding_activity.body_destructor_refusals==0&&manager_binding_activity.base_lock_refusals==0,"Native manager builder reached an unqualified service");\n report.update(J{{"native_manager_binding_attempts",manager_binding_activity.attempts},{"native_manager_binding_completed",manager_binding_activity.completed},{"native_manager_binding_refusals",manager_binding_activity.refusals},{"native_manager_binding_body_refusals",manager_binding_activity.body_destructor_refusals},{"native_manager_binding_lock_releases",manager_binding_activity.base_lock_releases}});'+anchor)
  return write(path,text,*args,**kwargs)
 Path.write_text=staged_write
 try:return parent.main()
 finally:
  Path.write_text=write
  from validate_native_contact_agent_dispatch_tu15 import sha
  report=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if report.exists():
   d=json.loads(report.read_text());d.setdefault('source_hashes',{}).update({name:sha(parent.ROOT/name)for name in('include/halo2/native_tu15_activation_manager_factory_binding.hpp','tools/validate_native_activation_manager_factory_binding_tu15.py')});d['scope']='Genuine sleeping-body world DATA and actual originalB9800 closure, same native-only3232E0 manager builder. Native allocation/listeners/constructors; unknown body lifetime and registered locks refuse explicitly. Other unrelated originalservices remain real independent callbacks, no wholeclosure/native-live claim.';report.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())

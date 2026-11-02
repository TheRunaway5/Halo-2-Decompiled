#!/usr/bin/env python3
"""Exact eight wrapped-family routes/getter/twoJSON and positivegate truth."""
import inspect
import validate_native_phantom_added_world_counters_tu15 as parent
from prepare_native_wrapped_factory_world_binding_tu15 import FIELDS
def main():
 parent.FIELDS=FIELDS;source=inspect.getsource(parent.main)
 old="start=world.index('            const auto collect_added=[&]{if(stats){');end=world.index('            try{native_tu15_notify_bound_gameplay_phantom_added',start);aggregate=world[start:end];branch=world[start:world.index('            return;',end)];assert branch.count('collect_added();')==2 and'catch(...){collect_added();throw;}'in branch"
 new="start=world.index('        const auto collect_wrapped=[&]{if(stats){');end=world.index('        try{',start);aggregate=world[start:end];branch=world[start:world.index('      if(target.identity==0x2f7190u)',end)];assert branch.count('collect_wrapped();')==2 and'catch(...){collect_wrapped();'in branch"
 assert old in source;source=source.replace(old,new)
 source=source.replace('native_tu15_phantom_added_binding.hpp','native_tu15_wrapped_factory_binding.hpp').replace('NativeTu15PhantomAddedBindingActivity','NativeTu15WrappedFactoryBindingActivity').replace('collect_added();}','collect_wrapped();}')
 source=source.replace('HALO2_REQUIRE_NATIVE_GAMEPLAY_PHANTOM_ADDED','HALO2_REQUIRE_NATIVE_WRAPPED_AGENT_FACTORY')
 old='NativeTu15PhysicsFrameExecutionStats good;good.native_phantom_added_listener_attempts=1;good.native_phantom_added_listener_calls=1;check(passes(good),"Qualified positive added gate rejected");check(!passes({}),"Added gate admitted no calls");'
 new='NativeTu15PhysicsFrameExecutionStats good;good.native_wrapped_factory_attempts=3;good.native_wrapped_factory_calls=3;good.native_wrapped_factory_wrapped_entries=1;good.native_wrapped_factory_transformed_entries=1;good.native_wrapped_factory_convex_entries=1;good.native_wrapped_factory_class_queries=5;check(passes(good),"Qualified wrapped gate rejected");check(!passes({}),"Wrapped gate admitted no calls");'
 assert old in source;source=source.replace(old,new)
 begin=source.index("  negatives={");end=source.index('\n  for f,v in negatives.items()',begin)
 source=source[:begin]+"  negatives={'native_wrapped_factory_attempts':4,'native_wrapped_factory_calls':2,'native_wrapped_factory_refusals':1,'native_wrapped_factory_wrapped_entries':0,'native_wrapped_factory_transformed_entries':0,'native_wrapped_factory_convex_entries':0,'native_wrapped_factory_class_queries':0,'native_wrapped_factory_unqualified_owners':1,'native_pair_creation_refusals':1}"+source[end:]
 source=source.replace('(0x282700,0xb1870)','(0x2f1cc0,0x2f0df0,0x2f7190,0x2e44d0,0x2e4eb0,0x2f8300)')
 begin=source.index("  source+='auto deletion=good;");end=source.index(";(project/'counters.cpp').write_text(source);",begin)
 source=source[:begin]+'''  source+='unsetenv("HALO2_REQUIRE_NATIVE_WRAPPED_AGENT_FACTORY");check(passes({}),"Disabled gate changed admission");return 0;}\\n' '''+source[end:]
 source=source.replace('negative_gate_controls=len(negatives)+3','negative_gate_controls=len(negatives)+7')
 environment=dict(parent.__dict__,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

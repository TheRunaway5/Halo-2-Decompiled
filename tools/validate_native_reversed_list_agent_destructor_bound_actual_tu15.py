#!/usr/bin/env python3
"""Same native-only destructor binding as World on genuine1474 child input."""
import inspect
import validate_native_reversed_list_agent_destructor_actual_tu15 as parent
def main():
 text=inspect.getsource(parent.main)
 anchor=" source=inspect.getsource(parent.main);source=source.replace('child.cases=cases','child.cases=actual_cases')"
 assert text.count(anchor)==1
 additions=[
  ('#include "halo2/native_tu15_reversed_list_agent_destructor.hpp"','#include "halo2/native_tu15_reversed_list_agent_destructor_binding.hpp"'),
  ('NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;','NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;NativeTu15PairDeletionNativeScope native_scope;NativeTu15ReversedListAgentDestructorActivity native_activity;'),
  ('native_tu15_delete_reversed_list_contact_agent(memory,{receiver},c.at("arguments")[0],fp,services,observe).identity;','native_tu15_delete_bound_reversed_list_contact_agent(memory,{receiver},c.at("arguments")[0],fp,services,native_scope,observe,&native_activity).identity;'),
  ('      check(calls==','      check(native_activity.attempts==1&&native_activity.completed==1&&native_activity.refusals==0&&native_activity.native_only.refused==0,"Native bound destructor count differs");native_scope.reject_unconverted(0xdeadbeefu);\n      check(calls=='),
 ]
 insertion='changes.extend('+repr(additions)+');'
 generic_anchor=" extra=''.join(";transform=" source=source.replace("+repr(generic_anchor)+","+repr(' '+insertion+'\n'+generic_anchor)+")\n"
 text=text.replace(anchor,anchor+'\n'+transform)
 environment=dict(parent.__dict__);exec(text,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

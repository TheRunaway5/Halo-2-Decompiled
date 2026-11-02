#!/usr/bin/env python3
"""Qualify the sole native-only pair-deletion adapter on genuine pair DATA."""
import inspect
import validate_native_pair_delete_native_children_tu15 as parent

def main():
    text=inspect.getsource(parent.main)
    anchor=" addition='\\n  '+extra+'\\n  '"
    assert text.count(anchor)==1
    changes=[
      ('#include "halo2/native_tu15_pair_deletion_binding.hpp"','#include "halo2/native_tu15_pair_deletion_binding.hpp"\n#include "halo2/native_tu15_pair_deletion_native_scope.hpp"'),
      ('NativeTu15PairDeletionBindingActivity binding;','NativeTu15PairDeletionBindingActivity binding;NativeTu15PairDeletionNativeScope native_scope;NativeTu15PairDeletionNativeScopeActivity scope_activity;'),
      ('native_tu15_delete_bound_pair(memory,handler,pair,{formal},fp,{services.destroy_agent,services.agents.release_manager},','native_tu15_delete_bound_pair_in_native_scope(memory,handler,pair,{formal},fp,{services.destroy_agent,services.agents.release_manager},native_scope,'),
      ('}});},&binding);check(bridge.calls.empty()','}});},&binding,&scope_activity);native_scope.reject_unconverted(0xdeadbeefu);check(scope_activity.refused==0,"Unexpected native-scope refusal");check(bridge.calls.empty()'),
      ('{"native_binding_attempts",binding.attempts}','{"native_scope_refusals",scope_activity.refused},{"native_binding_attempts",binding.attempts}'),
    ]
    transform=''.join('assert driver.count('+repr(old)+')==1;driver=driver.replace('+repr(old)+','+repr(new)+');' for old,new in changes)
    text=text.replace(anchor,' extra+='+repr(transform)+'\n'+anchor)
    environment=dict(parent.__dict__);environment['main']=None
    exec(text,environment)
    return environment['main']()

if __name__=='__main__':raise SystemExit(main())

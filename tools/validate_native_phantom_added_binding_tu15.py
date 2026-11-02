#!/usr/bin/env python3
"""Exact production added-listener builder plus readonly source-DATA observer."""
import inspect
import validate_native_phantom_added_listener_tu15 as parent
def main():
 source=inspect.getsource(parent.main)
 source=source.replace("'tools/validate_native_phantom_added_listener_tu15.py',*base.FILES", "'tools/validate_native_phantom_added_listener_tu15.py','include/halo2/native_tu15_phantom_added_binding.hpp','include/halo2/native_tu15_phantom_added_input_observer.hpp','tools/validate_native_phantom_added_binding_tu15.py',*base.FILES")
 source=source.replace('#include "halo2/native_tu15_phantom_added_listener.hpp"', '#include "halo2/native_tu15_phantom_added_binding.hpp"')
 before='native_tu15_notify_gameplay_phantom_added(memory,{c.at("arguments")[0]},fp,services,observe,&listener_activity)'
 assert source.count(before)==1
 source=source.replace(before,'native_tu15_notify_bound_gameplay_phantom_added(memory,c.at("receiver"),{c.at("arguments")[0]},fp,0,services,observe,&binding_activity)')
 before='NativeTu15PhantomAddedListenerStats listener_activity;';assert source.count(before)==1
 source=source.replace(before,'NativeTu15PhantomAddedBindingActivity binding_activity;unsigned observed_entries{},observed_deletes{};NativeTu15PhantomAddedInputObserverBinding input_observer(memory,[&](const auto&before,const auto&input){check(&before==&memory&&input.listener==c.at("receiver")&&input.event==c.at("arguments")[0]&&input.preserved_x87_depth==0,"Native added readonly input identity differs");if(input.phase==NativeTu15PhantomAddedInputPhase::ListenerEntry){check(input.owner==0x282700u,"Added entry observer owner differs");++observed_entries;}else{check(input.owner==0xba570u,"Added deletion observer owner differs");++observed_deletes;}});')
 before='      check(calls==c.at("allocator_calls"),name+" actual delete child boundary differs");';assert source.count(before)==1
 source=source.replace(before,before+r'\n      if(c.at("entry")==0x282700u){check(observed_entries==1&&observed_deletes==calls.size()&&binding_activity.attempts==1&&binding_activity.completed==unsigned(!failed&&!required)&&binding_activity.refusals==unsigned(failed||required),name+" native added production activity differs");}')
 environment=dict(parent.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

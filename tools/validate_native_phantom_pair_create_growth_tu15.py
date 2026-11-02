#!/usr/bin/env python3
"""Declared consecutive31FFC0 calls on the complete actual1471 BEFORE DATA.

An ordinary physical caller invokes unmodified source eight times. Its natural
count1/cap8 growth uses the real allocator and pool; no capacity or allocation
result is supplied. This is a declared caller sequence, not eight captured
multiplayer invocations. The exact single captured call is qualified separately.
"""
import inspect
import validate_native_phantom_pair_create_actual_tu15 as parent

def main():
 source=inspect.getsource(parent.main)
 needle="  fp=typed['floating_point'];all_cases="
 assert source.count(needle)==1
 setup=r'''  original=original.replace(' int3\n jmp *entry\n', ' int3\n mov $8,%ebp\n repeat_parent:\n push ${case["arguments"][0]}\n mov ${case["receiver"]},%ecx\n call *entry\n dec %ebp\n jne repeat_parent\n jmp {STOP}\n')
'''
 source=source.replace(needle,setup+needle)
 needle="  observer_path=scratch/'observer.cpp';observer_path.write_text(observer)"
 assert source.count(needle)==1
 source=source.replace(needle,"  observer=observer.replace('const auto site=sites.find(pc);', 'if(pc>=0x70000000u&&pc<0x70000100u){request(PTRACE_SINGLESTEP,child.pid);wait();continue;}const auto site=sites.find(pc);')\n"+needle)
 source=source.replace("assert all(c['after_esp']==base.SP+8", "assert all(c['after_esp']==base.SP")
 source=source.replace('try{native_tu15_create_phantom_pair(memory,c.at("arguments")[0],fp,pair_services);', 'try{for(unsigned invocation=0;invocation<8;++invocation)native_tu15_create_phantom_pair(memory,c.at("arguments")[0],fp,pair_services);')
 source=source.replace("'tools/validate_native_phantom_pair_create_actual_tu15.py','tools/validate_native_array_growth_tu15.py'", "'tools/validate_native_phantom_pair_create_actual_tu15.py','tools/validate_native_phantom_pair_create_growth_tu15.py','tools/validate_native_array_growth_tu15.py'")
 source=source.replace("scope=__doc__+' Actual selected callbacks", "declared_consecutive_calls=8,captured_consecutive_calls_claimed=False,physical_caller_bridge_range=[0x70000000,0x70000100],scope=__doc__+' Ordinary physical caller bridge control is outside TU1.5 owner operand metadata; unchanged source bodies remain checked. Actual selected callbacks")
 environment=dict(parent.__dict__,__doc__=__doc__);exec(source,environment);return environment['main']()

if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Genuine2E5320→native2F8B60 through real30E930 entry, no solver outcome."""
import inspect
from pathlib import Path
import validate_native_moving_shape_process_actual_tu15 as actual
import validate_native_moving_shape_process_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
EXTRA=('native_tu15_convex_contact_process.cpp','native_tu15_convex_contact_query.cpp','native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp')
def observer_source():
 text=(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()
 text=text.replace('if(pc==0x309060u||pc==0x309150u||pc==0x2f8b60u)','if(pc==0x30e930u)')
 text=text.replace('stop_owner==0x2f8b60u?16u:12u','20u')
 before='services.process_convex=[&](auto agent,auto first,auto second,auto config,auto output,auto&){boundary(0x2f8b60u,agent.identity,J::array({first.identity,second.identity,config.identity,output}));};'
 after='''services.process_convex=[&](auto agent,auto first,auto second,auto config,auto output,auto&state){
                NativeTu15ConvexProcessServices native;
                native.geometry.solve_simplex=[&](auto simplex,auto first_shape,auto second_shape,auto transform,auto support_cache,auto plane,auto&)->std::uint32_t{
                    check(stop_owner==0x30e930u&&boundary_args[0]==first_shape.identity&&boundary_args[1]==second_shape.identity&&boundary_args[3]==support_cache,"Actual simplex persistent formals differ");
                    std::array<std::byte,64>computed{};memory.read(transform,computed);check(original.read(boundary_args[2],64)==std::vector<std::byte>(computed.begin(),computed.end()),"Actual simplex relative transform DATA differs");
                    std::array<std::byte,1>flag{};memory.read(simplex+12u,flag);check(original.read(std::uint32_t(boundary_regs.rcx)+12u,1)==std::vector<std::byte>(flag.begin(),flag.end()),"Actual simplex initialized flag differs");
                    (void)plane;boundary_seen=true;throw std::runtime_error("actual-process-child-boundary");
                };
                native_tu15_process_convex_contact_agent(memory,{agent.identity},first,second,config,{output},{ProcessMemory::private_data+0x400u,ProcessMemory::private_data+0x700u},state,0,native,observe);
            };'''
 assert text.count(before)==1;text=text.replace(before,after);return text

def main():
 parent.OWNERS=(*parent.OWNERS,0x2f8b60)
 text=inspect.getsource(actual.main)
 text=text.replace("(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()","observer_source()")
 text=text.replace("'tools/validate_native_moving_shape_process_actual_tu15.py'","'tools/validate_native_moving_shape_process_actual_tu15.py','tools/validate_native_moving_shape_process_convex_actual_tu15.py'")
 text=text.replace('first_actual_child_entry_only=True','first_actual_child_entry_only=False,genuine_slow_solver_entry_only=True')
 text=text.replace('environment=dict(common.__dict__,parent=parent,__doc__=__doc__)','environment=dict(common.__dict__,parent=parent,observer_source=observer_source,__doc__=__doc__)')
 text=text.replace('native_tu15_activation_services.hpp', 'native_tu15_activation_services.hpp'+chr(34)+chr(92)*2+'n#include '+chr(34)+'halo2/native_tu15_convex_contact_process.hpp')
 environment=dict(actual.__dict__,parent=parent,observer_source=observer_source,UNITS=actual.UNITS+EXTRA,__doc__=__doc__)
 exec(text,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

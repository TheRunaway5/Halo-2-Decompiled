#!/usr/bin/env python3
"""Genuine1479 typed2E5320 native parent through its first real child entry.

All captured bytes, ordered retained operands/stores and FP are strict. The
source child entry is observed, never given a return/outcome. Native scratch
is disjoint arithmetic DATA; no GP/control postimage enters the kernel.
"""
import inspect
from pathlib import Path
import validate_native_look_reorientation_actual_tu15 as common
import validate_native_moving_shape_process_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
UNITS=('native_tu15_moving_shape_process.cpp','native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')
def main():
 text=inspect.getsource(common.main)
 text=text.replace("typed['owner']==0x18a2a0","typed['owner']==0x2e5320")
 text=text.replace('body=complete_body(code,0x18a2a0)','body=complete_body(code,0x2e5320)').replace("'0018a2a0.txt'","'002e5320.txt'")
 text=text.replace('meta=parent.metadata(image)','meta=parent.metadata(image,a.output)')
 text=text.replace("case=dict(entry=0x18a2a0,receiver=typed['control_index'],arguments=[]","case=dict(entry=0x2e5320,receiver=typed['agent'],arguments=[typed['first'],typed['second'],typed['configuration'],typed['output']]")
 text=text.replace('halo2/native_tu15_look_reorientation_binding.hpp','halo2/native_tu15_moving_shape_process.hpp"\\n#include "halo2/native_tu15_contact_agent_factory.hpp"\\n#include "halo2/native_tu15_activation_services.hpp')
 text=text.replace("(ROOT/'tools/observe_native_look_reorientation_actual_tu15.cpp').read_text()","(ROOT/'tools/observe_native_moving_shape_process_actual_tu15.cpp').read_text()")
 old="str(ROOT/'src/physics/native_tu15_look_reorientation.cpp')";assert text.count(old)==1;text=text.replace(old,','.join("str(ROOT/'src/physics'/"+repr(name)+")"for name in UNITS))
 begin=text.index('  files=(');end=text.index(';report.update(',begin)
 files=tuple('src/physics/'+name for name in UNITS)+('include/halo2/native_tu15_moving_shape_process.hpp','tools/observe_native_moving_shape_process_actual_tu15.cpp','tools/validate_native_moving_shape_process_actual_tu15.py','tools/validate_native_moving_shape_process_tu15.py')
 text=text[:begin]+'  files='+repr(files)+text[end:];text=text.replace('production_binding=True','production_binding=False,first_actual_child_entry_only=True')
 environment=dict(common.__dict__,parent=parent,__doc__=__doc__);exec(text,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

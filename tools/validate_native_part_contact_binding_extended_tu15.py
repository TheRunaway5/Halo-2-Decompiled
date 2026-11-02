#!/usr/bin/env python3
"""The same217 PART/surface/depth profiles using the extended evaluator."""
from pathlib import Path
import os
import validate_native_part_contact_binding_tu15 as parent

def main():
 base=parent.parent.parent.parent.parent.prop.base
 base.FILES=(*base.FILES,'include/halo2/native_tu15_part_contact_binding_extended.hpp',
  'include/halo2/native_tu15_function_evaluation_extended.hpp','src/core/native_tu15_function_evaluation_extended.cpp',
  'include/halo2/native_tu15_function_curves.hpp','src/core/native_tu15_function_curves.cpp',
  'include/halo2/native_tu15_function_control_point.hpp','src/core/native_tu15_function_control_point.cpp',
  'tools/validate_native_part_contact_binding_extended_tu15.py')
 write=Path.write_text;scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 def staged_write(path,text,*args,**kwargs):
  if path.is_relative_to(scratch):
   if path.name=='native_tu15_array_growth_tests.cpp':
    text=text.replace('"halo2/native_tu15_part_contact_binding.hpp"','"halo2/native_tu15_part_contact_binding_extended.hpp"')
    text=text.replace('"halo2/native_tu15_function_evaluation.hpp"','"halo2/native_tu15_function_evaluation_extended.hpp"')
    text=text.replace('NativeTu15PartContactBindingActivity','NativeTu15PartContactExtendedBindingActivity')
    text=text.replace('native_tu15_update_bound_part_contact_response(','native_tu15_update_bound_part_contact_response_extended(')
    text=text.replace('native_tu15_evaluate_authored_function(','native_tu15_evaluate_authored_function_extended(')
   if path.name=='CMakeLists.txt'and'part_contact_response.cpp'in text:
    old='src/core/native_tu15_function_evaluation.cpp'
    assert old in text
    text=text.replace(old,'src/core/native_tu15_function_evaluation_extended.cpp src/core/native_tu15_function_curves.cpp src/core/native_tu15_function_control_point.cpp')
  return write(path,text,*args,**kwargs)
 Path.write_text=staged_write
 try:return parent.main()
 finally:
  Path.write_text=write
  import sys,json
  report=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if report.exists():
   d=json.loads(report.read_text());d.update(extended_same_parent_compatibility=True,
    scope='Same217 declared fullPART/material/shader/currentdepth0 profiles using extended native evaluator and same source caller. Allsource stores/operands/banks/FP, no supplied function results. These existing cases selectkind0; other10kinds have separate whole492-site evaluator qualification, not generated-live-parent claim.')
   report.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())

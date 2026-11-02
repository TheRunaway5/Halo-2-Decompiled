#!/usr/bin/env python3
"""Compile32 point counter routes and PART/conveyor gate truth controls."""
import inspect
import validate_native_contact_point_binding_counters_tu15 as parent
from prepare_native_contact_point_reference_binding_tu15 import FIELDS as REFERENCE
from prepare_native_contact_point_part_binding_tu15 import FIELDS
def main():
 parent.FIELDS={**parent.FIELDS,**REFERENCE,**FIELDS};source=inspect.getsource(parent.main)
 source=source.replace('one.size()==16','one.size()==32')
 source=source.replace('negative_values={',"negative_values={'native_contact_point_constraint_reference_calls':1,'native_contact_point_constraint_reference_decrements':1,'native_contact_point_constraint_reference_runtime_refusals':1,'native_contact_point_part_calls':1,'native_contact_point_part_completions':1,'native_contact_point_part_refusals':1,'native_contact_point_part_function_refusals':1,'native_contact_point_part_function_fp_refusals':1,")
 source=source.replace('owners=(0x322f70,','owners=(0x1e0a60,0x184c50,0x355d0,0x13de70,0x322f70,')
 old='        source+=\'check(rejected==23,"Gate negative coverage differs");return 0;}\\n\'';assert source.count(old)==1
 extra='''check(rejected==35,"Gate negative coverage differs");
setenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_PART_RESPONSE","1",1);
check(!passes(good),"Positive PART gate admitted no release");
auto part=good;part.native_contact_point_part_requests=1;part.native_contact_point_part_calls=1;part.native_contact_point_part_completions=1;
check(passes(part),"Qualified PART gate rejected");
{auto bad=part;bad.native_contact_point_part_completions=0;check(!passes(bad),"PART gate admitted skipped completion");}
{auto bad=part;bad.native_contact_point_part_refusals=1;check(!passes(bad),"PART gate admitted refusal");}
{auto bad=part;bad.native_contact_point_part_requests=2;check(!passes(bad),"PART gate admitted unaccounted request");}
{auto bad=part;bad.native_contact_point_part_function_refusals=1;check(!passes(bad),"PART gate admitted unqualified source kind");}
{auto bad=part;bad.native_contact_point_part_function_fp_refusals=1;check(!passes(bad),"PART gate admitted FP refusal");}
setenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_CONVEYOR","1",1);
check(!passes(part),"Conveyor gate admitted no conveyor activity");
auto conveyor=part;conveyor.native_contact_point_part_material_queries=1;conveyor.native_contact_point_part_response_vectors=1;conveyor.native_contact_point_part_vector_properties=1;conveyor.native_contact_point_part_conveyor_bindings=1;conveyor.native_contact_point_part_function_calls=1;
check(passes(conveyor),"Qualified conveyor gate rejected");
'''
 for field in ('native_contact_point_part_material_queries','native_contact_point_part_response_vectors','native_contact_point_part_vector_properties','native_contact_point_part_conveyor_bindings','native_contact_point_part_function_calls'):
  extra+='{auto bad=conveyor;bad.'+field+'=0;check(!passes(bad),"Conveyor gate omitted '+field+'");}\n'
 extra+='unsetenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_PART_RESPONSE");unsetenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_CONVEYOR");return 0;}\n'
 source=source.replace(old,'        source+='+repr(extra))
 source=source.replace('positive_gate_controls=2,','positive_gate_controls=4,part_gate_controls=7,conveyor_gate_controls=7,')
 environment=dict(parent.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

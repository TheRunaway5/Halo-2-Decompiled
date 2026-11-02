#!/usr/bin/env python3
"""Compile all20 point counter routes and positive native-reference gate."""
import inspect
import validate_native_contact_point_binding_counters_tu15 as parent
from prepare_native_contact_point_reference_binding_tu15 import FIELDS
def main():
 parent.FIELDS={**parent.FIELDS,**FIELDS};source=inspect.getsource(parent.main)
 source=source.replace('one.size()==16','one.size()==20')
 source=source.replace("negative_values={","negative_values={'native_contact_point_constraint_reference_calls':1,'native_contact_point_constraint_reference_decrements':1,'native_contact_point_constraint_reference_runtime_refusals':1,")
 old='        source+=\'check(rejected==23,"Gate negative coverage differs");return 0;}\\n\''
 assert source.count(old)==1
 extra='''check(rejected==26,"Gate negative coverage differs");
setenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_REFERENCE_RELEASE","1",1);
check(!passes(good),"Positive reference gate admitted no release");
auto positive=good;positive.native_contact_point_effect_requests=1;positive.native_contact_point_constraint_reference_calls=1;positive.native_contact_point_constraint_reference_decrements=1;
check(passes(positive),"Qualified native reference gate rejected");
{auto bad=positive;bad.native_contact_point_constraint_reference_decrements=0;check(!passes(bad),"Reference gate admitted skipped decrement");}
{auto bad=positive;bad.native_contact_point_constraint_reference_runtime_refusals=1;check(!passes(bad),"Reference gate admitted runtime refusal");}
{auto bad=positive;bad.residual_owner_calls[0x22e1b0u]=1;check(!passes(bad),"Reference gate admitted original owner");}
{auto bad=positive;bad.native_contact_point_effect_requests=2;check(!passes(bad),"Reference gate admitted unaccounted effect request");}
unsetenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_REFERENCE_RELEASE");return 0;}
'''
 source=source.replace(old,'        source+='+repr(extra))
 source=source.replace('positive_gate_controls=2,','positive_gate_controls=3,reference_gate_controls=6,')
 environment=dict(parent.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':main()

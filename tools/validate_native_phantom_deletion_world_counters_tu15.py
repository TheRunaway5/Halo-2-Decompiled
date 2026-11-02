#!/usr/bin/env python3
"""Twenty actual added/deletion counter routes and positive deletion gate."""
import inspect
import validate_native_phantom_added_world_counters_tu15 as parent
from prepare_native_phantom_deletion_world_binding_tu15 import FIELDS
def main():
 parent.FIELDS={**parent.FIELDS,**{f:'+'.join('deletion.'+term for term in value.split('+'))for f,value in FIELDS.items()}}
 source=inspect.getsource(parent.main)
 source=source.replace('  for f,expression in FIELDS.items():','  leaf_values={}\n  for f,expression in FIELDS.items():')
 before="sentinel+=11;initializers.append('activity.'+expression+'='+str(sentinel)+'u;');expected[f]=sentinel;"
 assert source.count(before)==1
 after="value=0\n   for term in expression.split('+'):\n    if term not in leaf_values:\n     sentinel+=11;leaf_values[term]=sentinel;initializers.append('activity.'+term+'='+str(sentinel)+'u;')\n    value+=leaf_values[term]\n   expected[f]=value;"
 source=source.replace(before,after).replace("str(sentinel)+'u,\"Wrong aggregate", "str(value)+'u,\"Wrong aggregate")
 source=source.replace('first.size()==8','first.size()==20')
 source=source.replace('#include "halo2/native_tu15_phantom_added_binding.hpp"','#include "halo2/native_tu15_phantom_added_binding.hpp"\n#include "halo2/native_tu15_object_deletion_detaching_binding.hpp"')
 source=source.replace('using namespace halo2;using J=nlohmann::json;','using namespace halo2;using J=nlohmann::json;\nstruct Activity:NativeTu15PhantomAddedBindingActivity{NativeTu15ObjectDeletionDetachingBindingActivity deletion;};')
 source=source.replace('const NativeTu15PhantomAddedBindingActivity&activity','const Activity&activity').replace('int main(){NativeTu15PhantomAddedBindingActivity activity;','int main(){Activity activity;')
 source=source.replace("aggregate=world[start:end];", "aggregate='const auto&deletion_activity=activity.deletion;\\n'+world[start:end];")
 old="end=acceptance.index('\\n  }\\n',start)+len('\\n  }\\n');gate=acceptance[start:end]";assert source.count(old)==1
 source=source.replace(old,"end=acceptance.index('  if(std::getenv(\"HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP\"))',start);gate=acceptance[start:end]")
 source=source.replace("negatives={", "negatives={'native_phantom_object_deletion_attempts':1,'native_phantom_object_deletion_calls':1,'native_phantom_object_deletion_refusals':1,'native_phantom_object_deletion_unbound_services':1,")
 source=source.replace('deletion.native_phantom_added_delete_requests=1;check(passes(deletion)', 'deletion.native_phantom_added_delete_requests=1;deletion.native_phantom_object_deletion_attempts=1;deletion.native_phantom_object_deletion_calls=1;check(passes(deletion)')
 before='unsetenv("HALO2_REQUIRE_NATIVE_GAMEPLAY_PHANTOM_ADDED");check(passes({})'
 after='''setenv("HALO2_REQUIRE_NATIVE_PHANTOM_OBJECT_DELETION","1",1);check(!passes(good),"Deletion gate admitted no nativecalls");
auto complete=good;complete.native_phantom_added_delete_requests=1;complete.native_phantom_object_deletion_attempts=1;complete.native_phantom_object_deletion_calls=1;complete.native_phantom_object_detachment_calls=1;check(passes(complete),"Native deletion positivegate rejected");
'''
 for field in('native_phantom_object_deletion_calls','native_phantom_object_detachment_calls'):
  after+='{auto bad=complete;bad.'+field+'=0;check(!passes(bad),"Deletion gate missed '+field+'");}\n'
 for field in('native_phantom_object_deletion_refusals','native_phantom_object_deletion_unbound_services','native_phantom_object_detachment_refusals'):
  after+='{auto bad=complete;bad.'+field+'=1;check(!passes(bad),"Deletion gate missed '+field+'");}\n'
 for owner in(0xba570,0xc0f60,0xbbd10,0xbbd50):after+='{auto bad=complete;bad.residual_owner_calls['+str(owner)+']=1;check(!passes(bad),"Deletion gate admitted old child");}\n'
 after+='unsetenv("HALO2_REQUIRE_NATIVE_PHANTOM_OBJECT_DELETION");'+before
 # Parent emits this C++ fragment inside a single-quoted Python literal.
 source=source.replace(before,after.replace('\n','\\n'))
 source=source.replace('positive_gate_controls=3,','positive_gate_controls=4,deletion_gate_controls=11,')
 environment=dict(parent.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

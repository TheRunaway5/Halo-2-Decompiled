#!/usr/bin/env python3
"""Compare the guarded V4 accepted-contact helper on declared unequal masses."""
import inspect

import validate_native_accepted_contact_mass_scaling_tu15 as prior

STAGE = prior.ROOT / 'analysis/physics/native-accepted-body-contact-semantic-stage-v4-20261103/staged'


def observer():
    text = prior.observer()
    first = text.index('   NativeTu15PhysicsFrameFp fp{')
    last = text.index('   if(!failed)check(parent.completed==1', first)
    text = text[:first] + '''   NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};
   NativeTu15PairDeletionNativeScope scope;
   NativeTu15AcceptedContactScratch scratch{p+0x100u,p+0x104u,p+0x108u,p+0x10cu,p+0x110u,p+0x114u,p+0x118u,p+0x11cu,{p+0x200u},{p+0x300u,p+0x310u}};
   NativeTu15AcceptedBodyContactBindingActivity binding;auto&parent=binding.listener;auto&activity=binding.publication;
   memory.track=true;bool failed{};
   try{native_tu15_notify_bound_accepted_body_contact(view,typed.at("listener"),p+0x800u,
       {{p,p+4u,p+8u,p+12u},scratch},fp,0,scope,protected_observer,&binding);}catch(const DataFault&){failed=true;}
   memory.track=false;
   check(!failed&&binding.attempts==1&&binding.completed==1&&!binding.refusals&&!binding.role_refusals&&!scope.active(),
       "Bound unequal-mass helper refused or left active scope");
   check(parent.scale_queries==1&&parent.constraint_calls==1&&parent.constraint_updates==1&&
      binding.metadata.calls==1&&binding.metadata.completed==1&&!binding.metadata.refusals&&
      activity.row_initializations==1&&activity.lookup_completions==2,
      "Bound unequal-mass path did not complete selected native child owners");
''' + text[last:]
    return text


def main():
    source = inspect.getsource(prior.main)
    source = prior.replace(source, "'include/halo2/native_tu15_created_constraint_metadata_binding.hpp'];pins",
                           "'include/halo2/native_tu15_created_constraint_metadata_binding.hpp','include/halo2/native_tu15_accepted_body_contact_binding.hpp'];pins")
    source = prior.replace(source, "\\\"halo2/native_tu15_created_constraint_metadata_binding.hpp\\\"\\\\n#include",
                           "\\\"halo2/native_tu15_accepted_body_contact_binding.hpp\\\"\\\\n#include \\\"halo2/native_tu15_created_constraint_metadata_binding.hpp\\\"\\\\n#include")
    source = prior.replace(source, "'tools/validate_native_accepted_contact_mass_scaling_tu15.py');tool_pins",
                           "'tools/validate_native_accepted_contact_mass_scaling_tu15.py','tools/validate_native_accepted_contact_bound_mass_scaling_tu15.py');tool_pins")
    env = dict(prior.__dict__, ACCEPTED=STAGE, observer=observer, __doc__=__doc__)
    exec(source, env)
    env['main']()


if __name__ == '__main__':
    main()

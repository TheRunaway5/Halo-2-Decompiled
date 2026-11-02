#!/usr/bin/env python3
"""The production native-only body-update helper on full original fixtures."""
import inspect
import validate_native_contact_body_update_tu15 as parent
def main():
 text=inspect.getsource(parent.main)
 anchor=" source=source.replace(needle,extra+needle)";assert text.count(anchor)==1
 old='''        NativeTu15ContactBodyUpdateServices body_services;NativeTu15ContactBodyImpulseServices impulse_services;
        impulse_services.activate_body=[&](auto body,auto&current,auto){NativeTu15IslandActivationServices activation;native_tu15_activate_physics_body(memory,body,current,activation,observe);};
        body_services.apply_point_impulse=[&](auto body,auto impulse,auto point,auto&current,auto depth){native_tu15_apply_contact_body_point_impulse(memory,body,impulse,point,current,depth,impulse_services,observe);};
        native_tu15_update_contact_body(memory,c.at("manager"),receiver,bool(args[0]),bool(args[1]),args[2],args[3],{0x200e000u,0x200e010u},fp,0,body_services,observe);
'''
 new='''        NativeTu15ContactBodyUpdateNativeServices native;NativeTu15PairDeletionNativeScope native_scope;NativeTu15ContactBodyUpdateBindingActivity activity;
        native.activate_body=[&](auto body,auto&current,auto){NativeTu15IslandActivationServices activation;native_tu15_activate_physics_body(memory,body,current,activation,observe);};
        native_tu15_update_bound_contact_body(memory,c.at("manager"),receiver,bool(args[0]),bool(args[1]),args[2],args[3],{0x200e000u,0x200e010u},fp,0,native,native_scope,observe,&activity);
        check(activity.attempts==1&&activity.completed==1&&activity.refusals==0&&activity.update.unqualified_services==0&&activity.native_only.refused==0,"Native body binding counters differ");native_scope.reject_unconverted(0xdeadbeefu);
'''
 changes=[('#include "halo2/native_tu15_contact_body_update.hpp"','#include "halo2/native_tu15_contact_body_update_binding.hpp"'),(old,new)]
 transform=''.join('assert driver.count('+repr(a)+')==1;driver=driver.replace('+repr(a)+','+repr(b)+');'for a,b in changes)
 text=text.replace(anchor,' extra+='+repr(transform)+'\n'+anchor)
 environment=dict(parent.__dict__);exec(text,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())

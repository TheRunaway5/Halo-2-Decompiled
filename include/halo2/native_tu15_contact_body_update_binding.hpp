#pragma once
#include "halo2/native_tu15_contact_body_update.hpp"
#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
namespace halo2 {
struct NativeTu15ContactBodyUpdateNativeServices {
    // The World owner supplies its sole qualified body-activation composition.
    // An absent service remains a named315CB0 requirement when selected.
    std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,unsigned)> activate_body;
};
struct NativeTu15ContactBodyUpdateBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15ContactBodyUpdateStats update;
    NativeTu15PairDeletionNativeScopeActivity native_only;
};
inline void native_tu15_update_bound_contact_body(SourcePhysicsContactMemory&memory,
    std::uint32_t manager,std::uint32_t row,bool skip_impulse,bool reversed,
    std::uint32_t component_datum,std::uint32_t body_slot,NativeTu15ContactBodyUpdateScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ContactBodyUpdateNativeServices&native,
    NativeTu15PairDeletionNativeScope&scope,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ContactBodyUpdateBindingActivity*activity=nullptr){
    NativeTu15ContactBodyUpdateBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15PairDeletionNativeScopeGuard guard(scope,&out.native_only);
    NativeTu15ContactBodyImpulseServices impulse;impulse.activate_body=native.activate_body;
    NativeTu15ContactBodyUpdateServices services;
    services.apply_point_impulse=[&](auto body,auto packet,auto point,auto&current,auto current_depth){
        native_tu15_apply_contact_body_point_impulse(memory,body,packet,point,current,
            current_depth,impulse,observe,&out.update);
    };
    try{
        native_tu15_update_contact_body(memory,manager,row,skip_impulse,reversed,
            component_datum,body_slot,scratch,fp,depth,services,observe,&out.update);
        ++out.completed;
    }catch(...){++out.refusals;throw;}
}
}

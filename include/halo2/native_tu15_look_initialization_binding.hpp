#pragma once
#include "halo2/native_tu15_look_initialization.hpp"
#include <stdexcept>
namespace halo2 {
// Only the caller's actual FP fields are bound. No GP/SIMD register file or
// private return frame enters the native constructor interface.
struct NativeTu15LookInitializationFpFields {
    std::uint16_t& control_word;
    std::uint16_t& status;
    std::uint32_t& mxcsr;
    unsigned preserved_x87_depth;
};
struct NativeTu15LookInitializationBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15LookInitializationStats initialization;
};
inline void native_tu15_initialize_bound_look_controller(SourcePhysicsContactMemory&memory,
    std::uint32_t index,std::uint32_t unit,NativeTu15LookInitializationFpFields fields,
    bool observer_read_only,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15LookInitializationBindingActivity*activity=nullptr){
    NativeTu15LookInitializationBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15PhysicsFrameFp fp{fields.control_word,fields.status,fields.mxcsr};
    struct Publish {
        NativeTu15LookInitializationFpFields fields;const NativeTu15PhysicsFrameFp&fp;
        ~Publish(){fields.control_word=fp.control_word;fields.status=fp.status;fields.mxcsr=fp.mxcsr;}
    }publish{fields,fp};
    try{
        if(!observer_read_only)throw std::runtime_error("Native look initialization requires read-only store observation");
        native_tu15_initialize_look_controller(memory,index,unit,fp,fields.preserved_x87_depth,
            observe,&out.initialization);++out.completed;
    }catch(...){++out.refusals;throw;}
}
}

#pragma once
#include "halo2/native_tu15_look_reorientation.hpp"
#include "halo2/native_tu15_look_initialization_binding.hpp"
namespace halo2 {
struct NativeTu15LookReorientationBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15LookReorientationStats reorientation;
};
// Reuse the existing FP-field-only contract. No caller GP/SIMD state or
// volatile return image crosses this native controller boundary.
inline void native_tu15_reorient_bound_look_controller(SourcePhysicsContactMemory&memory,
    std::uint32_t index,std::uint32_t forward,NativeTu15LookInitializationFpFields fields,
    bool observer_read_only,const SourcePhysicsContactRecordObserver&observe = {},
    NativeTu15LookReorientationBindingActivity*activity = nullptr){
    NativeTu15LookReorientationBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15PhysicsFrameFp fp{fields.control_word,fields.status,fields.mxcsr};
    struct Publish {NativeTu15LookInitializationFpFields fields;const NativeTu15PhysicsFrameFp&fp;
        ~Publish(){fields.control_word=fp.control_word;fields.status=fp.status;fields.mxcsr=fp.mxcsr;}
    }publish{fields,fp};
    try{
        if(!observer_read_only)throw std::runtime_error("Native look reorientation requires read-only store observation");
        if(fields.preserved_x87_depth)throw std::runtime_error("Native look reorientation requires its qualified empty-x87 Host domain");
        native_tu15_reorient_look_controller(memory,index,forward,fp,fields.preserved_x87_depth,observe,&out.reorientation);
        ++out.completed;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2

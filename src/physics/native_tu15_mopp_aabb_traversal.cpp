#include "halo2/native_tu15_mopp_aabb_traversal.hpp"
#include "halo2/native_tu15_mopp_aabb_traversal_kernel.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));
        memory_.write(p,b);if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
};
class ValueFrame {
    SourcePhysicsContactMemory&storage_;Memory m_;std::uint32_t cache_,program_;
    NativeTu15MoppAabbVariables input_,local_{};unsigned depth_;
    NativeTu15PhysicsFrameFp&fp_;const NativeTu15MoppAabbTraversalServices&services_;
    const SourcePhysicsContactRecordObserver&observe_;NativeTu15MoppAabbTraversalStats*stats_;
    std::uint32_t saved_low_{},saved_lower_{};
    const NativeTu15MoppAabbVariables&variables(const native_mopp_aabb_detail::Progress&d)const{return d.variables?local_:input_;}
public:
    ValueFrame(SourcePhysicsContactMemory&storage,std::uint32_t cache,std::uint32_t program,
        const NativeTu15MoppAabbVariables&input,unsigned depth,NativeTu15PhysicsFrameFp&fp,
        const NativeTu15MoppAabbTraversalServices&services,const SourcePhysicsContactRecordObserver&observe,
        NativeTu15MoppAabbTraversalStats*stats):storage_(storage),m_(storage,observe),cache_(cache),program_(program),
        input_(input),depth_(depth),fp_(fp),services_(services),observe_(observe),stats_(stats){}
    void begin(native_mopp_aabb_detail::Progress&d){
        if(depth_>512u)throw std::runtime_error("Native MOPP AABB requires its recursion-capacity owner");
        d.cache=cache_;d.program=program_;
    }
    std::uint32_t read(native_mopp_aabb_detail::Read phase,native_mopp_aabb_detail::Progress&d,unsigned axis){
        using R=native_mopp_aabb_detail::Read;
        switch(phase){
        case R::opcode:{const auto value=m_.word(d.program,1);if(stats_)++stats_->opcodes;return value;}
        case R::encoded_byte:return m_.word(d.program+d.byte_offset,1);
        case R::projection_upper:case R::single_upper:case R::offset_upper:case R::slab_upper:return variables(d).upper[axis];
        case R::projection_lower:case R::single_lower:case R::offset_lower:case R::slab_lower:return variables(d).lower[axis];
        case R::scale_translation:return variables(d).translation_and_properties[axis];
        case R::scale_shift:return variables(d).translation_and_properties[4];
        case R::scale_property:return variables(d).translation_and_properties[5];
        case R::scale_key:return variables(d).translation_and_properties[3];
        case R::scale_lower:return m_.word(d.cache+0x20u+axis*4u);
        case R::scale_upper:return m_.word(d.cache+0x10u+axis*4u);
        case R::scale_local_z_lower:case R::scale_local_z_upper:return local_.translation_and_properties[2];
        case R::key_base:return local_.translation_and_properties[3];
        case R::preserved_property:return local_.translation_and_properties[5];
        case R::offset_saved_low_left:case R::offset_saved_low_right:return saved_low_;
        case R::offset_saved_lower_left:case R::offset_saved_lower_right:return saved_lower_;
        case R::wide_upper:return m_.word(d.cache+0x10u+axis*4u);
        case R::wide_lower:return m_.word(d.cache+0x20u+axis*4u);
        case R::leaf_base:return variables(d).translation_and_properties[3];
        case R::leaf_array:return m_.word(d.cache);
        case R::leaf_count:case R::leaf_index:case R::leaf_current:return m_.word(d.array+4u);
        case R::leaf_capacity:return m_.word(d.array+8u);
        case R::leaf_data:return m_.word(d.array);
        }
        throw std::runtime_error("Invalid native MOPP value input phase");
    }
    void write(native_mopp_aabb_detail::Write phase,native_mopp_aabb_detail::Progress&d,std::uint32_t value,unsigned axis){
        using W=native_mopp_aabb_detail::Write;
        switch(phase){
        case W::scale_translation:local_.translation_and_properties[axis]=value;break;
        case W::scale_shift:local_.translation_and_properties[4]=value;break;
        case W::scale_lower:local_.lower[axis]=value;break;
        case W::scale_upper:local_.upper[axis]=value;break;
        case W::scale_property:case W::property:case W::preserved_property:local_.translation_and_properties[5]=value;break;
        case W::scale_key:case W::key_base:local_.translation_and_properties[3]=value;break;
        case W::offset_low:saved_low_=value;break;
        case W::offset_lower:saved_lower_=value;break;
        case W::leaf_key:m_.word(0x309f1bu,d.data+d.count*4u,value);break;
        case W::leaf_count:m_.word(0x309f24u,d.array+4u,value);break;
        }
    }
    void event(native_mopp_aabb_detail::Event phase,native_mopp_aabb_detail::Progress&d){
        using E=native_mopp_aabb_detail::Event;
        if(phase==E::scale_commit)d.variables=1;
        else if(phase==E::key_local||phase==E::property_local){if(!d.variables)local_=input_;d.variables=1;}
        else if(phase==E::property_begin){
            if(d.opcode&3u)throw std::runtime_error("Native MOPP AABB requires its additional property-slot lifetime owner");
        }else if(phase==E::leaf_begin){if(stats_)++stats_->leaves;}
    }
    native_mopp_aabb_detail::Continuation recurse(native_mopp_aabb_detail::Progress&d){
        if(stats_)++stats_->recursive_calls;
        ValueFrame child(storage_,d.cache,d.child_program,variables(d),depth_+1u,fp_,services_,observe_,stats_);
        native_mopp_aabb_detail::Traversal<ValueFrame>(child).run();return{d.cache,d.variables,d.program,d.jump};
    }
    native_mopp_aabb_detail::Continuation grow(native_mopp_aabb_detail::Progress&d){
        NativeTu15ArrayGrowthMemory growth(storage_,{d.array},observe_);
        native_tu15_grow_array(growth,4u,fp_,services_.allocation);if(stats_)++stats_->growths;
        native_mopp_aabb_detail::Continuation resumed;resumed.key=d.key;resumed.array=d.array;return resumed;
    }
    void diagnostic(native_mopp_aabb_detail::Progress&){throw std::runtime_error("Native MOPP AABB requires its invalid-program diagnostic owner");}
};
}
void native_tu15_traverse_mopp_aabb(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t program,const NativeTu15MoppAabbVariables&variables,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15MoppAabbTraversalServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MoppAabbTraversalStats*stats){
    if(stats)++stats->calls;
    try{
        if(depth)throw std::runtime_error("Native MOPP AABB requires its nonempty x87 owner");
        ValueFrame frame(memory,cache,program,variables,0,fp,services,observe,stats);native_mopp_aabb_detail::Traversal<ValueFrame>(frame).run();if(stats)++stats->completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2

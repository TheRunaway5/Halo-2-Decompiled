#pragma once
#include "halo2/native_tu15_marker_owner_kernel.hpp"
#include "halo2/native_tu15_object_parent_mode_value.hpp"
#include <utility>

namespace halo2::native_marker_owner_detail {
template<class Read>struct Value{
    Read read;std::uint32_t object{},objects_global{0x4ecbd0u},tags_global{0x4f0414u},record{};
    void open_record(){
        const auto objects=read(objects_global,4u),rows=read(objects+0x44u,4u);
        record=read(rows+(object&65535u)*12u+8u,4u);
    }
    auto parent(){return read(record+0x14u,4u);}
    bool inherits_marker(){
        const auto tag=read(record,4u),tags=read(tags_global,4u);
        const auto definition=read(tags+(tag&65535u)*16u+8u,4u);
        return(read(definition+2u,1u)&0x10u)!=0u;
    }
    void promote(std::uint32_t parent){object=parent;}
    bool unit_candidate(){return(read(record+4u,1u)&1u)!=0u;}
    bool supported_kind(){return((1u<<(read(record+0xaau,1u)&31u))&0x1cu)!=0u;}
    bool pickup_parent(){
        return native_object_parent_mode_detail::value([&](auto address,auto width){return read(address,width);},object,objects_global);
    }
    std::uint32_t finish(bool inherited){return inherited?read(record+0x154u,4u):object;}
};
template<class Read>std::uint32_t value(Read read,std::uint32_t object,
    std::uint32_t objects_global=0x4ecbd0u,std::uint32_t tags_global=0x4f0414u){
    Value view{std::move(read),object,objects_global,tags_global};return lookup(view);
}
} // namespace halo2::native_marker_owner_detail

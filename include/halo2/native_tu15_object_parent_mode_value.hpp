#pragma once
#include "halo2/native_tu15_object_parent_mode_kernel.hpp"
#include <utility>
namespace halo2::native_object_parent_mode_detail {
template<class Read>struct Value {
    Read read;std::uint32_t global{0x4ecbd0u};
    auto objects(){return read(global,4u);}
    auto rows(std::uint32_t objects){return read(objects+0x44u,4u);}
    auto record(std::uint32_t rows,std::uint32_t index){return read(rows+index*12u+8u,4u);}
    auto flags(std::uint32_t record){return std::uint8_t(read(record+0x12cu,1u));}
    bool finish(std::uint8_t,bool result){return result;}
};
template<class Read>bool value(Read read,std::uint32_t object,std::uint32_t global=0x4ecbd0u){
    Value view{std::move(read),global};return lookup(view,object);
}
} // namespace halo2::native_object_parent_mode_detail

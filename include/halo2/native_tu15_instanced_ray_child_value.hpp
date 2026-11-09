#pragma once
#include "halo2/native_tu15_instanced_ray_child_kernel.hpp"

namespace halo2::native_instanced_ray_child_value_detail {
// Value arguments are already captured by the public API. The retained ABI
// view instead reads its caller key lazily after scenario publication.
template<class Words,class Narrow>struct View {
    Words&memory;Narrow&narrow;std::uint32_t collection,primitive_key,child_data;
    std::uint32_t child()const{return child_data;}
    void test_child(std::uint32_t)const{}
    void null_child()const{}
    std::uint32_t scenario()const{return memory.word(collection+12u);}
    std::uint32_t key()const{return primitive_key;}
    void store(native_instanced_ray_child_detail::Field field,std::uint32_t value,
        const native_instanced_ray_child_detail::Progress&){
        using F=native_instanced_ray_child_detail::Field;
        switch(field){
        case F::scenario:memory.word(0x24decbu,child_data+12u,value);break;
        case F::count:narrow(0x24ded2u,child_data+6u,value,2u);break;
        case F::flags:memory.word(0x24ded8u,child_data+8u,value);break;
        case F::table:memory.word(0x24dedfu,child_data,value);break;
        case F::key:memory.word(0x24dee5u,child_data+16u,value);break;
        }
    }
};
} // namespace halo2::native_instanced_ray_child_value_detail

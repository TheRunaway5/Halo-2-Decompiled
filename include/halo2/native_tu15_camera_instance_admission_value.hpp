#pragma once
#include "halo2/native_tu15_camera_instance_admission_kernel.hpp"

namespace halo2::native_camera_instance_admission_detail {
template<class Memory>struct Values {
    Memory&memory;std::uint32_t instance_value{},definition_value{},mask_value{};
    std::uint32_t instance()const{return instance_value;}
    std::uint32_t definition()const{return definition_value;}
    void index(const Progress&){}
    std::uint32_t read(Read kind,const Progress&d){
        switch(kind){
        case Read::count:return memory.word(d.definition+0x98u);
        case Read::rows:return memory.word(d.definition+0x9cu);
        case Read::last_flags:return memory.word(d.rows+d.count*8u-4u,1u);
        case Read::mask:return mask_value;
        case Read::scenario:return memory.word(0x4ecc18u);
        case Read::cluster_count:return memory.word(d.scenario+0xc4u);
        case Read::cluster_table:return memory.word(d.scenario+0xc8u);
        case Read::table_count:return memory.word(d.table+0x40u);
        case Read::mapping_rows:return memory.word(d.table+0x44u);
        case Read::mapping_entry:return memory.word(d.mapping+d.instance*4u,2u);
        }
        return 0u;
    }
};
} // namespace halo2::native_camera_instance_admission_detail

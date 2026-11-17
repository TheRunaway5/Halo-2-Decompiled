#pragma once
// Reuse only the existing complete ABI/80-bit packet and ordinary backed
// memory test utilities. The inherited-owner algorithm lives in production.
#define main object_parent_mode_unused_test_main
#include "native_tu15_object_parent_mode_tests.cpp"
#undef main
#include "gameplay/weapons/native_tu15_marker_owner.hpp"
#include "halo2/native_tu15_marker_owner_value.hpp"
#include <set>

namespace{
J marker_canonical_projection(J value){
    if(value.is_object())for(auto it=value.begin();it!=value.end();++it){
        if((it.key()=="instruction"||it.key()=="entry")&&it->is_number_unsigned()){
            const auto pc=it->get<std::uint32_t>();if(pc>=0xbacd0u&&pc<=0xbad51u)*it=pc-0x1140u;
        }else *it=marker_canonical_projection(std::move(*it));
    }else if(value.is_array())for(auto&entry:value)entry=marker_canonical_projection(std::move(entry));
    return value;
}
struct OwnerMemory:Memory{
    mutable J writes=J::array();
    std::set<std::uint32_t>write_sites;mutable unsigned write_count{};
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        // Source observer preimages are a separate retained-store protocol;
        // they are not fabricated physical CPU loads or CPU read-fault stops.
        if(!write_sites.contains(pc)){
            packets.push_back(capture());
            if(fault&&fault->at("phase")=="before_read"&&ordinal==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
            ++ordinal;reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});
        }
        std::memcpy(out.data(),at(address,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        packets.push_back(capture());
        if(fault&&fault->at("phase")=="before_write"&&write_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
        const auto before=hex(std::span(at(address,in.size()),in.size()));
        std::memcpy(const_cast<std::byte*>(at(address,in.size())),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});
        ++write_count;if(noisy)hostile();
    }
};
}

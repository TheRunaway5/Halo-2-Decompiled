#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2 {
// Native call-local DATA ownership.30A1E0/30A020 initialize every cache word
// used by309940. Unwritten holes are unprovided, rather than zero-filled or
// borrowed from a CPU/control frame. Every native consumer must use this view.
// This does not qualify raw private-cache preimage observers or address faults.
class NativeTu15MoppAabbPrivateCache final:public SourcePhysicsContactMemory {
    SourcePhysicsContactMemory&memory_;std::uint32_t identity_;
    std::array<std::byte,64>bytes_;std::array<bool,64>defined_{};
    bool touches(std::uint32_t p,std::size_t n)const{
        return std::uint64_t(p)<std::uint64_t(identity_)+64u&&std::uint64_t(identity_)<std::uint64_t(p)+n;
    }
    std::size_t offset(std::uint32_t p,std::size_t n)const{
        if(p<identity_||std::uint64_t(p)+n>std::uint64_t(identity_)+64u)
            throw std::runtime_error("Native MOPP AABB private cache requires its crossing DATA owner");
        return p-identity_;
    }
public:
    NativeTu15MoppAabbPrivateCache(SourcePhysicsContactMemory&memory,std::uint32_t identity)
        :memory_(memory),identity_(identity){
        if((identity&15u)||std::uint64_t(identity)+64u>0x100000000ull)
            throw std::runtime_error("Native MOPP AABB private cache requires aligned bounded DATA identity");
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(!out.empty()&&touches(p,out.size())){
            const auto first=offset(p,out.size());
            if(!std::all_of(defined_.begin()+first,defined_.begin()+first+out.size(),[](bool value){return value;}))
                throw std::runtime_error("Native MOPP AABB requires its unprovided cache DATA owner");
            std::copy_n(bytes_.begin()+first,out.size(),out.begin());return;
        }
        memory_.read(p,out);
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{
        if(touches(p,16)){read(p,out);return;}memory_.read_vector(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte>input)override{
        if(!input.empty()&&touches(p,input.size())){
            const auto first=offset(p,input.size());std::copy(input.begin(),input.end(),bytes_.begin()+first);
            std::fill_n(defined_.begin()+first,input.size(),true);return;
        }
        memory_.write(p,input);
    }
};
} // namespace halo2

#pragma once
#include "halo2/native_tu15_mopp_ray_setup.hpp"
#include <stdexcept>
namespace halo2 {
struct NativeTu15MoppPrivateRayActivity {
    std::uint64_t attempts{},completed{},refusals{},unprovided_reads{},unprovided_writes{};
    NativeTu15MoppRaySetupStats setup;
};
// The24DC90 caller does not define ray.filter_info. Its private30B890 copy
// into cache+40 is not a physics output. It may be omitted only while ALL
// native consumers use this same view and reject any read of either word.
// This is a retained-DATA specialization, never a raw private-stack claim.
class NativeTu15MoppPrivateRayMemory final:public SourcePhysicsContactMemory {
    SourcePhysicsContactMemory&memory_;std::uint32_t input_word_,cache_word_;
    NativeTu15MoppPrivateRayActivity&activity_;
    bool touches(std::uint32_t p,std::size_t n)const{
        const auto end=std::uint64_t(p)+n;
        return (p<std::uint64_t(input_word_)+4u&&std::uint64_t(input_word_)<end)||
            (p<std::uint64_t(cache_word_)+4u&&std::uint64_t(cache_word_)<end);
    }
public:
    NativeTu15MoppPrivateRayMemory(SourcePhysicsContactMemory&memory,std::uint32_t input,
        std::uint32_t cache,NativeTu15MoppPrivateRayActivity&activity):memory_(memory),
        input_word_(input+0x20u),cache_word_(cache+0x40u),activity_(activity){}
    void read(std::uint32_t p,std::span<std::byte>bytes)const override{
        if(!bytes.empty()&&touches(p,bytes.size())){++activity_.unprovided_reads;throw std::runtime_error("Native MOPP ray requires a defined filter-info DATA owner");}
        memory_.read(p,bytes);
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16>bytes)const override{
        if(touches(p,16)){++activity_.unprovided_reads;throw std::runtime_error("Native MOPP ray requires a defined vector/filter-info DATA owner");}
        memory_.read_vector(p,bytes);
    }
    void write(std::uint32_t p,std::span<const std::byte>bytes)override{
        if(!bytes.empty()&&touches(p,bytes.size())){++activity_.unprovided_writes;throw std::runtime_error("Native MOPP ray cannot synthesize unprovided filter-info DATA");}
        memory_.write(p,bytes);
    }
};
struct NativeTu15MoppPrivateRayServices {
    // Implementations must pass THIS view through every filter/shape/cast
    // provider; capturing the unguarded underlying memory is out of admission.
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t cache,std::uint32_t code,
        const NativeTu15MoppTraversalVariables&,const NativeTu15MoppTraversalBounds&,
        NativeTu15PhysicsFrameFp&,unsigned)>traverse;
};
void native_tu15_prepare_private_mopp_ray(SourcePhysicsContactMemory&,
    std::uint32_t cache,std::uint32_t collection,std::uint32_t code,
    std::uint32_t ray_input,std::uint32_t query_context,std::uint32_t collector,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15MoppPrivateRayServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15MoppPrivateRayActivity* = nullptr);
} // namespace halo2

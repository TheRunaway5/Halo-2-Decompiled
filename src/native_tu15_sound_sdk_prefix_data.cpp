#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace halo2 {
namespace {
void cold(SourceRuntimeMemory& memory,std::uint32_t address,std::size_t size) {
    memory.mount(address,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,
        "source sound SDK loader-zeroed constructor destination");
}
template<std::size_t N>void literal(SourceRuntimeMemory& memory,std::uint32_t address,
    const std::array<std::uint8_t,N>& bytes,bool writable=false) {
    if(!writable) {
        // Earlier startup owners may already retain these same kernel import
        // words, in separate regions. Reuse only complete immutable operands
        // with exactly the authenticated bytes; never replace their backing.
        std::size_t covered=0;
        memory.visit_committed([&](auto first,auto present){
            const auto begin=std::max<std::uint64_t>(first,address);
            const auto end=std::min<std::uint64_t>(std::uint64_t(first)+present.size(),std::uint64_t(address)+N);
            if(begin<end)covered+=std::size_t(end-begin);
        });
        if(covered) {
            if(covered!=N)throw std::invalid_argument("Sound SDK immutable operand is only partially owned");
            for(std::size_t i=0;i<N;++i)
                if(!memory.is_read_only(address+std::uint32_t(i),1)||
                   memory.view(address+std::uint32_t(i),1)[0]!=std::byte(bytes[i]))
                    throw std::invalid_argument("Sound SDK existing immutable operand differs in bytes or access");
            return;
        }
    }
    memory.mount(address,std::as_bytes(std::span(bytes)),writable?SourceRuntimeMemory::Access::ReadWrite:
        SourceRuntimeMemory::Access::ReadOnly,"source sound SDK immutable constructor/import operand or explicit configuration input");
}
}
void mount_native_tu15_sound_sdk_prefix_data(SourceRuntimeMemory& memory) {
#include "native_tu15_sound_sdk_prefix_data.inc"
}
}

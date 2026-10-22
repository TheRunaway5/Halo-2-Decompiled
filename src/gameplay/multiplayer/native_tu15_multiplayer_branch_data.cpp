#include "halo2/native_tu15_multiplayer_data.hpp"
#include <array>
#include <stdexcept>
#include <vector>

namespace halo2 {
namespace {
struct BranchData {std::uint32_t address;std::span<const std::byte> bytes;};
#include "gameplay/multiplayer/native_tu15_multiplayer_branch_data.inc"
}
void mount_native_tu15_multiplayer_branch_data(SourceRuntimeMemory& memory) {
    std::vector<BranchData> pending;
    // Validate all existing owners before mounting any uncovered interval.
    // Partial old declarations retain their exact native identity and access.
    for(const auto& row:original_branch_data) {
        std::vector<bool> shared(row.bytes.size());
        for(std::size_t i=0;i<row.bytes.size();++i) {
            try {
                const auto prior=memory.view(row.address+i,1);shared[i]=true;
                if(!memory.is_read_only(row.address+i,1)||prior[0]!=row.bytes[i])
                    throw std::invalid_argument("Original multiplayer branch data conflicts with its existing owner");
            }catch(const std::out_of_range&){}
        }
        for(std::size_t i=0;i<row.bytes.size();) {
            if(shared[i]){++i;continue;}
            const auto first=i++;
            while(i<row.bytes.size()&&!shared[i])++i;
            pending.push_back({row.address+std::uint32_t(first),row.bytes.subspan(first,i-first)});
        }
    }
    for(const auto& row:pending)
        memory.mount(row.address,row.bytes,SourceRuntimeMemory::Access::ReadOnly,
            "original TU1.5 complete multiplayer branch data");
}
}

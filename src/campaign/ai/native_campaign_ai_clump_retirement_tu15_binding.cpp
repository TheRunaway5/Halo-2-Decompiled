#include "halo2/native_campaign_ai_clump_retirement_tu15.hpp"
#include "halo2/native_campaign_ai_clump_callbacks_tu15.hpp"
#include "halo2/native_campaign_ai_clump_retirement_tu15_binding.hpp"
#include <array>
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> entry_calls{};}
bool native_campaign_ai_clump_retirement_tu15_owns_entry(std::uint32_t entry){return entry==0x272ec0u;}
void native_campaign_ai_clump_retirement_tu15_run(SourcePhysicsContactMemory &memory,
        std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
        const SourcePhysicsWorldRuntimeServices &services) {
    if(!native_campaign_ai_clump_retirement_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native AI clump retirement");
    entry_calls.fetch_add(1,std::memory_order_relaxed);
    if(abi.eflags&0x400u)throw std::invalid_argument("Native AI clump retirement needs the engine ABI forward string direction");
    const auto read=[&](std::uint32_t address){std::array<std::byte,4>b{};memory.read(address,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);return v;};
    if(services.instruction)services.instruction(entry);
    const auto clump=read(abi.gp[4]+4u);
    native_campaign_ai_clump_retirement_tu15(memory,clump,[&](const NativeCampaignAiReferenceInvalidationTu15&r){
        native_campaign_ai_clump_reference_invalidation_tu15(memory,r);
    });
    // This void callee consumes one argument. Dead volatile registers and its
    // retired private stack are not re-created by the semantic boundary.
    read(abi.gp[4]);abi.gp[4]+=8u;
}
std::uint64_t native_campaign_ai_clump_retirement_activity_tu15(){return entry_calls.load(std::memory_order_relaxed);}
}

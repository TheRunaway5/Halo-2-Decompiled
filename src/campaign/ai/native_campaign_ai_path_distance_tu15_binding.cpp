#include "halo2/native_campaign_ai_path_distance_tu15_binding.hpp"
#include "halo2/native_campaign_ai_path_distance_tu15.hpp"
#include <atomic>
#include <cstring>

namespace halo2 { namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
void retain_pending(SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPathDistancePendingTu15 &pending) {
    static_assert(sizeof(long double)>=10);
    constexpr unsigned count=3;
    if(abi.x87_depth+count>abi.x87.size())throw std::runtime_error("Path distance pending values exceed caller capacity");
    for(unsigned n=abi.x87_depth;n;--n)std::memcpy(&abi.x87[n-1u+count],&abi.x87[n-1u],sizeof(long double));
    for(unsigned n=0;n<count;++n){std::memset(&abi.x87[n],0,sizeof(long double));std::memcpy(&abi.x87[n],pending.previous_z_dy_dx[n].data(),10);}
    abi.x87_depth+=count;
}
}
bool native_campaign_ai_path_distance_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call) {
    return call.target==0x200c00u && (call.instruction==0x1b8938u || call.instruction==0x1bbf3bu) && !call.tail;
}
void native_campaign_ai_path_distance_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi) {
    if(!native_campaign_ai_path_distance_tu15_owns_call(call))
        throw std::invalid_argument("Native path distance requires its qualified caller");
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>3u)
        throw std::invalid_argument("Native path distance requires retained FP and five free temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {
        SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
    } publication{abi,fp};
    calls.fetch_add(1,std::memory_order_relaxed);
    const auto sp=abi.gp[4],actor=abi.gp[0];
    const auto pool=read(memory,0x5020c8u),rows=read(memory,pool+0x44u),record=rows+(actor&0xffffu)*0x888u;
    // Original200C09 XORPS precedes every actor guard read but follows both
    // pool reads. A rejected lookup keeps the original incoming scalar.
    abi.xmm[0].fill(0);
    std::uint32_t result;
    try {result=native_campaign_ai_actor_path_distance_tu15(memory,record,fp,abi.x87_depth,
        [&](const NativeCampaignAiProjectionCoordinateTu15 &coordinate){abi.xmm[0]={coordinate.bits,0,0,0};});}
    catch(const NativeCampaignAiPathDistancePendingErrorTu15 &error){retain_pending(abi,error.pending());throw;}
    abi.xmm[0]={result,0,0,0};
    // The completed scalar and actual FP status precede the real RET read.
    if(read(memory,sp)!=call.instruction+5u)throw std::runtime_error("Native path distance lost its actual return");
    abi.gp[4]=sp+4u;
}
std::uint64_t native_campaign_ai_path_distance_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}

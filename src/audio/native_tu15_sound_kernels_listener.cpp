// Semantic native listener selection, distance and cluster queries. The
// complete original closure and actual result-width contracts are independently
// qualified by the physical reference in validate_native_campaign_listeners_tu15.py.
#include "audio/native_campaign_audio_listeners_adapter.hpp"
namespace halo2::native_tu15_sound {
namespace {
template<CampaignRoutine Routine> void bridge(M& memory,Abi& abi,FP fp,const O& observer,const Services& services) {
    CampaignContext context{memory,fp,observer,services};Routine(context,abi);
}
// All original nonvolatile GPRs and preserved SIMD registers are owed. The
// two byte queries return AL only; their upper EAX bytes are dead scratch.
constexpr Kernel kernels[]{
    {0x12a810u,bridge<campaign_listeners_adapter::select>,{0xf9,0,0xc1}},
    {0x12a930u,bridge<campaign_listeners_adapter::distance>,{0xf8,0,0xf1}},
    {0x2287b0u,bridge<campaign_listeners_adapter::cut_off>,{0xf8,0,0xf8,0x01}},
    {0x253070u,bridge<campaign_listeners_adapter::scale>,{0xf8,0,0xf9}},
    {0x2531e0u,bridge<campaign_listeners_adapter::byte>,{0xf8,0,0xff,0x01}},
};
}
std::span<const Kernel> listener_kernels(){return kernels;}
} // namespace halo2::native_tu15_sound

#include "halo2/source_sound_sdk_pool.hpp"
#include <stdexcept>
namespace halo2 {
SourceSoundSdkPoolLayout source_sound_sdk_pool_layout(std::uint32_t requested){
    //80012E19..80012E6A: <=FD8 uses (bytes+27h)>>5 allocation units.
    //80013137..8001313E: the consumed header byte gives units*32-8.
    if(requested<=0xfd8u){const auto extent=((requested+0x27u)>>5u)<<5u;return{extent,8u,extent-8u,32u};}
    //8001E5E5..8001E5F9 counts full pages plus a nonzero low12-bit tail;
    // the wrapper's guard-page argument is0.8001DE61..8001DE64 returns
    // that actual page extent in bytes. Oversized native backing stops.
    const auto extent=(std::uint64_t(requested>>12u)+std::uint64_t((requested&0xfffu)!=0))<<12u;
    if(extent>0xffffffffu)throw std::runtime_error("Native SDK pool request exceeds retained32-bit backing");
    return{std::uint32_t(extent),0u,std::uint32_t(extent),4096u};
}
}

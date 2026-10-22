#include "halo2/cache.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/original_data.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/mcc_sound_geometry.hpp"
#include <algorithm>
#include <iostream>
#include <optional>
using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw FormatError(message);}
}
int main(int argc,char** argv)try {
    if(argc!=2||!std::filesystem::is_regular_file(argv[1]))return 77;
    const std::filesystem::path path=argv[1];
    Cache cache(path,path.parent_path(),OriginalData::native_tu15());
    check(cache.is_mcc(),"Sound timing cache regression requires actual MCC content");
    std::optional<MccSoundRuntimePlan> sound;
    const auto canonical=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u,
        [&](MccSoundRuntimePlan&& plan){check(!sound,"Duplicate sound owner");sound.emplace(std::move(plan));});
    check(bool(sound),"Actual native delivery lost its sound plan");
    const auto relocate=[&](unsigned at,std::size_t n){return canonical.runtime_address(at,n);};
    const auto timing=build_mcc_sound_timing_reference(cache,*sound,relocate);
    if(timing.bindings().empty())for(const auto& gap:timing.gaps())std::cerr<<gap.reason<<'\n';
    check(!timing.bindings().empty()&&!timing.stored_references().empty()&&!timing.permutation_references().empty(),
        "Actual complete MCC sound conversion lost its original timing bindings");
    const auto resources=plan_mcc_sound_geometry_resources(cache);
    check(!resources.empty(),"Actual Lockout facial resource regression input disappeared");
    const auto& resource=resources.front();
    const auto owner=std::ranges::find(sound->gestalts,resource.gestalt,&MccSoundRuntimeGestalt::owner);
    check(owner!=sound->gestalts.end(),"Facial resource lost its native gestalt");
    const auto offset=sound->blocks[10].address-sound->address+
        std::size_t(owner->first[10]+resource.index)*0x2cu+8u;
    check(Bytes(sound->bytes).u32(offset)==resource.encoded_offset&&resource.encoded_offset!=resource.source_file_offset,
        "Actual converted facial resource token differs");
    auto broken=*sound;broken.bytes[offset]^=std::byte{1};
    check(build_mcc_sound_timing_reference(cache,broken,relocate).bindings().empty(),
        "Changed facial resource token borrowed timing admission");
    broken=*sound;
    const auto permutation=timing.permutation_references().front().native_permutation-sound->address;
    broken.bytes[permutation+6]^=std::byte{1};
    check(build_mcc_sound_timing_reference(cache,broken,relocate).bindings().empty(),
        "Changed original permutation duration borrowed timing admission");
    std::cout<<"PASS actual MCC complete timing: "<<timing.bindings().size()<<" bindings, "<<resources.size()
        <<" facial resources; altered resource token and original duration rejected\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

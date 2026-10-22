#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_tu15_audio_preference.hpp"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(const auto value:bytes.first(std::min<std::size_t>(bytes.size(),16))) {
        const auto v=std::to_integer<unsigned>(value);result+=digits[v>>4];result+=digits[v&15];
    }
    return result;
}
}
int main(int argc,char **argv) {
    const bool host_output=argc==5&&std::string_view(argv[4])=="native-output";
    if(argc!=4&&!host_output){std::cerr<<"Supply prepared MCC map, owned save directory, bounded report path and optional native-output\n";return 1;}
    nlohmann::json report{{"passed",false},{"runtime_cache_source","prepared MCC only"},
        {"scope","Actual original driver component; weapon admission and playback remain open"},
        {"runtime_cache_path",argv[1]},{"native_world_constructor",true},
        {"source_entry",0x224ec0u},{"original_configuration_argument",0x455b20u},
        {"driver_call_completed",false},{"real_host_output_requested",host_output},
        {"source_results_or_ready_flags_supplied",false},{"full_weapon_parity",false}};
    std::deque<nlohmann::json> stores;std::uint64_t store_count{};
    std::unique_ptr<Cache> cache;
    std::unique_ptr<PhysicsLiveWorldTu15> world;
    std::shared_ptr<SdlSoundPlatform> host_resource;
    const auto inspect=[&](const char *phase) {
        auto &memory=world->memory();
        const auto word=[&](unsigned address){return Bytes(memory.view(address,4)).u32(0);};
        const auto state=word(0x4f2c50u),driver=word(0x52d674u);
        auto &row=report[phase];row["source_sound_system"]=state;row["source_driver"]=driver;
        row["device_initialized"]=std::to_integer<unsigned>(memory.view(state+0x78u,1)[0]);
        row["enabled"]=std::to_integer<unsigned>(memory.view(state+0x79u,1)[0]);
        if(driver)row["driver_ready"]=std::to_integer<unsigned>(memory.view(driver,1)[0]);
    };
    try {
        cache=std::make_unique<Cache>(argv[1]);
        if(!cache->is_mcc())throw FormatError("Sound startup requires prepared MCC content");
        std::filesystem::create_directories(argv[2]);
        world=std::make_unique<PhysicsLiveWorldTu15>(*cache,argv[2]);inspect("before");
        if(host_output) {
            // This is the existing application's actual persisted/default
            // speaker policy and a running host callback resource. Its owner
            // rejects dummy/disk output and publishes only SDK input390BE8.
            // Original386D4A still constructs every SDK field and result.
            host_resource=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{
                SdlSoundPlatformConfiguration::Delivery::DeviceCallback,256,{}});
            const auto configuration=std::filesystem::path(argv[2])/"menu.cfg";
            const auto saved=native_tu15_load_audio_preference(configuration);
            const auto unbound=world->memory().checkpoint();bool absent_rejected=false,foreign_rejected=false;
            try{world->bind_native_sound_output({},configuration);}catch(const std::invalid_argument&){absent_rejected=true;}
            std::thread foreign([&]{try{world->bind_native_sound_output(host_resource,configuration);}catch(const FormatError&){foreign_rejected=true;}});
            foreign.join();
            if(!absent_rejected||!foreign_rejected||world->memory().checkpoint()!=unbound)
                throw FormatError("Native sound resource admission changed memory or accepted an absent/foreign owner");
            world->bind_native_sound_output(host_resource,configuration);
            const auto bound=world->memory().checkpoint();bool duplicate_rejected=false;
            try{world->bind_native_sound_output(host_resource,configuration);}catch(const FormatError&){duplicate_rejected=true;}
            if(!duplicate_rejected||world->memory().checkpoint()!=bound)
                throw FormatError("Native sound resource admission replaced an existing owner");
            report["native_output_guards"]={{"absent_rejected",true},{"foreign_rejected",true},
                {"duplicate_rejected",true},{"rejections_memory_unchanged",true}};
            report["native_output"]={{"device",host_resource->device_id()},
                {"backend",SDL_GetCurrentAudioDriver()},{"channels",host_resource->output_channels()},
                {"configuration",configuration.string()},{"application_default",saved.application_default},
                {"speakers",saved.speakers},{"explicit_sdk_preference_input",Bytes(world->memory().view(0x390be8u,4)).u32(0)}};
        }
        // Actual12804B/128050 supplies455B20 to the complete224EC0 driver.
        // Use this world's real retained stack/TLS/floating-point call frame.
        const std::array arguments{0x455b20u};auto abi=world->prepare_source_call(arguments);
        world->run_actual(0x224ec0u,abi,[&](const auto &store) {
            ++store_count;if(stores.size()==256)stores.pop_front();
            stores.push_back({{"instruction",store.instruction},{"address",store.address},
                {"bytes",store.written.size()},{"before_prefix",hex(store.before)},
                {"written_prefix",hex(store.written)}});
        });
        report["driver_call_completed"]=true;report["return_eax"]=abi.gp[0];inspect("after");
        if(!(abi.gp[0]&255u))throw FormatError("Actual native sound driver initialization returned false");
        report["passed"]=true;
    }catch(const std::exception &error) {
        report["runtime_error"]=error.what();
        if(world)try{inspect("at_failure");}catch(const std::exception &e){report["inspection_error"]=e.what();}
    }
    report["observed_store_count"]=store_count;report["last_stores"]=stores;
    report["store_history_truncated"]=store_count>stores.size();
    std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';
    if(!output){std::cerr<<"Cannot retain bounded actual sound-startup report\n";return 1;}
    std::cout<<report.value("runtime_error",std::string("Actual native sound driver returned"))<<'\n';
    return report["passed"].get<bool>()?0:1;
}

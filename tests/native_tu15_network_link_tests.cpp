#include "halo2/source_game_storage_startup_tu15.hpp"
#include "halo2/source_network_storage_startup_tu15.hpp"
#include "halo2/native_tu15_network_link.hpp"
#include "halo2/native_tu15_network_startup.hpp"
#include "halo2/native_tu15_online_startup.hpp"
#include "halo2/native_tu15_title_address.hpp"
#include "halo2/native_tu15_rng_startup.hpp"
#include "halo2/source_network_session_startup_tu15.hpp"
#include "halo2/native_tu15_network_sockets.hpp"
#include "halo2/native_tu15_profile_file.hpp"
#include "halo2/source_cache_startup_tu15.hpp"
#include "halo2/native_tu15_language.hpp"
#include "halo2/source_input_startup_tu15.hpp"
#include "halo2/native_tu15_input_devices.hpp"
#include "halo2/source_bink_memory_startup_tu15.hpp"
#include "halo2/source_level_handles_startup_tu15.hpp"
#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/physics_renderer_storage_runtime.hpp"
#include "physics_renderer_storage_call_frames.hpp"
#include "halo2/source_startup_state_tu15.hpp"
#include "halo2/source_state_context_startup_tu15.hpp"
#include "halo2/native_tu15_player_startup_data.hpp"
#include "halo2/native_tu15_view_storage_data.hpp"
#include "halo2/source_static_registration_tu15.hpp"
#include "halo2/native_tu15_scalar_state_data.hpp"
#include "halo2/native_tu15_object_storage.hpp"
#include "halo2/source_aux_object_state_tu15.hpp"
#include "halo2/source_renderer_request_tu15.hpp"
#include "halo2/native_tu15_graphics_device.hpp"
#include "halo2/source_renderer_state_tu15.hpp"
#include "halo2/source_renderer_pool_tu15.hpp"
#include "halo2/native_tu15_renderer_palette.hpp"
#include "halo2/native_tu15_renderer_constants.hpp"
#include "halo2/native_tu15_renderer_depth.hpp"
#include "helpers/native_tu15_startup_test.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/online_service.hpp"
#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/native_tu15_arena_bootstrap.hpp"
#include "halo2/physics_game_arena_runtime.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_runtime_image.hpp"
#include <openssl/evp.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <set>
#include <SDL.h>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
void put(SourcePhysicsContactMemory& m,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));m.write(p,bytes);
}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    auto bytes=m.view(p,4);std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
std::string hex(std::span<const std::byte> bytes) {
    std::ostringstream out;for(auto b:bytes)out<<std::hex<<std::setfill('0')<<std::setw(2)<<std::to_integer<unsigned>(b);return out.str();
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,EVP_MAX_MD_SIZE> output{};unsigned size=0;
    check(EVP_Digest(bytes.data(),bytes.size(),output.data(),&size,EVP_sha256(),nullptr)==1,"SHA256 unavailable");return hex(std::as_bytes(std::span(output).first(size)));
}
struct Directory {
    std::filesystem::path path;
    Directory(){
#if defined(__linux__)
        std::string name="/tmp/halo2-native-game-startup-XXXXXX";check(::mkdtemp(name.data()),"Cannot create private startup directory");path=name;
#else
        throw std::runtime_error("Native startup directory unavailable");
#endif
    }
    ~Directory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
enum class IdentitySpace {Actual,Arena,Control,Pool,HeapWorld,Renderer};
struct Scope {
    IdentitySpace& space;IdentitySpace old;
    Scope(IdentitySpace& space,IdentitySpace value):space(space),old(space){space=value;}
    ~Scope(){space=old;}
};
struct MissingSubsystem final : std::exception {};
struct StartupVirtualControllers {
    std::array<SDL_Joystick*,4> joysticks{};std::vector<std::int32_t> instances;
    StartupVirtualControllers() {
        check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,SDL_GetError());
        try {
            for(unsigned i=0;i<4;++i) {
                SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;
                desc.naxes=SDL_CONTROLLER_AXIS_MAX;desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;desc.name="Native startup regression controller";
                const auto device=SDL_JoystickAttachVirtualEx(&desc);check(device>=0,SDL_GetError());
                instances.push_back(SDL_JoystickGetDeviceInstanceID(device));joysticks[i]=SDL_JoystickOpen(device);check(joysticks[i],SDL_GetError());
                for(int axis=0;axis<SDL_CONTROLLER_AXIS_MAX;++axis)check(SDL_JoystickSetVirtualAxis(joysticks[i],axis,axis>=4?-32768:0)==0,SDL_GetError());
            }
            SDL_GameControllerUpdate();
        } catch(...) {close();throw;}
    }
    void close()noexcept {
        for(auto* joystick:joysticks)if(joystick)SDL_JoystickClose(joystick);joysticks={};
        for(auto instance:instances)for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_JoystickGetDeviceInstanceID(i)==instance){SDL_JoystickDetachVirtual(i);break;}
        instances.clear();SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
    ~StartupVirtualControllers(){close();}
};
Json observation(const NativeNetworkLinkObservation& value) {
    return {{"interface_name",value.interface_name},{"administratively_up",value.administratively_up},
        {"carrier",value.carrier?Json(*value.carrier):Json(nullptr)},
        {"speed_mbps",value.speed_mbps?Json(*value.speed_mbps):Json(nullptr)},
        {"duplex",value.duplex==NativeNetworkDuplex::Full?"full":value.duplex==NativeNetworkDuplex::Half?"half":"unknown"}};
}
Json title_observation(const NativeTitleAddressObservation& value) {
    return {{"interface_name",value.interface_name},{"ethernet",value.ethernet},
        {"ipv4",value.ipv4?Json(*value.ipv4):Json(nullptr)},
        {"assignment",!value.assignment?"none":*value.assignment==NativeIpv4Assignment::Dhcp?"dhcp":"static"},
        {"gateway",value.gateway?Json(*value.gateway):Json(nullptr)},{"dns",value.dns},{"acquisition_pending",value.acquisition_pending}};
}
}
int halo2::run_native_tu15_startup_test(int argc,char** argv,RendererDeviceLease renderer_lease,RendererPaletteFactory palette_factory,RendererConstantMode constant_control,RendererDepthMode depth_control)try {
#if !defined(__linux__)
    return 77;
#endif
    std::filesystem::path reference,fixture_path,capture;bool advance_xnet=false,advance_online=false,advance_services=false,advance_full=false,advance_rng=false,advance_session=false,advance_cache=false,advance_input=false,advance_bink=false,advance_handles=false,advance_renderer=false,advance_clock=false,advance_context=false,advance_players=false,advance_views=false,advance_registration=false,advance_scalars=false,advance_objects=false,advance_aux_objects=false,advance_renderer_request=false,advance_graphics_device=false,advance_graphics_state=false,advance_graphics_pool=false,advance_graphics_palette=false,advance_graphics_constants=false,advance_graphics_depth=false;
    std::string input_devices_mode="host";
    for(int i=1;i<argc;i+=2) {
        check(i+1<argc,"Missing game startup test argument");const std::string_view option=argv[i];
        if(option=="--reference-xbe")reference=argv[i+1];else if(option=="--original-fixture")fixture_path=argv[i+1];
        else if(option=="--capture")capture=argv[i+1];
        else if(option=="--input-devices") {
            input_devices_mode=argv[i+1];check(input_devices_mode=="host" || input_devices_mode=="none" || input_devices_mode=="virtual-four","Unknown native input selection");
        }
        else if(option=="--frontier"){
            const auto value=std::string_view(argv[i+1]);check(value=="xnet-startup" || value=="online-startup" || value=="title-address" || (value=="network-complete" || value=="rng-complete" || value=="session-complete" || value=="cache-complete" || value=="input-complete" || value=="bink-memory" || value=="level-handles" || value=="renderer" || value=="clock" || value=="state-context" || value=="players" || value=="view-storage" || value=="static-registration" || value=="scalar-state" || value=="object-storage" || value=="aux-object-state" || value=="renderer-request" || value=="graphics-device" || value=="graphics-state" || value=="graphics-pool" || value=="graphics-palette" || value=="graphics-constants" || value=="graphics-depth"),"Unknown native network frontier");
            advance_xnet=true;advance_online=value!="xnet-startup";advance_graphics_depth=value=="graphics-depth";advance_graphics_constants=value=="graphics-constants" || advance_graphics_depth;advance_graphics_palette=value=="graphics-palette" || advance_graphics_constants;advance_graphics_pool=value=="graphics-pool" || advance_graphics_palette;advance_graphics_state=value=="graphics-state" || advance_graphics_pool;advance_graphics_device=value=="graphics-device" || advance_graphics_state;advance_renderer_request=value=="renderer-request" || advance_graphics_device;advance_aux_objects=value=="aux-object-state" || advance_renderer_request;advance_objects=value=="object-storage" || advance_aux_objects;advance_scalars=value=="scalar-state" || advance_objects;advance_registration=value=="static-registration" || advance_scalars;advance_views=value=="view-storage" || advance_registration;advance_players=value=="players" || advance_views;advance_context=value=="state-context" || advance_players;advance_clock=value=="clock" || advance_context;advance_renderer=value=="renderer" || advance_clock;advance_handles=value=="level-handles" || advance_renderer;advance_bink=value=="bink-memory" || advance_handles;advance_input=value=="input-complete" || advance_bink;advance_cache=value=="cache-complete" || advance_input;advance_session=value=="session-complete" || advance_cache;advance_rng=value=="rng-complete" || advance_session;advance_full=value=="network-complete" || advance_rng;
            advance_services=value=="title-address" || advance_full;
        }
        else throw std::runtime_error("Unknown game startup test argument");
    }
    check(reference.empty()==fixture_path.empty(),"Original startup comparison needs both reference and fixture");
    if(!reference.empty() && (!std::filesystem::is_regular_file(reference) || !std::filesystem::is_regular_file(fixture_path)))return 77;
    SourceRuntimeMemory declarations;mount_native_tu15_game_startup_data(declarations);mount_native_tu15_network_storage_data(declarations);if(advance_rng)mount_native_tu15_rng_startup_data(declarations);if(advance_session)mount_native_tu15_network_session_data(declarations);if(advance_cache)mount_native_tu15_cache_startup_data(declarations);if(advance_input)mount_native_tu15_input_startup_data(declarations);if(advance_bink)mount_native_tu15_bink_memory_startup_data(declarations);if(advance_handles)mount_native_tu15_level_handles_startup_data(declarations);if(advance_renderer)mount_native_tu15_renderer_startup_data(declarations);if(advance_clock)mount_native_tu15_clock_startup_data(declarations);if(advance_context)mount_native_tu15_state_context_startup_data(declarations);if(advance_players)mount_native_tu15_player_startup_data(declarations);if(advance_views)mount_native_tu15_view_storage_data(declarations);if(advance_registration)mount_native_tu15_static_registration_data(declarations);if(advance_scalars)mount_native_tu15_scalar_state_data(declarations);if(advance_objects)mount_native_tu15_object_storage_data(declarations);if(advance_aux_objects)mount_native_tu15_aux_object_state_data(declarations);if(advance_renderer_request)mount_native_tu15_renderer_request_data(declarations);if(advance_graphics_device)mount_native_tu15_graphics_device_data(declarations);if(advance_graphics_state)mount_native_tu15_renderer_state_data(declarations);if(advance_graphics_pool)mount_native_tu15_renderer_pool_data(declarations);if(advance_graphics_palette)mount_native_tu15_renderer_palette_data(declarations);std::size_t bytes=0,regions=0;
    declarations.visit_committed([&](auto,auto b){bytes+=b.size();++regions;});check(advance_session || (bytes==(advance_rng?816u:788u) && regions==(advance_rng?84u:77u)),"Network startup data includes unclassified storage");
    Json fixture;
    if(!reference.empty()) {
        auto original=load_source_runtime_image(reference,SourcePhysicsContactImage::TitleUpdate15);
        declarations.visit_committed([&](auto p,auto b){if(!std::ranges::equal(b,original.view(p,b.size()))) {std::ostringstream why;why<<"Native startup initializer identity differs from TU1.5 at "<<std::hex<<p;throw std::runtime_error(why.str());}});
        std::ifstream input(fixture_path);fixture=Json::parse(input);
        check(fixture.at("schema")== (advance_graphics_depth?"H2_NATIVE_TU15_GRAPHICS_DEPTH_ORIGINAL_V1":advance_graphics_constants?"H2_NATIVE_TU15_GRAPHICS_CONSTANTS_ORIGINAL_V1":advance_graphics_palette?"H2_NATIVE_TU15_GRAPHICS_PALETTE_ORIGINAL_V1":advance_graphics_pool?"H2_NATIVE_TU15_GRAPHICS_POOL_ORIGINAL_V1":advance_graphics_state?"H2_NATIVE_TU15_GRAPHICS_STATE_ORIGINAL_V1":advance_graphics_device?"H2_NATIVE_TU15_GRAPHICS_DEVICE_ORIGINAL_V1":advance_renderer_request?"H2_NATIVE_TU15_RENDERER_REQUEST_ORIGINAL_V1":advance_aux_objects?"H2_NATIVE_TU15_AUX_OBJECT_STATE_STARTUP_ORIGINAL_V1":advance_objects?"H2_NATIVE_TU15_OBJECT_STORAGE_STARTUP_ORIGINAL_V1":advance_scalars?"H2_NATIVE_TU15_SCALAR_STATE_STARTUP_ORIGINAL_V1":advance_registration?"H2_NATIVE_TU15_STATIC_REGISTRATION_STARTUP_ORIGINAL_V1":advance_views?"H2_NATIVE_TU15_VIEW_STORAGE_STARTUP_ORIGINAL_V1":advance_players?"H2_NATIVE_TU15_PLAYER_STARTUP_ORIGINAL_V1":advance_context?"H2_NATIVE_TU15_STATE_CONTEXT_STARTUP_ORIGINAL_V1":advance_clock?"H2_NATIVE_TU15_CLOCK_STARTUP_ORIGINAL_V1":advance_renderer?"H2_NATIVE_TU15_RENDERER_STARTUP_ORIGINAL_V1":advance_handles?"H2_NATIVE_TU15_LEVEL_HANDLES_STARTUP_ORIGINAL_V1":advance_bink?"H2_NATIVE_TU15_BINK_MEMORY_STARTUP_ORIGINAL_V1":advance_input?"H2_NATIVE_TU15_INPUT_STARTUP_ORIGINAL_V1":advance_cache?"H2_NATIVE_TU15_CACHE_STARTUP_ORIGINAL_V1":advance_session?"H2_NATIVE_TU15_NETWORK_SESSION_ORIGINAL_V1":advance_rng?"H2_NATIVE_TU15_RNG_STARTUP_ORIGINAL_V1":advance_full?"H2_NATIVE_TU15_NETWORK_COMPLETE_ORIGINAL_V1":advance_services?"H2_NATIVE_TU15_ONLINE_SERVICES_ORIGINAL_V1":advance_online?"H2_NATIVE_TU15_NETWORK_PLATFORM_ORIGINAL_V1":advance_xnet?"H2_NATIVE_TU15_NETWORK_CONNECTION_ORIGINAL_V1":"H2_NATIVE_TU15_NETWORK_LINK_ORIGINAL_V1") &&
            fixture.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
            fixture.at("world_correspondence_sha256")=="92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc" &&
            fixture.at("pool_correspondence_sha256")=="6d10227d9838a4e6e2e2db443df0c699bbb203a7843a2b6f756a569fd755b527" &&
            fixture.at("instruction_patches").empty() && fixture.at("supplied_gameplay_results").empty(),"Game startup oracle is not unchanged TU1.5 execution");
    }
    Directory directory;NativeTu15ArenaBootstrap startup(directory.path);auto& memory=startup.memory();mount_native_tu15_game_startup_data(memory);mount_native_tu15_network_storage_data(memory);
    std::vector<SourcePhysicsPoolResourceEvent> resources;
    SourcePhysicsPoolHostStorage raw(memory,0x10000000u,[&](const auto& event){resources.push_back(event);});
    NativeTu15NetworkLink link(memory);
    NativeTu15NetworkStartup network(memory);NativeTu15NetworkSockets sockets(memory,network);NativeTu15ProfileFile profile_file(memory,directory.path);
    std::unique_ptr<NativeTu15Language> language;
    if(advance_cache){mount_native_tu15_cache_startup_data(memory);language=std::make_unique<NativeTu15Language>(memory);}
    NativeTu15TitleAddress titles(memory,network);if(advance_session)mount_native_tu15_network_session_data(memory);
    std::unique_ptr<StartupVirtualControllers> input_fixture;
    std::unique_ptr<NativeTu15InputDevices> input_devices;
    if(advance_input) {
        mount_native_tu15_input_startup_data(memory);NativeTu15InputSelection selection;
        if(input_devices_mode=="none")selection.restricted=true;
        if(input_devices_mode=="virtual-four") {input_fixture=std::make_unique<StartupVirtualControllers>();selection={true,input_fixture->instances};}
        input_devices=std::make_unique<NativeTu15InputDevices>(memory,std::move(selection));
    }
    if(advance_bink)mount_native_tu15_bink_memory_startup_data(memory);
    if(advance_handles)mount_native_tu15_level_handles_startup_data(memory);
    if(advance_renderer)mount_native_tu15_renderer_startup_data(memory);
    if(advance_clock)mount_native_tu15_clock_startup_data(memory);
    if(advance_context)mount_native_tu15_state_context_startup_data(memory);
    if(advance_players)mount_native_tu15_player_startup_data(memory);
    if(advance_views)mount_native_tu15_view_storage_data(memory);
    if(advance_registration)mount_native_tu15_static_registration_data(memory);
    if(advance_scalars)mount_native_tu15_scalar_state_data(memory);
    if(advance_objects)mount_native_tu15_object_storage_data(memory);
    if(advance_aux_objects)mount_native_tu15_aux_object_state_data(memory);
    if(advance_renderer_request)mount_native_tu15_renderer_request_data(memory);
    std::unique_ptr<NativeTu15GraphicsDevice> graphics_device;
    std::unique_ptr<NativeTu15RendererPalette> graphics_palette;
    std::unique_ptr<NativeTu15RendererConstants> graphics_constants;
    std::unique_ptr<NativeTu15RendererDepth> graphics_depth;
    if(advance_graphics_state)mount_native_tu15_renderer_state_data(memory);
    if(advance_graphics_pool)mount_native_tu15_renderer_pool_data(memory);
    if(advance_graphics_device) {mount_native_tu15_graphics_device_data(memory);graphics_device=std::make_unique<NativeTu15GraphicsDevice>(memory,std::move(renderer_lease));}
    if(advance_graphics_palette) {mount_native_tu15_renderer_palette_data(memory);graphics_palette=std::make_unique<NativeTu15RendererPalette>(memory,*graphics_device,palette_factory);}
    if(advance_graphics_constants)graphics_constants=std::make_unique<NativeTu15RendererConstants>(memory,*graphics_device,constant_control);
    if(advance_graphics_depth)graphics_depth=std::make_unique<NativeTu15RendererDepth>(memory,*graphics_device,depth_control);
    NativeTu15StorageFormatting storage_formatting(memory);
    std::unique_ptr<NativeTu15RngStartup> rng;
    if(advance_rng) {
        auto clock=native_tu15_rng_clock();
        if(!fixture.is_null()) {
            // Replay measured native platform inputs through the same source
            // instructions. No game seed, rand result or heap bytes are input.
            clock.utc_filetime=[&]{return fixture.at("native_rng_inputs").at("utc_filetime").get<std::uint64_t>();};
            clock.monotonic_milliseconds=[&,index=0u]()mutable{return advance_session?fixture.at("native_millisecond_inputs").at(index++).get<std::uint32_t>():fixture.at("native_rng_inputs").at("monotonic_milliseconds").get<std::uint32_t>();};
        }
        rng=std::make_unique<NativeTu15RngStartup>(startup.process_heap(),std::move(clock));
    }
    std::shared_ptr<NativeTu15OnlineServices> online_services;std::unique_ptr<NativeTu15OnlineStartup> online;
    unsigned browser_requests=0;
    if(advance_services) {
        online_services=std::make_shared<NativeTu15OnlineServices>();
        online_services->login=std::make_shared<DiscordLogin>([&](const std::string&){++browser_requests;return false;});
        online_services->directory=std::make_shared<OnlineDirectory>(OnlineDirectoryOptions{{default_online_origin}});
        OnlineMatchmakingOptions match;match.endpoint=default_online_origin;match.maps={"zanzibar"};
        online_services->matchmaking=std::make_shared<OnlineMatchmaking>(online_services->login,match);
        OnlineAccountOptions account;account.endpoint=default_online_origin;
        online_services->account=std::make_shared<OnlineAccount>(online_services->login,account);
        online=std::make_unique<NativeTu15OnlineStartup>(memory,online_services);
        check(online_services->directory->snapshot().status==OnlineDirectoryStatus::Idle &&
            online_services->matchmaking->snapshot().status==OnlineMatchmakingStatus::Idle &&
            !online_services->account->snapshot().authenticated && !online_services->account->snapshot().busy &&
            online_services->login->snapshot().status==DiscordLoginStatus::SignedOut,
            "Actual desktop service objects were not initialized in their unauthenticated idle state");
        SourcePhysicsWorldRuntimeAbi probe{};probe.gp[4]=0x6000f000u;probe.eflags=2;
        put(memory,probe.gp[4],0x6e0c2u);put(memory,probe.gp[4]+4,probe.gp[4]+0x200);put(memory,probe.gp[4]+0x200,0);
        const auto before=probe.gp;NativeTu15OnlineStartup absent(memory,{});bool no_services=false;
        try{absent.try_call(memory,{0x6e0bdu,0x3aad19u,false},probe);}catch(const std::runtime_error&){no_services=true;}
        SourceRuntimeMemory foreign;bool wrong_bank=false;
        try{online->try_call(foreign,{0x6e0bdu,0x3aad19u,false},probe);}catch(const std::invalid_argument&){wrong_bank=true;}
        check(no_services && wrong_bank && probe.gp==before && !absent.initialized() && !online->initialized(),
            "Native online startup supplied completion without its actual service owners");
    }
    {
        SourceRuntimeMemory foreign;SourcePhysicsWorldRuntimeAbi probe{};probe.gp[4]=0x6000f000u;probe.eflags=2;
        const auto before=probe.gp;bool wrong_bank=false,wrong_site=false,wrong_frame=false;
        try{network.try_call(foreign,{0x6e097u,0x3d7656u,false},probe);}catch(const std::invalid_argument&){wrong_bank=true;}
        try{network.try_call(memory,{0x6e098u,0x3d7656u,false},probe);}catch(const std::invalid_argument&){wrong_site=true;}
        put(memory,probe.gp[4],0x6e09bu);
        try{network.try_call(memory,{0x6e097u,0x3d7656u,false},probe);}catch(const std::invalid_argument&){wrong_frame=true;}
        check(wrong_bank && wrong_site && wrong_frame && probe.gp==before && !network.service() && !network.xnet_references(),
            "Native network startup accepted a foreign caller or initialized a rejected frame");
    }
    // These are explicit API-input cases, not invented live interface results.
    check(native_tu15_network_link_flags({"test",true,true,100u,NativeNetworkDuplex::Full})==11u &&
        native_tu15_network_link_flags({"test",true,true,10u,NativeNetworkDuplex::Half})==21u &&
        native_tu15_network_link_flags({"test",true,true,1000u,NativeNetworkDuplex::Full})==9u &&
        native_tu15_network_link_flags({"test",true,false,100u,NativeNetworkDuplex::Full})==0u &&
        native_tu15_network_link_flags({"test",false,std::nullopt,std::nullopt,NativeNetworkDuplex::Unknown})==0u,
        "Native link observation encoding differs from its XNet ABI");
    bool unknown_carrier=false;
    try{native_tu15_network_link_flags({"test",true,std::nullopt,std::nullopt,NativeNetworkDuplex::Unknown});}catch(const std::invalid_argument&){unknown_carrier=true;}
    check(unknown_carrier,"Unknown native carrier was replaced by an offline result");
    // Refuse the wrong owner/site/frame before any host observation or ABI
    // mutation. These are API ownership checks, separate from live game input.
    {
        SourceRuntimeMemory foreign;
        SourcePhysicsWorldRuntimeAbi probe{};probe.gp[4]=0x6000f000u;probe.eflags=2;
        const auto unchanged=probe.gp;
        bool bad_bank=false,bad_site=false,bad_frame=false;
        try{link.try_call(foreign,{0x6de80u,0x37654du,false},probe);}catch(const std::invalid_argument&){bad_bank=true;}
        try{link.try_call(memory,{0x6de81u,0x37654du,false},probe);}catch(const std::invalid_argument&){bad_site=true;}
        put(memory,probe.gp[4],0x6de84u);
        try{link.try_call(memory,{0x6de80u,0x37654du,false},probe);}catch(const std::invalid_argument&){bad_frame=true;}
        check(bad_bank && bad_site && bad_frame && probe.gp==unchanged && !link.last_observation(),
            "Native link query accepted a foreign caller or mutated a rejected frame");
        check(!link.try_call(memory,{0x6de80u,0x37654eu,false},probe),"Native link owner consumed a foreign API");
    }
    constexpr std::uint32_t sp=0x6000f000u;
    const auto frame=[&]{SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=sp;a.eflags=2;if(advance_rng)a.fs_base=0x60200000u;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(memory,sp,0x60200000u);return a;};
    // An absent owner leaves the real first CALL outstanding, without allocating
    // game options or pretending that the already-constructed arena returned.
    auto missing=frame();bool rejected=false;
    try{source_run_game_storage_startup_tu15(memory,missing,{0x27f,0x1f80},{});}catch(const std::runtime_error&){rejected=true;}
    check(rejected && missing.gp[4]==sp-16 && word(memory,missing.gp[4])==0x13a708u && !word(memory,0x4f3218u),"Missing arena owner did not retain the actual caller frontier");
    IdentitySpace space=IdentitySpace::Actual;std::vector<std::uint32_t> instructions;Json stores=Json::array();
    SourcePhysicsContactRuntimeCall frontier{};std::size_t link_queries=0,noops=0;
    Json native_api_calls=Json::array(),title_calls=Json::array();
    std::uint32_t xnet_input_address=0;std::string xnet_input_bytes;
    std::set<std::uint32_t> pool_actual_instruction_sites;
    if(!fixture.is_null()) {
        const auto& mapping=fixture.at("pool_instruction_map");
        for(const auto& row:mapping.items())pool_actual_instruction_sites.insert(row.value().get<std::uint32_t>());
        for(const auto& row:mapping.items())check(!pool_actual_instruction_sites.contains(std::stoul(row.key(),nullptr,16)),
            "Pool canonical and actual instruction namespaces overlap");
    }
    const auto pc=[&](std::uint32_t value,bool instruction_callback=false) {
        if(fixture.is_null() || space==IdentitySpace::Actual)return value;
        // New pool builds report selected TU1.5 instruction IDs. Older pinned
        // archives report canonical IDs; store observers remain canonical.
        // The authenticated map's two site sets are disjoint, so accepting a
        // known actual instruction never guesses or double-translates a PC.
        if(instruction_callback && space==IdentitySpace::Pool && pool_actual_instruction_sites.contains(value))return value;
        std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<value;
        const auto* map=space==IdentitySpace::Arena?"canonical_instruction_map":space==IdentitySpace::Pool?"pool_instruction_map":space==IdentitySpace::HeapWorld?"world_instruction_map":space==IdentitySpace::Renderer?"renderer_instruction_map":"control_instruction_map";
        return fixture.at(map).at(key.str()).get<std::uint32_t>();
    };
    SourcePhysicsWorldRuntimeServices services;
    Json native_heap_commit_calls=Json::array(),native_graphics_call,native_palette_calls=Json::array(),native_constants_call,native_depth_call;
    std::optional<NativeTu15RendererDeviceFrame> graphics_frame;std::optional<SourcePhysicsWorldRuntimeAbi> graphics_entry;
    const std::uint32_t main_frontier_target=advance_renderer_request?0u:advance_aux_objects?0x12560u:advance_objects?0x10e3b0u:advance_scalars?0xb87f0u:advance_registration?0x1f3830u:advance_views?0x69220u:advance_players?0x1888a0u:advance_context?0x14dfe0u:advance_clock?0x1e7710u:advance_renderer?0x22c100u:
        advance_handles?0x134b00u:advance_bink?0x1a5410u:advance_input?0x1589d0u:advance_cache?0x21e5d0u:
        advance_session?0x21c0a0u:advance_rng?0x6e830u:advance_full?0x148d40u:0u;
    services.instruction=[&](auto instruction){if(rng)rng->instruction(instruction);instructions.push_back(pc(instruction,true));};
    services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer) {
        check(&bank==&memory,"Game startup crossed native bank owners");
        if(main_frontier_target && call.instruction==0x13a766u && call.target==main_frontier_target && !call.tail) {
            frontier=call;throw MissingSubsystem();
        }
        if(advance_renderer_request) {
            if(advance_graphics_depth && call.instruction==0x12669u && call.target==0x3ffdd0u && !call.tail) {
                frontier=call;throw MissingSubsystem();
            }
            if(advance_graphics_depth && call.target==0x3ff5d0u) {
                native_depth_call={{"instruction",call.instruction},{"target",call.target},{"entry_esp",abi.gp[4]},
                    {"entry_gp",abi.gp},{"entry_eflags",abi.eflags},{"argument",word(memory,abi.gp[4]+4)}};
                check(graphics_depth->try_call(bank,call,abi),"Native renderer rejected its depth-mode request");
                const auto config=graphics_depth->configuration();
                native_depth_call["returned_gp"]=abi.gp;native_depth_call["returned_eflags"]=abi.eflags;
                native_depth_call["mode"]=config.mode;native_depth_call["format"]=config.format;
                native_depth_call["depth_bits"]=config.depth_bits;native_depth_call["stencil_bits"]=config.stencil_bits;
                native_depth_call["attachment_handle"]=config.attachment_handle;native_depth_call["state_backing"]=config.state_backing.value;
                return;
            }
            if(advance_graphics_constants && !advance_graphics_depth && call.instruction==0x1265au && call.target==0x3ff5d0u && !call.tail) {
                frontier=call;throw MissingSubsystem();
            }
            if(advance_graphics_constants && call.target==0x400ef0u) {
                native_constants_call={{"instruction",call.instruction},{"target",call.target},{"entry_esp",abi.gp[4]},
                    {"entry_gp",abi.gp},{"entry_eflags",abi.eflags}};
                check(graphics_constants->try_call(bank,call,abi),"Native renderer rejected its constant-mode request");
                const auto config=graphics_constants->configuration();
                native_constants_call["returned_gp"]=abi.gp;native_constants_call["returned_eflags"]=abi.eflags;
                native_constants_call["mode"]=config.mode;native_constants_call["registers"]=config.registers;
                native_constants_call["application_owned"]=config.application_owned;
                native_constants_call["material_bytes"]=config.material_bytes;native_constants_call["extended_bytes"]=config.extended_bytes;
                native_constants_call["material_backing"]=config.material_backing.value;native_constants_call["extended_backing"]=config.extended_backing.value;
                return;
            }
            if(advance_graphics_palette && !advance_graphics_constants && call.instruction==0x12653u && call.target==0x400ef0u && !call.tail) {
                frontier=call;throw MissingSubsystem();
            }
            if(advance_graphics_palette && (call.target==0x1d3d0u || call.target==0x4025b0u)) {
                Json receipt={{"instruction",call.instruction},{"target",call.target},{"entry_esp",abi.gp[4]},
                    {"entry_gp",abi.gp},{"entry_eflags",abi.eflags}};
                receipt["arguments"]=Json::array();for(unsigned i=0;i<(call.target==0x1d3d0u?2u:1u);++i)
                    receipt["arguments"].push_back(word(memory,abi.gp[4]+4+4*i));
                check(graphics_palette->try_call(bank,call,abi),"Native renderer palette rejected its owned source API");
                receipt["returned_gp"]=abi.gp;receipt["returned_eflags"]=abi.eflags;
                receipt["handle"]=graphics_palette->handle();receipt["native_color_address"]=graphics_palette->native_address().value;
                receipt["native_device_address"]=graphics_device->native_address().value;
                receipt["color_sha256"]=digest(graphics_palette->colors());
                native_palette_calls.push_back(receipt);return;
            }
            if(advance_graphics_pool && !advance_graphics_palette && call.instruction==0x125e6u && call.target==0x1d3d0u && !call.tail) {
                frontier=call;throw MissingSubsystem();
            }
            if(advance_graphics_pool && source_renderer_pool_tu15_owns_entry(call.target)) {
                check(call.instruction==0x125d2u && !call.tail && word(memory,abi.gp[4])==0x125d7u,
                    "Renderer pool constructor caller frame differs");
                Scope scope(space,IdentitySpace::Actual);source_run_renderer_pool_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
            if(advance_graphics_pool && call.instruction==0x1c86au && call.target==0x13fc50u) {
                check(!call.tail && word(memory,abi.gp[4])==0x1c86fu,"Renderer generic pool constructor caller frame differs");
                Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
            if(advance_graphics_state && !advance_graphics_pool && call.instruction==0x125d2u && call.target==0x1c7f0u && !call.tail) {
                frontier=call;throw MissingSubsystem();
            }
            if(advance_graphics_state && source_renderer_state_tu15_owns_entry(call.target)) {
                const auto expected=call.target==0x16af0u?0x125c8u:0x125cdu;
                check(!call.tail && call.instruction==expected && word(memory,abi.gp[4])==expected+5 && abi.gp[6]==0x492368u,
                    "Renderer state/table constructor caller frame differs");
                Scope scope(space,IdentitySpace::Actual);source_run_renderer_state_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
            if(advance_graphics_device && !advance_graphics_state && call.instruction==0x125c8u && call.target==0x16af0u && !call.tail) {
                frontier=call;throw MissingSubsystem();
            }
            if(call.instruction==0x129adu && call.target==0x3ff320u && !call.tail) {
                if(!advance_graphics_device) {frontier=call;throw MissingSubsystem();}
                graphics_frame=native_tu15_renderer_device_frame(memory,call,abi);graphics_entry=abi;
                native_graphics_call={{"instruction",call.instruction},{"target",call.target},{"entry_esp",abi.gp[4]},
                    {"output_token",graphics_frame->output_token},{"parameters_token",graphics_frame->parameters_token},
                    {"flags",graphics_frame->flags},{"presentation",graphics_frame->presentation}};
                check(graphics_device->try_call(bank,call,abi),"Native renderer owner rejected its device request");
                native_graphics_call["returned_gp"]=abi.gp;native_graphics_call["handle"]=graphics_device->handle();
                native_graphics_call["native_device_address"]=graphics_device->native_address().value;
                native_graphics_call["native_renderer_live"]=graphics_device->initialized();return;
            }
            if(source_renderer_request_tu15_owns_entry(call.target)) {
                check(call.instruction==0x13a766u && call.target==0x12560u && !call.tail && word(memory,abi.gp[4])==0x13a76cu,
                    "Renderer request producer caller frame differs");
                Scope scope(space,IdentitySpace::Actual);source_run_renderer_request_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
        }
        if(advance_aux_objects && source_aux_object_state_tu15_owns_entry(call.target)) {
            check(call.instruction==0x13a766u && !call.tail && word(memory,abi.gp[4])==0x13a76cu,
                "Auxiliary arena-state constructor caller frame differs");
            Scope scope(space,IdentitySpace::Actual);source_run_aux_object_state_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_objects) {
            if(call.target==0x31d400u && call.instruction==0x10d9fbu) {
                check(!call.tail && word(memory,abi.gp[4])==0x10d9fdu,"Object-type literal RET callback frame differs");
                Scope scope(space,IdentitySpace::Actual);source_run_startup_noop_tu15(bank,abi,{0x27f,0x1f80},services);return;
            }
            if(storage_formatting.try_call(bank,call,abi))return;
            if(call.target==0x11f390u) {
                check((call.instruction==0x1d2ed3u || call.instruction==0x1d2ef2u || call.instruction==0x29ff97u) &&
                    !call.tail && word(memory,abi.gp[4])==call.instruction+5,"Object storage name wrapper caller frame differs");
                Scope scope(space,IdentitySpace::Arena);source_run_physics_game_arena_runtime(bank,0x11e0f0u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
            }
            if(call.target==0x127210u && call.instruction!=0x14dff2u) {
                const bool site=call.instruction==0xd6d81u || call.instruction==0x118401u || call.instruction==0x119101u ||
                    call.instruction==0xc5a81u || call.instruction==0xc1fe3u || call.instruction==0x16e3fbu;
                check(site && !call.tail && word(memory,abi.gp[4])==call.instruction+2 &&
                    abi.gp[1]==word(memory,0x51df2c) && abi.gp[7]==abi.gp[1] && word(memory,abi.gp[7])==0x45f1d8u,
                    "Object storage arena allocator caller frame differs");
                Scope scope(space,IdentitySpace::Arena);source_run_physics_game_arena_runtime(bank,0x125e50u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
            }
        }
        if(advance_registration && source_static_registration_tu15_owns_entry(call.target)) {
            check(call.target==0x69220u && call.instruction==0x13a766u && !call.tail && word(memory,abi.gp[4])==0x13a76cu,
                "Static registration constructor caller frame differs");
            Scope scope(space,IdentitySpace::Actual);
            source_run_static_registration_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_views && call.target==0x1eecf0u) {
            check(call.instruction==0x13a766u && !call.tail && word(memory,abi.gp[4])==0x13a76cu,
                "Action-cache storage constructor caller frame differs");
            Scope scope(space,IdentitySpace::Actual);
            source_run_combat_action_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_players && call.instruction==0x14dff2u && call.target==0x127210u) {
            check(!call.tail && word(memory,abi.gp[4])==0x14dff4u && word(memory,abi.gp[4]+4)==0x2210u &&
                abi.gp[7]==word(memory,0x51df2c) && word(memory,abi.gp[7])==0x45f1d8u,"Player storage arena allocator frame differs");
            Scope scope(space,IdentitySpace::Arena);source_run_physics_game_arena_runtime(bank,0x125e50u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_context && source_state_context_startup_tu15_owns_entry(call.target)) {
            Scope scope(space,IdentitySpace::Actual);source_run_state_context_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_clock && source_startup_state_tu15_owns_entry(call.target)) {
            Scope scope(space,IdentitySpace::Actual);source_run_startup_state_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_renderer) {
            if(call.target==0x134b00u && call.instruction==0x13a766u && !call.tail) {
                Scope scope(space,IdentitySpace::Renderer);source_run_physics_renderer_storage_runtime(bank,0x133760u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
            }
            if(call.target==0x2ddeb9u || call.target==0x2dd7c9u) {
                Scope scope(space,IdentitySpace::Actual);
                if(startup.process_heap().try_crt_growth_call(bank,call,abi,{0x27f,0x1f80},services,observer,[&](const auto& event) {
                    native_heap_commit_calls.push_back({{"instruction",event.instruction},{"base_cell",event.base_cell},{"size_cell",event.size_cell},
                        {"request",{event.request.base,event.request.zero_bits,event.request.bytes,event.request.allocation_type,event.request.protection}},
                        {"result",{event.result.status,event.result.base,event.result.bytes}}});
                }))return;
            }
            for(const auto& frame:physics_renderer_storage_frames::frames)if(frame.site==call.instruction && frame.target==call.target) {
                check(!call.tail && word(memory,abi.gp[4])==frame.return_address,"Renderer storage caller frame differs");
                using Module=physics_renderer_storage_frames::Module;
                if(frame.module==Module::Heap) {
                    Scope scope(space,IdentitySpace::HeapWorld);
                    check(startup.process_heap().try_renderer_allocation_call(bank,call,abi,{0x27f,0x1f80},services,observer),"Renderer heap owner rejected an authenticated call");return;
                }
                if(frame.module==Module::Pool) {
                    Scope scope(space,IdentitySpace::Pool);source_run_physics_pool_startup_runtime(bank,frame.entry,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
                }
            }
        }
        if(advance_handles && source_level_handles_startup_tu15_owns_entry(call.target)) {
            Scope scope(space,IdentitySpace::Actual);source_run_level_handles_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_bink && source_bink_memory_startup_tu15_owns_entry(call.target)) {
            Scope scope(space,IdentitySpace::Actual);source_run_bink_memory_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_input) {
            if(input_devices->try_call(bank,call,abi))return;
            if(source_input_startup_tu15_owns_entry(call.target)) {
                Scope scope(space,IdentitySpace::Actual);source_run_input_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
        }
        if(advance_cache) {
            if(language->try_call(bank,call,abi))return;
            if(source_cache_startup_tu15_owns_entry(call.target)) {
                Scope scope(space,IdentitySpace::Actual);source_run_cache_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
        }
        if(advance_session && (sockets.try_call(bank,call,abi) || profile_file.try_call(bank,call,abi)))return;
        if(advance_session && source_network_session_startup_tu15_owns_entry(call.target)) {
            Scope scope(space,IdentitySpace::Actual);source_run_network_session_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_session && source_physics_live_support_tu15_owns_entry(call.target)) {
            Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_rng) {
            if(rng->try_call(bank,call,abi))return;
            if(call.target==0x2db419u || call.target==0x2de70eu) {
                Scope scope(space,IdentitySpace::HeapWorld);
                if(startup.process_heap().try_crt_allocation_call(bank,call,abi,{0x27f,0x1f80},services,observer))return;
            }
            if(call.instruction==0x13a766u && call.target==0x148d40u && !call.tail) {
                Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
        }
        if(raw.try_call(bank,call,abi))return;
        if(link.try_call(bank,call,abi)){++link_queries;return;}
        if(advance_online) {
            const auto before_sp=abi.gp[4];
            if(network.try_call(bank,call,abi,observer)) {
                if(call.target==0x3d7656u){xnet_input_address=word(memory,before_sp+4);xnet_input_bytes=hex(memory.view(xnet_input_address,12));}
                native_api_calls.push_back({{"instruction",call.instruction},{"target",call.target},
                    {"return_eax",abi.gp[0]},{"entry_esp",before_sp},{"return_esp",abi.gp[4]}});return;
            }
        }
        if(advance_services && online->try_call(bank,call,abi)) {
            native_api_calls.push_back({{"instruction",call.instruction},{"target",call.target},
                {"return_eax",abi.gp[0]},{"entry_esp",abi.gp[4]-8},{"return_esp",abi.gp[4]}});return;
        }
        if(advance_full) {
            const auto before=abi.gp[4];
            if(titles.try_call(bank,call,abi,observer)) {
                const auto& value=*titles.last_observation();
                title_calls.push_back({{"instruction",call.instruction},{"entry_esp",before},{"return_esp",abi.gp[4]},
                    {"output",word(memory,before+4)},{"return_eax",abi.gp[0]},
                    {"observation",title_observation(value)},{"bytes",hex(native_tu15_title_address_bytes(value))}});return;
            }
            if(call.instruction==0x6e0d2u && call.target==0x707d0u && !call.tail) {
                Scope scope(space,IdentitySpace::Actual);source_run_network_availability_startup_tu15(bank,abi,{0x27f,0x1f80},services,observer);return;
            }
            if(call.instruction==0x6e0ebu && call.target==0x16e600u && !call.tail) {
                Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
            }
            if(call.instruction==0x13a766u && call.target==0x31d400u && !call.tail) {
                ++noops;Scope scope(space,IdentitySpace::Actual);source_run_startup_noop_tu15(bank,abi,{0x27f,0x1f80},services);return;
            }
        }
        if((call.instruction==0x13a703u || (advance_clock && call.instruction==0x13a766u)) && call.target==0x126640u && !call.tail) {
            Scope scope(space,IdentitySpace::Arena);
            source_run_physics_game_arena_runtime(bank,0x125280u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
        }
        if((call.instruction==0x13a736u || call.instruction==0x148d6fu) && call.target==0x166a80u && !call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,0x166a80u,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x13a75cu && call.target==0x32b3d6u && !call.tail) {
            Scope scope(space,IdentitySpace::Control);source_run_physics_world_runtime(bank,0x327866u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x13a766u && call.target==0x6de60u && !call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_network_storage_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if((call.instruction==0x7ecadu || call.instruction==0x6cab1u ||
            (advance_handles && (call.instruction==0x1a5422u || call.instruction==0x1a545bu)) ||
            (advance_graphics_pool && call.instruction==0x1c84cu)) && call.target==0x11f3f0u && !call.tail) {
            Scope scope(space,IdentitySpace::Pool);source_run_physics_pool_startup_runtime(bank,0x11e150u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observer);return;
        }
        if(call.instruction==0x7ecc9u && call.target==0x16e450u && !call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_physics_live_support_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_xnet && call.instruction==0x6de97u && call.target==0x6e010u && call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_network_connection_startup_tu15(bank,abi,{0x27f,0x1f80},services,observer);return;
        }
        if(advance_services && call.instruction==0x6e0cdu && call.target==0x70ce0u && !call.tail) {
            Scope scope(space,IdentitySpace::Actual);source_run_network_title_address_startup_tu15(bank,abi,{0x27f,0x1f80},services,observer);return;
        }
        if((advance_context && call.target==0x14dfe0u) ||
            (advance_clock && call.target==0x1e7710u) ||
            (advance_renderer && call.target==0x22c100u) ||
            (advance_handles && call.target==0x134b00u) ||
            (advance_bink && call.target==0x1a5410u) ||
            (advance_input && call.target==0x1589d0u) ||
            (advance_cache && !advance_input && call.target==0x21e5d0u) ||
            (advance_session && call.target==0x21c0a0u) ||
            (call.instruction==0x13a766u && call.target==0x6e830u && !call.tail) ||
            (call.instruction==0x13a766u && call.target==0x148d40u && !call.tail) ||
            (call.instruction==0x70d0cu && call.target==0x3d725du && !call.tail) ||
            (call.instruction==0x6e0bdu && call.target==0x3aad19u && !call.tail) ||
            (call.instruction==0x6e097u && call.target==0x3d7656u && !call.tail) ||
            (call.instruction==0x6de97u && call.target==0x6e010u && call.tail) ||
            (call.instruction==0x13a766u && call.target==0x31d400u && !call.tail)){frontier=call;throw MissingSubsystem();}
        std::ostringstream why;why<<"Unexpected game startup dependency at "<<std::hex<<call.instruction<<" -> "<<call.target;throw std::runtime_error(why.str());
    };
    const auto observe=[&](const SourcePhysicsContactRecordStore& store) {
        if(store.address>=0x60000000u && store.address<0x60100000u)return;
        stores.push_back({{"instruction",pc(store.instruction)},{"address",store.address},{"before",hex(store.before)},{"written",hex(store.written)}});
    };
    Json input_prelude;
    if(advance_input) {
        auto inputs=frame();source_run_input_startup_tu15(memory,0x1273c0u,inputs,{0x27f,0x1f80},services,observe);
        Json xmm=Json::array();for(const auto& value:inputs.xmm)xmm.push_back(hex(std::as_bytes(std::span(value))));
        input_prelude={{"gp",inputs.gp},{"eflags",inputs.eflags},{"xmm",xmm},{"x87_control",inputs.floating_point->x87_control_word},
            {"x87_status",inputs.x87_status},{"mxcsr",inputs.floating_point->mxcsr},{"instructions",instructions},{"stores",stores}};
        check(inputs.gp[4]==sp+4 && memory.view(0x4f2a88,1)[0]==std::byte{1},"Native input producer did not complete before game startup");
        if(!fixture.is_null()) {
            auto original=fixture.at("source_input_prelude"),actual=input_prelude;
            check(!((actual.at("eflags").get<unsigned>()^original.at("eflags").get<unsigned>())&0xcc5u),"Input prelude original flags differ");
            actual.erase("eflags");original.erase("eflags");
            for(const auto& row:actual.items())if(row.value()!=original.at(row.key())) {
                std::string detail=row.key();
                if(row.value().is_array() && original.at(row.key()).is_array()) {
                    const auto& expected=original.at(row.key());const auto count=std::min(row.value().size(),expected.size());
                    for(std::size_t i=0;i<count;++i)if(row.value()[i]!=expected[i]) {detail+=" at "+std::to_string(i)+" actual "+row.value()[i].dump()+" expected "+expected[i].dump();break;}
                    detail+=" sizes "+std::to_string(row.value().size())+"/"+std::to_string(expected.size());
                } else detail+=" actual "+row.value().dump()+" expected "+original.at(row.key()).dump();
                throw std::runtime_error("Input producer prelude differs from unchanged original execution: "+detail);
            }
        }
        instructions.clear();stores=Json::array();
    }
    auto state=frame();bool stopped=false;
    try{source_run_game_storage_startup_tu15(memory,state,{0x27f,0x1f80},services,observe);}catch(const MissingSubsystem&){stopped=true;}catch(const std::exception& error) {
        std::ostringstream why;why<<error.what()<<"; last source sites";
        for(auto i=instructions.size()>12?instructions.size()-12:0;i<instructions.size();++i)why<<' '<<std::hex<<instructions[i];
        why<<"; GP";for(auto r:state.gp)why<<' '<<std::hex<<r;throw std::runtime_error(why.str());
    }
    check(link.last_observation().has_value(),"Startup did not query actual native link state");
    const auto host=observation(*link.last_observation());const auto flags=native_tu15_network_link_flags(*link.last_observation());
    if(advance_xnet && !(flags&1u)) {std::cout<<"Active carrier is required for the native XNet-startup frontier comparison\n";return 77;}
    check(stopped && frontier.target==(advance_graphics_depth?0x3ffdd0u:advance_graphics_constants?0x3ff5d0u:advance_graphics_palette?0x400ef0u:advance_graphics_pool?0x1d3d0u:advance_graphics_state?0x1c7f0u:advance_graphics_device?0x16af0u:advance_renderer_request?0x3ff320u:advance_aux_objects?0x12560u:advance_objects?0x10e3b0u:advance_scalars?0xb87f0u:advance_registration?0x1f3830u:advance_views?0x69220u:advance_players?0x1888a0u:advance_context?0x14dfe0u:advance_clock?0x1e7710u:advance_renderer?0x22c100u:advance_handles?0x134b00u:advance_bink?0x1a5410u:advance_input?0x1589d0u:advance_cache?0x21e5d0u:advance_session?0x21c0a0u:advance_rng?0x6e830u:advance_full?0x148d40u:advance_services?0x3d725du:advance_online?0x3aad19u:advance_xnet?0x3d7656u:((flags&1u)?0x6e010u:0x31d400u)) && state.gp[6]==(advance_graphics_device?0x492368u:advance_aux_objects?0x45cu:advance_objects?0x3f0u:advance_scalars?0x3ccu:advance_registration?0x384u:advance_views?0x33cu:advance_players?0x2d0u:advance_context?0x2acu:advance_clock?0x288u:advance_renderer?0x21cu:advance_handles?0x1f8u:advance_bink?0x1d4u:advance_input?0x18cu:advance_cache?0x168u:advance_session?0x144u:advance_rng?0x90u:advance_full?0x6cu:(flags&1u)?0u:0x24u) &&
        state.gp[4]==sp-(advance_graphics_depth?0x28u:advance_graphics_constants?0x2cu:advance_graphics_palette?0x28u:advance_graphics_pool?0x30u:advance_graphics_device?0x28u:advance_renderer_request?0x80u:advance_full?16u:advance_services?0x1f0u:advance_xnet?0x1c8u:16u) && word(memory,state.gp[4])==(advance_graphics_depth?0x1266eu:advance_graphics_constants?0x1265fu:advance_graphics_palette?0x12658u:advance_graphics_pool?0x125ebu:advance_graphics_state?0x125d7u:advance_graphics_device?0x125cdu:advance_renderer_request?0x129b2u:advance_full?0x13a76cu:advance_services?0x70d11u:advance_online?0x6e0c2u:advance_xnet?0x6e09cu:0x13a76cu) && word(memory,0x56e1ccu)==flags,
        "Native link result did not select the original next initialization path");
    std::uint32_t parameters=0;std::string parameter_bytes;
    if(advance_xnet && !advance_full) {
        parameters=sp-0x1b8u;parameter_bytes=hex(memory.view(parameters,12));
        check(word(memory,state.gp[4]+4)==(advance_services?sp-0x1e8u:advance_online?parameters+0xc:parameters) &&
            state.gp[1]==(advance_services?sp-0x1e8u:parameters) && state.gp[5]==sp-20 &&
            word(memory,sp-16)==0x13a76cu && parameter_bytes=="0c000c0804081808080828ff" &&
            (!advance_online?std::ranges::all_of(memory.view(parameters+0x10,400),[](auto b){return b==std::byte{};}):
                (word(memory,parameters+0x10)==0x02020002u && word(memory,parameters+0xc)==0)) &&
            memory.view(0x4dc061u,1)[0]==std::byte(advance_services?1:0) && memory.view(0x4dc534u,1)[0]==std::byte{} && link_queries==2,
            "Native connection startup parameter/WSADATA initialization or pending XNet frame differs");
        if(advance_online)check(network.service() && network.xnet_references()==1 && network.winsock_references()==1 &&
            network.service()->private_pool_address().value>0xffffffffu && network.service()->configuration().max_sockets()==24,
            "Native platform APIs lack their real network context or widened pool owner");
        if(advance_services)check(online->initialized() && online->references()==1 && online->native_address().value>0xffffffffu &&
            !browser_requests && online_services->login->snapshot().status==DiscordLoginStatus::SignedOut &&
            !online_services->account->snapshot().authenticated && online_services->directory->snapshot().status==OnlineDirectoryStatus::Idle &&
            online_services->matchmaking->snapshot().status==OnlineMatchmakingStatus::Idle &&
            std::ranges::all_of(memory.view(sp-0x1e8u,36),[](auto b){return b==std::byte{};}),
            "Native online library startup authenticated, connected, or lost its owned context/title-address frame");
    } else if(!advance_full)check(link_queries==1,"Native link stage queried an unexpected API");
    if(advance_full) {
        parameters=xnet_input_address;parameter_bytes=xnet_input_bytes;
        check(online->initialized() && online->references()==1 && online->native_address().value>0xffffffffu &&
            network.service() && network.xnet_references()==1 && network.winsock_references()==1 &&
            title_calls.size()==2 && noops==(advance_scalars?9u:advance_registration?8u:advance_bink?7u:advance_session?6u:2u) && link_queries==2 &&
            memory.view(0x4dc061u,1)[0]==std::byte{1} && memory.view(0x4dc534u,1)[0]==std::byte{1},
            "Complete cold network startup lost platform owners, actual address calls, pool activation or callback ordering");
        const auto first=title_calls[0].at("bytes").get<std::string>(),last=title_calls[1].at("bytes").get<std::string>();
        check(hex(memory.view(0x4dc42cu,6))==first.substr(20,12) && hex(memory.view(0x4dc3f3u,36))==last &&
            memory.view(0x4dc3f1u,1)[0]==std::byte{1} && memory.view(0x4dc3f2u,1)[0]==std::byte{1},
            "Source network address publication differs from its actual host API inputs");
        check(!browser_requests && online_services->login->snapshot().status==DiscordLoginStatus::SignedOut &&
            !online_services->account->snapshot().authenticated && online_services->directory->snapshot().status==OnlineDirectoryStatus::Idle &&
            online_services->matchmaking->snapshot().status==OnlineMatchmakingStatus::Idle,
            "Native network startup changed actual online authentication or discovery state");
        auto rejected=state;rejected.gp[4]=sp-0x800u;const auto before=rejected.gp;
        put(memory,rejected.gp[4],0x70d11u);put(memory,rejected.gp[4]+4,sp+0x900u);
        bool wrong_output=false;try{titles.try_call(memory,{0x70d0cu,0x3d725du,false},rejected);}catch(const std::invalid_argument&){wrong_output=true;}
        SourceRuntimeMemory foreign;bool wrong_bank=false;
        try{titles.try_call(foreign,{0x70d0cu,0x3d725du,false},rejected);}catch(const std::invalid_argument&){wrong_bank=true;}
        check(wrong_output && wrong_bank && rejected.gp==before,"Native title-address query consumed a foreign bank or output frame");
    }
    const auto options=word(memory,0x4f3218u);const auto address=memory.native_address(options,0x1200);
    check(address.value>0xffffffffu && word(memory,0x4f2954u)==(advance_aux_objects?1518752u:advance_objects?1510384u:advance_scalars?24256u:advance_views?24228u:advance_players?18652u:advance_context?9628u:advance_clock?9400u:advance_rng?9364u:9356u) && (advance_rng || word(memory,0x4f295cu)==2408784992u),
        "Native game-options ownership, cursor or source CRC differs");
    check(digest(memory.view(options,0x1200))=="29f46cc48679c4bdb125ee2f8a32451e056532af46c4b0e3e32b46e95473c30a", "Source game-options clearing/selected-BSP state differs");
    Json rng_inputs;
    if(advance_rng) {
        check(rng->utc_filetime_input().has_value() && rng->monotonic_millisecond_input().has_value(),"Source RNG did not consume real native clock inputs");
        check(rng->rng_seed_millisecond_input().has_value(),"Source RNG seed has no owned clock input");
        const auto utc=*rng->utc_filetime_input();const auto tick=*rng->rng_seed_millisecond_input();
        const auto seed=std::uint32_t((utc-116444736000000000ull)/10000000ull)^41u^tick;
        const auto bank=word(memory,0x4f3cd8u),crt=word(memory,0x573840u);
        check(bank==options+0x1200u && word(memory,bank)==0x78a8u && (advance_session || word(memory,bank+4u)==seed) &&
            memory.native_address(bank,8).value>0xffffffffu && memory.native_address(crt,0x88).value>0xffffffffu &&
            word(memory,crt)==rng->thread_token() && word(memory,crt+4)==0xffffffffu &&
            word(memory,crt+0x14u)==0x29e2c0u && word(memory,crt+0x54u)==0x46ecd8u &&
            word(memory,0x60200000u)==0xffffffffu,
            "Original RNG/CRT state, native addresses or restored exception chain differs");
        rng_inputs={{"utc_filetime",utc},{"monotonic_milliseconds",tick},{"thread_token",rng->thread_token()}};
        if(!fixture.is_null()) {
            check(fixture.at("native_rng_inputs")==rng_inputs && fixture.at("arena_offset")==word(memory,0x4f2954u) &&
                fixture.at("arena_checksum")==word(memory,0x4f295cu),"Original RNG inputs or arena allocation/CRC differ");
            for(const auto& row:fixture.at("rng_fields"))check(row.at("sha256").get<std::string>()==digest(memory.view(
                row.at("address").get<std::uint32_t>(),row.at("bytes").get<std::size_t>())),"Original heap/RNG/CRT bank differs");
        }
    }
    const auto qos=word(memory,0x4dc538u);const auto native_qos=memory.native_address(qos,0x150);
    check(qos==0x10000000u && native_qos.value>0xffffffffu && raw.live_allocations()==(advance_graphics_pool?10u:advance_renderer?9u:advance_handles?8u:advance_session?6u:1u) && resources.size()==(advance_graphics_pool?10u:advance_renderer?9u:advance_handles?8u:advance_session?6u:1u) && resources[0].allocation &&
        resources[0].request==std::array<std::uint32_t,5>{0,0,0x150,0x101000,4} && resources[0].result==std::array<std::uint32_t,3>{0,0x10000000u,0x1000u},
        "Network QoS pool lacks its actual native allocation outcome");
    check(word(memory,qos+0x20u)==0x20 && word(memory,qos+0x24u)==8 && word(memory,0x4dc064u)==(advance_session?3u:1u),
        "Network source pool dimensions or callback registration differ");
    check(instructions.size()>1741 && stores.size()>=1322,"Native link did not advance original startup");
    // Native x87 stores reserved CW bit 6 as one; the original-code emulator
    // retains the zero requested by FLDCW. Compare every other control bit and
    // report both raw words, without altering the original instruction stream.
    check((state.floating_point->x87_control_word&~0x40u)==0x23fu && (advance_session?(state.floating_point->mxcsr&~0x3fu)==0x1f80u:state.floating_point->mxcsr==0x1f80u) &&
        !state.x87_depth && !state.x87_status && !state.active_mmx && (advance_session || state.xmm==decltype(state.xmm){}),
        "Native game startup changed defined floating-point state");
    if(!fixture.is_null()) {
        const auto& abi=fixture.at("game_frontier");
        check(fixture.at("host_link_observation")==host && fixture.at("native_link_flags")==flags,
            "Host network input changed; regenerate the offline reference for its actual observation");
        if(advance_full) {
            check(fixture.at("native_title_address_calls")==title_calls,
                "Host title-address input changed; regenerate its actual-input reference");
            check(fixture.at("title_address_flag_cases").size()==24,"Incomplete original LAN title-address flag matrix");
            for(const auto& row:fixture.at("title_address_flag_cases")) {
                NativeTitleAddressObservation input;input.acquisition_pending=row.at("pending").get<bool>();
                const auto assignment=row.at("assignment").get<std::string>();
                if(assignment!="none") {
                    input.ipv4=std::array<std::uint8_t,4>{192,0,2,1};
                    input.assignment=assignment=="dhcp"?NativeIpv4Assignment::Dhcp:NativeIpv4Assignment::Static;
                }
                if(row.at("gateway").get<bool>())input.gateway=std::array<std::uint8_t,4>{192,0,2,2};
                if(row.at("dns").get<bool>())input.dns.push_back("192.0.2.3");
                check(native_tu15_title_address_flags(input)==row.at("flags"),"Native LAN title flags differ from unchanged original scalar decoder");
            }
        }
        check(abi.at("gp").get<decltype(state.gp)>()==state.gp && (abi.at("eflags").get<std::uint32_t>()&0xcd5u)==(state.eflags&0xcd5u),"Original game startup frontier GP/defined flags differ");
        check(fixture.at("prefix_instructions").get<decltype(instructions)>()==instructions && fixture.at("prefix_stores")==stores,"Original game startup instruction trace or ordered stores differ");
        if(advance_xnet)check(fixture.at("xnet_startup_parameters")==parameters && fixture.at("xnet_startup_parameter_bytes")==parameter_bytes &&
            fixture.at("pending_stack_bytes").get<std::string>()==hex(memory.view(state.gp[4],sp-state.gp[4]+4)),
            "Original network startup parameters or complete pending stack differ");
        check(fixture.at("game_options")==options && fixture.at("game_options_sha256").get<std::string>()==digest(memory.view(options,0x1200)),"Original game-options bytes differ");
        check((abi.at("x87_control").get<unsigned>()&~0x40u)==(state.floating_point->x87_control_word&~0x40u) &&
            abi.at("mxcsr")==state.floating_point->mxcsr && abi.at("x87_status")==state.x87_status,
            "Original game startup defined floating-point controls/status differ");
        for(unsigned i=0;i<state.xmm.size();++i)check(abi.at("xmm")[i].get<std::string>()==hex(std::as_bytes(std::span(&state.xmm[i],1))),
            "Original game startup retained XMM state differs");
        check(fixture.at("qos_pool")==qos && fixture.at("qos_pool_sha256").get<std::string>()==digest(memory.view(qos,0x150)),
            "Original QoS pool constructor bytes differ");
        for(const auto& region:fixture.at("network_fields"))check(region.at("sha256").get<std::string>()==
            digest(memory.view(region.at("address").get<std::uint32_t>(),region.at("bytes").get<std::size_t>())),"Original complete network data bank differs");
    }
    if(advance_rng) {
        // Reject wrong native owners before a clock observation, allocation or
        // ABI mutation. These checks run after measuring the suspended prefix.
        auto probe=state;probe.gp[4]=sp-0x800u;probe.gp[5]=sp-0x700u;
        put(memory,probe.gp[4],0x32bbbcu);put(memory,probe.gp[4]+4,probe.gp[5]-4u);
        const auto before=probe.gp;const auto utc=rng->utc_filetime_input();
        bool wrong_output=false,wrong_bank=false,wrong_thread=false,heap_bank=false,heap_frame=false;
        try{rng->try_call(memory,{0x33a4d3u,0x80000080u,true},probe);}catch(const std::invalid_argument&){wrong_output=true;}
        SourceRuntimeMemory foreign;
        try{rng->try_call(foreign,{0x33a4d3u,0x80000080u,true},probe);}catch(const std::invalid_argument&){wrong_bank=true;}
        std::thread outsider([&]{try{rng->instruction(0x33b595u);}catch(const std::invalid_argument&){wrong_thread=true;}});outsider.join();
        try{startup.process_heap().try_crt_allocation_call(foreign,{0x3323abu,0x2db419u,false},probe,{0x27f,0x1f80},services);}catch(const std::invalid_argument&){heap_bank=true;}
        try{startup.process_heap().try_crt_allocation_call(memory,{0x3323abu,0x2db419u,false},probe,{0x27f,0x1f80},services);}catch(const std::invalid_argument&){heap_frame=true;}
        check(wrong_output && wrong_bank && wrong_thread && heap_bank && heap_frame && probe.gp==before && rng->utc_filetime_input()==utc,
            "Native RNG/CRT accepted a foreign bank, thread or caller frame");
    }
    if(advance_session) {
        SourceRuntimeMemory foreign;auto probe=state;probe.gp[4]=sp-0x800u;
        const auto before=probe.gp;const auto count=sockets.calls().size();
        bool socket_bank=false,socket_site=false,socket_frame=false,file_bank=false,file_frame=false;
        put(memory,probe.gp[4],0xb6de3u);
        try{sockets.try_call(foreign,{0xb6ddfu,0x3d7268u,false},probe);}catch(const std::invalid_argument&){socket_bank=true;}
        try{sockets.try_call(memory,{0xb6ddeu,0x3d7268u,false},probe);}catch(const std::invalid_argument&){socket_site=true;}
        try{sockets.try_call(memory,{0xb6ddfu,0x3d7268u,false},probe);}catch(const std::invalid_argument&){socket_frame=true;}
        try{profile_file.try_call(foreign,{0x139407u,0x2e0b1eu,false},probe);}catch(const std::invalid_argument&){file_bank=true;}
        try{profile_file.try_call(memory,{0x139407u,0x2e0b1eu,false},probe);}catch(const std::invalid_argument&){file_frame=true;}
        check(socket_bank && socket_site && socket_frame && file_bank && file_frame && probe.gp==before &&
            sockets.calls().size()==count && sockets.live_sockets()==4,
            "Native session APIs accepted a foreign bank/site/frame or allocated after rejecting it");
    }
    if(advance_cache) {
        SourceRuntimeMemory foreign;auto probe=state;probe.gp[4]=sp-0x800u;
        const auto before=probe.gp;const auto queries=language->queries();
        put(memory,probe.gp[4],0x33a4bdu);
        bool wrong_bank=false,wrong_frame=false,wrong_thread=false;
        try{language->try_call(foreign,{0x2e101cu,0x80000018u,true},probe);}catch(const std::invalid_argument&){wrong_bank=true;}
        try{language->try_call(memory,{0x2e101cu,0x80000018u,true},probe);}catch(const std::invalid_argument&){wrong_frame=true;}
        std::thread outsider([&]{try{language->try_call(memory,{0x2e101cu,0x80000018u,true},probe);}catch(const std::invalid_argument&){wrong_thread=true;}});outsider.join();
        check(wrong_bank && wrong_frame && wrong_thread && probe.gp==before && language->queries()==queries,
            "Native language query accepted a foreign bank/thread/frame or mutated rejected state");
    }
    Json session_raw_fields=Json::array();
    if(advance_session) {
        SourceRuntimeMemory declarations;mount_native_tu15_network_session_data(declarations);
        declarations.visit_committed([&](auto p,auto bytes){check(memory.native_address(p,bytes.size()).value>0xffffffffu,
            "Session storage backing narrowed into the source execution namespace");});
        for(const auto& event:resources)if(event.allocation) {
            const auto p=event.result[1],n=event.result[2];
            check(memory.native_address(p,n).value>0xffffffffu,"Session raw allocation backing was narrowed");
            session_raw_fields.push_back({{"address",p},{"bytes",n},{"sha256",digest(memory.view(p,n))}});
        }
        if(!fixture.is_null())check(fixture.at("session_raw_fields")==session_raw_fields,"Original complete session raw pages differ");
    }
    bool immutable=false;try{put(memory,0x44aed0u,0);}catch(const std::exception&){immutable=true;}
    check(immutable,"Native startup made subsystem identities writable");
    {
        SourceRuntimeMemory foreign;auto refused=state;bool rejected_bank=false;
        try{raw.try_call(foreign,{0x33786fu,0x800000b8u,false},refused);}catch(const std::exception&){rejected_bank=true;}
        check(rejected_bank && refused.gp==state.gp && raw.live_allocations()==(advance_graphics_pool?10u:advance_renderer?9u:advance_handles?8u:advance_session?6u:1u),
            "Shared raw allocator accepted a foreign native bank or changed its caller");
    }
    // After measuring the suspended source prefix, retire the actual owned
    // allocation through its complete source release method. No source job
    // resumes after this teardown-only check.
    {
        SourcePhysicsWorldRuntimeServices release;
        release.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto&) {
            if(!raw.try_call(bank,call,abi))throw std::runtime_error("Unowned QoS teardown dependency");
        };
        SourcePhysicsWorldRuntimeAbi done{};done.gp[4]=sp-0x800u;done.eflags=2;
        done.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        put(memory,done.gp[4],0x60200000u);put(memory,done.gp[4]+4u,qos);
        source_run_physics_pool_startup_runtime(memory,0x11e180u,done,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},release);
        bool released=false;try{memory.source_address(native_qos,1);}catch(const std::exception&){released=true;}
        check(released && raw.live_allocations()==(advance_graphics_pool?9u:advance_renderer?8u:advance_handles?7u:advance_session?5u:0u) && done.gp[4]==sp-0x800u+8u && resources.size()==(advance_graphics_pool?11u:advance_renderer?10u:advance_handles?9u:advance_session?7u:2u) && !resources.back().allocation,
            "QoS source release retained a native allocation or lost its caller frame");
    }
    Json report{{"schema",advance_graphics_depth?"H2_NATIVE_TU15_GRAPHICS_DEPTH_V1":advance_graphics_constants?"H2_NATIVE_TU15_GRAPHICS_CONSTANTS_V1":advance_graphics_palette?"H2_NATIVE_TU15_GRAPHICS_PALETTE_V1":advance_graphics_pool?"H2_NATIVE_TU15_GRAPHICS_POOL_V1":advance_graphics_state?"H2_NATIVE_TU15_GRAPHICS_STATE_V1":advance_graphics_device?"H2_NATIVE_TU15_GRAPHICS_DEVICE_V1":advance_renderer_request?"H2_NATIVE_TU15_RENDERER_REQUEST_V1":advance_aux_objects?"H2_NATIVE_TU15_AUX_OBJECT_STATE_STARTUP_V1":advance_objects?"H2_NATIVE_TU15_OBJECT_STORAGE_STARTUP_V1":advance_scalars?"H2_NATIVE_TU15_SCALAR_STATE_STARTUP_V1":advance_registration?"H2_NATIVE_TU15_STATIC_REGISTRATION_STARTUP_V1":advance_views?"H2_NATIVE_TU15_VIEW_STORAGE_STARTUP_V1":advance_players?"H2_NATIVE_TU15_PLAYER_STARTUP_V1":advance_context?"H2_NATIVE_TU15_STATE_CONTEXT_STARTUP_V1":advance_clock?"H2_NATIVE_TU15_CLOCK_STARTUP_V1":advance_renderer?"H2_NATIVE_TU15_RENDERER_STARTUP_V1":advance_handles?"H2_NATIVE_TU15_LEVEL_HANDLES_STARTUP_V1":advance_bink?"H2_NATIVE_TU15_BINK_MEMORY_STARTUP_V1":advance_input?"H2_NATIVE_TU15_INPUT_STARTUP_V1":advance_cache?"H2_NATIVE_TU15_CACHE_STARTUP_V1":advance_session?"H2_NATIVE_TU15_NETWORK_SESSION_V1":advance_rng?"H2_NATIVE_TU15_RNG_STARTUP_V1":advance_full?"H2_NATIVE_TU15_NETWORK_COMPLETE_V1":advance_services?"H2_NATIVE_TU15_ONLINE_SERVICES_V1":advance_online?"H2_NATIVE_TU15_NETWORK_PLATFORM_V1":advance_xnet?"H2_NATIVE_TU15_NETWORK_CONNECTION_V1":"H2_NATIVE_TU15_NETWORK_LINK_V1"},{"passed",true},{"typed_data_bytes",bytes},{"initializer_slots",68},
        {"instructions",instructions.size()},{"ordered_stores",stores.size()},{"frontier_target",frontier.target},{"frontier_gp",state.gp},
        {"x87_control",state.floating_point->x87_control_word},{"mxcsr",state.floating_point->mxcsr},{"game_options",options},
        {"excluded_reserved_x87_control_bit",6},{"defined_floating_point_state_verified",true},
        {"native_qos_pool_address_bits",64},{"native_qos_pool_bytes",0x150},{"native_raw_allocation_bytes",0x1000},
        {"source_qos_pool_constructor_complete",true},{"source_network_startup_complete",advance_full || !(flags&1u)},{"xnet_link_status_returned",true},
        {"host_link_observation",host},{"native_link_flags",flags},{"native_xnet_api_abi",true},{"xbox_phy_or_irql_emulation",false},
        {"link_query_count",link_queries},{"xnet_startup_parameters",parameters},{"xnet_startup_parameter_bytes",parameter_bytes},
        {"xnet_startup_returned",advance_online},{"winsock_startup_returned",advance_online},{"source_network_connection_startup_complete",advance_full},
        {"native_network_context_initialized",network.service()!=nullptr},{"xnet_references",network.xnet_references()},
        {"winsock_references",network.winsock_references()},
        {"native_api_calls",native_api_calls},
        {"native_online_initialized",advance_services && online->initialized()},{"native_online_authenticated",false},
        {"xonline_startup_returned",advance_services},{"title_address_query_returned",advance_full},
        {"native_title_address_calls",title_calls},{"source_qos_pool_activated",advance_full},{"source_noop_callbacks_completed",noops},
        {"native_title_address_foreign_bank_and_output_frame_rejected",advance_full},
        {"native_browser_requests",browser_requests},
        {"foreign_network_startup_bank_site_and_return_frame_rejected",true},
        {"foreign_link_bank_site_and_return_frame_rejected",true},
        {"native_raw_allocation_request",resources[0].request},{"native_raw_allocation_result",resources[0].result},
        {"foreign_allocator_bank_rejected",true},{"source_release_and_retired_native_pointer_verified",true},
        {"game_options_native_address_bits",64},{"source_game_storage_complete",false},{"world_startup_complete",false},
        {"reference_executable_is_runtime_input",false},{"original_cpu_comparison",!fixture.is_null()},{"normal_gameplay_admission",false}};
    if(advance_input) {
        report["source_input_prelude"]=input_prelude;report["source_input_initializer_complete"]=true;
        report["native_input_device_selection"]=input_devices_mode;
        report["initializer_slots_completed"]=advance_aux_objects?31:advance_objects?28:advance_scalars?27:advance_registration?25:advance_views?23:advance_players?20:advance_context?19:advance_clock?18:advance_renderer?15:advance_handles?14:advance_bink?13:11;report["input_fields"]=Json::array();
        report["native_input_calls"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_input_startup_data(fields);
        fields.visit_committed([&](auto p,auto bytes) {
            check(memory.native_address(p,bytes.size()).value>0xffffffffu,"Native input state backing was narrowed");
            report["input_fields"].push_back({{"address",p},{"bytes",bytes.size()},{"sha256",digest(memory.view(p,bytes.size()))}});
        });
        for(const auto& c:input_devices->calls()) {
            Json writes=Json::array();for(const auto& w:c.writes)writes.push_back({{"address",w.address},{"bytes",hex(w.bytes)}});
            report["native_input_calls"].push_back({{"instruction",c.instruction},{"target",c.target},{"entry_esp",c.entry_esp},
                {"return_esp",c.return_esp},{"return_eax",c.return_eax},{"arguments",c.arguments},{"writes",writes}});
        }
        report["native_input_live_controllers"]=input_devices->live_controllers();
        report["native_input_controller_addresses"]=input_devices->native_controller_addresses();
        report["native_input_address_bits"]=sizeof(void*)*8;
        if(!fixture.is_null())for(auto key:{"input_fields","native_input_calls"})check(report[key]==fixture.at(key),"Original input bindings or platform input receipts differ");
    }
    if(advance_cache) {
        check(language->queried() && profile_file.directory_probe().has_value() && profile_file.directory_creation().has_value() &&
            profile_file.directory_creation()->result==1 && std::filesystem::is_directory(directory.path/"sgf"),
            "Source cache startup did not consume native language or create its actual private directory");
        const auto& pref=language->preference();
        const auto& probe=*profile_file.directory_probe();const auto& create=*profile_file.directory_creation();
        report["native_language_preference"]={{"language",pref.language},{"country",pref.country},{"platform_value",pref.platform_value}};
        report["native_cache_directory_probe"]={{"instruction",probe.instruction},{"entry_esp",probe.entry_esp},{"return_esp",probe.return_esp},
            {"attributes",probe.attributes},{"error",probe.error},{"logical_path",probe.logical_path}};
        report["native_cache_directory_creation"]={{"instruction",create.instruction},{"entry_esp",create.entry_esp},{"return_esp",create.return_esp},
            {"result",create.result},{"error",create.error},{"logical_path",create.logical_path}};
        report["source_cache_startup_complete"]=true;report["source_game_language"]=word(memory,0x48c750u);
        report["cache_fields"]=Json::array();SourceRuntimeMemory cache_fields;mount_native_tu15_cache_startup_data(cache_fields);
        cache_fields.visit_committed([&](auto p,auto bytes){
            check(memory.native_address(p,bytes.size()).value>0xffffffffu,"SGF cache state backing was narrowed");
            report["cache_fields"].push_back({{"address",p},{"bytes",bytes.size()},{"sha256",digest(memory.view(p,bytes.size()))}});
        });
        if(!fixture.is_null()) {
            for(auto key:{"native_language_preference","native_cache_directory_probe","native_cache_directory_creation","source_game_language","cache_fields"})
                check(fixture.at(key)==report[key],"Original cache startup inputs or complete banks differ");
        }
    }
    if(advance_session) {
        check(sockets.live_sockets()==4 && profile_file.probe().has_value() &&
            memory.view(0x4dc3e8u,1)[0]==std::byte{1},"Source network session lost native sockets, actual profile absence or initialization flag");
        report["source_network_session_initializer_complete"]=true;
        report["session_storage_native_address_bits"]=64;report["session_raw_fields"]=session_raw_fields;
        report["native_session_foreign_bank_site_and_frames_rejected"]=true;
        report["native_millisecond_inputs"]=rng->monotonic_millisecond_inputs();
        report["native_socket_calls"]=Json::array();
        for(const auto& call:sockets.calls()) {
            Json row{{"instruction",call.instruction},{"target",call.target},{"entry_esp",call.entry_esp},
                {"return_esp",call.return_esp},{"result",call.result},{"arguments",call.arguments},{"outputs",Json::array()}};
            for(auto output:call.outputs)row["outputs"].push_back({{"address",output.address},{"value",output.value}});
            if(call.native_bound_endpoint)row["native_bound_endpoint"]={{"address",call.native_bound_endpoint->address},{"port",call.native_bound_endpoint->port}};
            report["native_socket_calls"].push_back(row);
        }
        const auto& probe=*profile_file.probe();report["native_profile_probe"]={{"instruction",probe.instruction},{"entry_esp",probe.entry_esp},
            {"return_esp",probe.return_esp},{"attributes",probe.attributes},{"error",probe.error},{"logical_path",probe.logical_path}};
        report["session_fields"]=Json::array();
        SourceRuntimeMemory session;mount_native_tu15_network_session_data(session);
        session.visit_committed([&](auto p,auto bytes){report["session_fields"].push_back({{"address",p},{"bytes",bytes.size()},{"sha256",digest(memory.view(p,bytes.size()))}});});
        if(!fixture.is_null()) {
            check(fixture.at("native_socket_calls")==report["native_socket_calls"] && fixture.at("native_profile_probe")==report["native_profile_probe"] &&
                fixture.at("native_millisecond_inputs")==report["native_millisecond_inputs"],"Native session resource or clock inputs differ from its measured original-code replay");
            for(const auto& field:fixture.at("session_fields"))check(field.at("sha256").get<std::string>()==digest(memory.view(
                field.at("address").get<std::uint32_t>(),field.at("bytes").get<std::size_t>())),"Original session constructor bank differs");
        }
    }
    if(advance_rng) {
        report["native_rng_inputs"]=rng_inputs;report["source_rng_initializer_complete"]=true;
        report["source_crt_heap_allocation_complete"]=true;report["source_crt_rand_result"]=41;
        report["native_rng_heap_foreign_bank_thread_and_frame_rejected"]=true;
        report["native_clocks_replayed_for_original_comparison"]=!fixture.is_null();
        report["arena_offset"]=word(memory,0x4f2954u);report["arena_checksum"]=word(memory,0x4f295cu);
        report["rng_fields"]=Json::array();
        for(auto [p,n]:{std::pair{0x4f3cd8u,4u},std::pair{0x573840u,4u},std::pair{word(memory,0x4f3cd8u),8u},
            std::pair{0x08000000u,0x1000u}})
            report["rng_fields"].push_back({{"address",p},{"bytes",n},{"sha256",digest(memory.view(p,n))}});
    }
    if(advance_online) {
        report["native_network_configuration"]=network.service()->configuration().bytes;
        report["native_network_private_pool_bytes"]=network.service()->configuration().private_pool_bytes();
        report["native_network_pool_address_bits"]=64;
        report["native_network_provider_probe_succeeded"]=true;
        // Teardown-only native API lifecycle check, after measuring the source
        // frontier. No source initializer is resumed after these test calls.
        auto end=frame();put(memory,sp,0x6e12cu);
        bool wrong_frame=false;try{network.try_call(memory,{0x6e12cu,0x3d775bu,false},end);}catch(const std::invalid_argument&){wrong_frame=true;}
        check(wrong_frame && network.xnet_references()==1,"Native cleanup consumed a different return frame");
        put(memory,sp,0x6e12cu);network.try_call(memory,{0x6e127u,0x3d7768u,false},end);
        check(network.service() && network.winsock_references()==0 && network.xnet_references()==1,"Winsock cleanup retired a referenced XNet context");
        end=frame();put(memory,sp,0x6e131u);network.try_call(memory,{0x6e12cu,0x3d775bu,false},end);
        check(!network.service() && !network.xnet_references(),"Native XNet cleanup retained its unreferenced context");
        report["native_network_reference_lifecycle_verified"]=true;
    }
    if(advance_services) {
        report["native_online_context_address_bits"]=64;report["native_online_references"]=online->references();
        auto warm=frame();put(memory,sp,0x6e0c2u);put(memory,sp+4,sp+0x200);put(memory,sp+0x200,0);
        const auto address=online->native_address();online->try_call(memory,{0x6e0bdu,0x3aad19u,false},warm);
        check(online->references()==2 && online->native_address()==address,"Warm online startup replaced its owned service context");
        online->release();check(online->initialized() && online->references()==1,"Online cleanup retired a referenced context");
        online->release();bool retired=false;try{online->native_address();}catch(const std::runtime_error&){retired=true;}
        check(retired && !online->initialized() && !online->references(),"Online cleanup retained a retired native context");
        check(online_services->login->snapshot().status==DiscordLoginStatus::SignedOut && !online_services->account->snapshot().authenticated,
            "Online context teardown changed actual account authentication");
        report["native_online_context_reference_lifecycle_verified"]=true;
    }
    if(advance_graphics_depth) {
        const auto config=graphics_depth->configuration();
        check(config.mode==2 && config.depth_bits==24 && config.stencil_bits==8 && config.format==129 &&
            config.attachment_handle && config.state_backing.value>0xffffffffu,
            "Native world W-depth mode lacks its actual D24S8 attachment and owner");
        auto probe=frame();probe.gp=native_depth_call.at("entry_gp").get<decltype(probe.gp)>();
        probe.gp[4]=sp-0x2400u;put(memory,probe.gp[4],0x1265fu);put(memory,probe.gp[4]+4,2);const auto before=probe;
        const SourcePhysicsContactRuntimeCall request{0x1265a,0x3ff5d0,false};SourceRuntimeMemory foreign;
        bool bank=false,thread=false,duplicate=false,mode=false,missing=false;
        try{graphics_depth->try_call(foreign,request,probe);}catch(const std::invalid_argument&){bank=true;}
        try{graphics_depth->try_call(memory,request,probe);}catch(const std::invalid_argument&){duplicate=true;}
        std::thread outsider([&]{try{graphics_depth->try_call(memory,request,probe);}catch(const std::invalid_argument&){thread=true;}});outsider.join();
        NativeTu15RendererDepth cold(memory,*graphics_device,depth_control),absent(memory,*graphics_device,{});
        put(memory,probe.gp[4]+4,1);
        try{cold.try_call(memory,request,probe);}catch(const std::invalid_argument&){mode=true;}
        put(memory,probe.gp[4]+4,2);
        try{absent.try_call(memory,request,probe);}catch(const std::runtime_error&){missing=true;}
        check(bank && thread && duplicate && mode && missing && probe.gp==before.gp && probe.eflags==before.eflags,
            "Rejected native depth mode changed its caller or admitted unavailable storage");
        report["native_depth_mode_rejections_preserve_state"]=true;
        report["native_renderer_depth_mode_complete"]=true;report["native_depth_mode_call"]=native_depth_call;
    }
    if(advance_graphics_constants) {
        const auto config=graphics_constants->configuration();
        check(config.mode==0x11 && config.registers==192 && config.application_owned &&
            config.material_backing.value>0xffffffffu && config.extended_backing.value>0xffffffffu && config.extended_bytes==2544 &&
            (advance_graphics_depth ? state.gp[1]==0x4035c && state.gp[2]==1 : word(memory,state.gp[4]+4)==2),
            "Native shader constant configuration lacks its actual storage or pending depth request");
        auto probe=frame();probe.gp=native_constants_call.at("entry_gp").get<decltype(probe.gp)>();
        probe.gp[4]=sp-0x2400u;put(memory,probe.gp[4],0x12658u);const auto before=probe.gp;
        const SourcePhysicsContactRuntimeCall request{0x12653,0x400ef0,false};SourceRuntimeMemory foreign;
        bool bank=false,thread=false,duplicate=false,mode=false,missing=false;
        try{graphics_constants->try_call(foreign,request,probe);}catch(const std::invalid_argument&){bank=true;}
        try{graphics_constants->try_call(memory,request,probe);}catch(const std::invalid_argument&){duplicate=true;}
        std::thread outsider([&]{try{graphics_constants->try_call(memory,request,probe);}catch(const std::invalid_argument&){thread=true;}});outsider.join();
        NativeTu15RendererConstants cold(memory,*graphics_device,constant_control),absent(memory,*graphics_device,{});
        auto changed=probe;changed.gp[0]=0x10;
        try{cold.try_call(memory,request,changed);}catch(const std::invalid_argument&){mode=true;}
        try{absent.try_call(memory,request,probe);}catch(const std::runtime_error&){missing=true;}
        check(bank && thread && duplicate && mode && missing && probe.gp==before,
            "Rejected native constant mode changed its caller or admitted unavailable storage");
        report["native_constant_mode_rejections_preserve_state"]=true;
        report["native_renderer_constant_mode_complete"]=true;report["native_constant_mode_call"]=native_constants_call;
    }
    if(advance_graphics_palette) {
        check(graphics_palette->initialized() && graphics_palette->bindings()==4 && native_palette_calls.size()==5 &&
            graphics_palette->native_address().value>0xffffffffu && memory.view(0x4922a8,1)[0]==std::byte{} &&
            std::ranges::all_of(memory.view(0x4915f0,384),[](auto value){return value==std::byte{};}),
            "Source palette calls lack their real native color owner, stage bindings or pending completion");
        for(unsigned stage=0;stage<4;++stage)check(palette_factory.bound(stage).native_address()==graphics_palette->native_address(),
            "Source palette stage is not bound to the actual renderer resource");
        auto probe=frame();probe.gp=native_palette_calls[0].at("entry_gp").get<decltype(probe.gp)>();
        probe.gp[4]=sp-0x2200u;put(memory,probe.gp[4],0x125ebu);put(memory,probe.gp[4]+4,0x474da8u);put(memory,probe.gp[4]+8,0x4915dcu);
        const auto before=probe.gp;const auto current_handle=graphics_palette->handle();
        const auto current_address=graphics_palette->native_address();SourceRuntimeMemory foreign;
        const SourcePhysicsContactRuntimeCall create{0x125e6,0x1d3d0,false};
        bool bad_bank=false,bad_thread=false,duplicate=false,missing=false,bad_site=false,bad_type=false,bad_input=false;
        try{graphics_palette->try_call(foreign,create,probe);}catch(const std::invalid_argument&){bad_bank=true;}
        try{graphics_palette->try_call(memory,create,probe);}catch(const std::invalid_argument&){duplicate=true;}
        std::thread outsider([&]{try{graphics_palette->try_call(memory,create,probe);}catch(const std::invalid_argument&){bad_thread=true;}});outsider.join();
        NativeTu15RendererPalette absent(memory,*graphics_device,{});
        try{absent.try_call(memory,create,probe);}catch(const std::runtime_error&){missing=true;}
        NativeTu15RendererPalette cold(memory,*graphics_device,palette_factory);
        put(memory,0x4915dc,0); // Explicit cold API input for rejection-only controls.
        try{cold.try_call(memory,{0x125e7,0x1d3d0,false},probe);}catch(const std::invalid_argument&){bad_site=true;}
        auto changed=probe;changed.gp[0]=2;
        try{cold.try_call(memory,create,changed);}catch(const std::invalid_argument&){bad_type=true;}
        put(memory,probe.gp[4]+4,0x474da9);
        try{cold.try_call(memory,create,probe);}catch(const std::invalid_argument&){bad_input=true;}
        put(memory,probe.gp[4]+4,0x474da8);put(memory,0x4915dc,current_handle);
        check(bad_bank && bad_thread && duplicate && missing && bad_site && bad_type && bad_input &&
            probe.gp==before && !absent.handle() && !cold.handle() && graphics_palette->bindings()==4 &&
            graphics_palette->native_address()==current_address,"Rejected native palette request changed its source/native resources");
        report["native_palette_rejections_preserve_state"]=true;
        report["native_renderer_palette_complete"]=true;report["native_palette_calls"]=native_palette_calls;
        report["renderer_palette_fields"]=Json::array();SourceRuntimeMemory fields;mount_native_tu15_renderer_palette_data(fields);
        fields.visit_committed([&](auto p,auto bytes){report["renderer_palette_fields"].push_back({{"address",p},{"bytes",bytes.size()},{"sha256",digest(memory.view(p,bytes.size()))}});});
        if(!fixture.is_null())check(report["renderer_palette_fields"]==fixture.at("renderer_palette_fields"),"Original palette API inputs or caller clear differ");
    }
    if(advance_graphics_pool) {
        const auto pool=word(memory,0x494328u);
        Json pool_words=Json::object();
        for(auto offset:{0x20u,0x24u,0x30u,0x34u,0x64u,0x68u,0x90u,0x94u})
            pool_words[std::to_string(offset)]=word(memory,pool+offset);
        check(resources[9].allocation && resources[9].request==std::array<std::uint32_t,5>{0,0,0x180,0x101000,4} &&
            pool==resources[9].result[1] && resources[9].result[2]==4096u &&
            word(memory,pool+0x20u)==0x1c7d0u && word(memory,pool+0x24u)==0x25bcb0u &&
            word(memory,pool+0x30u)==0x88u && word(memory,pool+0x34u)==0u &&
            word(memory,pool+0x64u)==pool+0x70u && word(memory,pool+0x68u)==0x77656565u &&
            word(memory,pool+0x90u)==8u && word(memory,pool+0x94u)==0x18u && memory.view(0x4922a8u,1)[0]==std::byte{},
            "Source renderer pool lacks its real allocation and original constructor fields: "+pool_words.dump());
        report["source_renderer_pool_constructor_complete"]=true;
        report["renderer_pool_body_correspondence_sha256"]="9b038202a04dcfa5fe34cfa62fd5ab1089087e67c24b85eef79f26efac4a1e2f";
        report["renderer_pool_fields"]=Json::array();SourceRuntimeMemory fields;mount_native_tu15_renderer_pool_data(fields);
        fields.visit_committed([&](auto p,auto b){
            check(memory.native_address(p,b.size()).value>0xffffffffu,"Renderer pool backing was narrowed");
            report["renderer_pool_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});
        });
        report["renderer_pool_fields"].push_back({{"address",pool},{"bytes",4096},{"sha256",digest(memory.view(pool,4096))}});
        if(!fixture.is_null())check(report["renderer_pool_fields"]==fixture.at("renderer_pool_fields") &&
            report["renderer_pool_body_correspondence_sha256"]==fixture.at("renderer_pool_body_correspondence_sha256"),
            "Original renderer pool construction differs");
    }
    if(advance_graphics_state) {
        check(word(memory,0x492368u)==0xffffffffu && word(memory,0x49238cu)==0u &&
            word(memory,0x493698u)==0x40400000u && memory.view(0x4922a8u,1)[0]==std::byte{},
            "Source renderer state selectors/scalars or pending completion differ");
        report["source_renderer_state_and_table_constructors_complete"]=true;
        report["renderer_state_body_correspondence_sha256"]="1f77feaba03a48dc965a1a4cab97eec411e1f2b27975f40fdfa1e095b2211919";
        report["renderer_state_fields"]=Json::array();SourceRuntimeMemory fields;mount_native_tu15_renderer_state_data(fields);
        fields.visit_committed([&](auto p,auto b){
            check(memory.native_address(p,b.size()).value>0xffffffffu,"Renderer state/table native backing was narrowed");
            report["renderer_state_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});
        });
        if(!fixture.is_null())check(report["renderer_state_fields"]==fixture.at("renderer_state_fields") &&
            report["renderer_state_body_correspondence_sha256"]==fixture.at("renderer_state_body_correspondence_sha256"),
            "Original renderer state/table construction differs");
    }
    if(advance_graphics_device) {
        check(graphics_device->initialized() && graphics_device->native_address().value>0xffffffffu &&
            memory.view(0x515e9c,1)[0]==std::byte{1} && memory.view(0x4922a8,1)[0]==std::byte{},
            "Source graphics publication lacks its real native renderer or bypassed later initialization");
        const SourcePhysicsContactRuntimeCall create{0x129ad,0x3ff320,false};auto probe=*graphics_entry;
        // The real128A0 suffix reuses its old argument cell while copying
        // defaults. Rejection controls use a fresh explicit caller frame.
        probe.gp[4]=sp-0x1e00u;put(memory,probe.gp[4],0x129b2u);put(memory,probe.gp[4]+4,probe.gp[4]+0x14u);
        for(unsigned i=0;i<17;++i)put(memory,probe.gp[4]+0x14u+4*i,graphics_frame->presentation[i]);
        const auto before=probe.gp;
        (void)native_tu15_renderer_device_frame(memory,create,probe);
        SourceRuntimeMemory foreign;NativeTu15GraphicsDevice absent(memory,{});
        bool bad_bank=false,bad_thread=false,bad_site=false,bad_flags=false,duplicate=false,missing=false;
        try{graphics_device->try_call(foreign,create,probe);}catch(const std::invalid_argument&){bad_bank=true;}
        try{graphics_device->try_call(memory,{0x129ae,0x3ff320,false},probe);}catch(const std::invalid_argument&){bad_site=true;}
        auto changed=probe;changed.gp[0]^=1;
        try{graphics_device->try_call(memory,create,changed);}catch(const std::invalid_argument&){bad_flags=true;}
        try{graphics_device->try_call(memory,create,probe);}catch(const std::invalid_argument&){duplicate=true;}
        put(memory,0x515e90,0); // Explicit cold API input for the missing-owner rejection only.
        try{absent.try_call(memory,create,probe);}catch(const std::runtime_error&){missing=true;}
        check(word(memory,0x515e90)==0,"Missing native renderer owner supplied a device handle");
        put(memory,0x515e90,graphics_device->handle());
        std::thread outsider([&]{try{graphics_device->try_call(memory,create,probe);}catch(const std::invalid_argument&){bad_thread=true;}});outsider.join();
        check(bad_bank && bad_thread && bad_site && bad_flags && duplicate && missing && probe.gp==before &&
            word(memory,0x515e90)==graphics_device->handle() && !absent.handle(),"Native graphics adapter supplied a device without its actual owner or accepted a rejected request");
        report["native_graphics_rejected_requests_preserve_state"]=true;
        report["native_graphics_device_call"]=native_graphics_call;report["native_port_renderer_bound"]=true;
        report["graphics_device_fields"]=Json::array();SourceRuntimeMemory fields;mount_native_tu15_graphics_device_data(fields);
        fields.visit_committed([&](auto p,auto b){report["graphics_device_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        if(!fixture.is_null())check(report["graphics_device_fields"]==fixture.at("graphics_device_fields"),"Original post-device graphics defaults, constructor or source publication differs");
    }
    if(advance_renderer_request) {
        const auto device=advance_graphics_device?*graphics_frame:native_tu15_renderer_device_frame(memory,frontier,state);
        check(device.output_backing.value>0xffffffffu && device.parameters_backing.value>0xffffffffu &&
            word(memory,device.output_token)==(advance_graphics_device?graphics_device->handle():0u) && memory.view(0x4922a8,1)[0]==std::byte{},
            "Pending renderer device request lost native backing or supplied an unowned device/completion result");
        const auto entry=advance_graphics_device?*graphics_entry:state;const SourcePhysicsContactRuntimeCall call{0x129ad,0x3ff320,false};
        auto rejected=entry;rejected.gp[0]^=1;bool bad_flags=false,bad_site=false,bad_output=false;
        try{native_tu15_renderer_device_frame(memory,call,rejected);}catch(const std::invalid_argument&){bad_flags=true;}
        try{native_tu15_renderer_device_frame(memory,{0x129ae,0x3ff320,false},entry);}catch(const std::invalid_argument&){bad_site=true;}
        rejected=entry;rejected.gp[1]+=4;
        try{native_tu15_renderer_device_frame(memory,call,rejected);}catch(const std::invalid_argument&){bad_output=true;}
        check(bad_flags && bad_site && bad_output,"Renderer device frame decoder accepted a foreign request");
        report["source_renderer_device_request_producer_complete"]=true;report["source_renderer_resource_initializer_complete"]=false;
        report["native_gpu_device_creation_attempted"]=false;report["renderer_device_request_frame_rejections_verified"]=true;
        report["renderer_device_request"]={{"parameters_token",device.parameters_token},{"output_token",device.output_token},
            {"flags",device.flags},{"presentation",device.presentation},{"native_output_address_bits",64},{"native_parameters_address_bits",64}};
        report["renderer_request_fields"]=Json::array();SourceRuntimeMemory fields;mount_native_tu15_renderer_request_data(fields);
        fields.visit_committed([&](auto p,auto b){report["renderer_request_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        report["renderer_request_fields"].push_back({{"address",device.parameters_token},{"bytes",68},{"sha256",digest(memory.view(device.parameters_token,68))}});
        if(!fixture.is_null())check(report["renderer_request_fields"]==fixture.at("renderer_request_fields"),"Original renderer viewport, device request or pending constructor fields differ");
    }
    if(advance_aux_objects) {
        const auto arena=word(memory,0x4f2950),first=word(memory,0x51daf0),second=word(memory,0x51daf4),third=word(memory,0x52be6c);
        check(first==arena+1510384u && second==first+0x1d88u && third==second+0x304u &&
            std::ranges::all_of(memory.view(first,0x1d88),[](auto b){return b==std::byte{};}) &&
            memory.view(second+0x300,1)[0]==std::byte{} && memory.view(third+0x20,1)[0]==std::byte{},
            "Auxiliary arena-state constructors lost their allocation order or source clears");
        report["source_aux_object_state_initializers_complete"]=true;report["aux_object_state_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_aux_object_state_data(fields);
        fields.visit_committed([&](auto p,auto b){report["aux_object_state_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        for(auto [p,n]:{std::pair{first,0x1d88u},std::pair{second,0x304u},std::pair{third,0x24u}}) {
            check(memory.native_address(p,n).value>0xffffffffu,"Auxiliary arena-state native backing was narrowed");
            report["aux_object_state_fields"].push_back({{"address",p},{"bytes",n},{"sha256",digest(memory.view(p,n))}});
        }
        if(!fixture.is_null())check(report["aux_object_state_fields"]==fixture.at("aux_object_state_fields"),"Original auxiliary arena-state bindings or bytes differ");
    }
    if(advance_objects) {
        const auto arena=word(memory,0x4f2950);constexpr std::uint32_t begin=24256,extent=1486128;
        check(storage_formatting.calls()==12 && memory.native_address(arena+begin,extent).value>0xffffffffu,
            "Object constructors did not own their complete native arena storage or formatting calls");
        auto probe=frame();probe.gp[4]=sp-0x1a00u;put(memory,probe.gp[4],0xbad0bad0u);
        const auto before=probe.gp;const auto calls=storage_formatting.calls();SourceRuntimeMemory foreign;
        bool bad_bank=false,bad_site=false,bad_frame=false,bad_thread=false;
        try{storage_formatting.try_call(foreign,{0x11f39fu,0x32ba80u,false},probe);}catch(const std::invalid_argument&){bad_bank=true;}
        try{storage_formatting.try_call(memory,{0x11f39eu,0x32ba80u,false},probe);}catch(const std::invalid_argument&){bad_site=true;}
        try{storage_formatting.try_call(memory,{0x11f39fu,0x32ba80u,false},probe);}catch(const std::invalid_argument&){bad_frame=true;}
        std::thread outsider([&]{try{storage_formatting.try_call(memory,{0x11f39fu,0x32ba80u,false},probe);}catch(const std::invalid_argument&){bad_thread=true;}});outsider.join();
        check(bad_bank && bad_site && bad_frame && bad_thread && probe.gp==before && storage_formatting.calls()==calls,
            "Object storage CRT owner accepted a foreign caller or changed rejected state");
        report["native_storage_format_foreign_bank_thread_site_frame_rejected"]=true;
        report["source_object_storage_initializer_complete"]=true;report["native_storage_format_calls"]=storage_formatting.calls();
        report["object_storage_fields"]=Json::array();SourceRuntimeMemory fields;mount_native_tu15_object_storage_data(fields);
        fields.visit_committed([&](auto p,auto b){report["object_storage_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        report["object_storage_fields"].push_back({{"address",arena+begin},{"bytes",extent},{"sha256",digest(memory.view(arena+begin,extent))}});
        if(!fixture.is_null())check(report["object_storage_fields"]==fixture.at("object_storage_fields"),"Original object pools, type chain, cluster references or bindings differ");
    }
    if(advance_scalars) {
        const auto storage=word(memory,0x52be68);
        check(storage==word(memory,0x4f63b8)+0xc9cu && word(memory,storage)==0x40857af5u &&
            word(memory,storage+4)==0x3f800000u && word(memory,storage+8)==0x3a902de0u &&
            std::ranges::all_of(memory.view(storage+12,16),[](auto b){return b==std::byte{};}) &&
            memory.native_address(storage,28).value>0xffffffffu,"Source1F3830 scalar/vector state or native ownership differs");
        report["source_scalar_state_initializer_complete"]=true;report["scalar_state_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_scalar_state_data(fields);
        fields.visit_committed([&](auto p,auto b){report["scalar_state_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        report["scalar_state_fields"].push_back({{"address",storage},{"bytes",28},{"sha256",digest(memory.view(storage,28))}});
        if(!fixture.is_null())check(report["scalar_state_fields"]==fixture.at("scalar_state_fields"),"Original scalar-state operands, binding or constructed bytes differ");
    }
    if(advance_registration) {
        check(word(memory,0x4dc04c)==0x5539b0u && word(memory,0x4dc050)==0x554bd4u && word(memory,0x4dc054)==0x555808u &&
            word(memory,0x555808)==17u && word(memory,0x55588c)==26u && memory.view(0x4dc040,1)[0]==std::byte{1},
            "Static registration source count, bindings or publication differs");
        report["source_static_registration_initializer_complete"]=true;report["static_registration_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_static_registration_data(fields);
        fields.visit_committed([&](auto p,auto b) {
            check(memory.native_address(p,b.size()).value>0xffffffffu,"Static registration native backing was narrowed");
            report["static_registration_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});
        });
        if(!fixture.is_null())check(report["static_registration_fields"]==fixture.at("static_registration_fields"),"Original static registration arrays, bindings or publication differs");
    }
    if(advance_views) {
        const auto view=word(memory,0x4f9b54),actions=word(memory,0x52be64),state_storage=word(memory,0x4f63b8);
        check(view==word(memory,0x4f54f0)+0x130u && actions==view+0x264u && state_storage==actions+0x6c8u &&
            word(memory,state_storage+0xc14)==0xffffffffu && memory.view(view,1)[0]==std::byte{},
            "Source view/action/storage constructors lost allocation order or authored fields");
        report["source_view_storage_initializers_complete"]=true;report["view_storage_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_view_storage_data(fields);
        fields.visit_committed([&](auto p,auto b){report["view_storage_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        for(auto [p,n]:{std::pair{view,0x264u},std::pair{actions,0x6c8u},std::pair{state_storage,0xc9cu}}) {
            check(memory.native_address(p,n).value>0xffffffffu,"View/action/storage native backing was narrowed");
            report["view_storage_fields"].push_back({{"address",p},{"bytes",n},{"sha256",digest(memory.view(p,n))}});
        }
        if(!fixture.is_null())check(report["view_storage_fields"]==fixture.at("view_storage_fields"),"Original view/action/storage constructed bytes differ");
    }
    if(advance_players) {
        const auto pool=word(memory,0x4f54f4),info=word(memory,0x4f54f0);
        check(pool==word(memory,0x5020c4)+0xe4u && info==pool+0x2210u && memory.native_address(pool,0x2210).value>0xffffffffu &&
            memory.native_address(info,0x130).value>0xffffffffu,"Player source constructors did not allocate their native pool/state");
        report["source_player_initializer_complete"]=true;report["player_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_player_startup_data(fields);
        fields.visit_committed([&](auto p,auto b){report["player_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        for(auto [p,n]:{std::pair{pool,0x2210u},std::pair{info,0x130u}})
            report["player_fields"].push_back({{"address",p},{"bytes",n},{"sha256",digest(memory.view(p,n))}});
        if(!fixture.is_null())check(report["player_fields"]==fixture.at("player_fields"),"Original player state, pool or bindings differ");
    }
    if(advance_context) {
        const auto context=word(memory,0x5020c4);
        check(context==word(memory,0x51df54)+0x24u && memory.native_address(context,0xe4).value>0xffffffffu &&
            std::ranges::all_of(memory.view(context,0xe4),[](auto b){return b==std::byte{};}),"Source1E7710 did not allocate and construct its own native state");
        report["source_state_context_initializer_complete"]=true;
        report["state_context_fields"]=Json::array({{{"address",0x5020c4},{"bytes",4},{"sha256",digest(memory.view(0x5020c4,4))}},
            {{"address",context},{"bytes",0xe4},{"sha256",digest(memory.view(context,0xe4))}}});
        if(!fixture.is_null())check(report["state_context_fields"]==fixture.at("state_context_fields"),"Original1E7710 binding or constructed state differs");
    }
    if(advance_clock) {
        const auto clock=word(memory,0x51df54);
        check(clock==word(memory,0x4f3cd8)+8u && memory.native_address(clock,0x24).value>0xffffffffu &&
            std::ranges::all_of(memory.view(clock,0x24),[](auto b){return b==std::byte{};}),"Source clock did not allocate and construct its own native storage");
        check(word(memory,0x50ebfc)==0 && word(memory,0x50ec00)==0 && word(memory,0x50ec04)==0xffffffffu && word(memory,0x50ec08)==0,
            "Source22C100 fields were not constructed in order");
        report["source_clock_initializer_complete"]=true;report["source_arena_reentry_callback_complete"]=true;report["clock_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_clock_startup_data(fields);
        fields.visit_committed([&](auto p,auto b){report["clock_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});});
        report["clock_fields"].push_back({{"address",clock},{"bytes",0x24},{"sha256",digest(memory.view(clock,0x24))}});
        if(!fixture.is_null())check(report["clock_fields"]==fixture.at("clock_fields"),"Original clock binding/state or preceding constructor fields differ");
    }
    if(advance_renderer) {
        for(auto context:{0x556e18u,0x5598f8u}) {
            const auto raw=word(memory,context);
            check(memory.native_address(raw,0x22974).value>0xffffffffu,"Renderer raw context backing was narrowed");
            for(auto offset:{0xcu,0x10u,0x14u,0x18u})
                check(memory.native_address(word(memory,context+offset),24).value>0xffffffffu,"Renderer heap descriptor backing was narrowed");
            if(const auto extra=word(memory,context+0x2acc))check(memory.native_address(extra,0x9c).value>0xffffffffu,"Renderer auxiliary descriptor backing was narrowed");
        }
        auto probe=frame();probe.gp[4]=sp-0x1800u;put(memory,probe.gp[4],0x137483u);put(memory,probe.gp[4]+4,24);
        const auto before=probe;const auto commits=native_heap_commit_calls.size();
        SourceRuntimeMemory foreign;
        bool wrong_bank=false,wrong_frame=false,wrong_site=false;
        try{startup.process_heap().try_renderer_allocation_call(foreign,{0x13610eu,0x32a3bbu,false},probe,{0x27f,0x1f80},services);}catch(const std::invalid_argument&){wrong_bank=true;}
        put(memory,probe.gp[4],0xbad0bad0u);
        try{startup.process_heap().try_renderer_allocation_call(memory,{0x13610eu,0x32a3bbu,false},probe,{0x27f,0x1f80},services);}catch(const std::invalid_argument&){wrong_frame=true;}
        put(memory,probe.gp[4],0x137483u);
        try{startup.process_heap().try_renderer_allocation_call(memory,{0x13610fu,0x32a3bbu,false},probe,{0x27f,0x1f80},services);}catch(const std::invalid_argument&){wrong_site=true;}
        check(wrong_bank && wrong_frame && wrong_site && probe.gp==before.gp && probe.eflags==before.eflags && native_heap_commit_calls.size()==commits,
            "Renderer allocation owner accepted a foreign bank/frame/site or changed rejected state");
        report["source_renderer_initializer_complete"]=true;report["native_heap_commit_calls"]=native_heap_commit_calls;
        report["native_renderer_heap_foreign_bank_frame_site_rejected"]=true;
        report["renderer_fields"]=Json::array();SourceRuntimeMemory fields;mount_native_tu15_renderer_startup_data(fields);
        fields.visit_committed([&](auto p,auto b) {
            check(memory.native_address(p,b.size()).value>0xffffffffu,"Renderer constructor backing was narrowed");
            report["renderer_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(memory.view(p,b.size()))}});
        });
        report["renderer_heap_fields"]=Json::array();memory.visit_committed([&](auto p,auto b) {
            if(p>=0x08000000u && p<0x08100000u)report["renderer_heap_fields"].push_back({{"address",p},{"bytes",b.size()},{"sha256",digest(b)}});
        });
        if(!fixture.is_null())for(auto key:{"renderer_fields","renderer_heap_fields","native_heap_commit_calls"})
            check(report[key]==fixture.at(key),"Original renderer descriptors, heap bytes or commitment calls differ");
    }
    if(advance_handles) {
        const auto levels=word(memory,0x4fafbc),multiplayer=word(memory,0x4fafb8);
        check(levels==0x10006000u && multiplayer==0x10007000u && (memory.view(levels+0x2a,1)[0]&std::byte{4})!=std::byte{} &&
            (memory.view(multiplayer+0x2a,1)[0]&std::byte{4})!=std::byte{},"Level-handle pools did not construct their real raw backing/ownership flags");
        for(unsigned i=0;i<64;++i)check(word(memory,0x4fadb8+i*8)==0xffffffffu && !word(memory,0x4fadbc+i*8),"Level-handle pairs were not source-initialized");
        report["source_level_handles_initializer_complete"]=true;report["level_handles_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_level_handles_startup_data(fields);
        fields.visit_committed([&](auto p,auto bytes) {
            check(memory.native_address(p,bytes.size()).value>0xffffffffu,"Level-handle constructor backing was narrowed");
            report["level_handles_fields"].push_back({{"address",p},{"bytes",bytes.size()},{"sha256",digest(memory.view(p,bytes.size()))}});
        });
        if(!fixture.is_null())check(report["level_handles_fields"]==fixture.at("level_handles_fields"),"Original level-handle constructor banks differ");
    }
    if(advance_bink) {
        check(word(memory,0x49130c)==0x159240u && word(memory,0x491310)==0x159340u && memory.view(0x4f5a58,1)[0]==std::byte{1},
            "Complete Bink memory startup did not publish its actual allocator/release identities");
        report["source_bink_memory_registration_complete"]=true;report["bink_memory_fields"]=Json::array();
        SourceRuntimeMemory fields;mount_native_tu15_bink_memory_startup_data(fields);
        fields.visit_committed([&](auto p,auto bytes) {
            check(memory.native_address(p,bytes.size()).value>0xffffffffu,"Bink constructor backing was narrowed");
            report["bink_memory_fields"].push_back({{"address",p},{"bytes",bytes.size()},{"sha256",digest(memory.view(p,bytes.size()))}});
        });
        if(!fixture.is_null())check(report["bink_memory_fields"]==fixture.at("bink_memory_fields"),"Original Bink allocator registration fields differ");
        report["bink_live_buffer_consumers_complete"]=false;
    }
    if(advance_input) {
        auto close=frame();close.gp[4]=sp-0xc00u;put(memory,close.gp[4],0x60200000u);
        source_run_input_startup_tu15(memory,0x127430u,close,{0x27f,0x1f80},services,observe);
        check(close.gp[4]==sp-0xc00u+4 && input_devices->live_controllers()==0,"Completed prefix input teardown retained resources");
        for(unsigned i=0;i<4;++i)check(!word(memory,0x4f2a9cu+i*4),"Completed prefix input teardown retained a source handle");
        report["native_input_teardown_verified"]=true;
    }
    if(advance_cache) {
        // Re-run the actual complete callback after recording the suspended
        // main prefix. The existing directory and language binding select the
        // original warm paths; no world/frame resumes after this check.
        auto warm=frame();warm.gp[4]=sp-0x800u;put(memory,warm.gp[4],0x60200000u);
        source_run_cache_startup_tu15(memory,0x21c0a0u,warm,{0x27f,0x1f80},services,observe);
        check(warm.gp[4]==sp-0x800u+4 && warm.gp[0]==word(memory,0x48c750u) && language->queries()==1 &&
            profile_file.directory_creations()==1 && profile_file.directory_probe()->attributes==0x10,
            "Warm source cache startup recreated its directory, re-queried language or lost its caller");
        for(const auto& row:report["cache_fields"])check(row["sha256"].get<std::string>()==digest(memory.view(
            row["address"].get<std::uint32_t>(),row["bytes"].get<std::size_t>())),"Warm cache startup changed its completed source bank");
        if(!fixture.is_null()) {
            check(fixture.at("language_remap_cases").size()==13,"Incomplete original language remap boundary matrix");
            for(const auto& row:fixture.at("language_remap_cases")) {
                auto test=frame();test.gp=row.at("initial_gp").get<decltype(test.gp)>();test.eflags=row.at("initial_eflags");
                put(memory,test.gp[4],0x60200000u);
                source_run_cache_startup_tu15(memory,0x11f440u,test,{0x27f,0x1f80},{});
                check(test.gp==row.at("gp").get<decltype(test.gp)>() && ((test.eflags^row.at("eflags").get<std::uint32_t>())&0xcd5u)==0,
                    "Native platform-to-game language remap differs from original CPU execution");
            }
            report["original_language_remap_cases_compared"]=13;
        }
        report["source_warm_cache_callback_verified"]=true;
        report["native_language_foreign_bank_thread_and_frame_rejected"]=true;
    }
    if(!capture.empty()){std::filesystem::create_directories(capture.parent_path());std::ofstream out(capture);out<<report.dump(2)<<'\n';check(bool(out),"Cannot write native game startup evidence");}
    std::cout<<"Native TU1.5 network link: actual host flags "<<flags<<"; "<<report["instructions"].get<std::size_t>()<<" source instructions and "<<report["ordered_stores"].get<std::size_t>()<<" stores; retained next initialization frontier; "
        <<(fixture.is_null()?"no executable input":"unchanged original CPU comparison passed")<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

#ifndef HALO2_NATIVE_GRAPHICS_STARTUP_MAIN
int main(int argc,char** argv) {return halo2::run_native_tu15_startup_test(argc,argv);}
#endif

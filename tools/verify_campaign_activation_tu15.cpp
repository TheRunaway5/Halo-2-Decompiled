#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/renderer.hpp"
#include "halo2/native_tu15_renderer_palette.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <thread>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4 && argc!=6)throw FormatError("Supply TU15_XBE OPENING_MAP bsp-start|map-start|sound-start [--graphics OUTPUT]");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    const bool bsp=std::string(argv[3])=="bsp-start";
    const bool sound=std::string(argv[3])=="sound-start";
    if(!bsp && !sound && std::string(argv[3])!="map-start")throw FormatError("Unknown original activation probe mode");
    const bool graphics=argc==6;
    if(graphics && std::string(argv[4])!="--graphics")throw FormatError("Unknown activation graphics option");
    struct Window {
        SDL_Window *value{};
        ~Window(){if(value)SDL_DestroyWindow(value);if(value)SDL_Quit();}
    }window;
    Mesh mesh;std::unique_ptr<Renderer> renderer;RendererPaletteFactory palettes;
    std::filesystem::path output;
    Camera camera;camera.field_of_view=60;
    if(graphics) {
        output=std::filesystem::absolute(argv[5]);
        if(std::filesystem::exists(output))throw FormatError("Preserve prior graphics evidence; supply a fresh output directory");
        std::filesystem::create_directories(output);
        if(SDL_Init(SDL_INIT_VIDEO)!=0)throw FormatError(SDL_GetError());
        window.value=SDL_CreateWindow("Original campaign activation palette probe",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
        if(!window.value)throw FormatError(SDL_GetError());
        Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);
        mesh.vertices={{{4,-2,-1},{1,1,1},{0,0},0},{{4,2,-1},{1,1,1},{0,0},0},{{4,0,2},{1,1,1},{0,0},0}};
        const auto shaders=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        renderer=std::make_unique<Renderer>(window.value,mesh,false,std::span<const SourceShadowPlanMaterial>{},nullptr,
            std::function<void()>{},nullptr,shaders?std::filesystem::path(shaders):std::filesystem::path{});
        palettes=renderer->palette_factory();
        std::cout<<"actual_renderer="<<renderer->device_name()<<'\n';
        std::cout<<"vulkan_validation_layer=disabled\n";
        if(!renderer->draw(camera,output/"before.ppm"))throw FormatError("Palette probe baseline GPU draw failed");
    }
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto path=(std::filesystem::temp_directory_path()/"halo2-campaign-activation-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Campaign activation probe storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &world=objects.tu15_world();
    if(sound)if(const auto settings=std::getenv("HALO2_TU15_USER_SETTINGS_EEPROM"))
        world.bind_sound_user_settings(settings);
    if(graphics)world.admit_renderer_palette(palettes);
    if(bsp)world.initialize_map();
    world.configure_campaign_session(1,1,30,0x12345678);
    auto &memory=world.memory();const auto game=Bytes(memory.view(0x4f3218u,4)).u32(0);
    const auto report=[&](const char *phase) {
        std::cout<<phase<<" native_physics="<<Bytes(memory.view(0x52be48u,4)).u32(0)
            <<" selected_bsp="<<std::int16_t(Bytes(memory.view(0x474c24u,2)).u16(0))
            <<" game_bsp="<<std::int16_t(Bytes(memory.view(game+2u,2)).u16(0))
            <<" game_active="<<std::to_integer<unsigned>(memory.view(game+0x1120u,1)[0])<<'\n'<<std::flush;
    };
    report("before");
    const auto report_palettes=[&] {
        if(!graphics)return;
        unsigned bound{};std::optional<NativeRuntimeAddress> identity;
        for(unsigned stage=0;stage<4;++stage) {
            RendererPalette palette;
            try {palette=palettes.bound(stage);}catch(const std::runtime_error &) {continue;}
            if(identity && *identity!=palette.native_address())throw FormatError("Original BSP palette stages have different native owners");
            identity=palette.native_address();
            if(identity->value<=0xffffffffu || !std::ranges::equal(palette.colors(),memory.view(0x474da8u,1024)))
                throw FormatError("Original BSP palette stage lacks its actual widened native color resource");
            ++bound;
        }
        std::cout<<"verified_native_palette_stages="<<bound<<'\n'<<std::flush;
        if(bound==4) {
            // Rejection controls use a separate declared API bank and a real
            // renderer resource. They cannot replace this world's bindings.
            SourceRuntimeMemory isolated,foreign;
            isolated.mount(0x474da8u,memory.view(0x474da8u,1024),SourceRuntimeMemory::Access::ReadOnly,"actual palette input");
            isolated.mount(0x4915dcu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"native palette result");
            isolated.mount(0x3000000u,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,"declared palette API caller");
            const auto put=[&](std::uint32_t at,std::uint32_t value) {
                std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));isolated.write(at,bytes);
            };
            bool absent{},duplicate{};
            try {NativeTu15RendererPalette missing(isolated,{});}catch(const std::runtime_error &){absent=true;}
            NativeTu15RendererPalette api(isolated,palettes);
            try {NativeTu15RendererPalette again(isolated,palettes);}catch(const std::invalid_argument &){duplicate=true;}
            SourcePhysicsWorldRuntimeAbi entry{};entry.gp[4]=0x3000000u;entry.eflags=0x8c7u;
            put(entry.gp[4],0x15b6du);put(entry.gp[4]+4,api.handle());
            unsigned rejected{};
            const auto reject=[&](auto &bank,SourcePhysicsContactRuntimeCall call,SourcePhysicsWorldRuntimeAbi input) {
                const auto gp=input.gp;const auto xmm=input.xmm;const auto flags=input.eflags;bool failed{};
                try {api.try_call(bank,call,input);}catch(const std::invalid_argument &){failed=true;}
                if(!failed || input.gp!=gp || input.xmm!=xmm || input.eflags!=flags || api.bindings()!=0)
                    throw FormatError("Rejected BSP palette API request changed its caller or native stage state");
                ++rejected;
            };
            reject(foreign,{0x15b68u,0x4025b0u,false},entry);
            reject(isolated,{0x15b68u,0x4025b0u,true},entry);
            reject(isolated,{0x15b69u,0x4025b0u,false},entry);
            auto changed=entry;changed.gp[0]=4;reject(isolated,{0x15b68u,0x4025b0u,false},changed);
            changed=entry;changed.gp[0]=1;reject(isolated,{0x15b68u,0x4025b0u,false},changed);
            put(entry.gp[4],0x15b6eu);reject(isolated,{0x15b68u,0x4025b0u,false},entry);put(entry.gp[4],0x15b6du);
            put(entry.gp[4]+4,api.handle()+1u);reject(isolated,{0x15b68u,0x4025b0u,false},entry);put(entry.gp[4]+4,api.handle());
            put(entry.gp[4],0x12602u);reject(isolated,{0x125fdu,0x4025b0u,false},entry);put(entry.gp[4],0x15b6du);
            bool wrong_thread{};
            std::thread outsider([&]{try {auto input=entry;api.try_call(isolated,{0x15b68u,0x4025b0u,false},input);}
                catch(const std::invalid_argument &){wrong_thread=true;}});outsider.join();
            if(!absent || !duplicate || !wrong_thread || rejected!=8 || api.bindings()!=0)
                throw FormatError("BSP palette rejection controls did not preserve native resource ownership");
            for(unsigned stage=0;stage<4;++stage)if(palettes.bound(stage).native_address()!=*identity)
                throw FormatError("Rejected palette API request replaced an actual world stage");
            std::cout<<"palette_rejection_controls=11\n";
            {
                // Preserve the existing cold native API route as well. These
                // are declared API caller inputs, not a completed Xbox boot.
                const auto world_palette=palettes.bound(0);
                SourceRuntimeMemory cold;
                cold.mount(0x515e90u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"cold native device output");
                cold.mount(0x474da8u,memory.view(0x474da8u,1024),SourceRuntimeMemory::Access::ReadOnly,"actual cold palette input");
                cold.mount(0x4915dcu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"cold palette output");
                cold.mount(0x3000000u,std::array<std::byte,128>{},SourceRuntimeMemory::Access::ReadWrite,"declared cold native graphics caller");
                const auto cold_put=[&](std::uint32_t at,std::uint32_t value) {
                    std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));cold.write(at,bytes);
                };
                SourcePhysicsWorldRuntimeAbi call{};call.gp[0]=0x40u;call.gp[1]=0x515e90u;call.gp[4]=0x3000000u;
                cold_put(call.gp[4],0x129b2u);cold_put(call.gp[4]+4,call.gp[4]+0x14u);
                constexpr std::array<std::uint32_t,17> presentation{640,480,6,0,0,1,0,0,1,42,1,60,1,0,0,0,0};
                for(unsigned n=0;n<presentation.size();++n)cold_put(call.gp[4]+0x14u+n*4u,presentation[n]);
                NativeTu15GraphicsDevice device(cold,renderer->device_lease(),renderer->target_resources());
                if(!device.try_call(cold,{0x129adu,0x3ff320u,false},call) || call.gp[0]!=0 ||
                   call.gp[4]!=0x3000008u || !device.initialized())throw FormatError("Cold native graphics API admission regressed");
                NativeTu15RendererPalette cold_palette(cold,device,palettes);
                call={};call.gp[0]=3;call.gp[4]=0x3000000u;
                cold_put(call.gp[4],0x125ebu);cold_put(call.gp[4]+4,0x474da8u);cold_put(call.gp[4]+8,0x4915dcu);
                if(!cold_palette.try_call(cold,{0x125e6u,0x1d3d0u,false},call) || call.gp[0]!=1 ||
                   call.gp[4]!=0x300000cu || !cold_palette.initialized())throw FormatError("Cold native palette creation regressed");
                constexpr std::array<std::uint32_t,4> sites{0x125fdu,0x1260eu,0x1261fu,0x1262fu};
                for(unsigned stage=0;stage<4;++stage) {
                    call={};call.gp[0]=stage;call.gp[4]=0x3000000u;call.eflags=0x8c7u;
                    cold_put(call.gp[4],sites[stage]+5u);cold_put(call.gp[4]+4,cold_palette.handle());
                    if(!cold_palette.try_call(cold,{sites[stage],0x4025b0u,false},call) || call.gp[0]!=stage ||
                       call.gp[4]!=0x3000008u || call.eflags!=0x8c7u ||
                       palettes.bound(stage).native_address()!=cold_palette.native_address())
                        throw FormatError("Cold native palette stage binding or caller ABI regressed");
                }
                if(cold_palette.bindings()!=4)throw FormatError("Cold native palette route lost a binding");
                for(unsigned stage=0;stage<4;++stage)palettes.bind(stage,world_palette);
                std::cout<<"cold_palette_api_regression_passed=1\n";
            }
            const auto read=[](const auto &p){std::ifstream in(p,std::ios::binary);return std::vector<char>{std::istreambuf_iterator<char>(in),{}};};
            if(!renderer->draw(camera,output/"control.ppm"))throw FormatError("Palette probe control GPU draw failed");
            const auto before=read(output/"before.ppm"),control=read(output/"control.ppm");
            if(before.empty() || before!=control)throw FormatError("Palette probe baseline changed before texture update");
            const std::array<std::byte,1> index{std::byte{0}};
            renderer->update_palettized_image(0,0,index,1,1,false);
            if(!renderer->draw(camera,output/"after.ppm"))throw FormatError("Bound original palette did not finish its GPU draw");
            const auto after=read(output/"after.ppm");
            if(control==after)throw FormatError("Bound original palette did not change the native GPU image");
            std::cout<<"palette_gpu_image_changed=1\n"<<std::flush;
            renderer->release_presentation();
            auto retired=entry;bool refused{};
            try {api.try_call(isolated,{0x15b68u,0x4025b0u,false},retired);}
            catch(const std::runtime_error &){refused=true;}
            if(!refused || retired.gp!=entry.gp || api.initialized() || palettes.valid())
                throw FormatError("Retired renderer admitted a palette API request or changed its caller");
            std::cout<<"retired_palette_owner_rejected=1\n"<<std::flush;
        }
    };
    std::uint32_t result{};
    try {
        if(bsp)result=world.call_actual(0x13a820u);
        else if(sound) {
            // Literal12804B..128050 supplies this authored policy to the
            // complete original driver constructor. Missing device/platform
            // ownership stops with its real partial state and caller ABI.
            result=world.call_actual(0x224ec0u,std::array{0x455b20u});
            if(!(result&255u))throw FormatError("Original sound driver constructor failed; no sound readiness admitted");
        }
        else {
            // Retain the native caller's actually admitted options separately
            // while the complete original map-start/reset owner consumes them.
            // No game/BSP active flag or successful callee result is supplied.
            constexpr std::uint32_t options=0x60080000u;
            const auto admitted=memory.view(game+8u,0x1118u);
            memory.write(options,admitted);
            auto input=world.prepare_source_call();input.gp[0]=options;
            world.run_actual(0x191df0u,input);result=input.gp[0]&255u;
        }
    }catch(const std::exception &failure) {
        report("stopped");
        // Faulted SDK resources can also reject a registered exit callback.
        // Preserve the initiating failure before world teardown reports that
        // secondary failure; neither path is successful activation.
        std::cerr<<"original_failure="<<failure.what()<<std::endl;
        report_palettes();throw;
    }
    report("after");
    report_palettes();
    std::cout<<"original_return="<<result<<"\nLIMIT isolated activation probe; completion, profile save and actual next-level loading remain separate\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

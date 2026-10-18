#define SDL_MAIN_HANDLED
#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/renderer.hpp"
#include <SDL.h>
#include <cstring>
#include <iostream>
#include <optional>
#include <thread>
using namespace halo2;
namespace {
template<class F> void rejected(F &&action,const char *message) {
    bool stopped=false;try{action();}catch(const std::exception &){stopped=true;}
    if(!stopped)throw FormatError(message);
}
SourcePhysicsWorldRuntimeAbi caller(unsigned first,unsigned data,unsigned sp) {
    SourcePhysicsWorldRuntimeAbi a{};a.gp={0x11,first,data,0x89abcdef,sp,0x76543210,0x13572468,0x24681357};
    a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};return a;
}
}
int main(int argc,char **argv)try {
    if(argc!=2 || !std::filesystem::exists(std::filesystem::path(argv[1])/"world.vert.spv"))return 77;
    SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());
    auto *window=SDL_CreateWindow("Source particle graphics ownership",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    if(!window)throw FormatError(SDL_GetError());
    SourceRuntimeMemory memory;memory.mount(0x60000000,std::vector<std::byte>(65536),
        SourceRuntimeMemory::Access::ReadWrite,"declared native SDK constant interface");
    mount_source_particle_graphics_layout_data_tu15(memory);
    for(const auto [p,n]:std::array<std::pair<unsigned,unsigned>,4>{{
        {0x4918e4,12},{0x492294,1},{0x4922f4,16},{0x4c2aa0,8}}})
        memory.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,
            "declared native view guard inputs; current shader/viewport remains unowned");
    std::array<ShaderVector,192> inherited{},input{},output{};
    for(unsigned n=0;n<192;++n)for(unsigned k=0;k<4;++k)input[n][k]=float(n*4+k+1);
    memory.write(0x60000100,std::as_bytes(std::span(input)));
    const std::array<unsigned,2> stack{0x25d25,768};memory.write(0x60008000,std::as_bytes(std::span(stack)));
    std::optional<SourceParticleGraphicsBindingTu15> binding;
    {
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(material);
        Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,argv[1]);
        const auto lease=renderer.device_lease();const auto control=renderer.constant_mode_control();auto targets=renderer.target_resources();
        rejected([&]{SourceParticleGraphicsBindingTu15 no_device({},control,targets.acquire());},"An unissued device lease was accepted");
        rejected([&]{SourceParticleGraphicsBindingTu15 no_control(lease,{},targets.acquire());},"An unissued constant control was accepted");
        binding.emplace(lease,control,targets.acquire());binding->begin(memory,inherited);
        std::array<std::byte,4> data{};
        rejected([&]{binding->read(0x411568,data);},"Unconfigured source graphics became visible");
        auto configure=caller(0,0,0x60008000);binding->configure(configure);
        if(!binding->configured() || configure.gp[0]!=1 || configure.gp[1]!=0x200 || configure.gp[4]!=0x60008004 ||
           binding->device_address()!=lease.acquire().native_address())throw FormatError("Whole400EF0/native renderer configuration differs");
        binding->read(0x411568,data);if(Bytes(data).u32(0)!=0x40f0c0)throw FormatError("Original source device alias differs");
        // A collection root must enter through its actual World/pass caller.
        // Adding shared downstream owners must not expose a queue-reset path.
        auto forbidden=caller(0,0,0x60008000);const auto untouched=forbidden;
        const std::array<unsigned,2> queue_canary{17,19};memory.write(0x4c2aa0,std::as_bytes(std::span(queue_canary)));
        unsigned root_instructions=0;SourcePhysicsWorldRuntimeServices root_services;
        root_services.instruction=[&](auto){++root_instructions;};
        const auto prior_commands=binding->commands().size();
        rejected([&]{binding->run(0x40e80,forbidden,root_services);},"A collection root bypassed its actual pass admission");
        if(root_instructions || forbidden.gp!=untouched.gp || forbidden.xmm!=untouched.xmm ||
           forbidden.eflags!=untouched.eflags || binding->commands().size()!=prior_commands ||
           std::memcmp(memory.view(0x4c2aa0,8).data(),queue_canary.data(),8))
            throw FormatError("Rejected collection root changed its queue, ABI or command bank");
        auto upload=caller(0,0x60000100,0x60008000);binding->run(0x401410,upload);binding->consume_constants(output);
        if(std::memcmp(input.data(),output.data(),sizeof(input)))throw FormatError("Original packets did not reach the renderer-owned constant consumer");
        rejected([&]{(void)binding->depth_configuration();},"Unconfigured depth became visible");
        rejected([&]{binding->configure_depth({});},"An unissued depth control was accepted");
        binding->configure_depth(renderer.depth_mode_control());
        const auto &depth=binding->depth_configuration();
        if(depth.mode!=2 || depth.depth_bits!=24 || depth.stencil_bits!=8 ||
           depth.attachment_handle!=targets.acquire().bank().depth_stencil.image)
            throw FormatError("Particle depth is not its actual renderer attachment");
        const auto &layout=binding->depth_layout();const auto attachment=targets.acquire().bank().depth_stencil;
        if(layout.width!=attachment.width || layout.height!=attachment.height || layout.normalized_format!=0x2e ||
           ((layout.format_word>>8)&255)!=0x2e || layout.depth_scale_bits!=0x4b7fffff)
            throw FormatError("Original D24S8 layout does not name the actual depth attachment metadata");
        rejected([&]{binding->configure_depth(renderer.depth_mode_control());},"Duplicate depth construction was accepted");
        rejected([&]{binding->read(0x410ac8,data);},"An unowned depth pointer reader was accepted");
        rejected([&]{binding->read(0xf900038e,data);},"An opaque depth identity became a made-up SDK header");
        rejected([&]{binding->read(0x4116c0,data);},"An unconstructed depth control cache was treated as zero");
        const std::array<unsigned,2> depth_stack{0x25d25,2};memory.write(0x60008000,std::as_bytes(std::span(depth_stack)));
        auto depth_call=caller(0,0,0x60008000);unsigned last_pc{};SourcePhysicsWorldRuntimeServices depth_services;
        depth_services.instruction=[&](auto p){last_pc=p;};std::string depth_stop;
        try{binding->run(0x3ff5d0,depth_call,depth_services);}catch(const FormatError &e){depth_stop=e.what();}
        if(last_pc!=0x3ff611 || depth_stop.find("13430")==std::string::npos || depth_call.gp[1]!=1)
            throw FormatError("Original depth setter did not use the real surface or stop at its actual control owner");
        // An unconfigured camera must fail before the original DIV rather
        // than turning zero boot bytes into NaN depth bounds. A valid declared
        // native plane pair reaches the next actual SDK field, not success.
        binding->begin(memory,inherited);
        const std::array<unsigned,3> view_record{1,0,0};memory.write(0x60000900,std::as_bytes(std::span(view_record)));
        const std::array<unsigned,2> occupied_queues{17,19};memory.write(0x4c2aa0,std::as_bytes(std::span(occupied_queues)));
        auto initialize=caller(0x60000900,0,0x60008000);last_pc=0;std::string view_stop;
        try{binding->run(0x13430,initialize,depth_services);}catch(const FormatError &e){view_stop=e.what();}
        if(last_pc!=0x13438 || view_stop.find("169D0")==std::string::npos)
            throw FormatError("Unconstructed camera planes reached original depth arithmetic");
        const std::array<unsigned,2> configured_planes{0x3c800000,0x44800000};memory.write(0x4922f4,std::as_bytes(std::span(configured_planes)));
        initialize=caller(0x60000900,0,0x60008000);last_pc=0;view_stop.clear();
        try{binding->run(0x13430,initialize,depth_services);}catch(const FormatError &e){view_stop=e.what();}
        if(last_pc!=0x1357a || view_stop.find("410cd8")==std::string::npos)
            throw FormatError("View initializer did not stop at its actual unowned SDK depth-bounds flags");
        // Zero near is meaningful for original oblique cameras; only an
        // unconfigured/nonfinite far plane must close this native interface.
        const unsigned oblique_near=0;memory.write(0x4922f4,std::as_bytes(std::span(&oblique_near,1)));
        initialize=caller(0x60000900,0,0x60008000);last_pc=0;view_stop.clear();
        try{binding->run(0x13430,initialize,depth_services);}catch(const FormatError &e){view_stop=e.what();}
        if(last_pc!=0x1357a || view_stop.find("410cd8")==std::string::npos)
            throw FormatError("Valid zero-near source camera was rejected before its actual SDK owner");
        std::array<unsigned,2> after_queues{};memory.read(0x4c2aa0,std::as_writable_bytes(std::span(after_queues)));
        if(after_queues!=occupied_queues)throw FormatError("Incomplete original view initializer cleared collected queues");
        rejected([&]{binding->read(0x40f0d0,data);},"An unknown device field received invented data");
        rejected([&]{binding->read(0x40f0c8,data);},"An unknown flag reader received invented semantics");
        rejected([&]{binding->write(0x411568,data);},"Source code replaced the live renderer alias");
        rejected([&]{auto again=caller(0,0,0x60008000);binding->configure(again);},"Duplicate mode construction was accepted");
        SourceRuntimeMemory other;rejected([&]{binding->begin(other,inherited);},"A graphics frame crossed source worlds");
        rejected([&]{binding->begin(*binding,inherited);},"A recursive source graphics memory owner was accepted");
        bool foreign_read=false;std::thread reader([&]{try{binding->read(0x411568,data);}catch(const std::exception &){foreign_read=true;}});reader.join();
        if(!foreign_read)throw FormatError("A graphics frame crossed issuing threads");
        bool foreign_constructor=false;auto moved_targets=targets.acquire();
        std::thread constructor([&,moved=std::move(moved_targets)]() mutable {
            try{SourceParticleGraphicsBindingTu15 wrong_thread(lease,control,std::move(moved));}catch(const std::exception &){foreign_constructor=true;}
        });constructor.join();if(!foreign_constructor)throw FormatError("Foreign-thread renderer capabilities constructed source graphics");
        // Real source attachment recreation invalidates the old use even
        // though this logical device and constant control are still alive.
        const auto renewed=targets.source_buffers(2);(void)renewed;
        if(binding->configured())throw FormatError("Recreated targets retained an active particle graphics view");
        rejected([&]{binding->read(0x411568,data);},"A stale target generation remained readable");
        binding.emplace(lease,control,targets.acquire());binding->begin(memory,inherited);
        configure=caller(0,0,0x60008000);binding->configure(configure);
    }
    if(binding->configured())throw FormatError("Renderer retirement retained an active particle graphics view");
    std::array<std::byte,4> data{};rejected([&]{binding->read(0x411568,data);},"A retired renderer remained readable");
    binding.reset();SDL_DestroyWindow(window);SDL_Quit();
    std::cout<<"PASS live renderer/whole constant-mode and upload ownership, unissued/unknown/foreign/stale/retired rejection\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

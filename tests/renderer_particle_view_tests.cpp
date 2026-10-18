#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {
template<class F>void rejected(F &&f,const char *why) {
    bool stopped=false;try{f();}catch(const std::exception &){stopped=true;}
    if(!stopped)throw FormatError(why);
}
}
int main(int argc,char **argv)try {
    if(argc!=4 && argc!=5)throw FormatError("Supply shaders, Backwash map, shared maps and optional new evidence directory");
    const std::filesystem::path out=argc==5?argv[4]:"";
    if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve earlier view evidence");std::filesystem::create_directories(out);}
    if(!std::filesystem::exists(std::filesystem::path(argv[1])/"particle.vert.spv") || !std::filesystem::exists(argv[2]))return 77;
    Cache cache(argv[2],argv[3]);const auto render=cache.load_particle_system_render_definition(0xe76105ec,0,0);
    auto material=std::make_shared<const ParticleMaterial>(cache.load_particle_material(render.particle_datum,15,0,0));
    SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());
    auto *window=SDL_CreateWindow("Native particle view owner",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    if(!window)throw FormatError(SDL_GetError());
    RendererParticleView retired;std::optional<RendererParticleView::Use> held;
    rejected([&]{(void)retired.acquire(0);},"Unissued view observed a resource");
    {
        Mesh mesh;Material m;m.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(m);
        Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,argv[1]);auto control=renderer.particle_view_inputs();retired=control;
        rejected([&]{(void)control.acquire(0);},"Unprepared view observed a resource");
        // Declared resource/viewport fixture, not a particle simulation or
        // positive map-visibility acceptance. Every observed bit must come
        // from the actual mapped descriptor bank and command recording.
        std::array<ParticleDraw,2> draws;
        for(unsigned i=0;i<draws.size();++i) {
            auto &d=draws[i];d.material=material;
            d.vertex.constants[8]={160,0,0,float(160+i*320)+.53125f};
            d.vertex.constants[9]={0,-240,0,240.53125f};
            d.vertex.constants[10]={160,-240,16777215,0};
            d.vertex.constants[11]={float(160+i*320)+.5f,240.5f,0,0};
            d.vertex.clip_range={0,16777215,255.99998474121094f,16777215};
            d.depth.clip_range=d.vertex.clip_range;d.depth.parameters={0,0,1,1};
        }
        Camera camera;std::array<ModelView,2> views;
        for(unsigned i=0;i<views.size();++i){views[i].camera=camera;views[i].left=i*320;views[i].top=0;views[i].width=320;views[i].height=480;views[i].full_surface=true;views[i].world_particles=std::span(&draws[i],1);}
        if(!renderer.draw(camera,{}, {},false,{},{},{},views))throw FormatError("Native view fixture draw failed");
        auto a=control.acquire(0),b=control.acquire(1);held=a;
        const auto &x=a.state(),&y=b.state();
        if(x.viewport!=std::array<float,6>{0,0,640,480,0,1} || x.scissor!=std::array<std::int32_t,4>{0,0,320,480} ||
           y.scissor!=std::array<std::int32_t,4>{320,0,320,480} || x.uniform_buffer!=y.uniform_buffer || x.descriptor==y.descriptor ||
           x.uniform_backing==y.uniform_backing || x.vertex_offset==y.vertex_offset || x.depth_offset<=x.vertex_offset ||
           x.surface_width!=640 || x.surface_height!=480 || x.pipeline!=y.pipeline ||
           a.device_address()!=renderer.device_lease().acquire().native_address())
            throw FormatError("Native view observations do not name their actual independent draw resources");
        for(unsigned i=0;i<2;++i) {
            const auto &s=i?y:x;const auto &d=draws[i];
            if(std::memcmp(s.viewport_constants.data(),d.vertex.constants.data()+8,64) ||
               std::memcmp(s.vertex_clip.data(),d.vertex.clip_range.data(),16) ||
               std::memcmp(s.depth_clip.data(),d.depth.clip_range.data(),16) ||
               std::memcmp(s.depth_parameters.data(),d.depth.parameters.data(),16))
                throw FormatError("Observed mapped GPU view bits differ from descriptor inputs");
        }
        rejected([&]{(void)control.acquire(2);},"Out-of-range view was accepted");
        auto consumed=draws;
        for(unsigned row=0;row<2;++row) {
            SourceRuntimeMemory memory;mount_source_particle_graphics_layout_data_tu15(memory);
            for(const auto [p,n]:std::array<std::pair<unsigned,unsigned>,7>{{{0x4918e4,12},{0x492294,1},
                {0x4922f4,16},{0x4c2aa0,32},{0x4c4b64,4},{0x4c515c,4},{0x60000000,65536}}})
                memory.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,
                    "declared original view caller globals and empty pre-collection queue fixture");
            const std::array<float,2> planes{.015625f,1024};memory.write(0x4922f4,std::as_bytes(std::span(planes)));
            const std::array<unsigned,3> frame{17,row,row?0x42000000u:0u};memory.write(0x60000100,std::as_bytes(std::span(frame)));
            const unsigned stop=0x60300000;memory.write(0x60008000,std::as_bytes(std::span(&stop,1)));
            SourceParticleGraphicsBindingTu15 binding(renderer.device_lease(),renderer.constant_mode_control(),renderer.target_resources().acquire());
            binding.begin(memory,draws[row].vertex.constants);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x11;abi.gp[4]=0x60008000;abi.eflags=0x202;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};binding.configure(abi);
            binding.configure_depth(renderer.depth_mode_control());binding.stage_program(renderer.particle_program_control(),material);
            const auto path=out/std::to_string(row);if(!out.empty())std::filesystem::create_directory(path);
            const auto bank=[&](const char *name){std::ofstream f(path/name,std::ios::binary);const auto emit=[&](unsigned p,std::span<const std::byte>b){unsigned n=b.size();f.write(reinterpret_cast<const char*>(&p),4);f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);};memory.visit_committed(emit);binding.visit_committed(emit);};
            abi.gp[1]=0x60000100;abi.gp[4]=0x60008000;const auto before=abi;
            const unsigned occupied=1,empty=0;memory.write(0x4c2aa0,std::as_bytes(std::span(&occupied,1)));
            rejected([&]{binding.initialize_view(control.acquire(row),abi);},"View reset erased a collected queue");
            if(Bytes(memory.view(0x4c2aa0,4)).u32(0)!=occupied)throw FormatError("Rejected view reset changed collected work");
            memory.write(0x4c2aa0,std::as_bytes(std::span(&empty,1)));
            std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;
            services.instruction=[&](unsigned pc){if(trace.empty() && !out.empty())bank("before.bin");trace.push_back(pc);};
            services.required_owner=[](auto &,const auto &,auto &,const auto &){throw FormatError("View initializer reached an unowned original callee");};
            binding.initialize_view(control.acquire(row),abi,services);
            auto produced=draws[row];binding.consume_draw(produced);
            consumed[row]=produced;
            if(row==0 && std::memcmp(&produced.vertex,&draws[row].vertex,sizeof(produced.vertex)))throw FormatError("Original view0 changed its actual shader inputs");
            if(row==1 && (produced.depth.clip_range[2]<=draws[row].depth.clip_range[2] || produced.depth.clip_range[3]!=16777215.f))
                throw FormatError("Original boundary1 did not publish its authored clip change");
            rejected([&]{binding.initialize_view(control.acquire(row),abi);},"Duplicate view reset was accepted");
            std::array<std::byte,4> bytes;rejected([&]{binding.read(0x4116c0,bytes);},"Unowned control-cache reader was accepted");
            if(!out.empty()) {
                bank("after.bin");const auto state=[](const auto &a){return nlohmann::json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};};
                std::ofstream(path/"case.json")<<nlohmann::json{{"entry",0x13430},{"before",state(before)},{"after",state(abi)},
                    {"instructions",trace},{"return_address",stop},{"completed",true},{"missing_owner",0},{"row",row},
                    {"native_view_observed",true},{"original_boot_history_claimed",false},{"command_bytes",binding.commands().size()},
                    {"consumed_depth_clip",produced.depth.clip_range},{"game_returns_supplied",nlohmann::json::array()},
                    {"gpu_returns_supplied",nlohmann::json::array()}}.dump(2)<<'\n';
            }
        }
        bool thread_rejected=false;std::thread worker([&]{thread_rejected=!a.valid();rejected([&]{(void)a.state();},"View crossed owning threads");});worker.join();
        if(!thread_rejected)throw FormatError("View lifetime crossed owning threads");
        for(unsigned i=0;i<2;++i)views[i].world_particles=std::span(&consumed[i],1);
        if(!renderer.draw(camera,{}, {},false,{},{},{},views))throw FormatError("Repeated view fixture draw failed");
        if(a.valid() || b.valid())throw FormatError("Prepared uniforms retained an earlier observation");
        for(unsigned i=0;i<2;++i) {
            const auto next=control.acquire(i);
            if(next.state().vertex_clip!=consumed[i].vertex.clip_range || next.state().depth_clip!=consumed[i].depth.clip_range)
                throw FormatError("Original view packet did not reach the actual GPU descriptor inputs");
        }
        rejected([&]{(void)a.state();},"Stale uniform pointer remained accessible");
        held=control.acquire(1);auto renewed=renderer.target_resources().source_buffers(2);(void)renewed;
        if(held->valid())throw FormatError("Recreated attachments retained an earlier view");
    }
    if(retired.valid() || held->valid())throw FormatError("Retired renderer retained a particle view");
    rejected([&]{(void)retired.acquire(0);},"Retired renderer exposed a view");
    SDL_DestroyWindow(window);SDL_Quit();
    std::cout<<"PASS actual descriptor-backed particle view bits, split scissor, independent offsets, unissued/unprepared/range/thread/prepare/target/retirement guards\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

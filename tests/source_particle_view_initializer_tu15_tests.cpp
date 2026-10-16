#include "halo2/source_particle_constant_storage_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
Json state(const SourcePhysicsWorldRuntimeAbi &a) {
    return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},
        {"x87_depth",a.x87_depth},{"x87_status",a.x87_status},
        {"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};
}
void export_bank(const std::filesystem::path &p,const SourceRuntimeMemory &source,
    const SourceParticleConstantStorageTu15 &storage) {
    std::ofstream out(p,std::ios::binary);const auto emit=[&](unsigned address,std::span<const std::byte> bytes) {
        const unsigned size=bytes.size();out.write(reinterpret_cast<const char*>(&address),4);
        out.write(reinterpret_cast<const char*>(&size),4);out.write(reinterpret_cast<const char*>(bytes.data()),size);
    };source.visit_committed(emit);storage.visit_committed(emit);if(!out)throw FormatError("Cannot export original view initializer");
}
}
int main(int argc,char **argv)try {
    if(argc>2)throw FormatError("Optional new view-initializer diagnostic directory");
    const auto output=argc==2?std::filesystem::path(argv[1]):std::filesystem::path{};
    if(!output.empty()) {if(std::filesystem::exists(output))throw FormatError("Preserve earlier view initializer evidence");std::filesystem::create_directories(output);}
    constexpr unsigned device=0x40f0c0,scratch=0x60000000,sp=scratch+0x8000,stop=0x60300000;
    unsigned count=0;
    for(const auto [fvf,boundary,cut,offset]:std::array<std::array<unsigned,4>,8>{{
        {2,0,0,0},{4,0,0,0},{2,1,0x43c80000,1},{4,1,0x43c80000,1},
        {2,1,0xbf800000,0},{4,1,0x45000000,0},{2,2,0x43c80000,0},{4,2,0x43c80000,1}}}) {
        SourceRuntimeMemory memory;mount_source_particle_graphics_layout_data_tu15(memory);
        for(const auto [p,n]:std::array<std::pair<unsigned,unsigned>,11>{{
            {device+8,0x2498},{0x411568,4},{0x4116c0,4},{0x4117b4,4},
            {0x4918e4,12},{0x4920d8,4},{0x492294,1},{0x4922f4,16},{0x4c2aa0,32},{0x4c4b64,4},{0x4c515c,4}}})
            memory.mount(p,std::vector<std::byte>(n,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
                "declared CPU-only view, current FVF and SDK field inputs; not original boot or native GPU state");
        memory.mount(scratch,std::vector<std::byte>(65536,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
            "declared native metadata stack and arguments");
        const auto put=[&](unsigned p,unsigned v){memory.write(p,std::as_bytes(std::span(&v,1)));};
        const auto word=[&](unsigned p){return Bytes(memory.view(p,4)).u32(0);};
        put(0x411568,device);put(device+8,0x200);put(0x4117b4,2);
        put(device+0x794,scratch+0x400);put(device+0x95c,0x3f800000);put(device+0x960,0x3f800000);
        put(device+0x944,0x3a800000);put(device+0x948,0x4b7fffff);put(device+0x93c,0x3c800000);
        put(device+0xee0,offset?320:0);put(device+0xee4,offset?240:0);
        put(device+0xee8,offset?320:640);put(device+0xeec,offset?240:480);
        put(device+0xef0,0);put(device+0xef4,0x3f800000);put(device+0xef8,0x3f000000);put(device+0xefc,0x3f000000);
        put(device+0x1c18,offset);put(0x4922f4,0x3c800000);put(0x4922f8,0x44800000);
        put(0x4c2aa0,17);put(0x4c2aa4,19);
        const std::array<unsigned,3> view{73+count,boundary,cut};memory.write(scratch+0x80,std::as_bytes(std::span(view)));
        SourceParticleConstantStorageTu15 storage;std::array<ShaderVector,192> inherited{};storage.begin(memory,inherited);
        // The shader flags are produced by the whole original FVF builder,
        // not chosen to skip400270. This is an explicit CPU layout input;
        // no Vulkan current-shader binding or SDK construction is claimed.
        SourcePhysicsWorldRuntimeAbi header{};header.gp[0]=scratch+0x400;header.gp[1]=fvf;header.gp[4]=sp;
        header.eflags=0x202;header.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(sp,stop);
        source_run_effect_particle_draw_tu15(storage,0x400a60,header,*header.floating_point,{});
        SourcePhysicsWorldRuntimeAbi a{};a.gp={0x12345678,scratch+0x80,0xabcdef01,0x89abcdef,sp,0x76543210,0x13572468,0x24681357};
        a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(sp,stop);
        const auto before=a;std::vector<unsigned> trace;unsigned missing{};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto p){trace.push_back(p);};
        services.required_owner=[&](auto &,const auto &call,auto &,const auto &) {
            missing=call.target;throw FormatError("Original view initializer reached required render-target/clear owner");
        };
        const auto path=output/std::to_string(count++);if(!output.empty()){std::filesystem::create_directory(path);export_bank(path/"before.bin",memory,storage);}
        bool complete=true;try{source_run_effect_particle_draw_tu15(storage,0x13430,a,*a.floating_point,services);}catch(const FormatError &){complete=false;}
        if(boundary==2) {
            if(complete || missing!=0x14ba0 || word(0x4c2aa0)!=17 || word(0x4c2aa4)!=19)
                throw FormatError("Boundary2 concealed its unowned target service or cleared queues before it");
        }else {
            if(!complete || missing || a.gp[4]!=sp+4 || a.gp[3]!=before.gp[3] || a.gp[6]!=before.gp[6] ||
               a.gp[7]!=before.gp[7] || (a.gp[0]&255)!=1 || a.x87_depth)
                throw FormatError("Whole original view initializer ABI differs");
            if(word(0x4116c0) || word(0x4c4b64) || word(0x4c2aa0) || word(0x4c2aa4) || word(0x4c515c) ||
               memory.view(0x4c2ab0,2)[0]!=std::byte{1} || memory.view(0x4c2ab0,2)[1]!=std::byte{1} ||
               !std::equal(memory.view(0x4918e4,12).begin(),memory.view(0x4918e4,12).end(),std::as_bytes(std::span(view)).begin()))
                throw FormatError("Original view/control/queue reset state differs");
            if(storage.commands().size()!=(fvf==2?40:20))throw FormatError("Original viewport/depth/control emission differs");
        }
        if(!output.empty()) {
            export_bank(path/"after.bin",memory,storage);
            std::ofstream(path/"case.json")<<Json{{"entry",0x13430},{"before",state(before)},{"after",state(a)},
                {"instructions",trace},{"return_address",complete?stop:missing},{"completed",complete},{"missing_owner",missing},
                {"fvf",fvf},{"boundary",boundary},{"emitted_bytes",storage.commands().size()},
                {"caller_history","Declared CPU current-FVF/viewport inputs; no GPU or original boot history"},
                {"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()}}.dump(2)<<'\n';
        }
    }
    std::cout<<"PASS6 complete original per-view initializers and2 exact required-target boundaries; original queue clearing, FVF/viewport/depth emission and ABI contracts\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

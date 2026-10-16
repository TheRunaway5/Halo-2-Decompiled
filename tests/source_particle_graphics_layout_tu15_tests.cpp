#include "halo2/source_particle_graphics_binding_tu15.hpp"
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
void export_bank(const std::filesystem::path &p,const SourceRuntimeMemory &memory) {
    std::ofstream out(p,std::ios::binary);
    memory.visit_committed([&](unsigned address,std::span<const std::byte> bytes) {
        const unsigned size=bytes.size();out.write(reinterpret_cast<const char*>(&address),4);
        out.write(reinterpret_cast<const char*>(&size),4);out.write(reinterpret_cast<const char*>(bytes.data()),size);
    });if(!out)throw FormatError("Cannot export original graphics layout arithmetic");
}
}
int main(int argc,char **argv)try {
    if(argc>2)throw FormatError("Optional new layout diagnostic directory");
    const auto output=argc==2?std::filesystem::path(argv[1]):std::filesystem::path{};
    if(!output.empty()) {
        if(std::filesystem::exists(output))throw FormatError("Preserve previous graphics layout evidence");
        std::filesystem::create_directories(output);
    }
    constexpr unsigned scratch=0xf7600000,sp=scratch+0x800,stop=scratch+0xffc;
    unsigned count=0;
    // Actual interface dimensions and odd-sized/maximum legal linear layouts.
    for(const auto [w,h]:std::array<std::pair<unsigned,unsigned>,6>{{
        {640,480},{1280,720},{1920,1080},{639,479},{1,1},{4096,4096}}}) {
        const auto layout=source_particle_d24s8_layout_tu15(w,h);
        if(layout.width!=w || layout.height!=h || layout.normalized_format!=0x2e ||
           ((layout.format_word>>8)&255)!=0x2e || layout.depth_scale_bits!=0x4b7fffff || !layout.storage_bytes || !layout.row_pitch)
            throw FormatError("D24S8 original/native metadata contract differs");
    }
    for(const auto [w,h]:std::array<std::pair<unsigned,unsigned>,3>{{{0,480},{640,0},{4097,480}}}) {
        bool rejected=false;try{(void)source_particle_d24s8_layout_tu15(w,h);}catch(const FormatError &){rejected=true;}
        if(!rejected)throw FormatError("Unrepresentable native depth dimensions were accepted");
    }
    const auto run=[&](unsigned entry,unsigned eax,unsigned ecx,unsigned edx,std::vector<unsigned> args,
                       std::optional<unsigned> format=std::nullopt) {
        SourceRuntimeMemory memory;mount_source_particle_graphics_layout_data_tu15(memory);
        memory.mount(scratch,std::vector<std::byte>(4096,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
            "declared native pure graphics layout inputs and stack");
        args.insert(args.begin(),stop);memory.write(sp,std::as_bytes(std::span(args)));
        if(format) {const unsigned word=*format<<8;memory.write(scratch+0x20c,std::as_bytes(std::span(&word,1)));}
        SourcePhysicsWorldRuntimeAbi a{};a.gp={eax,ecx,edx,0x89abcdef,sp,0x76543210,0x13572468,0x24681357};
        a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        const auto before=a;std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto p){trace.push_back(p);};
        services.required_owner=[](auto &,const auto &,auto &,const auto &) {
            throw FormatError("Pure graphics layout reached an unowned service");
        };
        const auto path=output/std::to_string(count++);
        if(!output.empty()){std::filesystem::create_directory(path);export_bank(path/"before.bin",memory);}
        source_run_effect_particle_draw_tu15(memory,entry,a,*a.floating_point,services);
        if(a.gp[4]!=sp+args.size()*4 || a.gp[3]!=before.gp[3] || a.gp[5]!=before.gp[5] ||
           a.gp[6]!=before.gp[6] || a.gp[7]!=before.gp[7] || a.x87_depth)
            throw FormatError("Original graphics layout stack/nonvolatile contract differs");
        if(entry==0x407ee0 && args.at(3)==0x2e && args.at(4)!=0) {
            const auto metadata=source_particle_d24s8_layout_tu15(eax,edx);
            if(metadata.row_pitch!=args.at(4) || metadata.storage_bytes!=a.gp[0] ||
               metadata.format_word!=Bytes(memory.view(scratch+0x100,4)).u32(0) ||
               metadata.size_word!=Bytes(memory.view(scratch+0x104,4)).u32(0))
                throw FormatError("Actual native depth metadata did not use the original constructor pitch/layout chain");
        }
        if(!output.empty()) {
            export_bank(path/"after.bin",memory);
            std::ofstream(path/"case.json")<<Json{{"entry",entry},{"before",state(before)},{"after",state(a)},
                {"instructions",trace},{"return_address",stop},{"caller_history","Declared native pure metadata interface; no original405CF0 allocation/boot/GPU history"},
                {"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()}}.dump(2)<<'\n';
        }
    };
    for(const auto f:{2u,6u,42u,44u,45u,0xffffu})run(0x407860,f,0x12345678,0xabcdef01,{});
    for(const auto f:{6u,0x2eu})for(const auto w:{1u,640u,1920u})run(0x407900,f,0x12345678,0xabcdef01,{w});
    for(const auto f:{0x2eu,0x2au,0x0cu})run(0x407ee0,f==0x2e?640:512,0x12345678,f==0x2e?480:256,
        {1,1,f,0,0,0,scratch+0x100,scratch+0x104});
    for(unsigned f=0x2a;f<=0x31;++f)run(0x407a50,scratch+0x100,scratch+0x200,0xabcdef01,{},f);
    for(const auto f:{0u,2u,4u,6u,8u,14u,0x112u,0x1d2u,0x00220802u})
        run(0x400a60,scratch+0x100,f,0xabcdef01,{});
    for(const auto [w,h,pitch]:std::array<std::array<unsigned,3>,2>{{{640,480,2560},{1920,1080,8192}}})
        run(0x407ee0,w,0x12345678,h,{1,1,0x2e,pitch,0,0,scratch+0x100,scratch+0x104});
    std::cout<<"PASS "<<count<<" complete original graphics layout calls; D24S8 dimensions, all8 depth dispatch slots and FVF arithmetic; no game/GPU returns supplied\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

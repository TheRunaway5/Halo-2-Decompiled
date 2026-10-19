#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_particle_constant_storage_tu15.hpp"
#include "halo2/source_particle_world_execution_tu15.hpp"
#include "halo2/source_particle_target_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr unsigned device=0x40f0c0, scratch=0x60000000, sp=scratch+0x8000;
constexpr unsigned stop=0x60300000, packet=scratch+0x1000;
void put(SourceRuntimeMemory &m,unsigned p,unsigned v) {
    m.write(p,std::as_bytes(std::span(&v,1)));
}
unsigned word(const SourceRuntimeMemory &m,unsigned p) {return Bytes(m.view(p,4)).u32(0);}
Json state(const SourcePhysicsWorldRuntimeAbi &a) {
    return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},
        {"x87_depth",a.x87_depth},{"x87_status",a.x87_status},
        {"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};
}
void bank(const std::filesystem::path &p,const SourceRuntimeMemory &m,
    const SourceParticleConstantStorageTu15 &storage) {
    std::ofstream f(p,std::ios::binary);
    const auto emit=[&](unsigned a,std::span<const std::byte> b) {
        const unsigned n=b.size();f.write(reinterpret_cast<const char*>(&a),4);
        f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);
    };
    m.visit_committed(emit);storage.visit_committed(emit);
    if(!f)throw FormatError("Cannot export original view-context evidence");
}
void inputs(SourceRuntimeMemory &m) {
    mount_source_particle_graphics_layout_data_tu15(m);
    mount_source_particle_target_data_tu15(m);
    for(const auto [p,n]:std::array<std::pair<unsigned,unsigned>,16>{{
        {device+8,0x1c20},{0x411568,4},{0x4117b8,0x50},{0x473570,4},
        {0x473628,8},{0x491e20,0x298},{0x4920d8,4},{0x4922f4,16},{0x4c2aa0,8},
        {0x4c4c5c,8},{0x515e94,8},{0x515edc,1},{scratch,65536},
        {0x515e3c,0x50},{0x52dfc4,12},{0x4c1378,0x1728}}})
        m.mount(p,std::vector<std::byte>(n,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
            "declared CPU view-context/SDK inputs; no original startup, GPU allocation or prior-frame history");
    put(m,0x411568,device);put(m,device+8,0x200);put(m,device+0x1928,1);
    put(m,device+0x794,scratch+0x3000);put(m,scratch+0x3004,2);
    put(m,device+0x95c,0x3f800000);put(m,device+0x960,0x3f800000);
    put(m,device+0x948,0x4b7fffff);put(m,device+0x1c18,2);
    put(m,device+0xef8,0x3f080000);put(m,device+0xefc,0x3f080000);
    const std::array<unsigned,6> viewport{320,0,320,240,0,0x3f800000};
    m.write(device+0xee0,std::as_bytes(std::span(viewport)));
    put(m,0x4922f4,0x3c800000);put(m,0x4922f8,0x44800000);
    put(m,0x4c2aa0,17);put(m,0x4c2aa4,19);
    put(m,0x473628,0x3f000000);put(m,0x47362c,0);
    // Explicit unavailable target records. The new original target owner
    // must reach its real SDK attachment service; no callee return supplied.
    put(m,0x4c1378+18*0x98+0x88,0);
    put(m,0x52dfc4,0);put(m,0x52dfc8,0);
    const std::byte zero{};m.write(0x515edc,std::span(&zero,1));
    put(m,sp,stop);
}
}
int main(int argc,char **argv)try {
    if(argc>2)throw FormatError("Optional new original view-context evidence directory");
    const std::filesystem::path out=argc==2?argv[1]:"";
    if(!out.empty()) {
        const char *scratch_env=std::getenv("HALO2_TEST_SCRATCH");
        if(!scratch_env || !std::filesystem::weakly_canonical(out).string().starts_with(
            std::filesystem::canonical(scratch_env).string()+"/"))
            throw FormatError("Raw view-context evidence must use owned HALO2_TEST_SCRATCH");
        if(std::filesystem::exists(out))throw FormatError("Preserve prior original view-context evidence");
        std::filesystem::create_directories(out);
    }
    for(unsigned row=0;row<47;++row) {
        SourceRuntimeMemory memory;inputs(memory);
        const bool context=row<6, selection=row>=6 && row<15;
        const unsigned mode=context?row/2:selection?(row==14?0xffff:row-6):0;
        SourceViewProjectionBank camera;
        for(unsigned n:{0,5,10,15})camera.projection[n]=1;
        camera.inverse_view.position={3,-2,1};camera.camera_to_world.position={-3,2,-1};
        if(context) {
            const std::uint16_t short_mode=mode,clear=0;
            memory.write(packet+2,std::as_bytes(std::span(&short_mode,1)));
            memory.write(packet+8,std::as_bytes(std::span(&clear,1)));
            const std::byte scaled{static_cast<unsigned char>(row%2)};
            memory.write(packet+7,std::span(&scaled,1));
            memory.write(packet+0x18,std::as_bytes(std::span(&camera.camera_to_world.position,1)));
            const std::array<std::int16_t,4> rect{0,320,240,640};
            memory.write(packet+0x48,std::as_bytes(std::span(rect)));
            put(memory,packet+0x5c,0x44800000);
            memory.write(packet+0x8c,std::as_bytes(std::span(&camera,1)));
            put(memory,sp+4,packet);
        }
        const unsigned enabled=context?0:selection?(mode?1:0):(row-15)&1;
        const bool two_sided=!context && !selection && ((row-15)&2);
        const bool dependent=!context && !selection && ((row-15)&4);
        const bool depth_present=!context && !selection && ((row-15)&8);
        put(memory,0x411800,two_sided);put(memory,0x4117fc,dependent);
        const bool matching=!context && !selection && ((row-15)&16);
        put(memory,0x4117bc,matching?0x1e00:0x1e01);
        put(memory,device+0x1a08,depth_present?scratch+0x4000:0);
        if(!context && !selection)put(memory,sp+4,enabled);
        SourceParticleConstantStorageTu15 storage;
        std::array<ShaderVector,192> inherited{};storage.begin(memory,inherited);
        SourcePhysicsWorldRuntimeAbi a{};
        a.gp={0x12345678,0xabcdef01,0x89abcdef,selection?mode:0x76543210,
            sp,0x24681357,0x13572468,0xfedcba98};
        a.xmm[0]={0x80000000,0x89abcdef,0x76543210,0xfedcba98};
        a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        const auto before=a;const unsigned entry=context?0x132b0:selection?0x15350:0x3ff670;
        std::vector<unsigned> trace;unsigned missing=0;
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.required_owner=[&](auto&,const auto &call,auto&,const auto&) {
            missing=call.target;throw FormatError("Original view-context downstream owner remains required");
        };
        const auto path=out/std::to_string(row);
        if(!out.empty()){std::filesystem::create_directory(path);bank(path/"before.bin",memory,storage);}
        bool completed=true;
        try {source_run_particle_world_execution_tu15(storage,entry,a,*a.floating_point,services);}
        catch(const FormatError&){if(!missing)throw;completed=false;}
        const unsigned expected_missing=context?0x401950:selection && mode>=1 && mode<=6?
            (mode==6?0x222b0:0x3ff720):0;
        if(missing!=expected_missing || completed==bool(missing))
            throw FormatError("Original view-context stopped at an unexpected owner");
        if(word(memory,0x4c2aa0)!=17 || word(memory,0x4c2aa4)!=19)
            throw FormatError("Original camera context reset the collected queues");
        const Bytes methods(storage.commands());
        if(context) {
            if(std::memcmp(memory.view(0x491e20,0x298).data(),memory.view(packet,0x298).data(),0x298) ||
                word(memory,0x4920d8)!=(mode==1?18u:0u) ||
                word(memory,0x515e94)!=0x901 || word(memory,0x515e98)!=0x900 ||
                methods.u32(0)!=0x41d84 || methods.u32(8)!=0x4032c || methods.u32(16)!=0x80394 ||
                std::find(trace.begin(),trace.end(),0x12f80)==trace.end())
                throw FormatError("Whole original context did not copy/publish its actual camera packet");
        } else if(selection) {
            if(!missing && (a.gp[4]!=sp+4 || Bytes(memory.view(0x473570,2)).u16(0)!=mode))
                throw FormatError("Completed original stencil selector has a wrong return/cache");
        } else {
            const unsigned stencil=(two_sided?2:0)|(dependent && (!enabled || matching)?1:0);
            if(storage.commands().size()!=16 || methods.u32(0)!=0x41d84 || methods.u32(4)!=stencil ||
                methods.u32(8)!=0x4032c || methods.u32(12)!=unsigned(enabled && depth_present) ||
                word(memory,0x4117b8)!=enabled || a.gp[4]!=sp+8)
                throw FormatError("Original stencil packet differs from its declared CPU inputs");
        }
        if(!out.empty()) {
            bank(path/"after.bin",memory,storage);
            std::ofstream(path/"case.json")<<Json{{"entry",entry},{"before",state(before)},
                {"after",state(a)},{"instructions",trace},{"return_address",missing?missing:stop},
                {"completed",completed},{"missing_owner",missing},{"row",row},
                {"native_gpu_or_startup_history_claimed",false},{"game_returns_supplied",Json::array()},
                {"gpu_returns_supplied",Json::array()},{"full_goal_complete",false}}.dump(2)<<'\n';
        }
    }
    std::cout<<"PASS6 original camera-context prefixes,9 stencil selections and32 complete stencil methods; queues retained and unknown target owners required\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

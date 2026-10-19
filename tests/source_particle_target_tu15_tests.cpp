#include "halo2/source_particle_target_tu15.hpp"
#include "halo2/source_particle_constant_storage_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_lens_flare_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr unsigned device=0x40f0c0, scratch=0x60000000, sp=scratch+0x8000;
constexpr unsigned stop=0x60300000, color=scratch+0x3000, depth=scratch+0x3100;
unsigned diagnostic_row=~0u,diagnostic_pc{};
void put(SourceRuntimeMemory &m,unsigned p,unsigned v) {m.write(p,std::as_bytes(std::span(&v,1)));}
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
    if(!f)throw FormatError("Cannot export original target-selection evidence");
}
struct Extent {unsigned format_word{},size_word{},depth_scale_bits{};};
Extent original_extent() {
    SourceRuntimeMemory m;mount_source_particle_graphics_layout_data_tu15(m);
    m.mount(scratch,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,
        "declared metadata-only original layout caller");
    SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=sp;a.eflags=0x202;
    a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(m,sp,stop);
    a.gp[0]=42;source_run_effect_particle_draw_tu15(m,0x407860,a,*a.floating_point,{});
    const auto format=a.gp[0];const std::array<unsigned,2> pitch_args{stop,640};
    m.write(sp,std::as_bytes(std::span(pitch_args)));a.gp[4]=sp;
    source_run_effect_particle_draw_tu15(m,0x407900,a,*a.floating_point,{});
    const std::array<unsigned,9> args{stop,1,1,format,a.gp[0],0,0,scratch+0x100,scratch+0x104};
    m.write(sp,std::as_bytes(std::span(args)));a.gp[4]=sp;a.gp[0]=640;a.gp[2]=480;
    source_run_effect_particle_draw_tu15(m,0x407ee0,a,*a.floating_point,{});
    Extent result{word(m,scratch+0x100),word(m,scratch+0x104),0};
    put(m,scratch+0x20c,result.format_word);put(m,sp,stop);
    a.gp[4]=sp;a.gp[0]=scratch+0x108;a.gp[1]=scratch+0x200;
    source_run_effect_particle_draw_tu15(m,0x407a50,a,*a.floating_point,{});
    result.depth_scale_bits=word(m,scratch+0x108);return result;
}
void inputs(SourceRuntimeMemory &m,const Extent &layout) {
    mount_source_particle_graphics_layout_data_tu15(m);
    mount_source_particle_target_data_tu15(m);
    for(const auto [p,n]:std::array<std::pair<unsigned,unsigned>,8>{{
        {device+8,0x1c20},{0x411568,0x2a0},{0x491e20,0x298},{0x4c1378,0x1730},
        {0x515e3c,0x50},{0x515edc,1},{0x473628,4},{0x52dfc4,12}}})
        m.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,
            "declared CPU target-cache/viewport inputs; no original SDK allocation or native GPU target history");
    m.mount(scratch,std::vector<std::byte>(65536,std::byte{0xa5}),
        SourceRuntimeMemory::Access::ReadWrite,"declared CPU target metadata, stack and matrix workspace");
    put(m,0x411568,device);put(m,device+8,0x200);put(m,device+0x1928,1);
    put(m,device+0x794,scratch+0x400);
    put(m,device+0x95c,0x3f800000);put(m,device+0x960,0x3f800000);
    put(m,device+0x954,640);put(m,device+0x958,480);
    put(m,device+0x948,layout.depth_scale_bits);put(m,device+0x1c18,2);
    put(m,device+0x1c08,0);put(m,device+0x1c0c,layout.depth_scale_bits);
    put(m,device+0x1c10,0);put(m,device+0x1c14,layout.depth_scale_bits);
    put(m,device+0xef8,0x3f000000);put(m,device+0xefc,0x3f000000);
    for(unsigned p:{device+0x1a04,device+0x1a14,device+0x1a18})put(m,p,color);
    // These are explicit CPU fragments for extent comparison only. SDK
    // Common/Data/refcount/physical-address contents are never supplied.
    const std::array<unsigned,2> extent{layout.format_word,layout.size_word};
    m.write(color+0xc,std::as_bytes(std::span(extent)));
    put(m,0x515e3c,color);put(m,0x515e44,depth);
    put(m,0x52dfc4,color);put(m,0x52dfc8,depth);
    const std::array<std::int16_t,4> rect{16,32,464,608};
    m.write(0x491e68,std::as_bytes(std::span(rect)));
    put(m,0x473628,0x3f000000);
    put(m,0x4c2aa0,17);put(m,0x4c2aa4,19);put(m,sp,stop);
}
}
int main(int argc,char **argv)try {
    if(argc>2)throw FormatError("Optional scratch output directory for original target evidence");
    const std::filesystem::path out=argc==2?argv[1]:"";
    if(!out.empty()) {
        const char *scratch_env=std::getenv("HALO2_TEST_SCRATCH");
        if(!scratch_env || !std::filesystem::weakly_canonical(out).string().starts_with(
            std::filesystem::canonical(scratch_env).string()+"/"))
            throw FormatError("Raw target evidence must use owned HALO2_TEST_SCRATCH");
        if(std::filesystem::exists(out))throw FormatError("Preserve earlier target evidence");
        std::filesystem::create_directories(out);
    }
    const auto layout=original_extent();
    // Exactly the33 non-null dispatch slots in original14F10/14EB0. Six
    // unsupported original enum values point at zero and are not admitted.
    const std::array<unsigned,33> targets{0,1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        17,18,19,20,21,22,23,25,26,27,29,33,34,35,36,37,38};
    constexpr unsigned full_count=20,lookup_count=24,cache_count=6;
    const unsigned total=full_count+targets.size()+lookup_count+cache_count;
    for(unsigned row=0;row<total;++row) {
        diagnostic_row=row;diagnostic_pc=0;
        SourceRuntimeMemory memory;inputs(memory,layout);
        SourceParticleConstantStorageTu15 storage;std::array<ShaderVector,192> inherited{};
        storage.begin(memory,inherited);
        SourcePhysicsWorldRuntimeAbi setup{};setup.gp[0]=scratch+0x400;setup.gp[1]=4;
        setup.gp[4]=sp;setup.eflags=0x202;
        setup.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        source_run_effect_particle_draw_tu15(storage,0x400a60,setup,*setup.floating_point,{});
        const std::array<float,16> projection{1.25f,0,0,0,0,1.75f,0,0,
            0,0,-1.0000152587890625f,-1,0,0,-.015625238418579102f,0};
        memory.write(scratch+0x1000,std::as_bytes(std::span(projection)));
        setup.gp[0]=1;setup.gp[2]=scratch+0x1000;setup.gp[4]=sp;put(memory,sp,stop);
        source_run_effect_particle_draw_tu15(storage,0x4020d0,setup,*setup.floating_point,{});
        SourcePhysicsWorldRuntimeAbi a{};
        a.gp={1,0xabcdef01,0x89abcdef,0x76543210,sp,0x24681357,color,0xfedcba98};
        a.xmm[0]={0x80000000,0x89abcdef,0x76543210,0xfedcba98};
        a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        unsigned entry=0x14ba0,expected_missing=0;
        const bool full=row<full_count;
        unsigned lookup_expected=0;
        if(full) {
            const unsigned mode=row%5;
            const std::uint16_t view_mode=mode==2?2:0;
            memory.write(0x491e22,std::as_bytes(std::span(&view_mode,1)));
            const std::byte scaled{static_cast<unsigned char>(mode>=3)};
            memory.write(0x491e27,std::span(&scaled,1));
            const std::byte disabled{static_cast<unsigned char>(row==14)};
            memory.write(0x515edc,std::span(&disabled,1));
            put(memory,0x473628,row==8?0xbf000000:row==9?0x40000000:row==18?0x7fc12345:0x3f000000);
            if(row>=10) {
                const std::array<std::int16_t,4> rect{-10,16,700,900};
                memory.write(0x491e68,std::as_bytes(std::span(rect)));
            }
            if(row%2) {a.gp[0]=0;put(memory,0x52dfc8,0);}
            if(row==4 || row==9)put(memory,0x4c1fe0,1);
            if(row==14 || row==19)put(memory,0x4c1eb0,1);
            const std::array<unsigned,3> args{stop,0,0};
            memory.write(sp,std::as_bytes(std::span(args)));
        } else if(row<full_count+targets.size()) {
            const auto target=targets[row-full_count];
            for(unsigned p=0x515e3c;p<=0x515e88;p+=4)put(memory,p,color);
            put(memory,0x515e44,depth);
            for(unsigned index=0;index<39;++index) {
                const auto record=0x4c1378+index*0x98;
                put(memory,record+0x88,1);put(memory,record+0x78,3);
                const std::byte available{};
                memory.write(record+0x95,std::span(&available,1));
            }
            const std::array<unsigned,3> args{stop,target,2};
            memory.write(sp,std::as_bytes(std::span(args)));
            put(memory,0x52dfc4,0);put(memory,0x52dfc8,0xffffffff);
            expected_missing=0x401950;
        } else if(row<full_count+targets.size()+lookup_count) {
            const unsigned index=row-full_count-targets.size(),record=0x4c1378+13*0x98;
            entry=index<12?0x1dcd0:0x1dd00;a.gp[0]=13;
            put(memory,record+0x88,index%6==0?0:1);
            const std::byte unavailable{static_cast<unsigned char>(index%6==1)};
            memory.write(record+0x95,std::span(&unavailable,1));
            put(memory,record+0x78,index%6==2?0:index%6==3?0xffffffff:3);
            a.gp[2]=index%6==3?0xffffffff:index%6==4?3:2;
            const bool admitted=index%6>=4;
            lookup_expected=entry==0x1dcd0?(admitted?record+0x18:0):
                (index%6==5?record+(a.gp[2]+1)*24:0);
            put(memory,sp,stop);
        } else {
            const auto index=row-full_count-targets.size()-lookup_count;
            entry=0x1cd10;a.gp[0]=index==1?0:depth;
            const unsigned old_color=index>=3?color+0x100:color;
            put(memory,0x52dfc4,old_color);
            put(memory,0x52dfc8,index==1?0:depth);
            if(index>=3) {
                // Read-only equality-test fragments, not an SDK allocation.
                put(memory,color,0x12345678);put(memory,old_color,index==5?0x87654321:0x12345678);
                memory.write(old_color+0xc,memory.view(color+0xc,8));
            }
            if(index==2)put(memory,0x52dfc8,0);
            if(index==5)expected_missing=0x401950;
            if(index==2 || index==3 || index==4)expected_missing=0x401c10;
            const std::array<unsigned,2> args{stop,unsigned(index&1)};
            memory.write(sp,std::as_bytes(std::span(args)));
        }
        const auto before=a;std::vector<unsigned> trace;unsigned missing=0;
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){diagnostic_pc=pc;trace.push_back(pc);};
        services.required_owner=[&](auto &m,const auto &call,auto &a,const auto &observe) {
            // Compose only actual lowered callees over the same CPU memory,
            // state and observer. Unknown SDK target calls still stop below.
            const auto fp=*a.floating_point;
            if(source_effect_particle_draw_tu15_owns_entry(call.target)) {
                source_run_effect_particle_draw_tu15(m,call.target,a,fp,services,observe);return;
            }
            if(source_particle_target_tu15_owns_entry(call.target)) {
                source_run_particle_target_tu15(m,call.target,a,fp,services,observe);return;
            }
            if(call.target==0x32cc0c) {
                source_run_lens_flare_tu15(m,call.target,a,fp,services,observe);return;
            }
            missing=call.target;throw FormatError("Original target selection requires its actual SDK owner");
        };
        const auto path=out/std::to_string(row);
        if(!out.empty()){std::filesystem::create_directory(path);bank(path/"before.bin",memory,storage);}
        bool completed=true;
        try {
            if(source_particle_target_tu15_owns_entry(entry))
                source_run_particle_target_tu15(storage,entry,a,*a.floating_point,services);
            else throw FormatError("Unowned target test entry");
        }
        catch(const FormatError&){if(!missing)throw;completed=false;}
        if(missing!=expected_missing || completed==bool(missing))
            throw FormatError("Original target selector reached an unexpected SDK owner at row"+std::to_string(row));
        if(word(memory,0x4c2aa0)!=17 || word(memory,0x4c2aa4)!=19)
            throw FormatError("Original target selection altered occupied particle queues");
        if(!missing) {
            if(a.gp[4]!=sp+(entry==0x14ba0?12:entry==0x1cd10?8:4) ||
                a.gp[3]!=before.gp[3] || a.gp[5]!=before.gp[5] || a.gp[6]!=before.gp[6] ||
                a.gp[7]!=before.gp[7] || a.x87_depth)
                throw FormatError("Original target selection return ABI differs");
            if(entry==0x1dcd0 || entry==0x1dd00) {
                if(a.gp[0]!=lookup_expected)throw FormatError("Original retained target lookup differs at row"+std::to_string(row));
            } else if(full) {
                if(!(word(memory,0x411570)&0x200) || trace.back()!=0x14eaa ||
                    std::find(trace.begin(),trace.end(),0x4021e0)==trace.end())
                    throw FormatError("Completed original target did not publish its viewport/matrix state");
                std::optional<std::array<unsigned,4>> expected_viewport;
                if(row==0)expected_viewport=std::array<unsigned,4>{32,16,576,448};
                if(row==2)expected_viewport=std::array<unsigned,4>{0,0,128,128};
                if(row==3)expected_viewport=std::array<unsigned,4>{16,8,288,224};
                //4021E0 preserves a negative origin as a raw unsigned word;
                // it clips the far edge, then subtracts that retained origin.
                if(row==10)expected_viewport=std::array<unsigned,4>{16,0xfffffff6,624,490};
                if(expected_viewport && std::memcmp(memory.view(device+0xee0,16).data(),
                    expected_viewport->data(),16))
                    throw FormatError("Original target viewport scaling/fixed128/clamp differs");
                for(unsigned n=0;n<16;++n)
                    if(!std::isfinite(std::bit_cast<float>(word(memory,device+0x980+n*4))))
                        throw FormatError("Original target viewport produced nonfinite matrix state");
            }
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
    std::cout<<"PASS83 original target/viewport/cache/record-lookup cases; occupied queues retained, actual SDK binding remains required\n";
}catch(const std::exception &e){
    std::cerr<<e.what()<<" (row "<<diagnostic_row<<", original instruction0x"
        <<std::hex<<diagnostic_pc<<")\n";return 1;
}

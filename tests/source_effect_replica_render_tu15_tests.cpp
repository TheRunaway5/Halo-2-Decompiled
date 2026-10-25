#include "halo2/source_effect_replica_render_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool v,const char *why){if(!v)throw FormatError(why);}
template<class F>void rejects(F &&f){try{f();}catch(const FormatError &){return;}throw FormatError("Invalid replica effect input was accepted");}
void put(SourceRuntimeMemory &m,unsigned at,std::uint32_t v) {
    std::array<std::byte,4> b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8));m.write(at,b);
}
void readback() {
    SourceRuntimeMemory m;constexpr unsigned pool=0x1000000u,name=0x1100000u;
    m.mount(pool,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared effect pool caller storage");
    const std::array<std::byte,8> text{std::byte{'e'},std::byte{'f'},std::byte{'f'},std::byte{'e'},std::byte{'c'},std::byte{'t'},std::byte{'s'},std::byte{}};
    m.mount(name,text,SourceRuntimeMemory::Access::ReadOnly,"actual pool name input");
    m.mount(0x4f720cu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"actual pool binding input");put(m,0x4f720cu,pool);
    const SourceRawDatumPoolServices storage{.memory=[&](auto at,auto n){return m.mutable_view(at,n);},.read=[&](auto at,auto n){return m.view(at,n);}};
    source_raw_datum_initialize_inplace(pool,name,256,400,0,0,storage);source_raw_datum_make_valid(pool,storage);
    const auto identity=source_raw_datum_allocate(pool,storage);check(identity!=0xffffffffu,"Original datum allocator declined its owned caller pool");
    const auto body=source_raw_datum_lookup(pool,identity,storage);check(body,"Allocated effect lacks a full-salt body");
    put(m,body+4,0xa100fffeu);put(m,body+0x48,0xffffffffu);put(m,body+0x58,0xffffffffu);
    const std::uint16_t flags=1;m.write(body+2,std::as_bytes(std::span{&flags,1}));
    const Vec3 color{.25f,.5f,1.5f},velocity{};
    m.write(body+0x24,std::as_bytes(std::span{&color,1}));m.write(body+0x3c,std::as_bytes(std::span{&velocity,1}));
    const auto v=source_effect_replica_render_tu15(m,identity,0xa100fffeu);
    check(v.identity==identity&&v.flags==1&&v.color.x==.25f&&v.color.y==.5f&&v.color.z==1.5f&&source_effect_replica_static_world_tu15(v),"Read-only original effect render fields changed");
    rejects([&]{source_effect_replica_render_tu15(m,identity^0x10000u,0xa100fffeu);});
    rejects([&]{source_effect_replica_render_tu15(m,identity,0xa101ffffu);});
    for(unsigned variant=0;variant<4;++variant) {
        auto other=v;if(variant==0)other.attached_owner=0x80000001u;if(variant==1)other.local_view=0;
        if(variant==2)other.flags|=2u;if(variant==3)other.velocity.x=.1f;
        check(!source_effect_replica_static_world_tu15(other),"Unowned attached/FP/animated/moving effect entered the closed WORLD producer");
    }
    source_raw_datum_release_unchecked(pool,identity,storage);
    rejects([&]{source_effect_replica_render_tu15(m,identity,0xa100fffeu);});
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original request fixture");std::ifstream in(argv[1]);nlohmann::json j;in>>j;
    check(j.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original request fixture image differs");
    ShaderFunctionTables tables;
    for(unsigned curve=0;curve<16;++curve) {
        const auto hex=j.at("transitions").at(curve).get<std::string>();check(hex.size()==2048,"Original transition table extent changed");
        for(unsigned n=0;n<1024;++n)tables.transition[curve][n]=std::stoul(hex.substr(n*2,2),nullptr,16);
    }
    unsigned count{};
    for(const auto &row:j.at("cases")) {
        const SourceEffectPartLensControlsTu15 part{row.at("scale_a"),row.at("scale_b"),row.at("curve")};
        const SourceEffectControls controls{row.at("controls").at(0),row.at("controls").at(1)};
        SourceEffectEventState clock;clock.elapsed=std::bit_cast<float>(row.at("elapsed_bits").get<std::uint32_t>());
        clock.target=std::bit_cast<float>(row.at("target_bits").get<std::uint32_t>());
        const auto actual=source_effect_part_lens_opacity_tu15(part,controls,clock,tables);
        check(std::bit_cast<std::uint32_t>(actual)==row.at("expected").at("opacity_bits").get<std::uint32_t>(),"Effect lens opacity differs from complete original17C710 request");++count;
    }
    check(count>=168,"Original effect request corpus was narrowed");readback();
    std::cout<<"PASS "<<count<<" complete original effect-lens request arithmetic cases and full-salt read-only render provenance controls\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

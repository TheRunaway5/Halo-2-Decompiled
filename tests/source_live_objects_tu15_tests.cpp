#include "halo2/source_live_objects_tu15.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <iostream>
#include <tuple>

using namespace halo2;
namespace {
void require(bool v,const char *message){if(!v)throw FormatError(message);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const FormatError&){failed=true;}require(failed,"Malformed source readback was accepted");}
constexpr std::uint32_t pool=0x10000,headers=0x11000,bits=0x18000,body=0x19000,other=0x1a000,object=0xe1000000;
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t v,unsigned n=4){
    auto b=m.mutable_view(p,n);for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8));
}
std::string hex(std::span<const std::byte> bytes){const char *digits="0123456789abcdef";std::string out;for(auto b:bytes){auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;}
}
int main(int argc,char **argv)try {
    SourceRuntimeMemory m;m.mount(pool,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"Explicit read-only object field fixture");
    put(m,pool+0x20,2048);put(m,pool+0x24,12);put(m,pool+0x29,1,1);put(m,pool+0x38,3);
    put(m,pool+0x44,headers);put(m,pool+0x48,bits);put(m,bits,7);
    for(unsigned i=0;i<3;++i){put(m,headers+i*12,0xe100+i,2);put(m,headers+i*12+3,i==2?3:11,1);}
    for(auto [header,address,type]:{std::tuple{headers,body,11u},std::tuple{headers+24u,other,3u}}) {
        put(m,header+6,0x300,2);put(m,header+8,address);put(m,address,0xe2300001+type);
        put(m,address+0x14,0xffffffffu);put(m,address+0xaa,type,1);put(m,address+0xb1,0xff,1);
    }
    put(m,body+0x118,20,2);put(m,body+0x11a,0x140,2);
    put(m,body+0x140,0xff02,2);put(m,body+0x142,0x0301,2);
    put(m,body+0x144,0x00800300);put(m,body+0x148,0x90010005);
    put(m,body+0x14c,0x000101ff);put(m,body+0x150,0xffffffffu);
    put(m,body+0x124,48,2);put(m,body+0x126,0x180,2);
    const std::array<Vec3,2> rgb{{{-0.f,.25f,.75f},{1,0,-1}}};m.write(body+0x198,std::as_bytes(std::span(rgb)));
    const auto native_pool=m.native_address(pool,0x4c);require(native_pool.value>0xffffffffu,"Readback truncated its native pool binding");
    const auto before=m.view(pool,0x20000);const std::vector<std::byte> saved(before.begin(),before.end());
    const auto live=source_live_objects_tu15(m,native_pool);require(live.size()==2 && live[0].object==object && live[1].type==3,"Live readback lost full identities or fabricated a reserved body's birth");
    const auto regions=source_live_model_regions_tu15(m,native_pool,object);const auto colors=source_live_object_colors_tu15(m,native_pool,object);
    require(regions.selection.variant_index==-1 && regions.selection.permutations==std::vector<std::uint8_t>{2,255} &&
        regions.selection.states[0].damage==3 && regions.selection.states[0].properties==128 &&
        regions.looping_effects==std::vector<std::uint32_t>{0x90010005,0xffffffffu},"Completed live regions changed their original bytes");
    require(colors.size()==rgb.size() && !std::memcmp(colors.data(),rgb.data(),sizeof(rgb)),"Current colors changed bits or read the initial bank");
    require(!std::memcmp(saved.data(),m.view(pool,saved.size()).data(),saved.size()),"Readback mutated the retained source bank");
    if(argc==2 && std::string_view(argv[1])=="--json") {
        nlohmann::json j{{"memory_address",pool},{"memory_hex",hex(saved)},{"object",object},
            {"region_count",regions.selection.permutations.size()},{"permutations",regions.selection.permutations},
            {"effects",regions.looping_effects},{"states",nlohmann::json::array()},{"color_bits",nlohmann::json::array()}};
        for(auto s:regions.selection.states)j["states"].push_back({std::uint8_t(s.variant_permutation),s.damage,s.properties});
        for(auto c:colors)j["color_bits"].push_back({std::bit_cast<std::uint32_t>(c.x),std::bit_cast<std::uint32_t>(c.y),std::bit_cast<std::uint32_t>(c.z)});
        std::cout<<j.dump()<<'\n';return 0;
    }
    rejects([&]{source_live_model_regions_tu15(m,native_pool,object+0x10000);});
    put(m,body+0x118,11,2);rejects([&]{source_live_model_regions_tu15(m,native_pool,object);});
    put(m,body+0x118,0,2);put(m,body+0x11a,0xffff,2);
    require(source_live_model_regions_tu15(m,native_pool,object).selection.permutations.empty(),"Absent regions consumed their unused offset");
    put(m,body+0x124,0,2);put(m,body+0x126,0xffff,2);
    require(source_live_object_colors_tu15(m,native_pool,object).empty(),"Absent colors consumed their unused offset");
    put(m,body+0x124,25,2);rejects([&]{source_live_object_colors_tu15(m,native_pool,object);});
    put(m,body+0xaa,3,1);rejects([&]{source_live_objects_tu15(m,native_pool);});
    std::cout<<"PASS TU1.5 live object/region/color readback and rejection\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

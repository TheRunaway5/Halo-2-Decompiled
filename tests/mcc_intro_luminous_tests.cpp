#include "halo2/cache.hpp"
#include "halo2/mcc_native_shaders.hpp"
#include "halo2/object_lighting.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *why){if(!value)throw FormatError(why);}
std::vector<std::byte> hex(std::string_view text) {
    check(text.size()%2==0,"Invalid captured metadata");std::vector<std::byte> out;
    for(unsigned i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(std::string(text.substr(i,2)),nullptr,16)));
    return out;
}
Tag tag(const Json& j){return {j.at("group"),j.at("name"),j.at("datum"),j.at("address"),j.at("size")};}
struct Fixture {
    Tag shader,stem;std::string payload_name;std::vector<std::byte> payload;
    std::map<std::uint32_t,Tag> names;std::map<std::uint32_t,unsigned> frames;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    explicit Fixture(const Json& j):shader(tag(j.at("shader"))),stem(tag(j.at("native_stem"))),
        payload_name(j.at("native_payload_name")),payload(hex(j.at("native_payload_hex").get<std::string>())) {
        for(const auto& t:j.at("namespace")){auto v=tag(t);names.emplace(v.datum,std::move(v));}
        for(const auto& [id,n]:j.at("bitmap_frame_counts").items())frames.emplace(std::stoul(id),n.get<unsigned>());
        for(const auto& r:j.at("regions"))regions.emplace_back(r.at(0),hex(r.at(1).get<std::string>()));
    }
    std::span<const std::byte> read(std::uint32_t at,std::size_t n)const {
        for(const auto& [base,data]:regions)if(at>=base && std::uint64_t(at)-base<=data.size() && n<=data.size()-(at-base))
            return std::span(data).subspan(at-base,n);
        throw FormatError("Captured MCC metadata read escaped its owner");
    }
    void word(std::uint32_t at,std::uint32_t value) {
        bool found=false;
        for(auto& [base,data]:regions)if(at>=base && std::uint64_t(at)-base+4<=data.size()) {
            for(unsigned i=0;i<4;++i)data[at-base+i]=std::byte(value>>(i*8));found=true;
        }
        check(found,"Negative control escaped its actual source bank");
    }
    std::optional<MccOwnedShaderProjection> run()const {
        return project_mcc_unanimated_tex_bump(shader,stem,payload_name,payload,
            [&](std::uint32_t at,std::size_t n){return read(at,n);},
            [&](std::uint32_t id)->const Tag*{const auto i=names.find(id);return i==names.end()?nullptr:&i->second;},
            [&](std::uint32_t id,std::uint32_t frame){const auto i=frames.find(id);return i!=frames.end() && frame<i->second;});
    }
};
}
int main(int argc,char**argv)try {
    check(argc==2 || argc==3,"Supply captured metadata and optional actual MCC introduction map");
    if(argc==3 && std::string_view(argv[2]).empty())return 77;
    Json j;std::ifstream(argv[1])>>j;
    check(j.at("schema")=="H2_MCC_INTRO_LUMINOUS_TU15_V1" &&
        j.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Luminous fixture lost its English TU1.5 authority");
    Fixture f(j);const auto before=f.regions;const auto out=f.run();
    check(bool(out),"Actual MCC introduction luminous material is still unconverted");
    check(f.regions==before,"Shader projection mutated original MCC banks");
    const Bytes root(f.read(f.shader.address,84)),source(f.read(root.u32(36),124)),result(out->bytes),native(f.payload);
    const auto pp=result.u32(36),native_pp=native.u32(92);
    check(result.u32(32)==1 && result.u32(pp+52)==60,"Projected original implementation extent differs");
    for(const auto [field,stride]:std::array<std::pair<unsigned,unsigned>,12>{{
            {4,12},{12,4},{20,16},{28,6},{36,2},{60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}}) {
        const auto count=source.u32(field);check(result.u32(pp+field)==count,"Authored input count changed");
        if(count)check(std::ranges::equal(result.slice(result.u32(pp+field+4),count*stride),
            f.read(source.u32(field+4),count*stride)),"Authored MCC parameter bytes changed");
    }
    const auto ranges=result.u32(pp+48),passes=native.u32(native_pp+20);
    for(unsigned n=0;n<18;++n)check(result.u16(ranges+n*2)==native.u16(passes+n*10+8),
        "Projected pass range differs from original introduction template");
    unsigned negatives=0;
    const auto reject=[&](auto mutate){auto bad=f;mutate(bad);check(!bad.run(),"Changed shader authoring was admitted");++negatives;};
    reject([](auto& b){b.payload[0]^=std::byte{1};});
    reject([](auto& b){b.payload_name[0]='f';});
    reject([](auto& b){b.shader.datum^=0x10000u;});
    reject([&](auto& b){b.word(root.u32(36)+60,1);}); // A real animation needs its separate owner.
    reject([&](auto& b){b.word(source.u32(44+4),0);});
    reject([&](auto& b){b.word(source.u32(4+4),0xdeadbeefu);});
    reject([&](auto& b){b.word(source.u32(20+4),0x7fc00000u);});
    reject([&](auto& b){b.word(source.u32(52+4),1);});
#ifndef HALO2_MCC_LUMINOUS_METADATA_ONLY
    if(argc==3) {
        Cache cache(argv[2]);check(cache.is_mcc(),"Luminous delivery requires actual imported MCC");
        const auto* shader=cache.query_tag({.group=TagGroup{"shad"},.name=f.shader.name,.require_address=true});
        check(shader && shader->datum==f.shader.datum && cache.is_preserved_native_shader(*shader),
            "Actual Cache did not wire the luminous projection");
        auto material=cache.load_material(shader->datum);
        check(material.passes.size()==60 && material.bitmaps.size()==7 &&
            material.pixel_constants.size()==8 && material.vertex_constants.size()==8,"Delivered material shape differs");
        const Bytes pixels(f.read(source.u32(16),32)),vectors(f.read(source.u32(24),128)),images(f.read(source.u32(8),84));
        for(unsigned n=0;n<8;++n) {
            check(material.pixel_constants[n]==pixels.u32(n*4),"MCC pixel constant replaced by a donor");
            for(unsigned c=0;c<4;++c)check(std::bit_cast<std::uint32_t>(material.vertex_constants[n][c])==vectors.u32(n*16+c*4),
                "MCC vector constant replaced by a donor");
        }
        for(unsigned n=0;n<7;++n)check(material.bitmaps[n].datum==images.u32(n*12) &&
            material.bitmaps[n].index==images.u32(n*12+4) &&
            std::bit_cast<std::uint32_t>(material.bitmaps[n].log_dimension)==images.u32(n*12+8),
            "MCC bitmap identity/frame/dimension replaced by an original-map value");
        check(prepare_dynamic_material(cache,material,{},true,0.f),"Actual luminous material still fails dynamic preparation");
    }
#endif
    std::cout<<"PASS actual MCC luminous parameters, original TU1.5 template ranges and "<<negatives<<" negative controls\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

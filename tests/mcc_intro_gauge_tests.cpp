#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
std::vector<std::byte> hex(const std::string &text) {
    check(text.size()%2==0,"Invalid original gauge metadata");std::vector<std::byte> result;
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string encoded(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto value:bytes){const auto b=std::to_integer<unsigned>(value);result+=digits[b>>4];result+=digits[b&15];}return result;
}
}
int main(int argc,char **argv)try {
    check(argc==3 || argc==4,"Supply authenticated gauge metadata and actual MCC introduction");
    if(!*argv[2])return 77;
    nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    const bool gauge=fixture.at("schema")=="H2_MCC_INTRO_GAUGE_TU15_V1";
    check((gauge || fixture.at("schema")=="H2_MCC_INTRO_CURRENT_TU15_V1") &&
        fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Gauge fixture lost its original TU1.5 authority");
    Cache cache(argv[2]);check(cache.is_mcc(),"Gauge delivery requires imported MCC");
    if(argc==4) {
        const auto *globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
        check(globals!=nullptr,"Gauge publication lost globals");
        const auto block=cache.read_tag_bytes(globals->address+0xc0,8);
        const auto sound=cache.read_tag_bytes(Bytes(block).u32(4),44);
        const auto *legacy=cache.find_tag({Bytes(sound).u32(12)},TagGroup{"sncl"});
        check(legacy!=nullptr,"Gauge publication lost Legacy classes");
        const auto root=cache.read_tag_bytes(legacy->address,8);check(Bytes(root).u32(0)==54,"Gauge publication changed class count");
        const auto rows=cache.read_tag_bytes(Bytes(root).u32(4),54*120);
        std::array<std::byte,32> digest{};unsigned size{};
        check(EVP_Digest(rows.data(),rows.size(),reinterpret_cast<unsigned char*>(digest.data()),&size,EVP_sha256(),nullptr)==1 && size==32,
            "Cannot authenticate published sound classes");
        const auto tag_json=[](const Tag &t){return nlohmann::json{{"group",t.group},{"name",t.name},{"datum",t.datum},{"address",t.address},{"size",t.size}};};
        nlohmann::json publication{{"globals",tag_json(*globals)},{"legacy",tag_json(*legacy)},
            {"sound_block",encoded(block)},{"sound_globals",encoded(sound)},{"class_root",encoded(root)},
            {"source_bank_sha256",encoded(digest)},
            {"source_maps",nlohmann::json::array({{{"map","00a_introduction"},{"sha256",fixture.at("mcc_map_sha256")}}})}};
        std::ofstream(argv[3])<<publication.dump(2)<<'\n';
    }
    const auto shader_name=fixture.at("shader").at("name").get<std::string>();
    const auto *tag=cache.query_tag({.group=TagGroup{"shad"},.name=shader_name,.require_address=true});
    check(tag && tag->datum==fixture.at("shader").at("datum") && cache.is_preserved_native_shader(*tag),
        "Introduction gauge has no original native material owner");
    auto material=cache.load_material(tag->datum);
    const auto bytes=hex(fixture.at(gauge?"original_gauge_payload_hex":"original_current_payload_hex"));const Bytes original(bytes);
    const auto pp=original.u32(36),pixels=original.u32(pp+16),vectors=original.u32(pp+24);
    check(material.bitmaps.size()==original.u32(pp+4) && material.pixel_constants.size()==original.u32(pp+12) &&
        material.vertex_constants.size()==original.u32(pp+20) && material.passes.size()==original.u32(pp+52) &&
        material.animation.overlays.size()==original.u32(pp+60),"Original material shape changed");
    for(unsigned n=0;n<material.pixel_constants.size();++n)check(material.pixel_constants[n]==original.u32(pixels+n*4),"Original gauge pixel binding changed");
    for(unsigned n=0;n<material.vertex_constants.size();++n)for(unsigned c=0;c<4;++c)
        check(std::bit_cast<unsigned>(material.vertex_constants[n][c])==original.u32(vectors+n*16+c*4),"Original gauge vector binding changed");
    const auto read=[&](unsigned address,std::size_t size) {
        for(const auto &region:fixture.at("regions")) {
            const auto start=region.at(0).get<unsigned>();auto data=hex(region.at(1));
            if(address>=start && std::uint64_t(address)-start+size<=data.size())
                return std::vector<std::byte>(data.begin()+address-start,data.begin()+address-start+size);
        }
        throw FormatError("MCC gauge fixture read escaped its raw bank");
    };
    const auto root_data=read(fixture.at("shader").at("address"),84);const Bytes root(root_data);
    const auto def_data=read(root.u32(36),124);const Bytes def(def_data);
    check(def.u32(60)==material.animation.overlays.size(),"Native material changed the authored MCC curve count");
    const auto overlays=read(def.u32(64),def.u32(60)*20);const Bytes source(overlays);
    for(unsigned n=0;n<material.animation.overlays.size();++n) {
        const auto &overlay=material.animation.overlays[n];
        check(overlay.input==source.u32(n*20) && overlay.range==source.u32(n*20+4) &&
            std::bit_cast<unsigned>(overlay.period)==source.u32(n*20+8) &&
            overlay.curve==read(source.u32(n*20+16),source.u32(n*20+12)),"Gauge lost an authored MCC function input/curve");
    }
    const auto images=read(def.u32(8),def.u32(4)*12);const Bytes authored(images);
    check(def.u32(4)==material.bitmaps.size(),"Native material changed the authored MCC bitmap count");
    for(unsigned n=0;n<material.bitmaps.size();++n)check(material.bitmaps[n].datum==authored.u32(n*12) &&
        material.bitmaps[n].index==authored.u32(n*12+4) &&
        std::bit_cast<unsigned>(material.bitmaps[n].log_dimension)==authored.u32(n*12+8),"Gauge escaped its actual MCC bitmap/frame/dimensions");
    check(prepare_dynamic_material(cache,material,{},true,0.f),"Original gauge still cannot prepare for a dynamic draw");
    std::cout<<"PASS original introduction material, "<<material.animation.overlays.size()<<" MCC curves and actual bitmap ownership\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

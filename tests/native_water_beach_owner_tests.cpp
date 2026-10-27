#include "halo2/cache.hpp"
#include "halo2/texture.hpp"
#include "halo2/water.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void need(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::vector<std::byte> read(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    need(bool(in)&&in.tellg()>0&&in.tellg()<65536,"Missing/excessive source resource sidecar");
    std::vector<std::byte> out(std::size_t(in.tellg()));in.seekg(0);
    need(bool(in.read(reinterpret_cast<char*>(out.data()),out.size())),"Incomplete source resource sidecar");return out;
}
std::string sha(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> digest{};unsigned size{};
    need(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)==1&&size==digest.size(),"Resource hash failed");
    constexpr char hex[]="0123456789abcdef";std::string out;
    for(auto byte:digest){out+=hex[byte>>4];out+=hex[byte&15];}return out;
}
}
int main(int argc,char** argv){try {
    need(argc==5,"Usage: native_water_beach_owner_tests REPO PREPARED_ROOT SOURCE_PROFILE REPORT");
    const auto root=std::filesystem::path(argv[1]),prepared=std::filesystem::path(argv[2]);
    const auto policy_bytes=read(argv[3]);
    need(sha(policy_bytes)=="7fbfffd1d4f9749d3614b0609ddd6d62caa3b72e83fe370899de595a90429e6e",
         "Complete original resource policy differs");
    const auto profile=Json::parse(reinterpret_cast<const char*>(policy_bytes.data()),
        reinterpret_cast<const char*>(policy_bytes.data()+policy_bytes.size()));
    need(profile.at("schema")=="H2_MCC_NATIVE_WATER_BEACH_OWNER_V1","Changed original bitmap profile");
    const auto source_path=root/profile.at("original").at("map").get<std::string>();
    const auto maps=prepared/"halo2/h2_maps_win64_dx11";
    Cache source(source_path),native(maps/"03a_oldmombasa.map",maps,{},prepared);
    const auto name=profile.at("tag").get<std::string>();
    const auto old=source.query_tag({.group=tag_groups::bitmap,.name=name});
    const auto current=native.query_tag({.group=tag_groups::bitmap,.name=name});
    need(old&&current&&old->address&&current->address&&old->size==2048&&current->size==2936,
         "Missing/truncated actual original/native typed bitmap owner");
    need(old->datum==2160402042u&&current->datum==0xd00023cfu&&
         source.bitmap_count(old->datum)==17&&native.bitmap_count(current->datum)==17,
         "Original/native complete salted frame ownership differs");
    const auto original_metadata=source.read_tag_bytes(old->address,old->size);
    const auto saved_metadata=read(prepared/"graphics-corrections/water-beach-source-v1/original-metadata.bin");
    const auto saved_low=read(prepared/"graphics-corrections/water-beach-source-v1/software-low-detail.bin");
    need(std::ranges::equal(original_metadata,saved_metadata)&&saved_low.size()==2176&&
         sha(original_metadata)==profile.at("original").at("complete_metadata_sha256").get<std::string>(),
         "Complete retained original metadata/software extent differs");
    std::size_t frames{},mips{},hardware{},decoded{},low_bytes{},controls{};
    Json results=Json::array();
    for(const auto& row:profile.at("original").at("frames")) {
        const auto index=row.at("frame").get<unsigned>();need(index==frames,"Original frame order differs");
        const auto a=source.load_bitmap_resource(old->datum,index),b=native.load_bitmap_resource(current->datum,index);
        need(a.width==128&&a.height==128&&a.depth==1&&a.type==0&&a.format==23&&
             a.width==b.width&&a.height==b.height&&a.depth==b.depth&&a.type==b.type&&a.format==b.format,
             "Native resource dimensions/format differ from original");
        for(unsigned slot=0;slot<3;++slot) {
            need(a.levels[slot]==b.levels[slot],"Native hardware partition changed an original byte");
            const auto& part=row.at("complete_hardware_partitions").at(slot);
            need(b.levels[slot].size()==part.at("source_bytes").get<std::size_t>(),"Native resource partition extent differs");
            if(!b.levels[slot].empty()) {
                need(sha(b.levels[slot])==part.at("sha256").get<std::string>(),"Native complete partition identity differs");++hardware;
            }
        }
        const auto before=source.load_bitmap(old->datum,index),after=native.load_bitmap(current->datum,index);
        need(before.width==after.width&&before.height==after.height&&before.format==after.format&&
             before.layers==after.layers&&before.depth==after.depth&&before.pixels==after.pixels&&
             before.mips.size()==3&&after.mips.size()==3&&
             sha(after.pixels)==row.at("decoded_rgba8_sha256").get<std::string>(),
             "Actual native Cache decoded image differs from source");
        for(unsigned level=0;level<3;++level) {
            const auto& x=before.mips[level];const auto& y=after.mips[level];
            need(x.width==y.width&&x.height==y.height&&x.depth==y.depth&&x.offset==y.offset&&x.size==y.size&&
                 sha(std::span<const std::byte>(after.pixels).subspan(y.offset,y.size))==
                    row.at("mips").at(level).at("decoded_rgba8_sha256").get<std::string>(),
                 "Actual native Cache complete decoded mip differs");++mips;
        }
        const auto low=source.load_bitmap_low_detail_resource(old->datum,index);
        need(low&&low->width==8&&low->height==8&&low->depth==1&&low->type==0&&low->format==14&&low->pixels.size()==128&&
             sha(low->pixels)==row.at("software_low_detail").at("sha256").get<std::string>()&&
             std::ranges::equal(low->pixels,std::span(saved_low).subspan(index*128,128)),
             "Complete retained original software low-detail payload differs");
        need(!native.load_bitmap_low_detail_resource(current->datum,index),"Prepared owner invented software low-detail selector state");
        low_bytes+=low->pixels.size();decoded+=after.pixels.size();++frames;
        results.push_back({{"frame",index},{"decoded_sha256",sha(after.pixels)}});
    }
    for(const auto [datum,index]:std::array<std::pair<std::uint32_t,unsigned>,3>{{
            {current->datum,17},{current->datum^0x10000u,0},{0xffffffffu,0}}}) {
        bool failed=false;try{(void)native.load_bitmap(datum,index);}catch(const FormatError&){failed=true;}
        need(failed,"Invalid frame/full-handle bitmap admission was accepted");++controls;
    }
    const auto shader=native.query_tag({.group=tag_groups::shader,
        .name=profile.at("consuming_native_shader").at("name").get<std::string>()});
    need(bool(shader),"Actual consuming native water shader absent");
    const auto material=native.load_material(shader->datum,false,false);
    need(!material.bitmaps.empty()&&material.bitmaps[0].datum==current->datum&&material.bitmaps[0].index==0,
         "Actual native water compiled bitmap role did not bind the restored owner");
    const auto water=native.load_bsp_water(1);
    std::size_t water_vertices{},water_materials{};
    for(const auto& definition:water.definitions) {
        water_vertices+=definition.geometry.vertices.size();
        water_materials+=definition.geometry.materials.size();
    }
    need(frames==17&&mips==51&&hardware==34&&decoded==1462272&&low_bytes==2176&&water_vertices>0,
         "Incomplete native source resource/water admission");
    Json report={{"schema","H2_NATIVE_WATER_BEACH_OWNER_ADMISSION_V1"},{"passed",true},
        {"scope","Actual original and prepared native Cache source resources/consumer; no GPU, selector-state or framebuffer parity claim"},
        {"original_datum",old->datum},{"prepared_datum",current->datum},{"prepared_address",current->address},
        {"frames",frames},{"mips",mips},{"exact_hardware_partitions",hardware},{"decoded_rgba_bytes",decoded},
        {"retained_original_metadata_bytes",saved_metadata.size()},{"retained_software_low_detail_bytes",low_bytes},
        {"strict_full_handle_frame_controls",controls},{"bsp1_water_vertices",water_vertices},{"bsp1_water_materials",water_materials},
        {"frames_sha256",results}};
    std::ofstream out(argv[4]);need(bool(out),"Missing native resource report owner");out<<report.dump(2)<<'\n';
    std::cout<<report.dump(2)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

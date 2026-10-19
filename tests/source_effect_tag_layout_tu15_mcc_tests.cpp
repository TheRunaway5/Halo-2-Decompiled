#include "halo2/source_effect_tag_layout_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <set>

namespace {using namespace halo2;
void check(bool value,const char* why){if(!value)throw FormatError(why);}
void actual(const char* path) {
    Cache cache(path);check(cache.is_mcc() && cache.name()=="00a_introduction","Requires selected MCC opening cache");
    unsigned trails{},decals{};std::array<unsigned,6> layers{};
    const std::array<std::array<unsigned,4>,6> expected{{
        {3940682092u,3940747629u,3,0},{3941075314u,3941140851u,4,3},
        {3949857272u,3949922809u,2,3},{3954575936u,3954641473u,2,3},
        {3978038178u,3954641473u,2,3},{3991538800u,3949922809u,2,3}}};
    std::set<unsigned> seen;
    for(const auto& t:cache.tags())if(t.address && (t.group=="cont" || t.group=="deca")) {
        const auto payload=cache.tag_payload(t.handle());const Bytes root(payload);
        const auto bitmap=root.u32(t.group=="cont"?0x34:0x8c);
        const auto* b=cache.find_tag({bitmap},tag_groups::bitmap);
        check(b,"Actual effect bitmap full identity is absent");
        const SourceEffectTagInputTu15 input{t,payload,*b,cache.tag_payload(b->handle())};
        if(t.group=="cont") {
            const auto count=root.u32(0xe8);
            const auto states=count?cache.read_tag_bytes(root.u32(0xec),std::size_t(count)*64):std::span<const std::byte>{};
            const auto result=qualify_source_contrail_tag_layout_tu15(input,states);
            const auto expected_row=std::ranges::find_if(expected,[&](const auto& row){return row[0]==t.datum;});
            check(expected_row!=expected.end() && seen.insert(t.datum).second &&
                result.bitmap.value==(*expected_row)[1] && result.point_states.count==std::int32_t((*expected_row)[2]) &&
                result.blend==(*expected_row)[3] && result.render_type==4 && result.bitmap_images==1,
                "Actual selected MCC contrail tuple changed");++trails;
        } else {
            const auto result=qualify_source_decal_tag_layout_tu15(input);++layers[result.layer];++decals;
        }
        for(unsigned image=0;image<cache.bitmap_count(bitmap);++image) {
            const auto texture=cache.load_bitmap(bitmap,image);
            check(texture.layers==1 && !texture.depth && !texture.pixels.empty(),
                "Actual MCC effect image is not a decoded two-dimensional resource");
        }
    }
    check(trails==6 && decals==31 && layers==std::array<unsigned,6>{0,0,12,6,6,7},
        "Actual selected MCC effect namespace or layers changed");
    const auto* globals=cache.query_tag({.group=tag_groups::globals,.require_address=true});
    check(globals,"Actual native rasterizer globals absent");
    const Bytes root(cache.tag_payload(globals->handle()));
    const Bytes raster(cache.read_tag_bytes(root.u32(0x10c),0x108));
    const Bytes shader_rows(cache.read_tag_bytes(raster.u32(0x5c),raster.u32(0x58)*8u));
    for(unsigned slot:{3u,4u}) {
        const auto shader=shader_rows.u32(slot*8+4);
        const auto code=cache.load_vertex_shader_code(shader,0);
        const auto declaration=cache.load_source_vertex_declaration(shader,0);
        qualify_source_effect_vertex_program_tu15(slot==3?tag_groups::decal:tag_groups::contrail,
            declaration.class_index,code,declaration.semantics);
        auto wrong=declaration.semantics;wrong.back()^=1u;bool failed=false;
        try{qualify_source_effect_vertex_program_tu15(slot==3?tag_groups::decal:tag_groups::contrail,0,code,wrong);}
        catch(const FormatError&){failed=true;}check(failed,"A changed actual vertex declaration was admitted");
    }
    std::cout<<"Actual MCC: "<<trails<<" contrails, "<<decals<<" decals, decoded images and two original programs\n";
}
}
int main(int argc,char** argv)try {
    if(argc!=2)return 2;
    if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    actual(argv[1]);
    std::cout<<"PASS selected MCC effect tag layouts; no World or source publication\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

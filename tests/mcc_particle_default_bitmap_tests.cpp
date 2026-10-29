#include "halo2/cache.hpp"
#include "halo2/particle_material.hpp"
#include "halo2/particle_material_validation.hpp"
#include "halo2/particle_sprite_bitmap_selection.hpp"
#include "halo2/source_particle_sprite_context.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>

using namespace halo2;
namespace {
void check(bool value,const char* message) {if(!value)throw FormatError(message);}
template<std::size_t N>void word(std::array<std::byte,N>& bank,unsigned offset,std::uint32_t value) {
    for(unsigned byte=0;byte<4;++byte)bank[offset+byte]=std::byte(value>>(byte*8));
}
void controls() {
    constexpr std::uint32_t expected=0xe1c30007u;
    std::array<std::byte,2> selectors{};std::array<std::byte,12> bitmaps{};word(bitmaps,0,expected);
    const auto owner=[](std::uint32_t id){return id==expected;};
    check(particle_sprite_default_bitmap(selectors,bitmaps,owner)==expected,
        "Compiled default selector lost its actual imported bitmap handle");
    const auto rejects=[&](auto selected,auto images) {
        bool rejected=false;try {(void)particle_sprite_default_bitmap(selected,images,owner);}
        catch(const FormatError&){rejected=true;}check(rejected,"Malformed default bitmap ownership was admitted");
    };
    auto outside=selectors;outside[0]=std::byte{1};rejects(outside,bitmaps);
    auto negative=selectors;negative[0]=std::byte{0xfe};negative[1]=std::byte{0xff};rejects(negative,bitmaps);
    auto salt=bitmaps;word(salt,0,expected^0x10000u);rejects(selectors,salt);
    auto absent=bitmaps;word(absent,0,0xffffffffu);rejects(selectors,absent);
    rejects(std::span<const std::byte>{},std::span(bitmaps));
    rejects(std::span(selectors).first(1),std::span(bitmaps));
    rejects(std::span(selectors),std::span(bitmaps).first(11));
}
}
int main(int argc,char** argv)try {
    controls();
    if(argc==2 && std::string_view(argv[1])=="--unit") {
        std::cout<<"PASS cooked sprite default selection and seven malformed-selector/handle controls\n";return 0;
    }
    check(argc==3,"Supply actual prepared MCC map and owned JSON report path");
    if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc(),"Default bitmap regression requires imported MCC content");
    const auto* particle=cache.query_tag({.group=tag_groups::particle,
        .name="effects\\objects\\vehicles\\wraith\\turrets\\mortar\\mortar_detonate_spill",.require_address=true});
    check(particle,"Actual Wraith mortar spill particle is absent");
    const Bytes root(cache.tag_payload(particle->handle()));
    check(root.u32(0x14)==0 && root.u32(0x8c)>0,
        "Default-valued particle fixture no longer omits only its authored parameter bank");
    const Bytes definition(cache.read_tag_bytes(root.u32(0x90),124));
    const auto selectors=cache.read_tag_bytes(definition.u32(0x60),std::size_t(definition.u32(0x5c))*2);
    const auto bitmaps=cache.read_tag_bytes(definition.u32(8),std::size_t(definition.u32(4))*12);
    const auto owner=[&](std::uint32_t id){return cache.query_tag({.handle=TagHandle{id},
        .group=tag_groups::bitmap,.require_address=true})!=nullptr;};
    const auto selected=particle_sprite_default_bitmap(selectors,bitmaps,owner);
    const auto sprite=cache.load_particle_sprite(particle->datum);
    check(sprite.bitmap==selected,"Sprite default path replaced its actual cooked bitmap identity");
    const auto context=source_particle_sprite_context(cache,particle->datum,sprite);
    check(context && context->has_bitmap,"Valid compiled default bitmap did not construct a drawable sprite context");
    const auto material=cache.load_particle_material(particle->datum,15,0,0);
    const auto validation=validate_particle_material(material);
    check(material.bitmaps[0] && material.bitmaps[0]->datum==selected,
        "Sprite and custom Vulkan material disagree about their imported default bitmap");
    auto changed=std::vector<std::byte>(bitmaps.begin(),bitmaps.end());
    const auto selected_row=std::size_t(Bytes(selectors).u16(0))*12;
    const auto wrong=selected^0x10000u;
    for(unsigned byte=0;byte<4;++byte)changed[selected_row+byte]=std::byte(wrong>>(byte*8));
    bool rejected=false;try {(void)particle_sprite_default_bitmap(selectors,changed,owner);}
    catch(const FormatError&){rejected=true;}
    check(rejected,"Changed full-salt default bitmap handle bypassed actual Cache ownership");
    nlohmann::json report{{"passed",true},{"map",cache.name()},{"particle",particle->name},
        {"bitmap_datum",selected},{"fragment",validation.fragment_kernel},
        {"source_parameter_count",root.u32(0x14)},{"negative_controls",8},
        {"scope","Actual cooked default sprite/material admission; no simulation or framebuffer claim"}};
    std::ofstream output(argv[2]);check(bool(output),"Cannot write default bitmap regression report");output<<report.dump(2)<<'\n';
    output.close();check(bool(output),"Cannot finish default bitmap regression report");
    std::cout<<"PASS actual Wraith mortar sprite: omitted authored parameter retains cooked bitmap "<<selected<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

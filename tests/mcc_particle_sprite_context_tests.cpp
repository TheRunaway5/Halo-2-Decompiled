#include "halo2/source_particle_sprite_context.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {void check(bool value,const char *why){if(!value)throw FormatError(why);}}
int main(int argc,char **argv)try {
    check(argc==3,"Supply actual MCC High Charity and authenticated forwarding fixture");
    if(!*argv[1])return 77;
    nlohmann::json fixture;std::ifstream(argv[2])>>fixture;
    check(fixture.at("schema")=="H2_MCC_PARTICLE_CONTEXT_FORWARDING_TU15_V1" &&
        fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Forwarding fixture lost its TU1.5 authority");
    Cache cache(argv[1]);check(cache.is_mcc() && cache.name()=="07a_highcharity","Forwarding test needs actual MCC High Charity");
    const auto name=fixture.at("particle").get<std::string>();
    const auto *particle=cache.query_tag({.group=tag_groups::particle,.name=name,.require_address=true});
    check(particle!=nullptr,"Actual forwarded particle is absent");
    const Bytes root(cache.read_tag_bytes(particle->address,0x94));
    const Bytes processed(cache.read_tag_bytes(root.u32(0x90),100));const auto selector=processed.u32(0x60);
    check(!cache.query_tag({.contains=TagAddressRange{selector,2},.require_address=true,.minimum_size=1}),
        "Actual fixture no longer exercises a moved typed allocation");
    const Tag *retained=nullptr;
    for(const auto &tag:cache.retained_tag_allocations())if(selector>=tag.address &&
        std::uint64_t(selector)-tag.address+2<=tag.size) {
        check(!retained,"Actual selector has ambiguous retained ownership");retained=&tag;
    }
    check(retained && retained->group==fixture.at("actual_retained_owner").at("group").get<std::string>() &&
        retained->name==fixture.at("actual_retained_owner").at("name").get<std::string>() &&
        retained->datum==fixture.at("actual_retained_owner").at("datum"),"Literal selector lost its exact salted source owner");
    const auto *current=cache.find_tag(retained->handle(),retained->type());
    check(current && current->name==retained->name && current->address!=retained->address,
        "Retained packet replaced the canonical shader identity");
    const auto before=cache.read_tag_bytes(retained->address,retained->size);
    const std::vector<std::byte> input(before.begin(),before.end());
    auto sprite=cache.load_particle_sprite(particle->datum);
    const auto actual=source_particle_sprite_context(cache,particle->datum,sprite);
    const auto &expected=fixture.at("original_prefix_fixture");
    check(bool(actual)==expected.at("draw"),"Actual MCC sprite admission differs from the named original prefix");
    check(actual && actual->selected_sequence==expected.at("sequence") && actual->has_bitmap==expected.at("has_bitmap") &&
        std::bit_cast<unsigned>(actual->width)==expected.at("width_bits"),"Actual MCC sprite tuple differs from its original reference");
    const auto *bitmap=cache.find_tag({sprite.bitmap},tag_groups::bitmap);
    check(bitmap && bitmap->name==expected.at("bitmap").get<std::string>(),"MCC sprite escaped its authored bitmap identity");
    auto bad=sprite;bad.bitmap^=0x10000u;bool rejected=false;
    try{source_particle_sprite_context(cache,particle->datum,bad);}catch(const FormatError&){rejected=true;}
    check(rejected,"Changed bitmap salt was silently admitted");
    check(std::ranges::equal(cache.read_tag_bytes(retained->address,retained->size),input),"Retained MCC input bytes changed");
    std::cout<<"PASS actual MCC retained selector, canonical identity, original sprite tuple and wrong-salt rejection\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

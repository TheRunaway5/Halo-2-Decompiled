#include "halo2/cache.hpp"
#include "halo2/audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include <algorithm>
#include <fstream>
#include <bit>
#include <cmath>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool ok,const std::string &why){if(!ok)throw FormatError(why);}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto b:bytes){auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
Json reference(const Cache &c,std::uint32_t datum) {
    if(TagHandle{datum}.is_null())return nullptr;
    auto *t=c.query_tag({.handle=TagHandle{datum}});need(t,"Missing full-salt presentation reference");
    return Json::array({t->group,t->name});
}
Json contract(const Cache &c,const Tag &tag) {
    const auto root=tag.address;
    const auto word=[&](auto at){return Bytes(c.read_tag_bytes(at,4)).u32(0);};
    const auto block=[&](std::uint32_t at,unsigned stride) {
        auto bytes=c.read_tag_bytes(at,8);Bytes b(bytes);auto count=b.u32(0),base=b.u32(4);
        need(count<=65536,"Excessive presentation block");
        std::vector<std::uint32_t> result;for(unsigned n=0;n<count;++n)result.push_back(base+n*stride);return result;
    };
    Json zoom{{"levels",std::bit_cast<std::int16_t>(Bytes(c.read_tag_bytes(root+0x1fe,2)).u16(0))},
        {"range_bits",hex(c.read_tag_bytes(root+0x200,8))},
        {"sound_in",reference(c,word(root+0x270))},{"sound_out",reference(c,word(root+0x278))}};
    Json barrels=Json::array(),triggers=Json::array();
    for(auto b:block(root+0x2d0,0xec)) {
        Json rows=Json::array();for(auto e:block(b+0xe4,0x34))rows.push_back({
            {"shot_bounds",hex(c.read_tag_bytes(e,4))},{"normal",reference(c,word(e+8))},
            {"misfire",reference(c,word(e+0x10))},{"empty",reference(c,word(e+0x18))}});
        barrels.push_back(rows);
    }
    for(auto t:block(root+0x2c8,0x40))triggers.push_back({{"charging",reference(c,word(t+0x34))},
        {"charging_damage",reference(c,word(t+0x3c))}});
    return Json{{"zoom",zoom},{"firing",barrels},{"triggers",triggers},{"ready",reference(c,word(root+0x144))}};
}
}
int main(int argc,char **argv)try {
    need(argc==4,"Supply independent original fixture, prepared MCC maps and report");
    if(!std::filesystem::is_regular_file(std::filesystem::path(argv[2])/"lockout.map"))return 77;
    std::ifstream in(argv[1]);const auto fixture=Json::parse(in);
    need(fixture.at("schema")=="H2_TU15_ALL_WEAPON_PRESENTATION_CONTRACTS_V1" &&
        fixture.at("scope_sha256")=="6855b4f33585b332ca82e4b688b9e29c6a7a04eca5757c1bc8fe9b20640c6a1b","Wrong original scope");
    unsigned checked{},maps{},dependency_pcm{};Json failures=Json::array();
    for(const auto &entry:fixture.at("maps")) {
        const auto path=std::filesystem::path(argv[2])/entry.at("name").get<std::string>();
        auto cache_owner=std::make_shared<Cache>(path,argv[2]);const auto &cache=*cache_owner;
        need(cache.is_mcc(),"Native catalog validation requires MCC-only runtime metadata");
        for(const auto &row:entry.at("weapons")) {
            auto name=row.at("weapon").get<std::string>();
            const auto *t=cache.query_tag({.group=tag_groups::weapon,.name=name,.require_address=true});
            need(t,"No resident native weapon "+name);const auto actual=contract(cache,*t);
            const auto &expected=fixture.at("contracts").at(row.at("contract").get<unsigned>());
            if(actual!=expected)failures.push_back({{"map",entry.at("name")},{"weapon",name},{"expected",expected},{"actual",actual}});
            ++checked;
        }
        auto *globals=cache.query_tag({.handle=cache.globals_tag(),.require_address=true});need(globals,"No native globals");
        const Bytes descriptor(cache.read_tag_bytes(globals->address+0x130,8));need(descriptor.u32(0)==1,"Player info count differs");
        const auto info=descriptor.u32(4);const auto word=[&](auto at){return Bytes(cache.read_tag_bytes(at,4)).u32(0);};
        Json binocular{{"levels",word(info+0xb8)},{"sound_in",reference(cache,word(info+0xc8))},{"sound_out",reference(cache,word(info+0xd0))}};
        if(binocular!=entry.at("binocular"))failures.push_back({{"map",entry.at("name")},{"binocular_expected",entry.at("binocular")},{"actual",binocular}});
        if(entry.at("name")=="01b_spacestation.map") {
            Cache donor(std::filesystem::path(argv[2])/"single_player_shared.map",argv[2]);
            // This uses the complete canonical delivery/relocation and actual
            // class authentication, without constructing a physics/device bank.
            const auto sound_metadata=MccSoundMetadataPublication::create(cache_owner);
            for(const auto *name:{"sound\\weapons\\sniper_rifle\\zoom_in","sound\\weapons\\sniper_rifle\\zoom_out"}) {
                const auto *a=cache.query_tag({.group=tag_groups::sound,.name=name,.require_address=true});
                const auto *b=donor.query_tag({.group=tag_groups::sound,.name=name,.require_address=true});
                need(a && b,"Cairo zoom dependency lacks its full native identity");
                const auto variation=sound_metadata.load(cache,a->handle());
                need(variation.datum==a->datum && variation.pitch_ranges.size()==1 && variation.permutations.size()==1,
                    "Cairo zoom dependency is absent from the complete native sound publication");
                const auto actual=cache.load_sound(a->datum),expected=donor.load_sound(b->datum);
                need(actual.data==expected.data && actual.sample_rate==expected.sample_rate && actual.channels==expected.channels &&
                    actual.codec==expected.codec && actual.samples==expected.samples && actual.sound_class==expected.sound_class &&
                    actual.packet_chunks.size()==expected.packet_chunks.size(),"Cairo zoom dependency changed its actual encoded payload/codec/extent");
                for(unsigned n=0;n<actual.packet_chunks.size();++n) {
                    const auto &x=actual.packet_chunks[n],&y=expected.packet_chunks[n];
                    need(x.bytes==y.bytes && x.pcm_bytes==y.pcm_bytes && x.reset_after==y.reset_after,"Cairo zoom changed codec packet boundaries");
                }
                const auto pcm=decode_sound(actual);double energy{};
                for(auto value:pcm.stereo){need(std::isfinite(value),"Cairo zoom decoded nonfinite PCM");energy+=double(value)*value;}
                need(pcm.frames()>0 && energy>0,"Cairo zoom dependency has no audible native PCM");++dependency_pcm;
            }
        }
        std::cout<<"Checked native "<<entry.at("name")<<" "<<entry.at("weapons").size()<<" weapons\n"<<std::flush;++maps;
    }
    need(maps==38 && checked==1110,"Incomplete full authored catalog");
    Json report{{"passed",failures.empty()},{"maps",maps},{"weapon_rows",checked},{"weapon_paths",63},
        {"failures",failures},{"cairo_zoom_pcm_dependencies",dependency_pcm},{"runtime_cache_source","MCC"},{"full_output_parity",false}};
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return failures.empty()?0:1;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

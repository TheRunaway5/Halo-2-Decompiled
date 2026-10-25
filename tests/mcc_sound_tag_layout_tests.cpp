#include "halo2/mcc_sound_tag_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace halo2;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T> void word(T& out,unsigned at,std::uint32_t value) {
    Bytes(out).require(at,4);for(unsigned n=0;n<4;++n)out[at+n]=std::byte(value>>(8*n));
}
template<class F> void format_rejected(F&& f) {
    bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}
    require(rejected,"Invalid metadata format admitted");
}
template<class F> void required(F&& f,MccSoundLayoutNeed expected) {
    bool rejected=false;try{f();}catch(const MccSoundLayoutRequirement& e){rejected=e.need()==expected;}
    require(rejected,"Unowned sound resource did not report its typed requirement");
}
}
int main(int argc,char**argv)try {
    using namespace halo2;
    std::array<std::byte,0x34> looping{};
    for(unsigned n=0;n<looping.size();++n)looping[n]=std::byte(n);
    word(looping,0,tag_groups::sound_looping.value);word(looping,4,0xffffffffu);
    word(looping,8,0x118u);word(looping,0x24,3);word(looping,0x28,0x81234560u);
    word(looping,0x2c,8);word(looping,0x30,0x82345670u);
    const auto original_loop=looping;
    const auto projected_loop=project_mcc_looping_sound_root(looping);
    require(Bytes(projected_loop).u32(0)==0x18u && Bytes(projected_loop).u32(0x1c)==3 &&
        Bytes(projected_loop).u32(0x20)==0x81234560u && Bytes(projected_loop).u32(0x24)==8 &&
        Bytes(projected_loop).u32(0x28)==0x82345670u,"Loop root retained MCC header/route bits or moved its native descriptors");
    require(std::ranges::equal(std::span(projected_loop).subspan(4),std::span(looping).subspan(12)),
        "Loop projection changed an original authored root field");
    require(looping==original_loop,"Loop projection modified its donor owner");
    format_rejected([&]{project_mcc_looping_sound_root(std::span(looping).first(0x33));});
    std::array<std::byte,0x2c> sound_globals;
    for(unsigned n=0;n<sound_globals.size();++n)sound_globals[n]=std::byte(n);
    word(sound_globals,4,0xe1234567u);word(sound_globals,12,0xf2345678u);word(sound_globals,0x28,0xa9871234u);
    const auto vanilla_globals=project_mcc_sound_globals(sound_globals);
    require(std::ranges::equal(vanilla_globals,std::span(sound_globals).subspan(8)),"Sound globals fields changed beyond removing remastered reference");
    require(Bytes(vanilla_globals).u32(4)==0xf2345678u && Bytes(vanilla_globals).u32(0x20)==0xa9871234u,
        "Legacy classes/gestalt full salted handles changed");

    std::array<std::byte,0x1c> sound;
    for(unsigned n=0;n<sound.size();++n)sound[n]=std::byte(n*7);
    sound[3]=std::byte{1};sound[4]=std::byte{0x34};sound[5]=std::byte{0x12};
    const std::array<std::byte,6> codecs{std::byte{1},std::byte{0},std::byte{1},std::byte{3},std::byte{1},std::byte{5}};
    // Four compiler bits move by two positions because MCC inserts two
    // playback-route bits. Exercise every possible serialized flag word.
    for(unsigned flags=0;flags<65536;++flags) {
        sound[0]=std::byte(flags);sound[1]=std::byte(flags>>8);
        const auto converted=project_mcc_sound_definition(sound,codecs);
        unsigned expected=flags&0x3ffu;
        for(const auto [mcc,native]:{std::pair(0x1000u,0x400u),{0x2000u,0x800u},
            {0x4000u,0x1000u},{0x8000u,0x2000u}})if(flags&mcc)expected|=native;
        require(Bytes(converted.definition).u16(0)==expected,
            "MCC route/default flag bits entered the Xbox gameplay flag domain");
        for(unsigned n=2;n<20;++n)if(n==2 || n>5)require(converted.definition[n]==sound[n],
            "Flag conversion changed another gameplay field");
    }
    sound[0]=std::byte{0x23};sound[1]=std::byte{0x74};
    const auto definition=project_mcc_sound_definition(sound,codecs);
    require(definition.codec_index==1 && definition.remastered_sound_index==0x1234 &&
        definition.definition[3]==std::byte{3} && definition.definition[4]==std::byte{1} &&
        definition.definition[5]==std::byte{5} && !definition.original_adpcm_admission(),"MCC codec index spoofed as an Xbox codec");
    for(unsigned n=0;n<20;++n)if(n==2 || n>5)
        require(definition.definition[n]==sound[n],"Sound gameplay scalar/duration/index bits changed");
    require(Bytes(definition.definition).u16(0)==0x1c23,
        "MCC Legacy-route bit became the original minimum-distance default");
    require(std::ranges::equal(definition.mcc_extension,std::span(sound).subspan(20)),"MCC extension lost from explicit sidecar");
    required([&]{definition.require_original_duration_domain();},MccSoundLayoutNeed::NativeCodecDuration);
    sound[3]=std::byte{0};const auto original_codec=project_mcc_sound_definition(sound,codecs);
    require(original_codec.original_adpcm_admission(),"Original ADPCM predicate differs");original_codec.require_original_duration_domain();
    sound[3]=std::byte{2};format_rejected([&]{project_mcc_sound_definition(sound,codecs);});

    std::vector<std::byte> class_rows(3*0x78);
    for(unsigned n=0;n<class_rows.size();++n)class_rows[n]=std::byte(n*11);
    const auto classes=project_mcc_sound_class_records(class_rows);
    require(classes.size()==3*0x5c,"Native sound class stride differs");
    for(unsigned n=0;n<3;++n)require(std::ranges::equal(std::span(classes).subspan(n*0x5c,0x5c),
        std::span(class_rows).subspan(n*0x78,0x5c)),"Authored class gain/ducking/priority bits changed");
    std::array<std::byte,8> class_root{};word(class_root,0,3);word(class_root,4,0x12345678);
    const auto projected_class_root=project_mcc_sound_class_root(class_root,0x80080000);
    require(Bytes(projected_class_root).u32(0)==3 && Bytes(projected_class_root).u32(4)==0x80080000,
        "Sound class owner count/address differs");

    std::array<std::byte,0x78> gestalt{};std::array<std::byte,8> language{};
    for(unsigned n=0;n<gestalt.size();++n)gestalt[n]=std::byte(n);
    word(gestalt,0x30,2);word(gestalt,0x70,4);word(language,0,3);
    MccSoundResourceBlocks resources;
    required([&]{project_mcc_sound_gestalt_root(gestalt,language,resources);},MccSoundLayoutNeed::NativePermutationOwner);
    resources.permutations=TagBlock{2,0x80090000};
    required([&]{project_mcc_sound_gestalt_root(gestalt,language,resources);},MccSoundLayoutNeed::NativeChunkOwner);
    resources.chunks=TagBlock{3,0x800a0000};
    required([&]{project_mcc_sound_gestalt_root(gestalt,language,resources);},MccSoundLayoutNeed::NativeExtraInfoOwner);
    resources.extra_info=TagBlock{4,0x800b0000};
    const auto root=project_mcc_sound_gestalt_root(gestalt,language,resources);
    for(const auto [from,to]:{std::pair(8,0),std::pair(0x10,8),std::pair(0x18,0x10),std::pair(0x20,0x18),
        std::pair(0x28,0x20),std::pair(0x38,0x30),std::pair(0x58,0x38),std::pair(0x68,0x48)})
        require(std::ranges::equal(std::span(root).subspan(to,8),std::span(gestalt).subspan(from,8)),
            "Common ugh gameplay descriptor changed");
    require(Bytes(root).u32(0x2c)==0x80090000 && Bytes(root).u32(0x44)==0x800a0000 &&
        Bytes(root).u32(0x54)==0x800b0000,"Native resource owners were replaced by donor descriptors");
    resources.chunks=TagBlock{4,0x800a0000};format_rejected([&]{project_mcc_sound_gestalt_root(gestalt,language,resources);});

    std::array<std::byte,16> permutation{};std::array<std::byte,8> info{};
    word(permutation,0,0x89abcdef);word(permutation,4,0x01234567);word(info,0,123456);word(info,4,0x00030002);
    required([&]{project_mcc_sound_permutation(permutation,info,{});},MccSoundLayoutNeed::NativeDecodedSampleExtent);
    const auto projected_permutation=project_mcc_sound_permutation(permutation,info,30000);
    require(Bytes(projected_permutation).u32(0)==0x89abcdef && Bytes(projected_permutation).u32(4)==0x01234567 &&
        Bytes(projected_permutation).u32(8)==30000 && Bytes(projected_permutation).u32(12)==0x00030002,
        "Permutation authored bits or native backend extent changed");
    std::array<std::byte,16> chunk{};word(chunk,0,0x123400);word(chunk,4,0xa0001234);word(chunk,8,480);word(chunk,12,0xffffffff);
    const auto projected_chunk=project_mcc_sound_chunk(chunk);
    require(Bytes(projected_chunk.chunk).u32(0)==0x123400 && Bytes(projected_chunk.chunk).u32(4)==0x80001234 &&
        Bytes(projected_chunk.chunk).u32(8)==0xffffffff && projected_chunk.decoded_bytes==480 && projected_chunk.reset_after,
        "MCC reset flag became an original size bit or runtime owner changed");
    word(chunk,12,7);required([&]{project_mcc_sound_chunk(chunk);},MccSoundLayoutNeed::ForeignRuntimeCacheHandle);

    unsigned actual_globals=0,actual_classes=0,actual_sounds=0,extended_codecs=0;
    std::map<std::uint32_t,unsigned> gestalt_owners;
    if(argc>1) {
        Cache cache(argv[1]);
        for(const auto& tag:cache.tags())if(tag.address && tag.size) {
            if(tag.group=="matg") {
                const Bytes g(cache.read_tag_bytes(tag.address,tag.size));const auto count=g.u32(0xc0),pointer=g.u32(0xc4);
                require(count==1,"Actual MCC Sound Globals count differs");
                const auto donor=cache.read_tag_bytes(pointer,0x2c);const std::vector<std::byte> before(donor.begin(),donor.end());
                const auto projected=project_mcc_sound_globals(donor);
                const auto* classes=cache.find_tag({Bytes(projected).u32(4)},tag_groups::sound_classes);
                const auto* gestalt=cache.find_tag({Bytes(projected).u32(0x20)},tag_groups::sound_cache_file_gestalt);
                require(classes && gestalt,"Projected Sound Globals references lost exact live identities");
                require(std::ranges::equal(before,cache.read_tag_bytes(pointer,0x2c)),"Sound Globals projection changed Cache donor");
                ++actual_globals;
            } else if(tag.group=="sncl") {
                const Bytes header(cache.read_tag_bytes(tag.address,8));const auto count=header.u32(0);
                const auto donor=cache.read_tag_bytes(header.u32(4),std::size_t(count)*0x78);
                const std::vector<std::byte> before(donor.begin(),donor.end());
                const auto projected=project_mcc_sound_class_records(donor);
                for(unsigned n=0;n<count;++n)require(std::ranges::equal(std::span(projected).subspan(n*0x5c,0x5c),donor.subspan(n*0x78,0x5c)),
                    "Actual class scalar bits changed");
                require(std::ranges::equal(before,cache.read_tag_bytes(header.u32(4),std::size_t(count)*0x78)),"Sound class projection changed Cache donor");
                actual_classes+=count;
            } else if(tag.group=="snd!") {
                const auto& owner=cache.mcc_sound_gestalt_owner(tag.handle());
                require(cache.find_tag(owner.handle(),tag_groups::sound_cache_file_gestalt)==&owner,
                    "Sound codec owner lost its salted live identity");
                const Bytes header(cache.read_tag_bytes(owner.address,0x78));
                const auto bank=cache.read_tag_bytes(header.u32(4),std::size_t(header.u32(0))*3);
                const auto donor=cache.read_tag_bytes(tag.address,0x1c);
                const std::vector<std::byte> before(donor.begin(),donor.end());
                const auto projected=project_mcc_sound_definition(donor,bank);
                const auto codec_at=std::size_t(std::to_integer<unsigned>(donor[3]))*3;
                require(std::ranges::equal(std::span(projected.definition).subspan(3,3),bank.subspan(codec_at,3)),
                    "Actual native header used raw codec index or wrong owning gestalt");
                const auto raw_flags=Bytes(donor).u16(0);
                require(Bytes(projected.definition).u16(0)==((raw_flags&0x3ffu)|((raw_flags&0xf000u)>>2)),
                    "Actual compiled distance/cone/pitch flags were not normalized");
                for(unsigned n=0;n<20;++n)if(n==2 || n>5)require(projected.definition[n]==donor[n],
                    "Actual native sound gameplay field bits changed");
                require(std::ranges::equal(before,cache.read_tag_bytes(tag.address,0x1c)),"Sound header projection changed Cache donor");
                if(projected.codec.sample_rate==3 || projected.codec.encoding==3 || projected.codec.compression==5)++extended_codecs;
                ++actual_sounds;++gestalt_owners[owner.datum];
            }
        }
    }
    std::cout<<"MCC sound layout projection tests passed; globals="<<actual_globals<<" class_rows="<<actual_classes
        <<" sounds="<<actual_sounds<<" extended_codecs="<<extended_codecs<<" gestalt_owners=";
    for(const auto [datum,count]:gestalt_owners)std::cout<<std::hex<<datum<<std::dec<<':'<<count<<',';
    std::cout<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

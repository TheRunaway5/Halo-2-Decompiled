#include "halo2/mcc_gameplay_scalar.hpp"
#include "halo2/source_effect_event.hpp"
#include "halo2/source_effect_random.hpp"
#ifdef HALO2_DESTRUCTION_CACHE_CHECK
#include "halo2/cache.hpp"
#endif
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t bits(const nlohmann::json &value){return std::stoul(value.get<std::string>(),nullptr,16);}
void put(std::span<std::byte> payload,std::uint32_t value){
    for(unsigned n=0;n<4;++n)payload[n]=std::byte(value>>(n*8));
}
std::array<std::uint32_t,2> event_words(std::uint32_t flags){
    std::array<std::uint32_t,2> words{0x12345678u,0x6a09e667u};
    SourceEffectRandom random{words[0],words[1]};
    // Declared component event range forces the original select operation to
    // draw once. These are not fabricated live clocks or destruction results.
    const std::array events{SourceEffectEventDefinition{0,.1f,.2f,0,0}};
    SourceEffectEventState state;
    check(source_effect_event_select(state,events,0,random.event(flags),{.synchronize=[] {}}),
        "Component event selection unexpectedly rejected its explicit input");
    return words;
}
#ifdef HALO2_DESTRUCTION_CACHE_CHECK
void check_cache(const nlohmann::json &manifest,const char *map,const char *maps){
    const Cache cache(map,maps);check(cache.is_mcc(),"Effect integration requires MCC content");
    unsigned matched=0;
    for(const auto &row:manifest.at("profiles"))for(const auto &tag:cache.select_tags({.group=tag_groups::effect,.require_address=true})){
        if(tag.name!=row.at("name").get<std::string>())continue;
        const auto flags=Bytes(cache.read_tag_bytes(tag.address,4)).u32(0);
        if(flags!=bits(row.at("target_bits")))
            throw std::runtime_error("Cache did not publish the original effect RNG flags: "+tag.name);
        ++matched;
    }
    check(matched>0,"MCC map has none of the source-backed named effects");
    std::cout<<"PASS MCC cache admission of "<<matched<<" exact effect flag words\n";
}
#endif
}
int main(int argc,char **argv)try {
    check(argc==2 || argc==4,"Supply PROFILE_MANIFEST [MCC_MAP MCC_MAPS]");
    std::ifstream input(argv[1]);const auto manifest=nlohmann::json::parse(input);
    const bool turret=manifest.at("schema")=="H2_MCC_TU15_TURRET_EFFECT_FLAGS_V1";
    check(turret || manifest.at("schema")=="H2_MCC_TU15_DESTRUCTION_EFFECT_FLAGS_V1","Wrong effect profile authority");
    check(manifest.at("profiles").size()==(turret?5u:25u),"Incomplete named effect census");
#ifdef HALO2_DESTRUCTION_CACHE_CHECK
    if(argc==4)check_cache(manifest,argv[2],argv[3]);
#else
    check(argc==2,"Standalone component build cannot claim Cache integration");
#endif
    unsigned converted=0;
    for(const auto &row:manifest.at("profiles")){
        const auto name=row.at("name").get<std::string>();
        const auto donor=bits(row.at("donor_bits")),target=bits(row.at("target_bits"));
        check((donor^target)==4u,"Reviewed tuple changes more than the original event RNG selector");
        auto bytes=std::vector<std::byte>(0x30,std::byte{0x57});put(bytes,donor);
        auto expected=bytes;put(expected,target);
        check(event_words(donor)!=event_words(target),"Regression does not distinguish the wrong original RNG stream");
        const auto receipt=convert_mcc_gameplay_scalar(tag_groups::effect,name,bytes);
        check(receipt && receipt->status==MccGameplayScalarStatus::Converted && bytes==expected,
            "Named MCC effect retains the wrong TU1.5 event RNG selector");
        check(event_words(Bytes(bytes).u32(0))==event_words(target),"Converted effect advances the wrong original RNG word");
        const auto again=convert_mcc_gameplay_scalar(tag_groups::effect,name,bytes);
        check(again && again->status==MccGameplayScalarStatus::AlreadyEquivalent && bytes==expected,"Effect conversion is not idempotent");
        put(bytes,donor|0x80000000u);const auto custom=bytes;
        const auto authored=convert_mcc_gameplay_scalar(tag_groups::effect,name,bytes);
        check(authored && authored->status==MccGameplayScalarStatus::AuthoredTupleUnrecognized && bytes==custom,
            "Effect conversion overwrote an unknown authored flag tuple");
        check(!convert_mcc_gameplay_scalar(tag_groups::effect,name+"_custom",bytes) && bytes==custom,
            "Effect conversion changed a custom named effect");
        check(!convert_mcc_gameplay_scalar(tag_groups::damage_effect,name,bytes) && bytes==custom,
            "Effect conversion reinterpreted a different tag group");
        auto short_bytes=std::vector<std::byte>(3,std::byte{0x57});const auto short_before=short_bytes;
        bool rejected=false;
        try{(void)convert_mcc_gameplay_scalar(tag_groups::effect,name,short_bytes);}
        catch(const FormatError&){rejected=true;}
        check(rejected && short_bytes==short_before,"Truncated effect flags were partially converted");
        ++converted;
    }
    std::cout<<"PASS "<<converted<<" exact effect tuples, source event RNG words, idempotence and custom-content guards\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/mcc_destruction_model_reference.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#ifdef HALO2_DESTRUCTION_MODEL_REFERENCE_CACHE_CHECK
#include "halo2/mcc_runtime_delivery.hpp"
#endif

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value){
    for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(n*8));
}
std::uint32_t get(std::span<const std::byte> bytes,std::size_t at){
    std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[at+n])<<(n*8);
    return value;
}
constexpr std::uint32_t base=0x61000000;
}
int main(int argc,char **argv)try {
    check(argc==2 || argc==4,"Supply original model-reference profile [MCC_MAP MCC_MAPS]");
    std::ifstream input(argv[1]);const auto profile=nlohmann::json::parse(input);
    check(profile.at("schema")=="H2_MCC_TU15_FLOOD_CLUMP_REFERENCE_V1","Wrong model-reference authority");
    const auto name=profile.at("model").get<std::string>(),effect=profile.at("donor_effect").get<std::string>();
    const auto handle=profile.at("response_words").at(4).get<std::uint32_t>();
    std::vector<std::byte> donor(0x800,std::byte{0x57});
    put(donor,0x60,1);put(donor,0x64,base+0x100);put(donor,0x1bc,1);put(donor,0x1c0,base+0x240);
    put(donor,0x24c,1);put(donor,0x250,base+0x300);
    std::vector<std::size_t> guarded;
    for(const auto &[offset,value]:profile.at("damage_words").items()) {
        const auto at=0x100+std::stoul(offset,nullptr,16);put(donor,at,std::stoul(value.get<std::string>(),nullptr,16));
        guarded.push_back(at);
    }
    for(const auto &[offset,value]:profile.at("section_words").items()) {
        const auto at=0x240+std::stoul(offset,nullptr,16);put(donor,at,std::stoul(value.get<std::string>(),nullptr,16));
        guarded.push_back(at);
    }
    put(donor,0x270,0);guarded.push_back(0x270);guarded.push_back(0x240);
    check(profile.at("response_words").size()==20,"Flood-clump response profile is incomplete");
    for(unsigned n=0;n<20;++n){put(donor,0x300+n*4,profile.at("response_words").at(n));if(n!=4)guarded.push_back(0x300+n*4);}
    auto bytes=donor;
    const auto read=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
        if(address<base || std::uint64_t(address-base)+size>bytes.size())throw FormatError("Model-reference metadata escaped");
        return std::span(bytes).subspan(address-base,size);
    };
    const auto resolve=[&](TagHandle h)->std::optional<MccGameplayReferenceIdentity>{
        if(h.value==handle)return MccGameplayReferenceIdentity{h,tag_groups::effect,effect};return {};
    };
    const MccGameplayReferenceAccess access{read,resolve,{}};
    const auto convert=[&](const MccGameplayReferenceAccess &a){return convert_mcc_destruction_model_references(tag_groups::model,name,base,a);};
    unsigned controls=0;
    for(const auto &section_name:profile.at("section_names")) {
        bytes=donor;put(bytes,0x240,section_name);const auto before=bytes;auto expected=before;put(expected,0x310,0xffffffff);
        auto receipt=convert(access);
        check(receipt && receipt->status==MccGameplayReferenceStatus::Converted && bytes==expected,
            "Known Flood-clump response retains a donor effect or changes unrelated bytes");
        check(receipt->field_count==1 && receipt->fields[0].address==base+0x30c &&
            receipt->fields[0].before.group==tag_groups::effect && receipt->fields[0].before.handle.value==handle &&
            receipt->fields[0].after.group==tag_groups::effect && receipt->fields[0].after.handle.is_null(),
            "Model-reference receipt lost the original null/group identity");
        auto no_resolver=access;no_resolver.resolve={};receipt=convert(no_resolver);
        check(receipt && receipt->status==MccGameplayReferenceStatus::AlreadyEquivalent && bytes==expected,
            "Original null effect is not idempotent without a donor resolver");
        for(const auto at:guarded) {
            bytes=before;put(bytes,at,get(bytes,at)^0x80000000u);const auto unknown=bytes;receipt=convert(access);
            check(receipt && receipt->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && bytes==unknown,
                "Unknown model-reference tuple was partially changed");++controls;
        }
        for(const auto at:{0x60u,0x1bcu,0x24cu}) {
            bytes=before;put(bytes,at,2);const auto unknown=bytes;receipt=convert(access);
            check(receipt && receipt->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && bytes==unknown,
                "Unknown model-reference shape was changed");++controls;
        }
        for(unsigned identity=0;identity<4;++identity) {
            bytes=before;auto altered=access;
            altered.resolve=[&](TagHandle h)->std::optional<MccGameplayReferenceIdentity>{
                if(identity==0)return {};
                return MccGameplayReferenceIdentity{TagHandle{h.value^(identity==1?0x10000u:0u)},
                    identity==2?tag_groups::projectile:tag_groups::effect,identity==3?std::string_view("unknown"):std::string_view(effect)};
            };
            receipt=convert(altered);
            check(receipt && receipt->status==(identity==0?MccGameplayReferenceStatus::RequiredDependencyUnavailable:
                MccGameplayReferenceStatus::AuthoredTupleUnrecognized) && bytes==before,
                "Unavailable or wrong full local effect identity was accepted");++controls;
        }
        for(unsigned malformed=0;malformed<11;++malformed) {
            bytes=before;auto altered=access;bool rejected=false;
            try {
                if(malformed==0)(void)convert_mcc_destruction_model_references(tag_groups::model,name,0xffffffe0,access);
                else {
                    if(malformed==1)altered.bytes={};
                    else if(malformed==2)altered.resolve={};
                    else if(malformed==3 || malformed==4)altered.bytes=[&](auto address,auto size){
                        auto view=read(address,size);return (malformed==3 || (address==base+0x310 && size==4))?view.first(size-1):view;
                    };
                    else put(bytes,malformed<7?0x64:malformed<9?0x1c0:0x250,malformed%2?0:0xffffffff);
                    (void)convert(altered);
                }
            }catch(const FormatError&){rejected=true;}
            // Deliberately malformed descriptor inputs stay byte-for-byte as supplied.
            auto unchanged=before;
            if(malformed>=5)put(unchanged,malformed<7?0x64:malformed<9?0x1c0:0x250,malformed%2?0:0xffffffff);
            check(rejected && bytes==unchanged,"Malformed model-reference metadata was partially changed");++controls;
        }
        bytes=before;
        check(!convert_mcc_destruction_model_references(tag_groups::vehicle,name,base,{}) &&
            !convert_mcc_destruction_model_references(tag_groups::model,name+"_custom",base,{}) && bytes==before,
            "Model-reference correction selected another group or custom path");
    }
#ifdef HALO2_DESTRUCTION_MODEL_REFERENCE_CACHE_CHECK
    if(argc==4) {
        const Cache cache(argv[2],argv[3]);check(cache.is_mcc(),"Model-reference native proof requires MCC");unsigned matched=0;
        for(const auto &tag:cache.tags())if(tag.address && tag.group=="hlmt" && tag.name==name) {
            const auto word=[&](auto address){return Bytes(cache.read_tag_bytes(address,4)).u32(0);};
            check(word(tag.address+0x60)==1,"Native Flood-clump damage block shape differs");const auto info=word(tag.address+0x64);
            check(word(info+0xbc)==1,"Native Flood-clump section shape differs");const auto section=word(info+0xc0);
            check(word(section+0xc)==1,"Native Flood-clump response shape differs");const auto response=word(section+0x10);
            check(word(response+0xc)==tag_groups::effect.value,"Native Flood-clump effect group differs");
            if(word(response+0x10)!=0xffffffff)throw FormatError("Native Cache Flood-clump response effect differs: expected=null actual=nonnull");
            const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u);
            check(delivery.issues().empty(),"Flood-clump metadata delivery has unresolved owners");const auto image=delivery.tag_image();
            const auto native_word=[&](std::uint32_t address){
                for(const auto &region:image.regions)if(address>=region.address && std::uint64_t(address)-region.address+4<=region.bytes.size())
                    return Bytes(region.bytes).u32(address-region.address);
                throw FormatError("Flood-clump native metadata escaped its owner");
            };
            const auto native_response=delivery.runtime_address(response,0x50);
            check(native_word(native_response+0xc)==tag_groups::effect.value && native_word(native_response+0x10)==0xffffffff,
                "Delivered Flood-clump response retains a donor effect");++matched;
        }
        check(matched==1,"Flood-clump model is absent or ambiguous");
    }
#else
    check(argc==2,"Pure model-reference regression cannot claim Cache admission");
#endif
    std::cout<<"PASS original Flood-clump null, whole-buffer preservation and "<<controls<<" unknown/malformed identity controls\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

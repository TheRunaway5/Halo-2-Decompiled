#include "halo2/mcc_destruction_model_contract.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#ifdef HALO2_DESTRUCTION_HUNTER_CACHE_CHECK
#include "halo2/mcc_runtime_delivery.hpp"
#endif

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value){
    for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(8*n));
}
std::uint32_t get(std::span<const std::byte> bytes,std::size_t at){
    std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[at+n])<<(8*n);
    return value;
}
constexpr std::uint32_t base=0x61000000;
}
int main(int argc,char **argv)try {
    check(argc==2 || argc==4,"Supply reviewed Hunter profile [MCC_MAP MCC_MAPS]");
    std::ifstream file(argv[1]);const auto profile=nlohmann::json::parse(file);
    check(profile.at("schema")=="H2_MCC_TU15_HUNTER_CONTRACT_V1","Wrong Hunter authored authority");
    const auto name=profile.at("model").get<std::string>();
    std::vector<std::byte> donor(0x800,std::byte{0x57});
    put(donor,0x60,1);put(donor,0x64,base+0x100);
    put(donor,0x1bc,2);put(donor,0x1c0,base+0x240);
    std::vector<std::size_t> guarded;
    for(const auto &[offset,value]:profile.at("damage_words").items()) {
        const auto at=0x100+std::stoul(offset,nullptr,16);
        put(donor,at,std::stoul(value.get<std::string>(),nullptr,16));guarded.push_back(at);
    }
    for(const auto section:{0x240u,0x278u}) {
        for(const auto &[offset,value]:profile.at("section_words").items()) {
            const auto at=section+std::stoul(offset,nullptr,16);
            put(donor,at,std::stoul(value.get<std::string>(),nullptr,16));guarded.push_back(at);
        }
        put(donor,section+0x30,0);guarded.push_back(section+0x30);
    }
    put(donor,0x27c,0);put(donor,0x284,0);put(donor,0x288,0);
    guarded.push_back(0x284);guarded.push_back(0x288);
    put(donor,0x24c,1);put(donor,0x250,base+0x300);
    auto bytes=donor;
    const auto read=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
        if(address<base || std::uint64_t(address-base)+size>bytes.size())throw FormatError("Hunter metadata escaped");
        return std::span(bytes).subspan(address-base,size);
    };
    const auto convert=[&]{return convert_mcc_destruction_model_contract(tag_groups::model,name,base,read);};
    unsigned controls=0;
    for(unsigned map=0;map<profile.at("response_tuples").size();++map) {
        const auto &tuple=profile.at("response_tuples").at(map);
        check(tuple.size()==20,"Hunter response profile is incomplete");
        bytes=donor;
        put(bytes,0x240,profile.at("section_names").at(map).at(0));
        put(bytes,0x278,profile.at("section_names").at(map).at(1));
        for(unsigned n=0;n<20;++n)put(bytes,0x300+n*4,tuple.at(n));
        const auto input=bytes;auto expected=input;
        for(const auto at:{0x244,0x24c,0x250})put(expected,at,0);
        auto receipt=convert();
        check(receipt && receipt->status==MccDestructionModelContractStatus::Converted && bytes==expected,
            "Known Hunter input retains the MCC body response or changes unrelated bytes");
        check(receipt->addresses==std::array<std::uint32_t,3>{base+0x244,base+0x24c,base+0x250} &&
            receipt->before==std::array<std::uint32_t,3>{2,1,base+0x300} &&
            receipt->after==std::array<std::uint32_t,3>{0,0,0},"Hunter conversion lost its descriptor receipt");
        receipt=convert();check(receipt && receipt->status==MccDestructionModelContractStatus::AlreadyEquivalent && bytes==expected,
            "Hunter original empty descriptor is not idempotent");
        auto fields=guarded;fields.push_back(0x240);fields.push_back(0x278);
        for(unsigned n=0;n<20;++n)fields.push_back(0x300+n*4);
        for(const auto at:fields) {
            bytes=input;put(bytes,at,get(bytes,at)^0x80000000u);const auto before=bytes;
            receipt=convert();check(receipt && receipt->status==MccDestructionModelContractStatus::AuthoredTupleUnrecognized && bytes==before,
                "An unknown Hunter authored tuple was partially converted");++controls;
        }
        for(const auto at:{0x60u,0x1bcu,0x24cu}) {
            bytes=input;put(bytes,at,3);const auto before=bytes;receipt=convert();
            check(receipt && receipt->status==MccDestructionModelContractStatus::AuthoredTupleUnrecognized && bytes==before,
                "An unknown Hunter block shape was changed");++controls;
        }
        bytes=input;
        for(unsigned malformed=0;malformed<6;++malformed) {
            const auto before=bytes;bool rejected=false;
            try {
                if(malformed==0)(void)convert_mcc_destruction_model_contract(tag_groups::model,name,0xffffffe0,read);
                else if(malformed==1)(void)convert_mcc_destruction_model_contract(tag_groups::model,name,base,{});
                else (void)convert_mcc_destruction_model_contract(tag_groups::model,name,base,
                    [&](auto address,auto size){
                        const auto result=read(address,size);
                        if((malformed==2 && size>4) || (size==4 && address==base+(malformed==3?0x244:malformed==4?0x24c:0x250)))
                            return result.first(size-1);
                        return result;
                    });
            }catch(const FormatError&){rejected=true;}
            check(rejected && bytes==before,"Malformed Hunter metadata was partially converted");++controls;
        }
    }
    const auto before=bytes;
    check(!convert_mcc_destruction_model_contract(tag_groups::vehicle,name,base,{}) &&
        !convert_mcc_destruction_model_contract(tag_groups::model,name+"_custom",base,{}) && bytes==before,
        "Hunter correction changed another group or custom name");
#ifdef HALO2_DESTRUCTION_HUNTER_CACHE_CHECK
    if(argc==4) {
        const Cache cache(argv[2],argv[3]);check(cache.is_mcc(),"Hunter Cache proof requires MCC");
        unsigned matched=0;
        for(const auto &tag:cache.tags())if(tag.address && tag.group=="hlmt" && tag.name==name) {
            const auto word=[&](auto address){return Bytes(cache.read_tag_bytes(address,4)).u32(0);};
            check(word(tag.address+0x60)==1,"Native Hunter damage block shape differs");
            const auto info=word(tag.address+0x64);check(word(info+0xbc)==2,"Native Hunter section shape differs");
            const auto sections=word(info+0xc0);
            for(const auto base:{0u,0x38u})for(const auto field:{4u,0xcu,0x10u}) {
                const auto actual=word(sections+base+field);
                if(actual)throw FormatError("Native Cache Hunter original empty response differs: section="+
                    std::to_string(base/0x38)+" field="+std::to_string(field)+" expected=0 actual="+std::to_string(actual));
            }
            const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u);
            check(delivery.issues().empty(),"Hunter native metadata delivery has unresolved owners");
            const auto image=delivery.tag_image();
            const auto native_word=[&](std::uint32_t address){
                for(const auto &region:image.regions)if(address>=region.address &&
                    std::uint64_t(address)-region.address+4<=region.bytes.size())
                    return Bytes(region.bytes).u32(address-region.address);
                throw FormatError("Hunter native metadata escaped its delivered owner");
            };
            const auto native_model=delivery.runtime_address(tag.address,0x68);
            check(native_word(native_model+0x60)==1,"Native Hunter damage count changed");
            const auto native_info=native_word(native_model+0x64);
            check(native_word(native_info+0xbc)==2,"Native Hunter section count changed");
            const auto native_sections=native_word(native_info+0xc0);
            check(native_sections==delivery.runtime_address(sections,0x70),"Hunter native section pointer was not relocated");
            for(const auto base:{0u,0x38u})for(const auto field:{4u,0xcu,0x10u})
                check(native_word(native_sections+base+field)==0,"Delivered native Hunter keeps a donor response");
            ++matched;
        }
        check(matched==1,"Native Hunter model is absent or ambiguous");
    }
#else
    check(argc==2,"Pure Hunter regression cannot claim native Cache admission");
#endif
    std::cout<<"PASS six exact Hunter tuples, whole-buffer preservation, idempotence and "<<controls
        <<" unknown/malformed input controls\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

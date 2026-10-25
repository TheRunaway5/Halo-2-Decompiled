#include "halo2/mcc_destruction_model_scalar.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void put(std::span<std::byte> bytes,std::size_t offset,std::uint32_t value){
    for(unsigned n=0;n<4;++n)bytes[offset+n]=std::byte(value>>(n*8));
}
constexpr std::uint32_t base=0x61000000u;
}
int main(int argc,char **argv)try {
    check(argc==2 || argc==4,"Supply reviewed model scalar manifest [MCC_MAP MCC_MAPS]");
    std::ifstream input(argv[1]);const auto manifest=nlohmann::json::parse(input);
    check(manifest.at("schema")=="H2_MCC_TU15_DESTRUCTION_MODEL_SCALAR_V1","Wrong model scalar authority");
    const auto name=manifest.at("model").get<std::string>();
    std::vector<std::byte> donor(0x800,std::byte{0x57});
    put(donor,0x60,1);put(donor,0x64,base+0x100);
    put(donor,0x100+0xbc,1);put(donor,0x100+0xc0,base+0x240);
    put(donor,0x240+0xc,1);put(donor,0x240+0x10,base+0x300);
    auto expected=donor;
    for(const auto &[offset,value]:manifest.at("donor_response").items())
        put(donor,0x300+std::stoul(offset,nullptr,16),std::stoul(value.get<std::string>(),nullptr,16));
    expected=donor;
    for(const auto &[offset,value]:manifest.at("original_response").items())
        put(expected,0x300+std::stoul(offset,nullptr,16),std::stoul(value.get<std::string>(),nullptr,16));
    check(donor!=expected,"Source fixture does not distinguish the wrong authored response");
    auto bytes=donor;
    const auto read=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
        if(address<base || std::uint64_t(address-base)+size>bytes.size())throw FormatError("Test metadata range escaped");
        return std::span(bytes).subspan(address-base,size);
    };
    const auto convert=[&]{return convert_mcc_destruction_model_scalars(tag_groups::model,name,base,read);};
    auto receipt=convert();
    check(receipt && receipt->status==MccGameplayUnitScalarStatus::Converted && bytes==expected,
        "Known MCC human turret retains a different original destruction-response tuple");
    check(receipt->field_count==1 && receipt->fields[0].address==base+0x304 &&
        receipt->fields[0].before[0]==0x02000000u && receipt->fields[0].after[0]==0,
        "Model scalar conversion lost its precise changed-field receipt");
    receipt=convert();
    check(receipt && receipt->status==MccGameplayUnitScalarStatus::AlreadyEquivalent && bytes==expected,
        "Original model tuple is not idempotent");
    unsigned unknown=0;
    for(const auto &[offset,value]:manifest.at("donor_response").items()){
        bytes=donor;const auto at=0x300+std::stoul(offset,nullptr,16);
        put(bytes,at,std::stoul(value.get<std::string>(),nullptr,16)^1u);const auto before=bytes;
        receipt=convert();check(receipt && receipt->status==MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized && bytes==before,
            "Unknown authored response was partially converted");++unknown;
    }
    for(const auto at:{0x60u,0x1bcu,0x24cu}){
        bytes=donor;put(bytes,at,2);const auto before=bytes;receipt=convert();
        check(receipt && receipt->status==MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized && bytes==before,
            "Unknown authored block shape was changed");
    }
    bytes=donor;const auto before=bytes;
    check(!convert_mcc_destruction_model_scalars(tag_groups::vehicle,name,base,{}) &&
        !convert_mcc_destruction_model_scalars(tag_groups::model,name+"_custom",base,{}) && bytes==before,
        "Model profile changed a different group or custom name");
    for(unsigned malformed=0;malformed<3;++malformed){
        bool rejected=false;
        try {
            if(malformed==0)(void)convert_mcc_destruction_model_scalars(tag_groups::model,name,0xffffffe0u,read);
            else if(malformed==1)(void)convert_mcc_destruction_model_scalars(tag_groups::model,name,base,{});
            else (void)convert_mcc_destruction_model_scalars(tag_groups::model,name,base,
                [&](auto address,auto size){return read(address,size).first(size-1);});
        }catch(const FormatError&){rejected=true;}
        check(rejected && bytes==before,"Malformed model metadata was partially converted");
    }
#ifdef HALO2_DESTRUCTION_MODEL_CACHE_CHECK
    if(argc==4){
        const Cache cache(argv[2],argv[3]);check(cache.is_mcc(),"Model scalar Cache check requires MCC");
        unsigned matched=0;
        for(const auto &tag:cache.tags())if(tag.address && tag.group=="hlmt" && tag.name==name){
            const auto word=[&](auto address){return Bytes(cache.read_tag_bytes(address,4)).u32(0);};
            check(word(tag.address+0x60)==1,"Loaded model has an unknown damage shape");const auto damage=word(tag.address+0x64);
            check(word(damage+0xbc)==1,"Loaded model has an unknown section shape");const auto section=word(damage+0xc0);
            check(word(section+0xc)==1,"Loaded model has an unknown response shape");const auto response=word(section+0x10);
            for(const auto &[offset,value]:manifest.at("original_response").items()){
                const auto actual=word(response+std::stoul(offset,nullptr,16));
                const auto expected=std::stoul(value.get<std::string>(),nullptr,16);
                if(actual!=expected)throw FormatError("Native Cache model response field +"+offset+
                    " differs: expected="+std::to_string(expected)+" actual="+std::to_string(actual));
            }
            ++matched;
        }
        check(matched>0,"MCC map has no reviewed resident human-turret model");
    }
#else
    check(argc==2,"Pure model component cannot claim native Cache admission");
#endif
    std::cout<<"PASS exact original model tuple, whole-buffer preservation, "<<unknown
        <<" authored tuple controls, block shapes, idempotence and malformed input atomicity\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/cache.hpp"
#include "halo2/mcc_native_character_shared.hpp"
#include <algorithm>
#include <array>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <string_view>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_character_shared_profiles.inc"
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
constexpr std::array<std::pair<unsigned,unsigned>,5> template_fields{{
    {0,10},{8,2},{16,10},{24,6},{32,4}}};
void require(bool value,const char* message){if(!value)throw FormatError(message);}
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(i*8));
}
std::vector<std::byte> unhex(const Json& value) {
    const auto text=value.get<std::string>();require(text.size()%2==0,"Invalid character registry extent");
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid character registry hex");
    };
    std::vector<std::byte> result(text.size()/2);
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte((digit(text[i*2])<<4)|digit(text[i*2+1]));
    return result;
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> out{};unsigned size{};
    require(EVP_Digest(bytes.data(),bytes.size(),out.data(),&size,EVP_sha256(),nullptr)==1&&size==out.size(),
        "Cannot hash complete original character template");
    constexpr char hex[]="0123456789abcdef";std::string text;
    for(const auto byte:out){text+=hex[byte>>4];text+=hex[byte&15];}return text;
}
}
std::optional<MccOwnedShaderProjection> project_mcc_character_shared(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead& read_source,const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame,
    const std::function<std::string(std::uint32_t)>& string_id) {
    try {
        if(!read_source||!find||!owns_frame||!string_id)return {};
        static const auto registry=Json::parse(native_character_shared_profiles);
        const Json* profile=nullptr;
        for(const auto& candidate:registry.at("profiles"))
            if(candidate.at("template")==native_stem.name&&candidate.at("native_record").at("payload")==payload_name)
                profile=&candidate;
        if(!profile)return {};
        const auto same=[&](const Tag& tag) {
            const auto* actual=find(tag.datum);
            return actual&&actual->datum==tag.datum&&actual->group==tag.group&&actual->name==tag.name&&
                actual->address==tag.address&&actual->size==tag.size;
        };
        require(shader.group=="shad"&&shader.datum!=0xffffffffu&&shader.address&&shader.size>=84&&same(shader),
            "Character shader lost its complete typed source owner");
        require(native_stem.group=="stem"&&native_stem.datum!=0xffffffffu&&native_stem.address&&same(native_stem)&&
            source_stem.group=="stem"&&source_stem.datum==native_stem.datum&&source_stem.name==native_stem.name&&
            source_stem.address&&source_stem.size>=96&&source_stem.size<=16u*1024u*1024u,
            "Character shader lost its independent source/native template owners");
        require(payload_name.size()==68&&payload_name.substr(64)==".bin"&&
            payload.size()==profile->at("native_record").at("bytes").get<std::size_t>()&&
            native_stem.size==payload.size()&&digest(payload)==payload_name.substr(0,64),
            "Character shader native template differs from its complete original authority");
        const auto source=[&](std::uint32_t at,std::size_t count) {
            require(count<=16u*1024u*1024u&&std::uint64_t(at)+count<=0x100000000ull,
                "Character source allocation overflow");
            const auto raw=read_source(at,count);require(raw.size()==count,"Character source allocation extent mismatch");
            return std::vector<std::byte>(raw.begin(),raw.end());
        };
        auto template_root=source(source_stem.address,96);
        const Bytes template_data(template_root);
        require(template_data.u32(88)==3,"Character source template platform count changed");
        const auto definitions=source(template_data.u32(92),120);
        word(template_root,92,0);
        const auto& graph=profile->at("source_graph");
        require(template_root==unhex(graph.at("root_hex")),"Complete character source template root changed");
        for(unsigned platform=0;platform<3;++platform) {
            std::vector<std::byte> normalized(definitions.begin()+platform*40,definitions.begin()+(platform+1)*40);
            const Bytes definition(normalized);const auto& expected=graph.at("definitions").at(platform);
            for(unsigned i=0;i<template_fields.size();++i) {
                const auto [field,stride]=template_fields[i];const auto& array=expected.at("arrays").at(i);
                const auto count=definition.u32(field),pointer=definition.u32(field+4);
                require(array.at("field")==field&&count==array.at("count").get<unsigned>()&&(count||!pointer),
                    "Character source template bank shape changed");
                auto raw=count?source(pointer,std::size_t(count)*stride):std::vector<std::byte>{};
                if(field==16)for(unsigned pass=0;pass<count;++pass) {
                    const Bytes data(raw);const auto* actual=find(data.u32(pass*10+4));
                    const auto& reference=array.at("typed_references").at(pass);
                    require(data.u32(pass*10)==0x73706173u&&actual&&actual->datum==data.u32(pass*10+4)&&
                        actual->group==reference.at(0).get<std::string>()&&actual->name==reference.at(1).get<std::string>(),
                        "Character template pass escaped its complete typed namespace");
                    word(raw,pass*10+4,0);
                }
                require(raw==unhex(array.at("raw_hex")),"Complete character source compiled remap bank changed");
                word(normalized,field+4,0);
            }
            require(normalized==unhex(expected.at("definition_hex")),"Complete character template definition changed");
        }
        const auto root_data=source(shader.address,84);const Bytes root(root_data);
        require(root.u32(0)==0x7374656du&&root.u32(4)==native_stem.datum,"Character material template ownership changed");
        auto normalized_root=root_data;
        for(const unsigned at:{4u,16u,36u,48u})word(normalized_root,at,0);
        require(normalized_root==unhex(profile->at("source_root_hex")),"Complete character source root changed");
        const auto shader_definitions=source(root.u32(36),372);
        require(std::all_of(shader_definitions.begin()+124,shader_definitions.end(),[](std::byte b){return b==std::byte{};}),
            "Character shader has additional platform authoring");
        const Bytes definition(std::span<const std::byte>(shader_definitions).first(124));
        require(definition.u32(0)==native_stem.datum,"Character shader definition template owner changed");
        std::vector<std::byte> normalized_definition(shader_definitions.begin(),shader_definitions.begin()+124);
        word(normalized_definition,0,0);
        for(const auto& [field,stride]:fields){(void)stride;word(normalized_definition,field+4,0);}
        require(normalized_definition==unhex(profile->at("source_definition_hex")),"Complete character shader bank shape changed");
        std::array<std::vector<std::byte>,14> banks;
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];const auto count=definition.u32(field),pointer=definition.u32(field+4);
            require(count||!pointer,"Empty character shader bank has a pointer");
            if(count)banks[i]=source(pointer,std::size_t(count)*stride);
        }
        const Bytes images(banks[0]);
        require(banks[0].size()==profile->at("images").size()*12,"Character image role count changed");
        auto normalized_images=banks[0];
        for(unsigned i=0;i<profile->at("images").size();++i) {
            const auto& expected=profile->at("images").at(i);const auto handle=images.u32(i*12);
            const auto* image=find(handle);
            require(image&&image->datum==handle&&image->group=="bitm"&&image->address&&image->size>=76&&
                image->group==expected.at(0).at(0).get<std::string>()&&image->name==expected.at(0).at(1).get<std::string>()&&
                images.u32(i*12+4)==expected.at(1).get<unsigned>()&&images.u32(i*12+8)==expected.at(2).get<unsigned>()&&
                owns_frame(handle,images.u32(i*12+4)),"Character image escaped its actual typed frame/setup");
            word(normalized_images,i*12,0);
        }
        require(normalized_images==unhex(profile->at("source_banks").at("4")),"Complete character image setup changed");
        std::vector<std::vector<std::byte>> curves;
        auto normalized_overlays=banks[7];
        const Bytes overlays(banks[7]);
        require(banks[7].size()==profile->at("curves").size()*20,"Character animation overlay count changed");
        for(unsigned i=0;i<profile->at("curves").size();++i) {
            const auto input=overlays.u32(i*20);
            require((input?string_id(input):std::string{})==profile->at("input_names").at(i).get<std::string>(),
                "Character animation input escaped its actual StringID namespace");
            const auto expected=unhex(profile->at("curves").at(i));
            require(overlays.u32(i*20+12)==expected.size()&&!expected.empty(),"Character function extent changed");
            auto curve=source(overlays.u32(i*20+16),expected.size());
            require(curve==expected,"Character authored function differs from its complete original authority");
            curves.push_back(std::move(curve));word(normalized_overlays,i*20+16,0);
        }
        require(normalized_overlays==unhex(profile->at("source_banks").at("60")),
            "Character authored animation input, range, period or roles changed");
        for(unsigned i=1;i<fields.size();++i)if(i!=7)
            require(banks[i]==unhex(profile->at("source_banks").at(std::to_string(fields[i].first))),
                "Character material escaped its complete literal shared parameter setup");
        const auto runtime=source(root.u32(16),80);auto normalized_runtime=runtime;
        for(const auto& [at,index]:profile->at("runtime_reference_slots").items())if(index.get<int>()>=0) {
            const unsigned offset=unsigned(std::stoul(at)),slot=index.get<unsigned>();
            require(Bytes(runtime).u32(offset)==images.u32(slot*12),"Character runtime bitmap role changed");
            word(normalized_runtime,offset,0);
        }
        require(normalized_runtime==unhex(profile->at("runtime_hex")),"Complete character runtime setup changed");
        require(root.u32(44)<=64&&(root.u32(44)||!root.u32(48)),"Invalid character predicted resource extent");
        const auto predictions=root.u32(44)?source(root.u32(48),std::size_t(root.u32(44))*8):std::vector<std::byte>{};
        const Bytes prediction_data(predictions);
        require(root.u32(44)==profile->at("prediction_roles").size(),"Character predicted resource count changed");
        for(unsigned i=0;i<root.u32(44);++i) {
            const auto* image=find(prediction_data.u32(i*8+4));
            const auto& expected=profile->at("prediction_roles").at(i);
            require(prediction_data.u16(i*8)==0&&image&&image->datum==prediction_data.u32(i*8+4)&&
                image->group=="bitm"&&image->address&&image->size>=76&&
                image->group==expected.at(0).at(0).get<std::string>()&&image->name==expected.at(0).at(1).get<std::string>()&&
                prediction_data.u16(i*8+2)==expected.at(1).get<unsigned>()&&owns_frame(image->datum,prediction_data.u16(i*8+2)),
                "Character prediction escaped its actual typed bitmap/frame");
        }
        // This finite data conversion preserves the admitted authored images,
        // input IDs and full function bytes. Its other output banks are the
        // complete paired original setup; no shader instructions are decoded.
        for(unsigned i=1;i<fields.size();++i)if(i!=7)
            banks[i]=unhex(profile->at("native_banks").at(std::to_string(fields[i].first)));
        MccOwnedShaderProjection result{root_data,{}};
        const auto append=[&](unsigned field,std::span<const std::byte> raw) {
            if(raw.empty()){word(result.bytes,field,0);return;}
            const auto offset=(result.bytes.size()+15)&~std::size_t(15);
            require(offset+raw.size()<=16u*1024u*1024u,"Excessive character owned projection");
            result.bytes.resize(offset);result.bytes.insert(result.bytes.end(),raw.begin(),raw.end());
            word(result.bytes,field,std::uint32_t(offset));result.pointers.emplace_back(field,std::uint32_t(offset));
        };
        append(16,runtime);word(result.bytes,28,0);word(result.bytes,32,1);
        append(36,std::span<const std::byte>(shader_definitions).first(124));
        const auto output_definition=Bytes(result.bytes).u32(36);
        append(48,predictions);word(result.bytes,80,0);
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];require(banks[i].size()%stride==0,"Incomplete character output bank");
            word(result.bytes,output_definition+field,std::uint32_t(banks[i].size()/stride));
            append(output_definition+field+4,banks[i]);
        }
        const auto output_overlays=Bytes(result.bytes).u32(output_definition+64);
        for(unsigned i=0;i<curves.size();++i)append(output_overlays+i*20+16,curves[i]);
        return result;
    }catch(const FormatError&){return {};}
}
}

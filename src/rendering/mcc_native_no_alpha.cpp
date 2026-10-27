#include "halo2/cache.hpp"
#include "halo2/mcc_native_no_alpha.hpp"
#include <algorithm>
#include <array>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_no_alpha_profiles.inc"
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},{60,20},
    {68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
constexpr std::array<std::pair<unsigned,unsigned>,5> template_fields{{
    {0,10},{8,2},{16,10},{24,6},{32,4}}};
void require(bool value,const char* message){if(!value)throw FormatError(message);}
void word(std::vector<std::byte>& raw,std::size_t at,std::uint32_t value) {
    Bytes(raw).require(at,4);
    for(unsigned i=0;i<4;++i)raw[at+i]=std::byte(value>>(i*8));
}
std::vector<std::byte> unhex(const Json& value) {
    const auto text=value.get<std::string>();require(text.size()%2==0,"Invalid NoAlpha source hex");
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid NoAlpha source digit");
    };
    std::vector<std::byte> result(text.size()/2);
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte((digit(text[i*2])<<4)|digit(text[i*2+1]));
    return result;
}
std::string digest(std::span<const std::byte> raw) {
    std::array<unsigned char,32> output{};unsigned size{};
    require(EVP_Digest(raw.data(),raw.size(),output.data(),&size,EVP_sha256(),nullptr)==1&&size==output.size(),
        "Cannot hash complete NoAlpha source setup");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(const auto value:output){result+=hex[value>>4];result+=hex[value&15];}return result;
}
const Json& registry(){static const auto value=Json::parse(native_no_alpha_profiles);return value;}
bool same(const Tag& tag,const MccShaderFind& find) {
    const auto* actual=find(tag.datum);
    return actual&&actual->datum==tag.datum&&actual->group==tag.group&&actual->name==tag.name&&
        actual->address==tag.address&&actual->size==tag.size;
}
std::vector<std::byte> source(const MccShaderRead& read,std::uint32_t at,std::size_t size) {
    require(size<=16u*1024u*1024u&&std::uint64_t(at)+size<=0x100000000ull,"NoAlpha source allocation overflow");
    const auto raw=read(at,size);require(raw.size()==size,"NoAlpha source allocation extent mismatch");
    return {raw.begin(),raw.end()};
}
void qualify(const Tag& stem,const Tag& pass,const MccShaderRead& read,const MccShaderFind& find) {
    const auto& setup=registry().at("setup");
    require(read&&find&&stem.group=="stem"&&pass.group=="spas"&&same(stem,find)&&same(pass,find)&&
        stem.name==setup.at("source_stem").at("name").get<std::string>()&&
        pass.name==setup.at("source_pass").at("name").get<std::string>()&&
        stem.datum==setup.at("source_stem").at("datum").get<std::uint32_t>()&&
        pass.datum==setup.at("source_pass").at("datum").get<std::uint32_t>()&&stem.address&&pass.address&&
        stem.size==setup.at("source_stem").at("size").get<unsigned>()&&
        pass.size==setup.at("source_pass").at("size").get<unsigned>(),"NoAlpha complete typed setup owner changed");
    require(digest(source(read,stem.address,stem.size))==setup.at("source_stem_sha256").get<std::string>()&&
        digest(source(read,pass.address,pass.size))==setup.at("source_pass_sha256").get<std::string>(),
        "Complete NoAlpha pass/template differs from authenticated authoring");
    // The complete seven-class bytecode owners are the same shared owners as
    // the ordinary PC pass; no inference from a vrtx/pass name is sufficient.
    for(const auto& vertex:setup.at("vertex_owners")) {
        const auto handle=vertex.at("datum").get<std::uint32_t>();const auto* actual=find(handle);
        require(actual&&actual->datum==handle&&actual->group=="vrtx"&&actual->address&&
            actual->name==vertex.at("name").get<std::string>()&&actual->size==vertex.at("bytes").get<unsigned>()&&
            digest(source(read,actual->address,actual->size))==vertex.at("sha256").get<std::string>(),
            "NoAlpha complete seven-class vertex owner changed");
    }
    for(const auto& row:setup.at("external_pass_allocations"))
        require(source(read,row.at("address"),row.at("bytes"))==unhex(row.at("hex")),
            "NoAlpha external default/compiled shader allocation changed");
    const auto raw=source(read,stem.address,96);const Bytes root(raw);
    const auto definitions=source(read,root.u32(92),120);const auto& graph=setup.at("source_graph");
    auto normalized=raw;word(normalized,92,0);
    require(normalized==unhex(graph.at("root_hex")),"NoAlpha source template root changed");
    for(unsigned platform=0;platform<3;++platform) {
        std::vector<std::byte> definition(definitions.begin()+platform*40,definitions.begin()+(platform+1)*40);
        const Bytes values(definition);const auto& expected=graph.at("definitions").at(platform);
        for(unsigned i=0;i<template_fields.size();++i) {
            const auto [field,stride]=template_fields[i];const auto& array=expected.at("arrays").at(i);
            const auto count=values.u32(field),pointer=values.u32(field+4);
            require(count==array.at("count").get<unsigned>()&&(count||!pointer),"NoAlpha template bank extent changed");
            auto bank=count?source(read,pointer,std::size_t(count)*stride):std::vector<std::byte>{};
            if(field==16)for(unsigned j=0;j<count;++j) {
                const Bytes bytes(bank);const auto handle=bytes.u32(j*10+4);const auto* actual=find(handle);
                const auto& reference=array.at("typed_references").at(j);
                require(bytes.u32(j*10)==0x73706173u&&actual&&actual->datum==handle&&
                    actual->group==reference.at(0).get<std::string>()&&actual->name==reference.at(1).get<std::string>(),
                    "NoAlpha pass escaped its complete typed remap namespace");
                word(bank,j*10+4,0);
            }
            require(bank==unhex(array.at("raw_hex")),"NoAlpha complete LOD/remap/setup bank changed");
            word(definition,field+4,0);
        }
        require(definition==unhex(expected.at("definition_hex")),"NoAlpha complete template definition changed");
    }
}
void native_payload(const Tag& tag,std::span<const std::byte> payload,const Json& record,const MccShaderFind& find) {
    require(tag.group==record.at("group").get<std::string>()&&tag.name==record.at("name").get<std::string>()&&
        tag.address&&same(tag,find)&&tag.size==payload.size()&&payload.size()==record.at("bytes").get<unsigned>()&&
        digest(payload)+".bin"==record.at("payload").get<std::string>(),"NoAlpha original TU program authority changed");
}
}

std::optional<MccNoAlphaNativeOwners> project_mcc_no_alpha_native_owners(
    const Tag& stem,const Tag& pass,const Tag& normal_stem,const Tag& normal_pass,
    std::span<const std::byte> stem_payload,std::span<const std::byte> pass_payload,
    const MccShaderRead& read,const MccShaderFind& find,const MccShaderNamedFind& named) {
    try {
        if(!read||!find||!named)return {};
        qualify(stem,pass,read,find);const auto& setup=registry().at("setup");
        const auto& stem_record=setup.at("native_stem_record");const auto& pass_record=setup.at("native_pass_record");
        native_payload(normal_stem,stem_payload,stem_record,find);native_payload(normal_pass,pass_payload,pass_record,find);
        const auto clone=[&](std::span<const std::byte> payload,const Json& record,bool template_owner) {
            MccOwnedShaderProjection output{{payload.begin(),payload.end()},{}};
            require(record.at("strings").empty(),"Unreviewed NoAlpha native StringID relocation");
            for(const auto& pointer:record.at("pointers")) {
                const auto field=pointer.at(0).get<unsigned>(),target=pointer.at(1).get<unsigned>();
                require(target<output.bytes.size()&&Bytes(output.bytes).u32(field)==target,"NoAlpha native pointer provenance changed");
                output.pointers.emplace_back(field,target);
            }
            unsigned redirected=0;
            for(const auto& reference:record.at("references")) {
                const auto field=reference.at(0).get<unsigned>();const auto group=reference.at(1).at(0).get<std::string>();
                const auto name=reference.at(1).at(1).get<std::string>();const Tag* target=nullptr;
                if(template_owner&&group=="spas"&&name==normal_pass.name){target=&pass;++redirected;}
                else target=named(group,name);
                require(target&&target->group==group&&target->name==(target==&pass?pass.name:name)&&target->address&&
                    target->size&&target->datum!=0xffffffffu&&same(*target,find),"NoAlpha native program dependency changed");
                word(output.bytes,field,target->datum);
            }
            if(template_owner)require(redirected==2,"NoAlpha LOD pass roles changed");
            return output;
        };
        MccNoAlphaNativeOwners result{clone(stem_payload,stem_record,true),clone(pass_payload,pass_record,false)};
        // Exact source2B035..2B096 supplies RGB00010101 to NV0970358.
        // The MCC NoAlpha authoring changes only impl0's write-mask bit A.
        const Bytes raw(result.pass.bytes);const auto pp=raw.u32(32),implementations=raw.u32(pp+4);
        require(raw.u32(pp)==4,"NoAlpha original implementation census changed");
        const auto packed=raw.u16(implementations+2);
        const unsigned first=packed&511u,count=packed>>9;
        const auto states=raw.u32(pp+20);unsigned changed=0;
        for(unsigned i=first;i<first+count;++i)if(std::to_integer<unsigned>(raw.slice(states+i*5,1)[0])==67) {
            require(raw.u32(states+i*5+1)==0x01010101u,"NoAlpha original RGBA state changed");
            word(result.pass.bytes,states+i*5+1,0x00010101u);++changed;
        }
        require(changed==1,"NoAlpha original color-state owner changed");
        return result;
    }catch(const FormatError&){return {};}
}

std::optional<MccOwnedShaderProjection> project_mcc_no_alpha_material(
    const Tag& shader,const Tag& stem,const Tag& pass,const Tag& normal_stem,
    std::span<const std::byte> payload,const MccShaderRead& read,const MccShaderFind& find,
    const MccShaderBitmapFrame& frames,const MccShaderString& string_id) {
    try {
        if(!read||!find||!frames||!string_id||shader.group!="shad"||!shader.address||shader.size<84||!same(shader,find))return {};
        const auto root_data=source(read,shader.address,84);const Bytes root(root_data);
        if(root.u32(4)!=stem.datum)return {};
        qualify(stem,pass,read,find);const auto& setup=registry().at("setup");
        native_payload(normal_stem,payload,setup.at("native_stem_record"),find);
        auto normalized=root_data;for(const auto at:{4u,16u,36u,48u})word(normalized,at,0);
        const Json* profile=nullptr;
        for(const auto& candidate:registry().at("profiles"))if(normalized==unhex(candidate.at("root_hex")))profile=&candidate;
        if(!profile)return {};
        require(string_id(root.u32(8))==profile->at("source_string").get<std::string>(),"NoAlpha material StringID namespace changed");
        const auto definitions=source(read,root.u32(36),372);
        require(std::all_of(definitions.begin()+124,definitions.end(),[](std::byte b){return b==std::byte{};}),"NoAlpha additional platform authoring changed");
        std::vector<std::byte> definition(definitions.begin(),definitions.begin()+124);const Bytes values(definition);
        require(values.u32(0)==stem.datum,"NoAlpha NIEx template owner changed");
        std::array<std::vector<std::byte>,14> banks;
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];const auto count=values.u32(field),pointer=values.u32(field+4);
            require(count||!pointer,"NoAlpha empty NIEx bank carries an allocation");
            if(count)banks[i]=source(read,pointer,std::size_t(count)*stride);
            word(definition,field+4,0);
        }
        word(definition,0,0);require(definition==unhex(profile->at("definition_hex")),"NoAlpha complete NIEx shape changed");
        for(unsigned i=1;i<fields.size();++i)
            require(banks[i]==unhex(profile->at("source_banks").at(std::to_string(fields[i].first))),"NoAlpha complete authored parameter/curve bank changed");
        auto images=banks[0];const Bytes image_words(banks[0]);
        require(images.size()==profile->at("images").size()*12,"NoAlpha image role count changed");
        for(unsigned i=0;i<profile->at("images").size();++i) {
            const auto& expected=profile->at("images").at(i);const auto handle=image_words.u32(i*12);
            if(handle==0xffffffffu){require(expected.at(0).at(1)=="","NoAlpha missing image role changed");continue;}
            const auto* actual=find(handle);
            require(actual&&actual->datum==handle&&actual->group==expected.at(0).at(0).get<std::string>()&&actual->group=="bitm"&&
                actual->name==expected.at(0).at(1).get<std::string>()&&actual->address&&actual->size>=76&&
                image_words.u32(i*12+4)==expected.at(1).get<unsigned>()&&image_words.u32(i*12+8)==expected.at(2).get<unsigned>()&&
                frames(handle,image_words.u32(i*12+4)),"NoAlpha image role/frame/log owner changed");word(images,i*12,0);
        }
        require(images==unhex(profile->at("images_hex")),"NoAlpha complete image bank changed");
        auto runtime=source(read,root.u32(16),80);const Bytes runtime_words(runtime);
        for(const auto& [field,slot]:profile->at("runtime_slots").items())if(slot.get<int>()>=0) {
            const auto at=unsigned(std::stoul(field));require(runtime_words.u32(at)==image_words.u32(slot.get<unsigned>()*12),"NoAlpha runtime image identity changed");word(runtime,at,0);
        }
        require(runtime==unhex(profile->at("runtime_hex")),"NoAlpha complete runtime setup changed");
        auto predictions=source(read,root.u32(48),root.u32(44)*8);const Bytes predicted(predictions);
        require(predictions.size()==profile->at("prediction_roles").size()*8,"NoAlpha prediction census changed");
        for(unsigned i=0;i<profile->at("prediction_roles").size();++i) {
            const auto& expected=profile->at("prediction_roles").at(i);const auto handle=predicted.u32(i*8+4);const auto* actual=find(handle);
            require(actual&&actual->datum==handle&&actual->group==expected.at(0).at(0).get<std::string>()&&
                actual->name==expected.at(0).at(1).get<std::string>()&&predicted.u32(i*8)==expected.at(1).get<unsigned>()&&
                frames(handle,predicted.u16(i*8+2)),"NoAlpha prediction frame/role changed");word(predictions,i*8+4,0);
        }
        require(predictions==unhex(profile->at("predictions_hex")),"NoAlpha complete prediction setup changed");
        // A private, fully qualified view selects the existing proven data
        // converter. It does not publish a name alias: output retains stem's
        // actual full salt, SID, parameters and NoAlpha native owners above.
        Tag qualified=normal_stem;qualified.datum=stem.datum;
        const MccShaderFind qualified_find=[&](std::uint32_t handle)->const Tag* {
            return handle==stem.datum?&qualified:find(handle);
        };
        return project_mcc_unanimated_tex_bump(shader,qualified,
            setup.at("native_stem_record").at("payload").get<std::string>(),payload,read,qualified_find,frames);
    }catch(const FormatError&){return {};}
}
}

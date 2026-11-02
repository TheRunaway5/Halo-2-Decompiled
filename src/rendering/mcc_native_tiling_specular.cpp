#include "halo2/cache.hpp"
#include "halo2/mcc_native_tiling_specular.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_tiling_specular_profiles.inc"
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
constexpr std::array<std::pair<unsigned,unsigned>,5> template_fields{{
    {0,10},{8,2},{16,10},{24,6},{32,4}}};
void need(bool ok,const char* why){if(!ok)throw FormatError(why);}
void put(std::vector<std::byte>& data,unsigned at,unsigned value,unsigned width=4) {
    Bytes(data).require(at,width);for(unsigned n=0;n<width;++n)data[at+n]=std::byte(value>>(n*8));
}
std::vector<std::byte> unhex(const std::string& text) {
    need(text.size()%2==0,"Invalid static tiling-specular profile extent");
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid static tiling-specular profile byte");
    };
    std::vector<std::byte> result(text.size()/2);
    for(unsigned n=0;n<result.size();++n)result[n]=std::byte((digit(text[n*2])<<4)|digit(text[n*2+1]));
    return result;
}
std::string digest(std::span<const std::byte> data) {
    std::array<unsigned char,32> hash{};unsigned count{};
    need(EVP_Digest(data.data(),data.size(),hash.data(),&count,EVP_sha256(),nullptr)==1&&count==hash.size(),
        "Cannot authenticate original tiling-specular template");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(auto v:hash){result+=hex[v>>4];result+=hex[v&15];}return result;
}
std::vector<std::byte> pass_ranges(std::span<const std::byte> data) {
    need(data.size()%10==0,"Incomplete tiling-specular pass bank");
    std::vector<std::byte> result;
    for(unsigned n=0;n<data.size()/10;++n){result.push_back(data[n*10+8]);result.push_back(data[n*10+9]);}
    return result;
}
unsigned channel(unsigned bits) {
    // TU1.5 1349E0/134AD0: exact float*255, FISTP under FPCW027F.
    // Integer significand arithmetic preserves nearest-even independently of
    // host rounding, with the original signed-int32 indefinite result.
    const auto exponent=(bits>>23)&255u;need(exponent!=255,"Nonfinite tiling-specular color");
    const auto sign=bits>>31;const std::uint64_t limit=sign?0x80000000ull:0x7fffffffull;
    const std::uint64_t scaled=((bits&0x7fffffu)|(exponent?0x800000u:0u))*std::uint64_t(255);
    const auto shift=exponent?int(exponent)-150:-149;std::uint64_t rounded{};
    if(shift>=0) {
        if(shift>=32||scaled>(limit>>unsigned(shift)))return 0x80000000u;
        rounded=scaled<<unsigned(shift);
    }else {
        const auto right=unsigned(-shift);
        if(right<64) {
            rounded=scaled>>right;
            const auto remainder=scaled&((std::uint64_t(1)<<right)-1),half=std::uint64_t(1)<<(right-1);
            if(remainder>half||(remainder==half&&(rounded&1)))++rounded;
        }
    }
    if(rounded>limit)return 0x80000000u;
    return sign?0u-unsigned(rounded):unsigned(rounded);
}
}
std::optional<MccOwnedShaderProjection> project_mcc_tiling_specular(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead& read_source,const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame,
    const MccShaderAliasOwner& alias_owner) {
    try {
        static const auto profile=Json::parse(native_tiling_specular_profile);
        if(native_stem.name!=profile.at("template").get<std::string>()||!read_source||!find||!owns_frame)return {};
        const Json* record=nullptr;
        for(const auto& candidate:profile.at("native_records"))if(candidate.at("payload")==payload_name){record=&candidate;break;}
        if(!record)return {};
        const auto same=[&](const Tag& value){const auto* actual=find(value.datum);
            return actual&&actual->datum==value.datum&&actual->group==value.group&&actual->name==value.name&&
                actual->address==value.address&&actual->size==value.size;};
        need(shader.group=="shad"&&shader.datum!=0xffffffffu&&shader.address&&same(shader)&&
            (shader.size>=84||(!shader.size&&alias_owner&&alias_owner(shader,84))),
            "Tiling-specular shader lost its complete source owner");
        need(native_stem.group=="stem"&&native_stem.datum!=0xffffffffu&&native_stem.address&&same(native_stem)&&
            source_stem.group=="stem"&&source_stem.datum==native_stem.datum&&source_stem.name==native_stem.name&&
            source_stem.address&&source_stem.size>=96&&source_stem.size<=16u*1024u*1024u,
            "Tiling-specular template lost its source/native ownership");
        need(payload_name.size()==68&&payload_name.substr(64)==".bin"&&payload.size()==record->at("bytes").get<unsigned>()&&
            native_stem.size==payload.size()&&digest(payload)==payload_name.substr(0,64),
            "Tiling-specular native template differs from its complete original payload");
        const auto source=[&](unsigned address,std::size_t size) {
            need(size<=16u*1024u*1024u&&std::uint64_t(address)+size<=0x100000000ull,
                "Tiling-specular source allocation overflow");
            const auto bytes=read_source(address,size);need(bytes.size()==size,"Tiling-specular source extent mismatch");
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };
        const auto& graph=profile.at("source_graph");
        auto template_root=source(source_stem.address,96);const Bytes template_data(template_root);
        need(template_data.u32(88)==3,"Tiling-specular template platform count changed");
        const auto definitions=source(template_data.u32(92),120);put(template_root,92,0);
        need(template_root==unhex(graph.at("root_hex")),"Complete tiling-specular source template changed");
        std::array<std::vector<std::byte>,5> source_template;
        for(unsigned platform=0;platform<3;++platform) {
            const auto& expected=graph.at("definitions").at(platform);
            std::vector<std::byte> raw(definitions.begin()+platform*40,definitions.begin()+(platform+1)*40);
            const Bytes definition(raw);
            for(unsigned n=0;n<template_fields.size();++n) {
                const auto [field,stride]=template_fields[n];const auto count=definition.u32(field),pointer=definition.u32(field+4);
                const auto& array=expected.at("arrays").at(n);
                need(count==array.at("count").get<unsigned>()&&(count||!pointer),"Complete tiling-specular compiler shape changed");
                auto bank=count?source(pointer,std::size_t(count)*stride):std::vector<std::byte>{};
                if(!platform)source_template[n]=bank;
                if(field==16)for(unsigned i=0;i<count;++i) {
                    const Bytes b(bank);const auto* pass=find(b.u32(i*10+4));const auto& name=array.at("typed_references").at(i);
                    need(b.u32(i*10)==0x73706173u&&pass&&pass->datum==b.u32(i*10+4)&&
                        pass->group==name.at(0).get<std::string>()&&pass->name==name.at(1).get<std::string>(),
                        "Tiling-specular compiler pass escaped its exact typed namespace");
                    put(bank,i*10+4,0);
                }
                need(bank==unhex(array.at("raw_hex")),"Complete tiling-specular compiler remap graph changed");put(raw,field+4,0);
            }
            need(raw==unhex(expected.at("definition_hex")),"Complete tiling-specular compiler definition changed");
        }
        const auto root_data=source(shader.address,84);const Bytes root(root_data);
        need(root.u32(0)==0x7374656du&&root.u32(4)==native_stem.datum&&root.u32(12)==1&&root.u32(20)==1&&
            !root.u32(24)&&!root.u32(28)&&root.u32(32)==3&&root.u32(56)==0xffffffffu&&!root.u32(76)&&!root.u32(80)&&
            root.u16(60)<=6&&root.u16(62)<=3&&root.u16(64)<=3&&std::isfinite(root.f32(68))&&std::isfinite(root.f32(72)),
            "Tiling-specular shader escaped its complete source contract");
        const auto shader_definitions=source(root.u32(36),372);
        need(std::all_of(shader_definitions.begin()+124,shader_definitions.end(),[](auto b){return b==std::byte{};}),
            "Tiling-specular shader acquired additional platform authoring");
        const Bytes definition(std::span<const std::byte>(shader_definitions).first(124));
        need(definition.u32(0)==native_stem.datum&&!definition.u32(116)&&!definition.u32(120),
            "Tiling-specular postprocess ownership changed");
        std::array<std::vector<std::byte>,14> data;
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];const auto count=definition.u32(field),pointer=definition.u32(field+4);
            need(count==profile.at("roles").at("source_counts").at(std::to_string(field)).get<unsigned>()&&(count||!pointer),
                "Tiling-specular complete bank count or empty ownership changed");
            if(count)data[i]=source(pointer,std::size_t(count)*stride);
        }
        need(data[4]==source_template[1]&&data[5]==pass_ranges(source_template[2]),
            "Tiling-specular shader scheduling differs from its complete compiler graph");
        const auto native_lods=unhex(profile.at("native_arrays").at(0).at("raw_hex"));
        for(unsigned i=0;i<3;++i)need(Bytes(data[3]).u16(i*6+4)==Bytes(source_template[0]).u16(i*10)&&
            Bytes(data[3]).u16(i*6+4)==Bytes(native_lods).u16(i*10),"Tiling-specular authored LOD range changed");
        need(std::all_of(data[6].begin(),data[6].end(),[](auto b){return b==std::byte{};}),
            "Tiling-specular animation requires a complete native owner");
        need(Bytes(data[1]).u32(12)==0,"Active MCC midrange tint requires an additional native owner");
        const auto bitmap=[&](unsigned handle,unsigned frame) {
            const auto* tag=find(handle);return handle!=0xffffffffu&&tag&&tag->datum==handle&&tag->group=="bitm"&&
                tag->address&&tag->size>=76&&!tag->name.empty()&&owns_frame(handle,frame);
        };
        const Bytes images(data[0]);
        for(unsigned i=0;i<7;++i)need(bitmap(images.u32(i*12),images.u32(i*12+4))&&std::isfinite(images.f32(i*12+8)),
            "Tiling-specular bitmap lost its complete typed frame owner");
        for(unsigned index:{2u,12u,13u})for(unsigned at=0;at<data[index].size();at+=4)
            need(std::isfinite(Bytes(data[index]).f32(at)),"Tiling-specular has a nonfinite authored value");
        const Bytes properties(data[11]);
        for(unsigned i=0;i<5;++i)need((properties.u16(i*4)==0xffffu||properties.u16(i*4)<7)&&properties.u16(i*4+2)==0xffffu,
            "Tiling-specular bitmap animation requires an unproved owner");
        const Bytes vectors(data[2]);std::vector<std::byte> pixels(7*4),native_vectors;
        const auto& pixel_roles=profile.at("roles").at("pixel_sources");
        for(unsigned i=0;i<7;++i) {
            const auto& role=pixel_roles.at(i);const auto index=role.at("source").get<unsigned>();unsigned value{};
            if(role.at("kind")=="pixel")value=Bytes(data[1]).u32(index*4);
            else if(role.at("component")=="a")value=channel(vectors.u32(index*16+12))<<24;
            else value=((channel(vectors.u32(index*16))&255)<<16)|
                ((channel(vectors.u32(index*16+4))&255)<<8)|(channel(vectors.u32(index*16+8))&255);
            put(pixels,i*4,value);
        }
        for(const auto& index:profile.at("roles").at("vector_sources")) {
            const auto vector=std::span(data[2]).subspan(index.get<unsigned>()*16,16);
            native_vectors.insert(native_vectors.end(),vector.begin(),vector.end());
        }
        data[1]=std::move(pixels);data[2]=std::move(native_vectors);
        data[4]=unhex(profile.at("native_arrays").at(1).at("raw_hex"));
        data[5]=pass_ranges(unhex(profile.at("native_arrays").at(2).at("raw_hex")));
        data[6]=std::vector<std::byte>(profile.at("native_arrays").at(3).at("count").get<unsigned>()*10);
        const auto runtime=source(root.u32(16),80);const Bytes runtime_data(runtime);
        for(unsigned at:{0u,8u,44u,52u})need(runtime_data.u32(at+4)==0xffffffffu||
            (runtime_data.u32(at)==0x6269746du&&bitmap(runtime_data.u32(at+4),0)),"Tiling-specular runtime bitmap owner changed");
        const auto predicted_count=root.u32(44);need(predicted_count<=64&&(predicted_count||!root.u32(48)),"Invalid tiling-specular prediction extent");
        const auto predictions=predicted_count?source(root.u32(48),predicted_count*8):std::vector<std::byte>{};const Bytes prediction(predictions);
        for(unsigned i=0;i<predicted_count;++i)need(!prediction.u16(i*8)&&bitmap(prediction.u32(i*8+4),prediction.u16(i*8+2)),
            "Tiling-specular prediction lost its complete source bitmap/frame");
        MccOwnedShaderProjection output{root_data,{}};
        const auto append=[&](unsigned field,std::span<const std::byte> bytes) {
            if(bytes.empty()){put(output.bytes,field,0);return;}
            const auto at=(output.bytes.size()+15)&~std::size_t(15);
            need(at+bytes.size()<=16u*1024u*1024u,"Excessive owned tiling-specular projection");
            output.bytes.resize(at);output.bytes.insert(output.bytes.end(),bytes.begin(),bytes.end());
            put(output.bytes,field,unsigned(at));output.pointers.emplace_back(field,unsigned(at));
        };
        append(16,runtime);put(output.bytes,32,1);append(36,std::span<const std::byte>(shader_definitions).first(124));
        const auto pp=Bytes(output.bytes).u32(36);append(48,predictions);
        for(unsigned i=0;i<fields.size();++i){const auto [field,stride]=fields[i];
            put(output.bytes,pp+field,unsigned(data[i].size()/stride));append(pp+field+4,data[i]);}
        return output;
    }catch(const FormatError&){return {};}
}
}

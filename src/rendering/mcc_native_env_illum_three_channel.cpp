#include "halo2/cache.hpp"
#include "halo2/mcc_native_env_illum_three_channel.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_env_illum_three_channel_profiles.inc"
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
    need(text.size()%2==0,"Invalid static environment/illumination profile extent");
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid static environment/illumination profile byte");
    };
    std::vector<std::byte> result(text.size()/2);
    for(unsigned n=0;n<result.size();++n)result[n]=std::byte((digit(text[n*2])<<4)|digit(text[n*2+1]));
    return result;
}
std::string digest(std::span<const std::byte> data) {
    std::array<unsigned char,32> hash{};unsigned count{};
    need(EVP_Digest(data.data(),data.size(),hash.data(),&count,EVP_sha256(),nullptr)==1&&count==hash.size(),
        "Cannot authenticate original environment/illumination template");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(auto v:hash){result+=hex[v>>4];result+=hex[v&15];}return result;
}
std::pair<unsigned,unsigned> range(unsigned packed,unsigned total) {
    const auto first=packed&511u,count=packed>>9;
    need(first<=total&&count<=total-first,"Environment/illumination range escaped its complete bank");
    return {first,count};
}
std::vector<std::byte> pass_ranges(std::span<const std::byte> data) {
    need(data.size()%10==0,"Incomplete environment/illumination pass bank");
    std::vector<std::byte> result;
    for(unsigned n=0;n<data.size()/10;++n){result.push_back(data[n*10+8]);result.push_back(data[n*10+9]);}
    return result;
}
}
std::optional<MccOwnedShaderProjection> project_mcc_env_illum_three_channel(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead& read_source,const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame,
    const MccShaderAliasOwner& alias_owner) {
    try {
        // This immutable registry contains complete compiler metadata and
        // exact original payload identities. It is not a shader program.
        static const auto profile=Json::parse(native_env_illum_three_channel_profile);
        if(native_stem.name!=profile.at("template").get<std::string>()||!read_source||!find||!owns_frame)return {};
        const Json* record=nullptr;
        for(const auto& candidate:profile.at("native_records"))if(candidate.at("payload")==payload_name){record=&candidate;break;}
        if(!record)return {};
        const auto same=[&](const Tag& value){const auto* actual=find(value.datum);
            return actual&&actual->datum==value.datum&&actual->group==value.group&&actual->name==value.name&&
                actual->address==value.address&&actual->size==value.size;};
        need(shader.group=="shad"&&shader.datum!=0xffffffffu&&shader.address&&same(shader)&&
            (shader.size>=84||(!shader.size&&alias_owner&&alias_owner(shader,84))),
            "Environment/illumination shader lost its complete source owner");
        need(native_stem.group=="stem"&&native_stem.datum!=0xffffffffu&&native_stem.address&&same(native_stem)&&
            source_stem.group=="stem"&&source_stem.datum==native_stem.datum&&source_stem.name==native_stem.name&&
            source_stem.address&&source_stem.size>=96&&source_stem.size<=16u*1024u*1024u,
            "Environment/illumination template lost its source/native ownership");
        need(payload_name.size()==68&&payload_name.substr(64)==".bin"&&payload.size()==record->at("bytes").get<unsigned>()&&
            native_stem.size==payload.size()&&digest(payload)==payload_name.substr(0,64),
            "Environment/illumination native template differs from its complete original payload");
        const auto source=[&](unsigned address,std::size_t size) {
            need(size<=16u*1024u*1024u&&std::uint64_t(address)+size<=0x100000000ull,
                "Environment/illumination source allocation overflow");
            const auto bytes=read_source(address,size);need(bytes.size()==size,"Environment/illumination source extent mismatch");
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };
        const auto& graph=profile.at("source_graph");
        auto template_root=source(source_stem.address,96);const Bytes template_data(template_root);
        need(template_data.u32(88)==3,"Environment/illumination template platform count changed");
        const auto definitions=source(template_data.u32(92),120);put(template_root,92,0);
        need(template_root==unhex(graph.at("root_hex")),"Complete environment/illumination source template changed");
        std::array<std::vector<std::byte>,5> source_template;
        for(unsigned platform=0;platform<3;++platform) {
            const auto& expected=graph.at("definitions").at(platform);
            std::vector<std::byte> raw(definitions.begin()+platform*40,definitions.begin()+(platform+1)*40);
            const Bytes definition(raw);
            for(unsigned n=0;n<template_fields.size();++n) {
                const auto [field,stride]=template_fields[n];const auto count=definition.u32(field),pointer=definition.u32(field+4);
                const auto& array=expected.at("arrays").at(n);
                need(count==array.at("count").get<unsigned>()&&(count||!pointer),"Complete environment/illumination compiler shape changed");
                auto bank=count?source(pointer,std::size_t(count)*stride):std::vector<std::byte>{};
                if(!platform)source_template[n]=bank;
                if(field==16)for(unsigned i=0;i<count;++i) {
                    const Bytes b(bank);const auto* pass=find(b.u32(i*10+4));const auto& name=array.at("typed_references").at(i);
                    need(b.u32(i*10)==0x73706173u&&pass&&pass->datum==b.u32(i*10+4)&&
                        pass->group==name.at(0).get<std::string>()&&pass->name==name.at(1).get<std::string>(),
                        "Environment/illumination compiler pass escaped its exact typed namespace");
                    put(bank,i*10+4,0);
                }
                need(bank==unhex(array.at("raw_hex")),"Complete environment/illumination compiler remap graph changed");
                put(raw,field+4,0);
            }
            need(raw==unhex(expected.at("definition_hex")),"Complete environment/illumination compiler definition changed");
        }
        const auto root_data=source(shader.address,84);const Bytes root(root_data);
        need(root.u32(0)==0x7374656du&&root.u32(4)==native_stem.datum&&root.u32(12)==1&&root.u32(20)==1&&
            !root.u32(24)&&!root.u32(28)&&root.u32(32)==3&&root.u32(56)==0xffffffffu&&!root.u32(76)&&!root.u32(80)&&
            root.u16(60)<=6&&root.u16(62)<=3&&root.u16(64)<=3&&std::isfinite(root.f32(68))&&std::isfinite(root.f32(72)),
            "Environment/illumination shader escaped its complete clock-only source contract");
        const auto shader_definitions=source(root.u32(36),372);
        need(std::all_of(shader_definitions.begin()+124,shader_definitions.end(),[](auto b){return b==std::byte{};}),
            "Environment/illumination shader acquired additional platform authoring");
        const Bytes definition(std::span<const std::byte>(shader_definitions).first(124));
        need(definition.u32(0)==native_stem.datum&&!definition.u32(116)&&!definition.u32(120),
            "Environment/illumination postprocess ownership changed");
        constexpr std::array<unsigned,14> fixed_counts{7,14,15,3,75,18,62,0,0,0,0,5,2,6};
        std::array<std::vector<std::byte>,14> data;
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];const auto count=definition.u32(field),pointer=definition.u32(field+4);
            if(i>=7&&i<=10)need(count<=(i==10?512u:64u),"Excessive environment/illumination clock bank");
            else need(count==fixed_counts[i],"Environment/illumination complete bank count changed");
            need(count||!pointer,"Empty environment/illumination bank has an unowned pointer");
            if(count)data[i]=source(pointer,std::size_t(count)*stride);
        }
        need(data[4]==source_template[1]&&data[5]==pass_ranges(source_template[2]),
            "Environment/illumination shader scheduling differs from its complete compiler graph");
        const auto native_lods=unhex(profile.at("native_arrays").at(0).at("raw_hex"));
        for(unsigned i=0;i<3;++i)need(Bytes(data[3]).u16(i*6+4)==Bytes(source_template[0]).u16(i*10)&&
            Bytes(data[3]).u16(i*6+4)==Bytes(native_lods).u16(i*10),"Environment/illumination authored LOD range changed");
        const auto bitmap=[&](unsigned handle,unsigned frame) {
            const auto* tag=find(handle);return handle!=0xffffffffu&&tag&&tag->datum==handle&&tag->group=="bitm"&&
                tag->address&&tag->size>=76&&!tag->name.empty()&&owns_frame(handle,frame);
        };
        const Bytes images(data[0]);
        for(unsigned i=0;i<7;++i)need(bitmap(images.u32(i*12),images.u32(i*12+4))&&std::isfinite(images.f32(i*12+8)),
            "Environment/illumination bitmap lost its complete typed frame owner");
        for(unsigned index:{2u,12u,13u})for(unsigned at=0;at<data[index].size();at+=4)
            need(std::isfinite(Bytes(data[index]).f32(at)),"Environment/illumination has a nonfinite authored value");
        const Bytes properties(data[11]);
        for(unsigned i=0;i<5;++i)need((properties.u16(i*4)==0xffffu||properties.u16(i*4)<7)&&properties.u16(i*4+2)==0xffffu,
            "Environment/illumination bitmap animation requires an unproved owner");
        const auto overlays=definition.u32(60);
        need(definition.u32(68)==overlays&&definition.u32(76)==overlays,
            "Environment/illumination clock parameters differ from complete paired ownership");
        std::vector<std::vector<std::byte>> curves;
        const Bytes functions(data[7]),references(data[8]),parameters(data[9]);
        for(unsigned i=0;i<overlays;++i) {
            need(references.u16(i*4)==i&&references.u16(i*4+2)==0xffffu&&parameters.u16(i*2)==(512u|i),
                "Environment/illumination clock acquired an unproved parameter binding");
            need(!functions.u32(i*20)&&!functions.u32(i*20+4)&&std::isfinite(functions.f32(i*20+8))&&functions.f32(i*20+8)>0,
                "Environment/illumination requires an unproved non-clock input/range producer");
            const auto size=functions.u32(i*20+12);
            need(size>=20&&size<=4096,"Invalid environment/illumination native curve extent");
            auto curve=source(functions.u32(i*20+16),size);
            need(std::to_integer<unsigned>(curve[0])<11,"Unknown environment/illumination native curve type");
            curves.push_back(std::move(curve));
        }
        const Bytes implementations(data[6]);const auto mappings=unsigned(data[10].size()/4);
        const auto original_maps=data[10];const Bytes original_mapping(original_maps);
        std::vector<bool> visited(mappings,false);
        const auto& rules=profile.at("roles").at("pixel_mapping_rules");
        for(unsigned si=0;si<62;++si) {
            for(unsigned field:{0u,2u,4u,8u})need(range(implementations.u16(si*10+field),mappings).second==0,
                "Environment/illumination animation escaped its paired pixel-only roles");
            const auto [first,count]=range(implementations.u16(si*10+6),mappings);if(!count)continue;
            const auto key=std::to_string(si);need(rules.contains(key),"Environment/illumination animated pass has no original mapping");
            struct Mapping {unsigned order,destination,component,parameter;};std::vector<Mapping> ordered;std::set<unsigned> orders;
            for(unsigned i=first;i<first+count;++i) {
                need(!visited[i],"Environment/illumination animation remap has multiple owners");visited[i]=true;
                const auto destination=unsigned(original_mapping.u16(i*4)),component=std::to_integer<unsigned>(original_maps[i*4+2]),
                    parameter=std::to_integer<unsigned>(original_maps[i*4+3]);
                const Json* selected=nullptr;
                for(const auto& rule:rules.at(key))if(rule.at(0)==destination&&rule.at(1)==component){need(!selected,"Ambiguous static illumination mapping");selected=&rule;}
                need(selected&&parameter<overlays,"Environment/illumination animation destination/parameter has no original counterpart");
                const auto order=selected->at(4).get<unsigned>();need(orders.insert(order).second,"Duplicate illumination constant destination");
                ordered.push_back({order,selected->at(2),selected->at(3),parameter});
            }
            std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a.order<b.order;});
            for(unsigned i=0;i<count;++i){const auto& row=ordered[i];const auto at=(first+i)*4;
                put(data[10],at,row.destination,2);put(data[10],at+2,row.component,1);put(data[10],at+3,row.parameter,1);}
        }
        need(std::all_of(visited.begin(),visited.end(),[](bool v){return v;}),"Environment/illumination has unowned residual animation rows");
        for(const auto [removed,first]:std::array{std::pair{9u,6u},std::pair{35u,32u}})
            need(std::ranges::equal(std::span(data[6]).subspan(removed*10,10),std::span(data[6]).subspan(first*10,10)),
                "Discarded PC shadow implementation carries distinct authored animation");
        std::vector<std::byte> native_implementations;
        for(const auto& index:profile.at("roles").at("implementation_sources")) {
            const auto bytes=std::span(data[6]).subspan(index.get<unsigned>()*10,10);
            native_implementations.insert(native_implementations.end(),bytes.begin(),bytes.end());
        }
        data[6]=std::move(native_implementations);
        for(unsigned n=0;n<4;++n)std::swap(data[1][8*4+n],data[1][9*4+n]);
        data[4]=unhex(profile.at("native_arrays").at(1).at("raw_hex"));
        data[5]=pass_ranges(unhex(profile.at("native_arrays").at(2).at("raw_hex")));
        const auto runtime=source(root.u32(16),80);const Bytes runtime_data(runtime);
        for(unsigned at:{0u,8u,44u,52u})need(runtime_data.u32(at+4)==0xffffffffu||
            (runtime_data.u32(at)==0x6269746du&&bitmap(runtime_data.u32(at+4),0)),"Environment/illumination runtime bitmap owner changed");
        const auto predicted_count=root.u32(44);need(predicted_count<=64&&(predicted_count||!root.u32(48)),"Invalid illumination prediction extent");
        const auto predictions=predicted_count?source(root.u32(48),predicted_count*8):std::vector<std::byte>{};const Bytes prediction(predictions);
        for(unsigned i=0;i<predicted_count;++i)need(!prediction.u16(i*8)&&bitmap(prediction.u32(i*8+4),prediction.u16(i*8+2)),
            "Environment/illumination prediction lost its complete source bitmap/frame");
        MccOwnedShaderProjection output{root_data,{}};
        const auto append=[&](unsigned field,std::span<const std::byte> bytes) {
            if(bytes.empty()){put(output.bytes,field,0);return;}
            const auto at=(output.bytes.size()+15)&~std::size_t(15);
            need(at+bytes.size()<=16u*1024u*1024u,"Excessive owned environment/illumination projection");
            output.bytes.resize(at);output.bytes.insert(output.bytes.end(),bytes.begin(),bytes.end());
            put(output.bytes,field,unsigned(at));output.pointers.emplace_back(field,unsigned(at));
        };
        append(16,runtime);put(output.bytes,32,1);append(36,std::span<const std::byte>(shader_definitions).first(124));
        const auto pp=Bytes(output.bytes).u32(36);append(48,predictions);
        for(unsigned i=0;i<fields.size();++i){const auto [field,stride]=fields[i];
            put(output.bytes,pp+field,unsigned(data[i].size()/stride));append(pp+field+4,data[i]);}
        const auto out_overlays=Bytes(output.bytes).u32(pp+64);
        for(unsigned i=0;i<curves.size();++i)append(out_overlays+i*20+16,curves[i]);
        return output;
    }catch(const FormatError&){return {};}
}
}

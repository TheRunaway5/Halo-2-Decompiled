#include "halo2/cache.hpp"
#include "halo2/mcc_native_world_default_roles.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <string_view>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_world_default_roles_profiles.inc"
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
constexpr std::array<std::pair<unsigned,unsigned>,5> template_fields{{
    {0,10},{8,2},{16,10},{24,6},{32,4}}};
void require(bool ok,const char* message){if(!ok)throw FormatError(message);}
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
std::vector<std::byte> unhex(std::string_view text) {
    require(text.size()%2==0,"Invalid static default-role bytes");
    std::vector<std::byte> result(text.size()/2);
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid static default-role hex");
    };
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte((digit(text[i*2])<<4)|digit(text[i*2+1]));
    return result;
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> out{};unsigned size{};
    require(EVP_Digest(bytes.data(),bytes.size(),out.data(),&size,EVP_sha256(),nullptr)==1&&size==out.size(),
            "Cannot hash exact native cooked-template source");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(auto v:out){result+=hex[v>>4];result+=hex[v&15];}return result;
}
const Json* profile_for(const Tag& stem,std::string_view payload) {
    // Immutable build-time registry, not a program or instruction stream.
    static const auto registry=Json::parse(native_world_default_roles_profiles);
    for(const auto& profile:registry.at("profiles"))
        if(profile.at("template")==stem.name&&profile.at("native_record").at("payload")==payload)return &profile;
    return nullptr;
}
std::vector<std::byte> shader_lods(std::span<const std::byte> template_lods) {
    require(template_lods.size()%10==0,"Invalid qualified template LOD bank");
    std::vector<std::byte> result(template_lods.size()/10*6);
    for(unsigned i=0;i<template_lods.size()/10;++i) {
        result[i*6+4]=template_lods[i*10];result[i*6+5]=template_lods[i*10+1];
    }
    return result;
}
std::vector<std::byte> shader_pass_ranges(std::span<const std::byte> passes) {
    require(passes.size()%10==0,"Invalid qualified template pass bank");
    std::vector<std::byte> result(passes.size()/10*2);
    for(unsigned i=0;i<passes.size()/10;++i) {
        result[i*2]=passes[i*10+8];result[i*2+1]=passes[i*10+9];
    }
    return result;
}
}

std::optional<MccOwnedShaderProjection> project_mcc_world_default_roles(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead& read_source,const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame,
    const MccShaderBitmapByName& find_bitmap) {
    try {
        const auto* profile=profile_for(native_stem,payload_name);
        if(!profile||!read_source||!find||!owns_frame||!find_bitmap)return {};
        const auto same=[&](const Tag& tag) {
            const auto* actual=find(tag.datum);
            return actual&&actual->datum==tag.datum&&actual->group==tag.group&&actual->name==tag.name&&
                actual->address==tag.address&&actual->size==tag.size;
        };
        require(shader.group=="shad"&&shader.datum!=0xffffffffu&&shader.address&&shader.size>=84&&same(shader),
                "Default-role material lost its actual full-salt source owner");
        require(native_stem.group=="stem"&&native_stem.datum!=0xffffffffu&&native_stem.address&&same(native_stem)&&
                source_stem.group=="stem"&&source_stem.datum==native_stem.datum&&source_stem.name==native_stem.name&&
                source_stem.address&&source_stem.size>=96&&source_stem.size<=16u*1024u*1024u,
                "Default-role material lost an independently owned source/native template");
        const auto& native_record=profile->at("native_record");
        require(payload_name.size()==68&&payload_name.substr(64)==".bin"&&
                payload.size()==native_record.at("bytes").get<std::size_t>()&&native_stem.size==payload.size()&&
                digest(payload)==payload_name.substr(0,64),
                "Default-role native template differs from its complete authenticated original payload");
        const auto source=[&](std::uint32_t at,std::size_t size) {
            require(size<=16u*1024u*1024u&&std::uint64_t(at)+size<=0x100000000ull,
                    "Default-role source allocation overflow");
            const auto bytes=read_source(at,size);
            require(bytes.size()==size,"Default-role source allocation extent mismatch");
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };

        // Require the complete literal PC compiler's typed remap graph, not
        // its tag name. Named authoring tables are absent in these inputs.
        const auto& graph=profile->at("source_graph");
        auto source_template=source(source_stem.address,96);const Bytes template_root(source_template);
        require(template_root.u32(88)==3,"Default-role source template platform count changed");
        const auto definitions=source(template_root.u32(92),3*40);
        word(source_template,92,0);
        require(source_template==unhex(graph.at("root_hex").get<std::string>()),
                "Complete source MCC template root differs from paired remap proof");
        std::array<std::vector<std::byte>,5> source_template_banks;
        for(unsigned platform=0;platform<3;++platform) {
            const auto& expected=graph.at("definitions").at(platform);
            std::vector<std::byte> normalized(definitions.begin()+platform*40,definitions.begin()+(platform+1)*40);
            const Bytes definition(normalized);
            for(unsigned n=0;n<template_fields.size();++n) {
                const auto [field,stride]=template_fields[n];
                const auto count=definition.u32(field),pointer=definition.u32(field+4);
                const auto& array=expected.at("arrays").at(n);
                require(array.at("field")==field&&count==array.at("count").get<unsigned>(),
                        "Complete source MCC template bank count changed");
                auto bank=count?source(pointer,std::size_t(count)*stride):std::vector<std::byte>{};
                require(count||pointer==0,"Empty source MCC template bank has a pointer");
                if(!platform)source_template_banks[n]=bank;
                if(field==16)for(unsigned i=0;i<count;++i) {
                    const Bytes pass(bank);const auto* tag=find(pass.u32(i*10+4));
                    const auto& reference=array.at("typed_references").at(i);
                    require(pass.u32(i*10)==0x73706173u&&tag&&tag->datum==pass.u32(i*10+4)&&
                            tag->group==reference.at(0).get<std::string>()&&
                            tag->name==reference.at(1).get<std::string>(),
                            "Source MCC template pass escaped its exact typed namespace");
                    word(bank,i*10+4,0);
                }
                require(bank==unhex(array.at("raw_hex").get<std::string>()),
                        "Complete source MCC compiled remap data differs from paired proof");
                word(normalized,field+4,0);
            }
            require(normalized==unhex(expected.at("definition_hex").get<std::string>()),
                    "Complete source MCC template definition differs from paired proof");
        }

        const auto root_data=source(shader.address,84);const Bytes root(root_data);
        require(root.u32(0)==0x7374656du&&root.u32(4)==native_stem.datum&&root.u32(12)==1&&
                root.u32(20)==profile->at("shader_type_flags").get<unsigned>()&&root.u32(24)==0&&
                root.u32(32)==3&&root.u32(56)==0xffffffffu&&root.u32(76)==0&&
                root.u16(60)<=6&&root.u16(62)<=3&&root.u16(64)<=3&&
                std::isfinite(root.f32(68))&&std::isfinite(root.f32(72)),
                "Default-role shader root is outside its source-proved unanimated contract");
        const auto shader_definitions=source(root.u32(36),3*124);
        require(std::all_of(shader_definitions.begin()+124,shader_definitions.end(),[](std::byte b){return b==std::byte{};}),
                "Default-role shader has additional platform authoring");
        const Bytes definition(std::span<const std::byte>(shader_definitions).first(124));
        require(definition.u32(0)==native_stem.datum&&!definition.u32(116)&&!definition.u32(120),
                "Default-role shader postprocess ownership changed");
        std::array<std::vector<std::byte>,14> banks;
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];
            const auto count=profile->at("source_counts").at(std::to_string(field)).get<unsigned>();
            require(definition.u32(field)==count,"Default-role shader bank shape differs from paired proof");
            const auto pointer=definition.u32(field+4);
            require(count||!pointer,"Empty cooked shader bank has a pointer");
            if(count)banks[i]=source(pointer,std::size_t(count)*stride);
        }
        require(banks[3]==shader_lods(source_template_banks[0])&&banks[4]==source_template_banks[1]&&
                banks[5]==shader_pass_ranges(source_template_banks[2])&&
                std::all_of(banks[6].begin(),banks[6].end(),[](std::byte b){return b==std::byte{};}),
                "Default-role shader scheduling does not match its complete source compiler graph");
        const auto bitmap=[&](std::uint32_t handle,std::uint32_t frame) {
            const auto* tag=find(handle);
            return handle!=0xffffffffu&&tag&&tag->datum==handle&&tag->group=="bitm"&&tag->address&&tag->size>=76&&
                !tag->name.empty()&&owns_frame(handle,frame);
        };
        const Bytes images(banks[0]),vectors(banks[2]),properties(banks[11]);
        for(unsigned i=0;i<banks[0].size()/12;++i)
            require(bitmap(images.u32(i*12),images.u32(i*12+4))&&std::isfinite(images.f32(i*12+8)),
                    "Default-role bitmap lost its complete typed source frame");
        for(unsigned i=0;i<banks[11].size()/4;++i)
            require((properties.u16(i*4)==0xffffu||properties.u16(i*4)<banks[0].size()/12)&&
                    properties.u16(i*4+2)==0xffffu,"Default-role bitmap property requires unproved animation");
        for(unsigned index:{2u,12u,13u}) {
            const Bytes values(banks[index]);
            for(unsigned at=0;at<banks[index].size();at+=4)
                require(std::isfinite(values.f32(at)),"Default-role material has a nonfinite authored value");
        }
        const auto runtime=source(root.u32(16),80);const Bytes runtime_data(runtime);
        for(unsigned at:{0u,8u,44u,52u})
            require(runtime_data.u32(at+4)==0xffffffffu||
                    (runtime_data.u32(at)==0x6269746du&&bitmap(runtime_data.u32(at+4),0)),
                    "Default-role runtime property lost its complete bitmap owner");
        const auto diffuse=profile->at("runtime_diffuse_slot").get<int>();
        require(diffuse<0?runtime_data.u32(4)==0xffffffffu:
                runtime_data.u32(4)==images.u32(unsigned(diffuse)*12),
                "Default-role runtime diffuse role differs from paired literal source setup");
        const auto prediction_count=root.u32(44);
        require(prediction_count<=64,"Excessive cooked predicted resources");
        const auto predictions=prediction_count?source(root.u32(48),prediction_count*8):std::vector<std::byte>{};
        const Bytes prediction_data(predictions);
        for(unsigned i=0;i<prediction_count;++i)
            require(prediction_data.u16(i*8)==0&&bitmap(prediction_data.u32(i*8+4),prediction_data.u16(i*8+2)),
                    "Default-role prediction lost its complete bitmap/frame owner");

        // These finite byte maps are paired asset-format conversions. GPU
        // programs are the precompiled native TU1.5 kernels; none are read or
        // executed by this conversion.
        const auto& default_role=profile->at("default_bitmap");
        const auto default_owner=find_bitmap(default_role.at("name").get<std::string>());
        require(default_owner&&same(default_owner->tag)&&default_owner->tag.group=="bitm"&&
                default_owner->tag.name==default_role.at("name").get<std::string>()&&
                bitmap(default_owner->tag.datum,0)&&!owns_frame(default_owner->tag.datum,1),
                "Original default role lost its exact resident typed single-frame bitmap owner");
        const auto default_handle=default_owner->tag.datum;
        const auto& texture=default_owner->texture;const auto& expected=default_role.at("texture");
        require(texture.width==expected.at("width")&&texture.height==expected.at("height")&&
                texture.format==expected.at("format")&&texture.depth==expected.at("depth")&&
                texture.layers==expected.at("layers")&&texture.pixels.size()==expected.at("bytes")&&
                texture.mips.size()==expected.at("mips").size()&&
                digest(std::span<const std::byte>(texture.pixels.data(),texture.pixels.size()))==expected.at("sha256").get<std::string>(),
                "Original default role texture differs from every authenticated source mip");
        std::size_t extent{};
        for(unsigned index=0;index<texture.mips.size();++index) {
            const auto& mip=texture.mips[index];const auto& wanted=expected.at("mips").at(index);
            require(mip.offset==extent&&mip.offset<=texture.pixels.size()&&mip.size<=texture.pixels.size()-mip.offset&&
                    mip.width==wanted.at("width")&&mip.height==wanted.at("height")&&mip.depth==1&&
                    mip.size==wanted.at("bytes")&&digest(std::span<const std::byte>(texture.pixels.data(),texture.pixels.size()).subspan(mip.offset,mip.size))==wanted.at("sha256").get<std::string>(),
                    "Original default role mip owner/order differs");
            extent+=mip.size;
        }
        require(extent==texture.pixels.size(),"Original default role has unowned trailing texture bytes");
        const auto default_uv=unhex(profile->at("default_uv_hex").get<std::string>());
        require(default_uv.size()==16,"Invalid original default UV role extent");
        const auto operation=profile->at("mapping").get<std::string>();
        const auto old_vectors=banks[2];
        if(operation=="tex_bump_three_detail_blend") {
            require(banks[0].size()==72&&banks[1].size()==8&&old_vectors.size()==144,
                    "Original three-detail source role shape changed");
            banks[2].clear();
            for(unsigned index:{1u,2u,3u,4u,0u,7u,8u,5u,6u})
                banks[2].insert(banks[2].end(),old_vectors.begin()+index*16,old_vectors.begin()+(index+1)*16);
        } else {
            require(operation=="tex_env"&&banks[0].size()==60&&banks[1].size()==24&&old_vectors.size()==112,
                    "Original environment source role shape changed");
            banks[1].resize(20);
        }
        banks[2].insert(banks[2].end(),default_uv.begin(),default_uv.end());
        std::vector<std::byte> parameter(12);
        word(parameter,0,default_handle);word(parameter,4,default_role.at("frame").get<std::uint32_t>());
        word(parameter,8,default_role.at("log_word").get<std::uint32_t>());
        const auto insertion=default_role.at("insert_slot").get<std::size_t>()*12;
        require(insertion<=banks[0].size(),"Original default role slot escaped its complete source image bank");
        banks[0].insert(banks[0].begin()+insertion,parameter.begin(),parameter.end());

        const Bytes native(payload);
        require(native.u32(88)==1,"Native template has additional postprocess definitions");
        const auto pp=native.u32(92);native.require(pp,40);
        std::array<std::vector<std::byte>,5> native_banks;
        for(unsigned i=0;i<template_fields.size();++i) {
            const auto [field,stride]=template_fields[i];
            const auto count=profile->at("native_shape").at(std::to_string(field)).get<unsigned>();
            require(native.u32(pp+field)==count,"Complete original native template shape changed");
            const auto span=native.slice(native.u32(pp+field+4),std::size_t(count)*stride);
            native_banks[i]={span.begin(),span.end()};
        }
        banks[3]=shader_lods(native_banks[0]);banks[4]=native_banks[1];
        banks[5]=shader_pass_ranges(native_banks[2]);
        banks[6].assign(profile->at("native_shape").at("24").get<std::size_t>()*10,std::byte{});
        MccOwnedShaderProjection result{root_data,{}};
        const auto append=[&](unsigned field,std::span<const std::byte> bytes) {
            if(bytes.empty()){word(result.bytes,field,0);return;}
            const auto offset=(result.bytes.size()+15)&~std::size_t(15);
            require(offset+bytes.size()<=16u*1024u*1024u,"Excessive owned cooked projection");
            result.bytes.resize(offset);result.bytes.insert(result.bytes.end(),bytes.begin(),bytes.end());
            word(result.bytes,field,std::uint32_t(offset));result.pointers.emplace_back(field,std::uint32_t(offset));
        };
        append(16,runtime);word(result.bytes,28,0);word(result.bytes,32,1);
        append(36,std::span<const std::byte>(shader_definitions).first(124));
        const auto output_definition=Bytes(result.bytes).u32(36);
        append(48,predictions);word(result.bytes,80,0);
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];
            require(banks[i].size()%stride==0,"Default-role projection produced an incomplete typed bank");
            word(result.bytes,output_definition+field,std::uint32_t(banks[i].size()/stride));
            append(output_definition+field+4,banks[i]);
        }
        return result;
    }catch(const FormatError&){return {};}
}
}

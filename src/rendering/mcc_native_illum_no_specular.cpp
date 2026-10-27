#include "halo2/cache.hpp"
#include "halo2/mcc_native_illum_no_specular.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <string_view>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_illum_no_specular_profiles.inc"
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
    require(text.size()%2==0,"Invalid static illumination material-profile bytes");
    std::vector<std::byte> result(text.size()/2);
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid static illumination material-profile hex");
    };
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte((digit(text[i*2])<<4)|digit(text[i*2+1]));
    return result;
}
std::string digest(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> out{};unsigned size{};
    require(EVP_Digest(bytes.data(),bytes.size(),out.data(),&size,EVP_sha256(),nullptr)==1&&size==out.size(),
            "Cannot hash exact native illumination material-template source");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(auto v:out){result+=hex[v>>4];result+=hex[v&15];}return result;
}
const Json* profile_for(const Tag& stem,std::string_view payload) {
    // Immutable build-time registry, not a program or instruction stream.
    static const auto registry=Json::parse(native_illum_no_specular_profiles);
    for(const auto& profile:registry.at("profiles"))
        if(profile.at("template")==stem.name&&profile.at("native_record").at("payload")==payload)return &profile;
    return nullptr;
}
std::vector<std::byte> shader_lods(std::span<const std::byte> template_lods,std::span<const std::byte> authored_lods) {
    require(template_lods.size()%10==0&&authored_lods.size()==template_lods.size()/10*6,
            "Invalid paired illumination LOD bank");
    for(unsigned i=0;i<template_lods.size()/10;++i)
        require(Bytes(template_lods).u16(i*10)==Bytes(authored_lods).u16(i*6+4),
                "Paired illumination animation mask escaped its exact template LOD range");
    return {authored_lods.begin(),authored_lods.end()};
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

std::optional<MccOwnedShaderProjection> project_mcc_clock_illum_no_specular(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead& read_source,const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame) {
    try {
        const auto* profile=profile_for(native_stem,payload_name);
        if(!profile||!read_source||!find||!owns_frame)return {};
        const auto same=[&](const Tag& tag) {
            const auto* actual=find(tag.datum);
            return actual&&actual->datum==tag.datum&&actual->group==tag.group&&actual->name==tag.name&&
                actual->address==tag.address&&actual->size==tag.size;
        };
        require(shader.group=="shad"&&shader.datum!=0xffffffffu&&shader.address&&shader.size>=84&&same(shader),
                "Illumination material lost its actual full-salt source owner");
        require(native_stem.group=="stem"&&native_stem.datum!=0xffffffffu&&native_stem.address&&same(native_stem)&&
                source_stem.group=="stem"&&source_stem.datum==native_stem.datum&&source_stem.name==native_stem.name&&
                source_stem.address&&source_stem.size>=96&&source_stem.size<=16u*1024u*1024u,
                "Illumination material lost an independently owned source/native template");
        const auto& native_record=profile->at("native_record");
        require(payload_name.size()==68&&payload_name.substr(64)==".bin"&&
                payload.size()==native_record.at("bytes").get<std::size_t>()&&native_stem.size==payload.size()&&
                digest(payload)==payload_name.substr(0,64),
                "Illumination material native template differs from its complete authenticated original payload");
        const auto source=[&](std::uint32_t at,std::size_t size) {
            require(size<=16u*1024u*1024u&&std::uint64_t(at)+size<=0x100000000ull,
                    "Illumination material source allocation overflow");
            const auto bytes=read_source(at,size);
            require(bytes.size()==size,"Illumination material source allocation extent mismatch");
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };

        // Require the complete literal PC compiler's typed remap graph, not
        // its tag name. Named authoring tables are absent in these inputs.
        const auto& graph=profile->at("source_graph");
        auto source_template=source(source_stem.address,96);const Bytes template_root(source_template);
        require(template_root.u32(88)==3,"Illumination material source template platform count changed");
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
                root.u32(20)==profile->at("shader_type_flags").get<unsigned>()&&root.u32(24)==0&&root.u32(28)==0&&
                root.u32(32)==3&&root.u32(56)==0xffffffffu&&root.u32(76)==0&&
                root.u16(60)<=6&&root.u16(62)<=3&&root.u16(64)<=3&&
                std::isfinite(root.f32(68))&&std::isfinite(root.f32(72)),
                "Illumination material shader root is outside its source-proved clock-only contract");
        const auto shader_definitions=source(root.u32(36),3*124);
        require(std::all_of(shader_definitions.begin()+124,shader_definitions.end(),[](std::byte b){return b==std::byte{};}),
                "Illumination material shader has additional platform authoring");
        const Bytes definition(std::span<const std::byte>(shader_definitions).first(124));
        require(definition.u32(0)==native_stem.datum&&!definition.u32(116)&&!definition.u32(120),
                "Illumination material shader postprocess ownership changed");
        std::array<std::vector<std::byte>,14> banks;
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];
            const auto count=profile->at("source_counts").at(std::to_string(field)).get<unsigned>();
            require(definition.u32(field)==count,"Illumination material shader bank shape differs from paired proof");
            const auto pointer=definition.u32(field+4);
            require(count||!pointer,"Empty illumination material shader bank has a pointer");
            if(count)banks[i]=source(pointer,std::size_t(count)*stride);
        }
        require(banks[3]==shader_lods(source_template_banks[0],unhex(profile->at("source_role_banks").at("28").get<std::string>()))&&banks[4]==source_template_banks[1]&&
                banks[5]==shader_pass_ranges(source_template_banks[2]),
                "Illumination material shader scheduling does not match its complete source compiler graph");
        const auto bitmap=[&](std::uint32_t handle,std::uint32_t frame) {
            const auto* tag=find(handle);
            return handle!=0xffffffffu&&tag&&tag->datum==handle&&tag->group=="bitm"&&tag->address&&tag->size>=76&&
                !tag->name.empty()&&owns_frame(handle,frame);
        };
        const Bytes images(banks[0]);
        for(unsigned i=0;i<banks[0].size()/12;++i)
            require(bitmap(images.u32(i*12),images.u32(i*12+4))&&std::isfinite(images.f32(i*12+8)),
                    "Illumination material bitmap lost its complete typed source frame");
        for(unsigned index:{6u,8u,9u,10u,11u})
            require(banks[index]==unhex(profile->at("source_role_banks").at(std::to_string(fields[index].first)).get<std::string>()),
                    "Illumination material differs from its complete paired source animation roles");
        const auto& overlay_profile=profile->at("clock_overlays");
        require(banks[7].size()==overlay_profile.size()*20,"Illumination material overlay count changed");
        const Bytes overlays(banks[7]);
        std::vector<std::vector<std::byte>> curves;
        for(unsigned i=0;i<overlay_profile.size();++i) {
            const auto& expected=overlay_profile.at(i);
            require(overlays.u32(i*20)==0&&overlays.u32(i*20+4)==0,
                    "Illumination material requires an unauthenticated external input/range producer");
            const auto period=overlays.u32(i*20+8);
            const auto& periods=expected.at("period_words");
            require(std::find(periods.begin(),periods.end(),Json(period))!=periods.end(),
                    "Illumination material period differs from literal authored source");
            auto expected_curve=unhex(expected.at("curve_hex").get<std::string>());
            require(!expected_curve.empty()&&expected_curve.size()<=4096&&overlays.u32(i*20+12)==expected_curve.size(),
                    "Illumination material function extent differs from its original curve authority");
            auto curve=source(overlays.u32(i*20+16),expected_curve.size());
            require(curve==expected_curve,"Illumination material curve differs from its complete original authority");
            curves.push_back(std::move(curve));
        }
        for(unsigned index:{2u,12u,13u}) {
            const Bytes values(banks[index]);
            for(unsigned at=0;at<banks[index].size();at+=4)
                require(std::isfinite(values.f32(at)),"Illumination material has a nonfinite authored value");
        }
        const auto runtime=source(root.u32(16),80);const Bytes runtime_data(runtime);
        for(unsigned at:{0u,8u,44u,52u})
            require(runtime_data.u32(at+4)==0xffffffffu||
                    (runtime_data.u32(at)==0x6269746du&&bitmap(runtime_data.u32(at+4),0)),
                    "Illumination material runtime property lost its complete bitmap owner");
        const auto diffuse=profile->at("runtime_diffuse_slot").get<int>();
        require(diffuse<0?runtime_data.u32(4)==0xffffffffu:
                runtime_data.u32(4)==images.u32(unsigned(diffuse)*12),
                "Illumination material runtime diffuse role differs from paired literal source setup");
        const auto prediction_count=root.u32(44);
        require(prediction_count<=64&&(prediction_count||!root.u32(48)),
                "Invalid illumination material predicted resource extent");
        const auto predictions=prediction_count?source(root.u32(48),prediction_count*8):std::vector<std::byte>{};
        const Bytes prediction_data(predictions);
        for(unsigned i=0;i<prediction_count;++i)
            require(prediction_data.u16(i*8)==0&&bitmap(prediction_data.u32(i*8+4),prediction_data.u16(i*8+2)),
                    "Illumination material prediction lost its complete bitmap/frame owner");

        // Finite data admission only: literal clock curves and remap roles
        // are retained for the existing native constant writer. No shader
        // program is read or executed by this conversion.
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
        banks[3]=shader_lods(native_banks[0],unhex(profile->at("native_role_banks").at("28").get<std::string>()));banks[4]=native_banks[1];
        banks[5]=shader_pass_ranges(native_banks[2]);
        // The paired TU implementation has one pixel slot. Two PC-only
        // words and one PC-only implementation do not participate in its
        // native compiler roles. Preserve the authored slot0 value exactly.
        require(banks[1].size()==12,"Illumination source pixel bank changed");
        banks[1].resize(4);
        banks[6]=unhex(profile->at("native_role_banks").at("52").get<std::string>());
        banks[10]=unhex(profile->at("native_role_banks").at("84").get<std::string>());
        require(banks[6].size()==profile->at("native_shape").at("24").get<std::size_t>()*10,
                "Illumination material implementation roles differ from the complete native template");
        MccOwnedShaderProjection result{root_data,{}};
        const auto append=[&](unsigned field,std::span<const std::byte> bytes) {
            if(bytes.empty()){word(result.bytes,field,0);return;}
            const auto offset=(result.bytes.size()+15)&~std::size_t(15);
            require(offset+bytes.size()<=16u*1024u*1024u,"Excessive owned illumination material projection");
            result.bytes.resize(offset);result.bytes.insert(result.bytes.end(),bytes.begin(),bytes.end());
            word(result.bytes,field,std::uint32_t(offset));result.pointers.emplace_back(field,std::uint32_t(offset));
        };
        append(16,runtime);word(result.bytes,28,0);word(result.bytes,32,1);
        append(36,std::span<const std::byte>(shader_definitions).first(124));
        const auto output_definition=Bytes(result.bytes).u32(36);
        append(48,predictions);word(result.bytes,80,0);
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];
            require(banks[i].size()%stride==0,"Illumination material projection produced an incomplete typed bank");
            word(result.bytes,output_definition+field,std::uint32_t(banks[i].size()/stride));
            append(output_definition+field+4,banks[i]);
        }
        const auto output_overlays=Bytes(result.bytes).u32(output_definition+64);
        for(unsigned i=0;i<curves.size();++i)
            append(output_overlays+i*20+16,curves[i]);
        return result;
    }catch(const FormatError&){return {};}
}
}

#include "halo2/cache.hpp"
#include "halo2/mcc_native_shaders.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <string_view>

namespace halo2 {
namespace {
using Json=nlohmann::json;
#include "rendering/mcc_native_world_cooked_profiles.inc"
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
constexpr std::array<std::pair<unsigned,unsigned>,5> template_fields{{
    {0,10},{8,2},{16,10},{24,6},{32,4}}};
constexpr std::string_view resident_illum_payload=
    "ffbc8f0460d8beaf3e5349ba896d63d42fb7bbbccd7b18b9735ba19ccce06bec.bin";
void require(bool ok,const char* message){if(!ok)throw FormatError(message);}
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
std::vector<std::byte> unhex(std::string_view text) {
    require(text.size()%2==0,"Invalid static cooked-profile bytes");
    std::vector<std::byte> result(text.size()/2);
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid static cooked-profile hex");
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
    static const auto registry=Json::parse(native_world_cooked_profiles);
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

static std::optional<MccOwnedShaderProjection> project_world_cooked(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead& read_source,const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame,
    bool resident_template) {
    try {
        const auto* profile=profile_for(native_stem,payload_name);
        if(!profile||!read_source||!find||!owns_frame)return {};
        const auto same=[&](const Tag& tag) {
            const auto* actual=find(tag.datum);
            return actual&&actual->datum==tag.datum&&actual->group==tag.group&&actual->name==tag.name&&
                actual->address==tag.address&&actual->size==tag.size;
        };
        require(shader.group=="shad"&&shader.datum!=0xffffffffu&&shader.address&&shader.size>=84&&same(shader),
                "Cooked material lost its actual full-salt source owner");
        require(native_stem.group=="stem"&&native_stem.datum!=0xffffffffu&&native_stem.address&&same(native_stem)&&
                source_stem.group=="stem"&&source_stem.datum==native_stem.datum&&source_stem.name==native_stem.name&&
                source_stem.address&&source_stem.size>=96&&source_stem.size<=16u*1024u*1024u,
                "Cooked material lost an independently owned source/native template");
        const auto& native_record=profile->at("native_record");
        require(native_stem.size==payload.size()&&(resident_template ||
                (payload_name.size()==68&&payload_name.substr(64)==".bin"&&
                 payload.size()==native_record.at("bytes").get<std::size_t>()&&digest(payload)==payload_name.substr(0,64))),
                "Cooked native template differs from its complete authenticated original payload");
        const auto source=[&](std::uint32_t at,std::size_t size) {
            require(size<=16u*1024u*1024u&&std::uint64_t(at)+size<=0x100000000ull,
                    "Cooked source allocation overflow");
            const auto bytes=read_source(at,size);
            require(bytes.size()==size,"Cooked source allocation extent mismatch");
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };

        // Require the complete literal PC compiler's typed remap graph, not
        // its tag name. Named authoring tables are absent in these inputs.
        const auto& graph=profile->at("source_graph");
        auto source_template=source(source_stem.address,96);const Bytes template_root(source_template);
        require(template_root.u32(88)==3,"Cooked source template platform count changed");
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
                "Cooked shader root is outside its source-proved unanimated contract");
        const auto shader_definitions=source(root.u32(36),3*124);
        require(std::all_of(shader_definitions.begin()+124,shader_definitions.end(),[](std::byte b){return b==std::byte{};}),
                "Cooked shader has additional platform authoring");
        const Bytes definition(std::span<const std::byte>(shader_definitions).first(124));
        require(definition.u32(0)==native_stem.datum&&!definition.u32(116)&&!definition.u32(120),
                "Cooked shader postprocess ownership changed");
        std::array<std::vector<std::byte>,14> banks;
        for(unsigned i=0;i<fields.size();++i) {
            const auto [field,stride]=fields[i];
            const auto count=profile->at("source_counts").at(std::to_string(field)).get<unsigned>();
            require(definition.u32(field)==count,"Cooked shader bank shape differs from paired proof");
            const auto pointer=definition.u32(field+4);
            require(count||!pointer,"Empty cooked shader bank has a pointer");
            if(count)banks[i]=source(pointer,std::size_t(count)*stride);
        }
        require(banks[3]==shader_lods(source_template_banks[0])&&banks[4]==source_template_banks[1]&&
                banks[5]==shader_pass_ranges(source_template_banks[2])&&
                std::all_of(banks[6].begin(),banks[6].end(),[](std::byte b){return b==std::byte{};}),
                "Cooked shader scheduling does not match its complete source compiler graph");
        const auto bitmap=[&](std::uint32_t handle,std::uint32_t frame) {
            const auto* tag=find(handle);
            return handle!=0xffffffffu&&tag&&tag->datum==handle&&tag->group=="bitm"&&tag->address&&tag->size>=76&&
                !tag->name.empty()&&owns_frame(handle,frame);
        };
        const Bytes images(banks[0]),vectors(banks[2]),properties(banks[11]);
        for(unsigned i=0;i<banks[0].size()/12;++i)
            require(bitmap(images.u32(i*12),images.u32(i*12+4))&&std::isfinite(images.f32(i*12+8)),
                    "Cooked bitmap lost its complete typed source frame");
        for(unsigned i=0;i<banks[11].size()/4;++i)
            require((properties.u16(i*4)==0xffffu||properties.u16(i*4)<banks[0].size()/12)&&
                    properties.u16(i*4+2)==0xffffu,"Cooked bitmap property requires unproved animation");
        for(unsigned index:{2u,12u,13u}) {
            const Bytes values(banks[index]);
            for(unsigned at=0;at<banks[index].size();at+=4)
                require(std::isfinite(values.f32(at)),"Cooked material has a nonfinite authored value");
        }
        const auto runtime=source(root.u32(16),80);const Bytes runtime_data(runtime);
        for(unsigned at:{0u,8u,44u,52u})
            require(runtime_data.u32(at+4)==0xffffffffu||
                    (runtime_data.u32(at)==0x6269746du&&bitmap(runtime_data.u32(at+4),0)),
                    "Cooked runtime property lost its complete bitmap owner");
        const auto diffuse=profile->at("runtime_diffuse_slot").get<int>();
        require(diffuse<0?runtime_data.u32(4)==0xffffffffu:
                runtime_data.u32(4)==images.u32(unsigned(diffuse)*12),
                "Cooked runtime diffuse role differs from paired literal source setup");
        const auto prediction_count=root.u32(44);
        require(prediction_count<=64,"Excessive cooked predicted resources");
        const auto predictions=prediction_count?source(root.u32(48),prediction_count*8):std::vector<std::byte>{};
        const Bytes prediction_data(predictions);
        for(unsigned i=0;i<prediction_count;++i)
            require(prediction_data.u16(i*8)==0&&bitmap(prediction_data.u32(i*8+4),prediction_data.u16(i*8+2)),
                    "Cooked prediction lost its complete bitmap/frame owner");

        // These finite byte maps are paired asset-format conversions. GPU
        // programs are the precompiled native TU1.5 kernels; none are read or
        // executed by this conversion.
        const auto operation=profile->at("mapping").get<std::string>();
        if(operation=="drop_pc_only_pixels")banks[1].clear();
        else if(operation=="swap_environment_pixels") {
            require(banks[1].size()==16,"Invalid paired environment color bank");
            const auto a=Bytes(banks[1]).u32(4),b=Bytes(banks[1]).u32(8);
            word(banks[1],4,b);word(banks[1],8,a);
        } else if(operation=="environment_combined"||operation=="environment_alpha") {
            require(banks[2].size()>48,"Invalid paired environment vector bank");
            std::vector<std::byte> result;
            const auto append=[&](unsigned first,unsigned count) {
                result.insert(result.end(),banks[2].begin()+first,banks[2].begin()+first+count);
            };
            append(32,16);append(16,16);append(0,16);
            if(operation=="environment_alpha") {
                const auto offset=result.size();result.resize(offset+16);
                word(result,offset+8,0x3f800000u);word(result,offset+12,0x3f800000u);
            }
            append(48,unsigned(banks[2].size()-48));banks[2]=std::move(result);
            if(operation=="environment_combined") {
                std::vector<std::byte> pixels(4);word(pixels,0,0x00ffffffu);
                pixels.insert(pixels.end(),banks[1].begin(),banks[1].end());banks[1]=std::move(pixels);
            }
        }else require(operation=="copy","Unknown finite cooked-profile conversion");

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
            require(banks[i].size()%stride==0,"Cooked projection produced an incomplete typed bank");
            word(result.bytes,output_definition+field,std::uint32_t(banks[i].size()/stride));
            append(output_definition+field+4,banks[i]);
        }
        return result;
    }catch(const FormatError&){return {};}
}

std::optional<MccOwnedShaderProjection> project_mcc_unanimated_world_cooked(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead& read,const MccShaderFind& find,const MccShaderBitmapFrame& frames) {
    return project_world_cooked(shader,source_stem,native_stem,payload_name,payload,read,find,frames,false);
}

std::optional<MccOwnedShaderProjection> project_mcc_resident_one_alpha_env_illum_template(
    const Tag& source_stem,const Tag& resident_pass,const Tag& resident_vertex,
    const MccShaderRead& read,const MccShaderFind& find) {
    try {
        const auto* profile=profile_for(source_stem,resident_illum_payload);
        if(!profile || !read || !find)return {};
        const auto source=[&](std::uint32_t address,std::size_t extent) {
            require(address&&extent&&extent<=16u*1024u*1024u&&
                    std::uint64_t(address)+extent<=0x100000000ull,"Resident cooked metadata extent changed");
            const auto bytes=read(address,extent);
            require(bytes.size()==extent,"Resident cooked metadata read is incomplete");
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };
        const auto same=[&](const Tag& owner) {
            const auto* actual=find(owner.datum);
            return actual&&actual->datum==owner.datum&&actual->group==owner.group&&actual->name==owner.name&&
                actual->address==owner.address&&actual->size==owner.size;
        };
        require(resident_pass.group=="spas"&&resident_pass.name=="shaders\\shader_passes\\transparent\\default_transparent_one_sided"&&
                resident_pass.size==576&&same(resident_pass)&&resident_vertex.group=="vrtx"&&
                resident_vertex.name=="rasterizer\\vertex_shaders_xbox\\transparent_generic_reflection"&&
                resident_vertex.size==5608&&same(resident_vertex),"Resident reflection program owners changed");
        auto pass=source(resident_pass.address,resident_pass.size);
        auto vertex=source(resident_vertex.address,resident_vertex.size);
        const auto normalize=[&](auto& bytes,const Tag& owner,const auto& pointers) {
            for(const auto& [field,target]:pointers) {
                require(std::uint64_t(owner.address)+target<=0xffffffffu&&
                        Bytes(bytes).u32(field)==owner.address+target,"Resident reflection pointer ownership changed");
                word(bytes,field,target);
            }
        };
        constexpr std::array<std::pair<unsigned,unsigned>,5> pass_pointers{{
            {32,36},{40,124},{48,464},{56,432},{64,480}}};
        constexpr std::array<std::pair<unsigned,unsigned>,15> vertex_pointers{{
            {8,12},{20,208},{28,216},{48,892},{56,904},{76,1708},{84,1720},{104,1708},
            {112,2604},{132,3520},{140,3536},{160,3520},{168,4532},{188,3520},{196,4532}}};
        normalize(pass,resident_pass,pass_pointers);normalize(vertex,resident_vertex,vertex_pointers);
        require(Bytes(pass).u32(380)==resident_vertex.datum,"Resident reflection vertex reference changed");
        word(pass,380,0);
        // Complete already-resident03a pass/vertex payloads, including all
        // state, sampler, program and declaration bytes; only addresses and
        // the independently checked full vertex handle are normalized.
        require(digest(pass)=="d8c09130f1242ce045116fa3cc2c6a03425733f5df800ae4db0ccd40f3d99312"&&
                digest(vertex)=="9b8a3c3c0e1d8e21eef32be5a16dfd02cc362b33825d2e2665a290dcf61fefe2",
                "Resident reflection pass or vertex bytes changed");
        const auto* current=find(source_stem.datum);
        require(source_stem.group=="stem"&&source_stem.address&&source_stem.size>=96&&source_stem.size<=16u*1024u*1024u&&
                std::uint64_t(source_stem.address)+source_stem.size<=0x100000000ull&&current&&
                current->datum==source_stem.datum&&current->group==source_stem.group&&current->name==source_stem.name,
                "Resident cooked source template identity changed");
        const auto& graph=profile->at("source_graph");
        auto root=source(source_stem.address,96);
        require(Bytes(root).u32(88)==3,"Resident cooked source platform count changed");
        const auto definitions=source(Bytes(root).u32(92),120);word(root,92,0);
        require(root==unhex(graph.at("root_hex").get<std::string>()),"Resident cooked source template root changed");
        std::array<std::vector<std::byte>,5> banks;
        for(unsigned platform=0;platform<3;++platform) {
            auto definition=std::vector<std::byte>(definitions.begin()+platform*40,definitions.begin()+(platform+1)*40);
            const auto& expected=graph.at("definitions").at(platform);
            for(unsigned n=0;n<template_fields.size();++n) {
                const auto [field,stride]=template_fields[n];const auto count=Bytes(definition).u32(field),pointer=Bytes(definition).u32(field+4);
                const auto& array=expected.at("arrays").at(n);
                require(array.at("field")==field&&count==array.at("count").get<unsigned>(),"Resident cooked source bank shape changed");
                auto bank=count?source(pointer,std::size_t(count)*stride):std::vector<std::byte>{};
                require(count||!pointer,"Resident cooked empty source bank has a pointer");
                if(!platform)banks[n]=bank;
                if(field==16)for(unsigned pass_index=0;pass_index<count;++pass_index) {
                    const auto at=pass_index*10;const auto* actual=find(Bytes(bank).u32(at+4));
                    const auto& expected_pass=array.at("typed_references").at(pass_index);
                    require(Bytes(bank).u32(at)==0x73706173u&&actual&&actual->datum==Bytes(bank).u32(at+4)&&
                            actual->datum==resident_pass.datum&&actual->group==expected_pass.at(0).get<std::string>()&&
                            actual->name==expected_pass.at(1).get<std::string>(),"Resident cooked typed pass reference changed");
                    word(bank,at+4,0);
                }
                require(bank==unhex(array.at("raw_hex").get<std::string>()),"Resident cooked source state/remap bytes changed");
                word(definition,field+4,0);
            }
            require(definition==unhex(expected.at("definition_hex").get<std::string>()),"Resident cooked source definition changed");
        }
        // Paired source proof: every other template row is identical. These
        // four pixel destinations are the existing Vulkan pass's registers.
        constexpr std::array<std::uint32_t,4> pixels{{0x00040000u,0x01030400u,0x02040400u,0x03040040u}};
        for(unsigned n=0;n<pixels.size();++n)word(banks[4],(n+4)*4,pixels[n]);
        word(root,88,1);MccOwnedShaderProjection result{std::move(root),{}};
        result.bytes.resize(136);word(result.bytes,92,96);result.pointers.emplace_back(92,96);
        for(unsigned n=0;n<template_fields.size();++n) {
            const auto [field,stride]=template_fields[n];const auto at=(result.bytes.size()+3)&~std::size_t(3);
            result.bytes.resize(at);result.bytes.insert(result.bytes.end(),banks[n].begin(),banks[n].end());
            word(result.bytes,96+field,unsigned(banks[n].size()/stride));word(result.bytes,100+field,unsigned(at));
            result.pointers.emplace_back(100+field,unsigned(at));
        }
        return result;
    }catch(const FormatError&){return {};}
}

std::optional<MccOwnedShaderProjection> project_mcc_resident_one_alpha_env_illum_material(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    const Tag& resident_pass,const Tag& resident_vertex,const MccShaderRead& read,
    const MccShaderFind& find,const MccShaderBitmapFrame& frames) {
    try {
        auto expected=project_mcc_resident_one_alpha_env_illum_template(source_stem,resident_pass,resident_vertex,read,find);
        if(!expected || native_stem.size!=expected->bytes.size())return {};
        const auto installed_bytes=read(native_stem.address,native_stem.size);
        auto installed=std::vector<std::byte>(installed_bytes.begin(),installed_bytes.end());
        for(const auto& [field,target]:expected->pointers) {
            require(std::uint64_t(native_stem.address)+target<=0xffffffffu&&
                    Bytes(installed).u32(field)==native_stem.address+target,"Resident cooked template relocation changed");
            word(installed,field,target);
        }
        require(installed==expected->bytes,"Resident cooked template differs from its qualified source projection");
        return project_world_cooked(shader,source_stem,native_stem,resident_illum_payload,expected->bytes,read,find,frames,true);
    }catch(const FormatError&){return {};}
}
}

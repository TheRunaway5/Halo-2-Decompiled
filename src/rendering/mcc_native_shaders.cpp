#include "halo2/cache.hpp"
#include "halo2/mcc_native_shaders.hpp"
#include "halo2/mcc_native_no_alpha.hpp"
#include "halo2/mcc_native_world_animated.hpp"
#include "halo2/mcc_native_illum_no_specular.hpp"
#include "halo2/mcc_native_illum_three_channel.hpp"
#include "halo2/mcc_native_world_default_roles.hpp"
#include "halo2/mcc_native_character_shared.hpp"
#include "halo2/mcc_native_library.hpp"
#include "halo2/mcc_prepared_map_owner.hpp"
#include "halo2/mcc_hud_shader_reference.hpp"
#include <cmath>
#include "tags/directory.hpp"
#include "tags/native_directory.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace halo2 {
namespace {
using Json=nlohmann::json;
constexpr std::size_t maximum=128u*1024u*1024u;
std::vector<std::byte> read(const std::filesystem::path& path,std::size_t limit) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    if(!stream || stream.tellg()<0 || std::uint64_t(stream.tellg())>limit)throw FormatError("Invalid preserved native shader file");
    std::vector<std::byte> result(static_cast<std::size_t>(stream.tellg()));stream.seekg(0);
    if(!result.empty() && !stream.read(reinterpret_cast<char*>(result.data()),std::streamsize(result.size())))
        throw FormatError("Truncated preserved native shader file");
    return result;
}
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
std::uint64_t fnv(std::span<const std::byte> bytes) {
    std::uint64_t result=14695981039346656037ull;
    for(auto b:bytes)result=(result^std::to_integer<unsigned>(b))*1099511628211ull;
    return result;
}
std::shared_ptr<const ShaderFunctionTables> function_tables(std::span<const std::byte> tables) {
    if(tables.size()!=28*1024 || fnv(tables)!=0x22708b4cc478ddb7ull)
        throw FormatError("Unsupported preserved shader function tables");
    auto functions=std::make_shared<ShaderFunctionTables>();
    for(unsigned i=0;i<28;++i)for(unsigned j=0;j<1024;++j)
        (i<12?functions->periodic[i][j]:functions->transition[i-12][j])=std::to_integer<std::uint8_t>(tables[i*1024+j]);
    return functions;
}
struct CloneNode {unsigned first,count;};
struct CloneField {unsigned offset,stride,child;bool data;};
struct NativeTemplateVariant {
    std::string_view name,payload;std::size_t bytes;std::uint64_t fingerprint;
};
#include "rendering/mcc_native_template_variants.inc"
// v1143 independently re-exported all34 template/pass/program records from
// the original introduction map, with English TU1.5 as the offline authority.
// Admit this exact resident variant; other env_illum variants stay unproved.
constexpr NativeTemplateVariant intro_env_illum_variant{
    "shaders\\shader_templates\\opaque\\tex_bump_env_illum",
    "78952d72898712a11c855e4b7ca2d6d5851893f579dbce3e0e320172ef3692da.bin",
    1878,0xb6237bd9fa8bb23eull};
// Paired original Elongation/MCC cooked-material audit. The retained NV2A
// template/pass closure owns transparency; the renamed MCC shader is not a
// material-name alias. Only its complete audited authored tuple is admitted.
constexpr NativeTemplateVariant two_alpha_env_illum_variant{
    "shaders\\shader_templates\\transparent\\two_alpha_env_illum",
    "4072549c86ac5fd6d3b52326d5d9677141c149a30a5c15c3ad740d1c95393b58.bin",
    312,0x3eaa28e88faf323aull};
#include "assets/mcc/mcc_globals_clone_layout.inc"
template<class Read>
std::vector<std::byte> clone_globals(std::span<const std::byte> original,std::uint32_t source,
                                    std::uint32_t target,const Read& read) {
    if(original.size()<0x284 || original.size()>maximum || std::uint64_t(source)+original.size()>0x100000000ull)
        throw FormatError("Invalid MCC shader globals allocation");
    std::vector<std::byte> out(original.begin(),original.end());
    std::set<std::tuple<unsigned,std::uint32_t,std::size_t>> visited;
    std::map<std::tuple<std::uint32_t,unsigned,unsigned>,std::size_t> copied;
    auto walk=[&](auto&& self,unsigned node,std::uint32_t address,std::size_t output,unsigned depth)->void {
        if(depth>32 || visited.size()>1000000)throw FormatError("Excessive shader globals pointer graph");
        if(!visited.emplace(node,address,output).second)return;
        const auto layout=clone_nodes.at(node);
        for(unsigned i=0;i<layout.count;++i) {
            const auto field=clone_fields.at(layout.first+i);
            if(std::uint64_t(address)+field.offset+8>0x100000000ull)throw FormatError("Shader globals descriptor overflow");
            const Bytes descriptor(read(address+field.offset,8));
            const auto count=descriptor.u32(0),pointer=descriptor.u32(4);
            if(!count){word(out,output+field.offset+4,0);continue;}
            const auto bytes=std::size_t(count)*field.stride;
            if(bytes>maximum || (!field.data && count>65536) || std::uint64_t(pointer)+bytes>0x100000000ull)
                throw FormatError("Excessive shader globals child bank");
            std::size_t offset;
            if(pointer>=source && std::uint64_t(pointer)-source<=original.size() && bytes<=original.size()-(pointer-source))
                offset=pointer-source;
            else {
                const auto key=std::tuple(pointer,count,field.stride);
                auto it=copied.find(key);
                if(it==copied.end()) {
                    offset=(out.size()+15)&~std::size_t(15);
                    if(offset>maximum || bytes>maximum-offset)throw FormatError("Excessive shader globals allocation");
                    const auto bank=read(pointer,bytes);out.resize(offset);out.insert(out.end(),bank.begin(),bank.end());
                    it=copied.emplace(key,offset).first;
                }
                offset=it->second;
            }
            if(std::uint64_t(target)+offset>0xffffffffu)throw FormatError("Shader globals relocation overflow");
            word(out,output+field.offset+4,target+std::uint32_t(offset));
            if(!field.data)for(unsigned n=0;n<count;++n)self(self,field.child,pointer+n*field.stride,offset+std::size_t(n)*field.stride,depth+1);
        }
    };
    walk(walk,0,source,0,0);return out;
}
}

std::optional<MccOwnedShaderProjection> project_mcc_unanimated_tex_bump(
    const Tag& shader,const Tag& native_stem,std::string_view native_payload_name,
    std::span<const std::byte> native_payload,const MccShaderRead& read_source,
    const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame) {
    return project_mcc_unanimated_tex_bump(shader,native_stem,native_payload_name,
        native_payload,read_source,find,owns_frame,{});
}

namespace {
std::optional<MccOwnedShaderProjection> project_mcc_unanimated_template(
    const Tag& shader,const Tag& native_stem,std::string_view native_payload_name,
    std::span<const std::byte> native_payload,const MccShaderRead& read_source,
    const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame,
    const MccShaderDefaultDetail& detail,bool transparent) {
    constexpr std::array<std::uint16_t,8> ordinary_ranges{0x800,0x804,0xe08,0xa0f,0x214,0x815,0x819,0x81d};
    constexpr std::array<std::uint16_t,12> dprs_ranges{0x800,0x204,0x805,0xe09,0xa10,0x215,0x816,0x81a,0x21e,0xa1f,0x224,0x825};
    constexpr std::array<std::uint16_t,15> env_ranges{0x200,0x801,0x805,0xe09,0xa10,0x215,0x816,0x81a,0x81e,0xe22,0xa29,0x22e,0x82f,0x833,0x837};
    constexpr std::array<std::uint16_t,12> illum_ranges{0x800,0x204,0x805,0xe09,0xa10,0x215,0x816,0x21a,0x81b,0x21f,0x820,0x224};
    constexpr std::array<std::uint16_t,18> env_illum_ranges{
        0x200,0x801,0x205,0x806,0xe0a,0xa11,0x216,0x817,0x81b,
        0x21f,0x820,0xe24,0xa2b,0x230,0x831,0x835,0x239,0x83a};
    constexpr std::array<std::uint16_t,1> transparent_ranges{0x200};
    struct TemplateContract {
        std::string_view name;
        unsigned bitmaps,pixels,vectors,lods,layers,source_implementations,implementations,remaps;
        std::span<const std::uint16_t> pass_ranges;
        bool restore_detail=false;
        unsigned diffuse_slot=2,optional_bitmap_slot=1;
        unsigned shader_type=1;
        bool transparent_tuple=false;
    };
    const std::array<TemplateContract,8> profiles{{
        {native_template_names[0],
         5,2,9,2,50,33,32,116,ordinary_ranges},
        {native_template_names[1],
         5,2,9,2,50,33,32,116,ordinary_ranges},
        {native_template_names[2],
         6,4,13,2,50,41,40,168,dprs_ranges},
        {native_template_names[3],
         6,7,7,3,75,59,57,238,env_ranges},
        {native_template_names[4],
         5,6,6,2,50,33,32,142,ordinary_ranges,true},
        {native_template_names[5],
         6,4,12,2,50,37,36,126,illum_ranges,false,1,0xffffffffu},
        {intro_env_illum_variant.name,
         7,8,8,3,75,62,60,253,env_illum_ranges},
        {two_alpha_env_illum_variant.name,
         5,4,10,1,25,1,1,18,transparent_ranges,false,0,0xffffffffu,5,true}
    }};
    const auto profile=std::find_if(profiles.begin(),profiles.end(),[&](const auto& p) {
        return native_stem.name==p.name && p.transparent_tuple==transparent;
    });
    const auto found_variant=std::find_if(native_template_variants.begin(),native_template_variants.end(),[&](const auto& v) {
        return native_stem.name==v.name && native_payload_name==v.payload;
    });
    const NativeTemplateVariant* variant=found_variant==native_template_variants.end()?nullptr:&*found_variant;
    if(native_stem.name==intro_env_illum_variant.name && native_payload_name==intro_env_illum_variant.payload)
        variant=&intro_env_illum_variant;
    if(transparent && native_stem.name==two_alpha_env_illum_variant.name &&
       native_payload_name==two_alpha_env_illum_variant.payload)variant=&two_alpha_env_illum_variant;
    try {
        if(!read_source || !find || !owns_frame || shader.group!="shad" ||
           shader.datum==0xffffffffu || shader.name.empty() || !shader.address || (shader.size && shader.size<84) ||
           native_stem.group!="stem" || profile==profiles.end() || !variant ||
           native_stem.datum==0xffffffffu || !native_stem.address || native_stem.size!=variant->bytes ||
           native_payload.size()!=variant->bytes || fnv(native_payload)!=variant->fingerprint)return {};
        const auto same=[&](const Tag& t) {
            const auto* owned=find(t.datum);
            return owned && owned->datum==t.datum && owned->group==t.group &&
                owned->name==t.name && owned->address==t.address && owned->size==t.size;
        };
        if(!same(shader) || !same(native_stem))return {};
        const auto source=[&](std::uint32_t at,std::size_t size) {
            if(std::uint64_t(at)+size>0x100000000ull)throw FormatError("tex_bump source overflow");
            const auto bytes=read_source(at,size);
            if(bytes.size()!=size)throw FormatError("tex_bump source extent mismatch");
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };
        const auto root_data=source(shader.address,84);const Bytes root(root_data);
        if(root.u32(0)!=0x7374656du || root.u32(4)!=native_stem.datum ||
           root.u32(12)!=1 || root.u32(20)!=profile->shader_type || root.u32(24)!=0 ||
           root.u32(32)!=3 || root.u32(56)!=0xffffffffu || root.u32(76)!=0 ||
           root.u16(60)>6 || root.u16(62)>3 || root.u16(64)>3 ||
           !std::isfinite(root.f32(68)) || !std::isfinite(root.f32(72)))return {};
        const auto definitions=source(root.u32(36),3*124);
        if(std::any_of(definitions.begin()+124,definitions.end(),[](std::byte b){return b!=std::byte{};}))return {};
        const Bytes definition{std::span<const std::byte>(definitions).first(124)};
        if(definition.u32(0)!=native_stem.datum || definition.u32(116) || definition.u32(120))return {};
        const std::array<std::pair<unsigned,unsigned>,14> shape{{
            {4,profile->bitmaps},{12,profile->pixels},{20,profile->vectors},
            {28,profile->lods},{36,profile->layers},{44,unsigned(profile->pass_ranges.size())},
            {52,profile->source_implementations},
            {60,0},{68,0},{76,0},{84,0},{92,5},{100,2},{108,6}}};
        constexpr std::array<unsigned,14> strides{12,4,16,6,2,2,10,20,4,2,4,4,12,4};
        std::array<std::vector<std::byte>,14> banks;
        for(unsigned i=0;i<shape.size();++i) {
            const auto [field,count]=shape[i];
            if(definition.u32(field)!=count)return {};
            if(count)banks[i]=source(definition.u32(field+4),std::size_t(count)*strides[i]);
            else if(definition.u32(field+4))return {};
        }
        const Bytes native(native_payload);
        if(native.u32(88)!=1 || native.u32(92)!=96)return {};
        const auto native_bank=[&](unsigned field,unsigned count,unsigned stride) {
            if(native.u32(96+field)!=count)throw FormatError("native tex_bump contract changed");
            const auto bytes=native.slice(native.u32(100+field),std::size_t(count)*stride);
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        };
        const auto native_lods=native_bank(0,profile->lods,10);
        const auto native_layers=native_bank(8,profile->layers,2);
        const auto native_passes=native_bank(16,unsigned(profile->pass_ranges.size()),10);
        (void)native_bank(24,profile->implementations,6);(void)native_bank(32,profile->remaps,4);
        std::vector<std::byte> expected_lods(profile->lods*6),native_ranges(profile->pass_ranges.size()*2);
        for(unsigned i=0;i<profile->lods;++i) {
            const Bytes lod(native_lods);
            expected_lods[i*6+4]=std::byte(lod.u16(i*10));
            expected_lods[i*6+5]=std::byte(lod.u16(i*10)>>8);
        }
        const Bytes source_ranges(banks[5]),original_passes(native_passes);
        for(unsigned i=0;i<profile->pass_ranges.size();++i) {
            if(source_ranges.u16(i*2)!=profile->pass_ranges[i])return {};
            native_ranges[i*2]=std::byte(original_passes.u16(i*10+8));
            native_ranges[i*2+1]=std::byte(original_passes.u16(i*10+8)>>8);
        }
        if(banks[3]!=expected_lods || banks[4]!=native_layers ||
           std::any_of(banks[6].begin(),banks[6].end(),[](std::byte b){return b!=std::byte{};}))return {};
        const auto bitmap=[&](std::uint32_t handle,std::uint32_t frame,bool allow_none) {
            if(handle==0xffffffffu)return allow_none;
            const auto* t=find(handle);
            return t && t->datum==handle && t->group=="bitm" && t->address && t->size &&
                !t->name.empty() && owns_frame(handle,frame);
        };
        if(profile->restore_detail) {
            if(!detail.tag || !detail.frame0 || !same(*detail.tag) || detail.tag->group!="bitm" ||
               detail.tag->name!="shaders\\default_bitmaps\\bitmaps\\default_detail" ||
               detail.tag->size<76 || !bitmap(detail.tag->datum,0,false))return {};
            const auto& texture=*detail.frame0;
            if(texture.width!=4 || texture.height!=4 || texture.format!=11 || texture.layers!=1 || texture.depth ||
               texture.mips.size()!=3 || texture.pixels.size()!=84 || fnv(texture.pixels)!=0x9fadd11877449cb8ull)return {};
            constexpr std::array<unsigned,3> widths{4,2,1},offsets{0,64,80},sizes{64,16,4};
            for(unsigned i=0;i<3;++i) {
                const auto& mip=texture.mips[i];
                if(mip.width!=widths[i] || mip.height!=widths[i] || mip.depth!=1 ||
                   mip.offset!=offsets[i] || mip.size!=sizes[i])return {};
            }
        }
        const Bytes images(banks[0]),vectors(banks[2]),properties(banks[11]);
        for(unsigned i=0;i<profile->bitmaps;++i)
            if(!bitmap(images.u32(i*12),images.u32(i*12+4),i==profile->optional_bitmap_slot) ||
               !std::isfinite(images.f32(i*12+8)))return {};
        for(unsigned i=0;i<5;++i)
            if((properties.u16(i*4)!=0xffffu && properties.u16(i*4)>=profile->bitmaps) ||
               properties.u16(i*4+2)!=0xffffu)return {};
        for(unsigned at=0;at<banks[2].size();at+=4)if(!std::isfinite(vectors.f32(at)))return {};
        for(unsigned i:{12u,13u}) {
            const Bytes values(banks[i]);
            for(unsigned at=0;at<banks[i].size();at+=4)if(!std::isfinite(values.f32(at)))return {};
        }
        if(transparent) {
            // Named PC parameter tables were stripped from these caches. Do
            // not generalize their pixel ordering from a template/name match.
            // Pin the full paired authored tuple, retaining each actual donor
            // bitmap/frame/log word, vector, runtime word and auxiliary bank.
            auto normalized_root=root_data;
            for(unsigned at:{4u,8u,16u,36u,48u})word(normalized_root,at,0);
            if(fnv(normalized_root)!=0xb9859542742a8d8bull ||
               fnv(banks[1])!=0x6712cc415d173fb4ull ||
               fnv(banks[2])!=0xb4dee1cef01ff42dull ||
               fnv(banks[11])!=0x2d84c39da70df3f2ull ||
               fnv(banks[12])!=0x36ce2b4e9b089fc5ull ||
               fnv(banks[13])!=0x2db68b09fea79ad8ull)return {};
            constexpr std::array<std::string_view,5> names{
                "scenarios\\bitmaps\\reflection_maps\\reflection_ss_hallway_stainless",
                "shaders\\default_bitmaps\\bitmaps\\color_white",
                "scenarios\\bitmaps\\human\\military\\glass\\glass_smudged_grey",
                "shaders\\default_bitmaps\\bitmaps\\default_additive",
                "scenarios\\bitmaps\\human\\military\\glass\\glass_smudged"};
            auto normalized_images=banks[0];
            for(unsigned i=0;i<names.size();++i) {
                const auto* image=find(images.u32(i*12));
                if(!image || image->name!=names[i])return {};
                word(normalized_images,i*12,0);
            }
            if(fnv(normalized_images)!=0xef1330b936927f17ull)return {};
        }
        const auto runtime_data=source(root.u32(16),80);const Bytes runtime(runtime_data);
        for(unsigned at:{0u,8u,44u,52u})
            if(runtime.u32(at+4)!=0xffffffffu &&
               (runtime.u32(at)!=0x6269746du || !bitmap(runtime.u32(at+4),0,false)))return {};
        if(transparent) {
            if(runtime.u32(4)!=0xffffffffu || runtime.u32(48)!=0xffffffffu ||
               runtime.u32(12)!=images.u32(3*12) || runtime.u32(56)!=images.u32(4*12))return {};
            auto normalized_runtime=runtime_data;
            for(unsigned at:{4u,12u,48u,56u})word(normalized_runtime,at,0);
            if(fnv(normalized_runtime)!=0xf4000f44f9d55108ull)return {};
        }else if(runtime.u32(4)!=images.u32(profile->diffuse_slot*12))return {};
        const auto predicted_count=root.u32(44);
        if(predicted_count>64)return {};
        auto predicted=predicted_count?source(root.u32(48),predicted_count*8):std::vector<std::byte>{};
        const Bytes predictions(predicted);
        for(unsigned i=0;i<predicted_count;++i)
            if(predictions.u16(i*8)!=0 || !bitmap(predictions.u32(i*8+4),predictions.u16(i*8+2),false))return {};
        if(transparent) {
            if(predicted_count!=5)return {};
            for(unsigned i=0;i<5;++i)
                if(predictions.u32(i*8)!=0 || predictions.u32(i*8+4)!=images.u32(i*12))return {};
            // Exact original cooked order from the paired source data. Every
            // authored word survives; no alpha/color value is fabricated.
            const auto first=Bytes(banks[1]).u32(4),second=Bytes(banks[1]).u32(8);
            word(banks[1],4,second);word(banks[1],8,first);
        }
        if(profile->restore_detail) {
            const auto predictions_for=[&](const std::vector<std::byte>& records) {
                const Bytes images(records);std::vector<std::byte> result;
                std::set<std::pair<std::uint32_t,std::uint32_t>> seen;
                for(unsigned at=0;at<records.size();at+=12) {
                    const auto handle=images.u32(at),frame=images.u32(at+4);
                    if(handle==0xffffffffu)continue;
                    if(frame>0xffffu)throw FormatError("tex_bump prediction frame exceeds its original word");
                    if(!seen.emplace(handle,frame).second)continue;
                    const auto offset=result.size();result.resize(offset+8);
                    word(result,offset,frame<<16);word(result,offset+4,handle);
                }
                return result;
            };
            if(predicted!=predictions_for(banks[0]))return {};
            std::vector<std::byte> original_detail(12);
            word(original_detail,0,detail.tag->datum);word(original_detail,8,1064697328u);
            banks[0].insert(banks[0].begin()+4*12,original_detail.begin(),original_detail.end());
            const auto transform=banks[2].size();banks[2].resize(transform+16);
            word(banks[2],transform,0x3f800000u);word(banks[2],transform+4,0x3f800000u);
            for(unsigned i=0;i<5;++i)if(Bytes(banks[11]).u16(i*4)==4) {
                banks[11][i*4]=std::byte{5};banks[11][i*4+1]=std::byte{};
            }
            predicted=predictions_for(banks[0]);
        }
        banks[5]=native_ranges;banks[6].resize(profile->implementations*10);
        MccOwnedShaderProjection out{root_data,{}};
        const auto append=[&](unsigned field,std::span<const std::byte> bytes) {
            if(bytes.empty()){word(out.bytes,field,0);return;}
            const auto offset=(out.bytes.size()+15)&~std::size_t(15);
            out.bytes.resize(offset);out.bytes.insert(out.bytes.end(),bytes.begin(),bytes.end());
            word(out.bytes,field,std::uint32_t(offset));out.pointers.emplace_back(field,std::uint32_t(offset));
        };
        append(16,runtime_data);word(out.bytes,28,0);word(out.bytes,32,1);
        append(36,std::span(definitions).first(124));const auto pp=Bytes(out.bytes).u32(36);
        word(out.bytes,44,std::uint32_t(predicted.size()/8));append(48,predicted);word(out.bytes,80,0);
        for(unsigned i=0;i<shape.size();++i) {
            const auto field=pp+shape[i].first;
            const auto count=i==6?profile->implementations:
                shape[i].second+(profile->restore_detail && (i==0 || i==2)?1u:0u);
            word(out.bytes,field,count);append(field+4,banks[i]);
        }
        return out;
    } catch(const FormatError&) {return {};}
}
}

std::optional<MccOwnedShaderProjection> project_mcc_unanimated_tex_bump(
    const Tag& shader,const Tag& native_stem,std::string_view native_payload_name,
    std::span<const std::byte> native_payload,const MccShaderRead& read_source,
    const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame,
    const MccShaderDefaultDetail& detail) {
    return project_mcc_unanimated_template(shader,native_stem,native_payload_name,
        native_payload,read_source,find,owns_frame,detail,false);
}

std::optional<MccOwnedShaderProjection> project_mcc_transparent_two_alpha_env_illum(
    const Tag& shader,const Tag& native_stem,std::string_view native_payload_name,
    std::span<const std::byte> native_payload,const MccShaderRead& read_source,
    const MccShaderFind& find,const MccShaderBitmapFrame& owns_frame) {
    return project_mcc_unanimated_template(shader,native_stem,native_payload_name,
        native_payload,read_source,find,owns_frame,{},true);
}

std::optional<MccOwnedShaderProjection> project_mcc_native_default_vector_cube(
    const Tag& missing,const Tag& donor,const Texture& frame3,const MccShaderRead& source) {
    // Independently paired Gasgiant, Floodlab and Colossus original/MCC image
    // owners. Frame3 is the complete original blue vector cube; the other
    // three donor frames contain different black/white/gray images.
    if(missing.group!="bitm" || missing.name!="shaders\\default_bitmaps\\bitmaps\\default_vector_cube_map" ||
       missing.datum==0xffffffffu || missing.address || missing.size ||
       donor.group!="bitm" || donor.name!="rasterizer\\default_cube_map" ||
       donor.datum==0xffffffffu || donor.datum==missing.datum || !donor.address || donor.size!=992 ||
       std::uint64_t(donor.address)+donor.size>0x100000000ull)return {};
    if(frame3.width!=4 || frame3.height!=4 || frame3.depth || frame3.layers!=6 || frame3.format!=11 ||
       frame3.pixels.size()!=504 || frame3.mips.size()!=18)return {};
    constexpr std::array<unsigned,3> widths{4,2,1},offsets{0,64,80},sizes{64,16,4};
    for(unsigned face=0;face<6;++face)for(unsigned level=0;level<3;++level) {
        const auto& mip=frame3.mips[face*3+level];
        if(mip.width!=widths[level] || mip.height!=widths[level] || mip.depth!=1 ||
           mip.offset!=face*84+offsets[level] || mip.size!=sizes[level])return {};
    }
    constexpr std::array<unsigned,4> rgba{128,128,255,255};
    for(unsigned i=0;i<frame3.pixels.size();++i)
        if(std::to_integer<unsigned>(frame3.pixels[i])!=rgba[i%4])return {};
    try {
        const auto header=source(donor.address,76);
        const Bytes root(header);
        if(root.u32(64)!=donor.address+80 || root.u32(72)!=donor.address+320)return {};
        std::vector<std::byte> normalized_root(header.begin(),header.end());
        word(normalized_root,64,0);word(normalized_root,72,0);
        if(fnv(normalized_root)!=0x9ef101dcc7825d49ull)return {};
        const auto descriptor=source(root.u32(72)+3*168,168);
        const Bytes record(descriptor);
        if(record.u32(28)!=donor.datum || !record.u32(80))return {};
        std::vector<std::byte> normalized_record(descriptor.begin(),descriptor.end());
        word(normalized_record,28,0);
        if(fnv(normalized_record)!=0x835c5441aa3e10c0ull)return {};
        MccOwnedShaderProjection out;
        out.bytes.assign(header.begin(),header.end());
        // Original identity has no bitmap sequence and a single frame. Keep
        // every actual MCC frame descriptor/resource word, including +24's
        // authored within-bitmap offset; only its owning datum changes.
        word(out.bytes,60,0);word(out.bytes,64,0);
        word(out.bytes,68,1);word(out.bytes,72,76);
        out.bytes.insert(out.bytes.end(),descriptor.begin(),descriptor.end());
        word(out.bytes,76+28,missing.datum);
        out.pointers.emplace_back(72,76);
        return out;
    }catch(const FormatError&){return {};}
}

void Cache::initialize_mcc_native_shaders(const MccPreparedMapOwner* owner) {
    if(!owner)return;
    const auto root=owner->cache_root/"native-shaders";
    const auto snapshot=[&](const std::filesystem::path& path)->const std::string& {
        const auto key=path.lexically_relative(owner->cache_root).generic_string();
        const auto found=owner->metadata.find(key);
        if(found==owner->metadata.end())throw FormatError("Native shader metadata lacks its admitted snapshot: "+key);
        return found->second;
    };
    const auto table_snapshot=[&](const std::filesystem::path& path) {
        const auto& raw=snapshot(path);
        return function_tables(std::as_bytes(std::span(raw.data(),raw.size())));
    };
    const MccNativeLibraryRead library_snapshot=[&](const std::filesystem::path& path) {return snapshot(path);};
    // Scalar function evaluation is also needed by PC-exclusive/mod maps which
    // have no retained Xbox material counterpart. This never admits MCC D3D
    // material programs as native passes; those gaps remain explicit.
    if(owner->metadata.contains("native-shaders/function-tables.bin"))
        function_tables_=table_snapshot(root/"function-tables.bin");
    if(!owner->binding || owner->binding->mode==MccNativeBankMode::None)return;
    const auto& binding=*owner->binding;
    const bool hud_only=binding.mode==MccNativeBankMode::AuthenticatedHudBootstrap;
    const auto directory=hud_only?binding.bootstrap_dependency:binding.bank_directory;
    const auto owned_tables=table_snapshot(directory/"function-tables.bin");
    Json bank;
    std::map<std::string,std::filesystem::path> supplemental_payloads;
    bool additional_world_programs=false;
    std::map<std::pair<std::string,std::string>,std::string> vehicle_semantics;
    std::map<std::string,std::filesystem::path> vehicle_payloads;
    if(hud_only) {
        supplemental_payloads=bootstrap_mcc_native_hud_library(bank,path_,tags_,
            [&](std::uint32_t address,std::size_t size) {
                return Bytes(storage_).slice(metadata_offset(address,size),size);
            },[&](std::uint32_t id){return string_id(id);},root,
            binding.input_bytes,binding.input_sha256,library_snapshot);
        if(supplemental_payloads.empty())return;
        const auto pc_sky=supplement_mcc_native_pc_sky_library(bank,path_,tags_,
            [&](std::uint32_t address,std::size_t size) {
                return Bytes(storage_).slice(metadata_offset(address,size),size);
            },root,binding.input_bytes,binding.input_sha256,library_snapshot);
        supplemental_payloads.insert(pc_sky.begin(),pc_sky.end());
        if(!pc_sky.empty()) {
            const auto pc_world=supplement_mcc_native_pc_world_library(bank,path_,root,
                binding.input_bytes,binding.input_sha256,library_snapshot);
            supplemental_payloads.insert(pc_world.begin(),pc_world.end());
            if(!pc_world.empty()) {
                const auto programs=supplement_mcc_native_pc_world_programs(bank,path_,root,
                    binding.input_bytes,binding.input_sha256,library_snapshot);
                supplemental_payloads.insert(programs.begin(),programs.end());
                additional_world_programs=!programs.empty();
                if(additional_world_programs) {
                    auto vehicles=supplement_mcc_native_vehicle_library(bank,path_,tags_,
                        [&](std::uint32_t address,std::size_t size) {
                            return Bytes(storage_).slice(metadata_offset(address,size),size);
                        },[&](std::uint32_t handle,std::uint32_t frame){return frame<bitmap_count(handle);},
                        Bytes(storage_).u32(48),[&](std::uint32_t id){return string_id(id);},
                        root,binding.input_bytes,binding.input_sha256,library_snapshot);
                    supplemental_payloads.insert(vehicles.payloads.begin(),vehicles.payloads.end());
                    vehicle_payloads=vehicles.payloads;
                    vehicle_semantics=std::move(vehicles.semantic_names);
                }
            }
        }
        function_tables_=owned_tables;
    } else {
        bank=Json::parse(snapshot(directory/"library.json"));
    }
    if(bank.at("schema")!="H2_PRESERVED_NATIVE_SHADERS_V1" ||
       bank.at("map")!=(hud_only?name_:binding.library_namespace))
        throw FormatError("Preserved shader library identity mismatch");
    if(!hud_only)supplemental_payloads=supplement_mcc_native_library(bank,root,tags_,library_snapshot);
    const auto payload_path=[&](const std::string& filename) {
        const auto found=supplemental_payloads.find(filename);
        return found==supplemental_payloads.end()?directory/"payloads"/filename:found->second;
    };
    const std::set<std::string> shader_groups{"shad","stem","spas","vrtx"};
    const auto& records=bank.at("shader_tags");
    if(!records.is_array() || records.size()>65536)throw FormatError("Excessive preserved shader directory");
    std::map<std::uint32_t,Tag> source_world_templates;
    if(additional_world_programs)for(const auto& tag:tags_)
        if(tag.group=="stem"&&tag.address&&tag.size)source_world_templates.emplace(tag.datum,tag);
    std::map<std::pair<std::string,std::string>,std::uint32_t> identities;
    for(const auto& t:tags_)if(t.datum!=0xffffffffu)
        if(!identities.emplace(std::pair(t.group,t.name),t.datum).second)throw FormatError("Duplicate MCC tag identity for native shader binding");
    auto bind=[&](const std::string& group,const std::string& name) {
        if(auto found=identities.find({group,name});found!=identities.end())return found->second;
        if(!shader_groups.contains(group) && group!="bitm")throw FormatError("Preserved shader references a non-shader asset group");
        if(!tag_group(TagGroup::from_string(group)))throw FormatError("Native shader tag directory group failure");
        const auto handle=detail::insert_native_tag(tags_,group,name).value;
        identities.emplace(std::pair(group,name),handle);return handle;
    };
    std::set<std::uint32_t> restored;
    std::set<std::uint32_t> referenced_shaders;
    for(const auto& record:records) {
        const auto group=record.at("group").get<std::string>(),name=record.at("name").get<std::string>();
        if(!shader_groups.contains(group) || !restored.insert(bind(group,name)).second)
            throw FormatError("Invalid/duplicate preserved shader definition");
        for(const auto& ref:record.at("references")) {
            const auto group=ref.at(1).at(0).get<std::string>();
            const auto handle=bind(group,ref.at(1).at(1));
            if(shader_groups.contains(group))referenced_shaders.insert(handle);
        }
    }
    if(!hud_only)for(const auto& ref:bank.at("rasterizer").at("references")) {
        const auto group=ref.at(1).at(0).get<std::string>();
        const auto handle=bind(group,ref.at(1).at(1));
        if(shader_groups.contains(group))referenced_shaders.insert(handle);
    }
    // A nonresident original shader reference stays unresolved. Never fill
    // that hole with a same-named MCC D3D program and parse it as NV2A.
    for(auto handle:referenced_shaders)if(!restored.contains(handle)) {
        auto& t=tags_[handle&0xffffu];
        if(t.address&&t.size)retained_tag_allocations_.push_back(t);
        t.address=0;t.size=0;
    }
    // Append missing shader parameter names to the owned native StringID bank;
    // existing gameplay IDs keep their exact indices and lengths.
    std::vector<std::uint32_t> offsets;std::vector<std::byte> strings;
    std::map<std::string,std::uint32_t> string_ids;
    {
        const Bytes source(storage_);const auto count=source.u32(48),bytes=source.u32(56),table=source.u32(60),text=source.u32(52);
        if(count>65536)throw FormatError("Excessive MCC string bank");
        const auto original=source.slice(text,bytes);strings.assign(original.begin(),original.end());
        for(unsigned i=0;i<count;++i) {
            offsets.push_back(source.u32(std::size_t(table)+i*4));
            if(offsets.back()==0xffffffffu)continue;
            const auto value=source.string(std::size_t(text)+offsets.back(),bytes-offsets.back());
            string_ids.emplace(value,(std::uint32_t(value.size())<<24)|i);
        }
    }
    auto bind_string=[&](const std::string& value) {
        if(auto found=string_ids.find(value);found!=string_ids.end())return found->second;
        if(value.size()>255 || offsets.size()>=65536)throw FormatError("Preserved shader StringID capacity failure");
        const auto id=(std::uint32_t(value.size())<<24)|std::uint32_t(offsets.size());
        offsets.push_back(std::uint32_t(strings.size()));
        for(char c:value)strings.push_back(std::byte(static_cast<unsigned char>(c)));
        strings.push_back(std::byte{});
        string_ids.emplace(value,id);return id;
    };
    auto prepare=[&](const Json& record,std::uint64_t va) {
        const auto filename=record.at("payload").get<std::string>();
        if(filename.size()!=68 || filename.substr(64)!=".bin" || filename.find_first_not_of("0123456789abcdef")<64)
            throw FormatError("Invalid preserved shader payload filename");
        std::vector<std::byte> payload;
        if(const auto owned=vehicle_payloads.find(filename);owned!=vehicle_payloads.end()) {
            const auto& raw=snapshot(owned->second);
            if(raw.size()>16u*1024u*1024u)throw FormatError("Excessive snapshotted native vehicle payload");
            const auto bytes=std::as_bytes(std::span(raw.data(),raw.size()));
            payload.assign(bytes.begin(),bytes.end());
        } else payload=read(payload_path(filename),16u*1024u*1024u);
        if(payload.size()!=record.at("bytes").get<std::size_t>() ||
           fnv(payload)!=std::stoull(record.at("fnv64").get<std::string>(),nullptr,16))
            throw FormatError("Preserved shader payload fingerprint mismatch");
        const auto at=std::size_t(std::int64_t(va)-metadata_magic_);
        if(va+payload.size()>0x100000000ull || at+payload.size()>1024ull*1024*1024 ||
           (!vista_metadata_regions_.empty() && va+payload.size()-vista_metadata_regions_.front().first>maximum))
            throw FormatError("Excessive preserved shader allocation");
        std::set<std::size_t> fixups;
        for(const auto& fixup:record.at("pointers")) {
            const auto location=fixup.at(0).get<std::size_t>(),offset=fixup.at(1).get<std::size_t>();
            if(offset>=payload.size() || !fixups.insert(location).second)throw FormatError("Invalid native shader pointer relocation");
            word(payload,location,std::uint32_t(va+offset));
        }
        for(const auto& fixup:record.at("references")) {
            const auto location=fixup.at(0).get<std::size_t>();
            if(!fixups.insert(location).second)throw FormatError("Overlapping native shader fixup");
            word(payload,location,bind(fixup.at(1).at(0),fixup.at(1).at(1)));
        }
        for(const auto& fixup:record.at("strings")) {
            const auto location=fixup.at(0).get<std::size_t>();
            if(!fixups.insert(location).second)throw FormatError("Overlapping native shader string fixup");
            word(payload,location,bind_string(fixup.at(1)));
        }
        return payload;
    };
    auto next_address=[&] {return (std::uint64_t(std::int64_t(storage_.size())+metadata_magic_)+15)&~std::uint64_t(15);};
    if(!hud_only) {
        // bind() creates this unresolved owner only when the admitted native
        // library actually references it. An existing resident image wins.
        const auto identity=identities.find({"bitm","shaders\\default_bitmaps\\bitmaps\\default_vector_cube_map"});
        if(identity!=identities.end()) {
            const Tag missing=tags_.at(identity->second&0xffffu);
            const auto* donor=query_tag({.group=tag_groups::bitmap,.name="rasterizer\\default_cube_map",.require_address=true});
            if(!missing.address && !missing.size && donor) {
                const Tag source=*donor;
                std::optional<MccOwnedShaderProjection> projected;
                try {
                    const auto frame=load_bitmap(source.datum,3);
                    projected=project_mcc_native_default_vector_cube(missing,source,frame,
                        [&](auto address,auto size){return read_tag_bytes(address,size);});
                }catch(const FormatError&){} // Unproved data retains its explicit missing-image gap.
                if(projected) {
                    const auto va=next_address();
                    if(va+projected->bytes.size()>0x100000000ull)
                        throw FormatError("Native default bitmap allocation overflow");
                    for(const auto& [field,target]:projected->pointers)
                        word(projected->bytes,field,std::uint32_t(va+target));
                    storage_.resize(std::size_t(std::int64_t(va)-metadata_magic_));
                    storage_.insert(storage_.end(),projected->bytes.begin(),projected->bytes.end());
                    auto& owned=tags_.at(missing.datum&0xffffu);
                    if(owned.datum!=missing.datum)throw FormatError("Native default bitmap full datum changed");
                    owned.address=std::uint32_t(va);owned.size=std::uint32_t(projected->bytes.size());
                    preserved_native_pointer_maps_.push_back({{owned.datum},owned.address,owned.size,std::move(projected->pointers)});
                    std::vector<std::pair<std::uint32_t,std::uint32_t>> retained_regions;
                    for(const auto& allocation:retained_tag_allocations_)
                        retained_regions.emplace_back(allocation.address,allocation.size);
                    vista_metadata_regions_=tag_metadata_regions(tags_,retained_regions);
                }
            }
        }
    }
    auto append=[&](const Json& record) {
        const auto va=next_address();const auto payload=prepare(record,va);
        storage_.resize(std::size_t(std::int64_t(va)-metadata_magic_));storage_.insert(storage_.end(),payload.begin(),payload.end());
        return std::pair(std::uint32_t(va),std::uint32_t(payload.size()));
    };
    const auto retain_pointer_map=[&](const Json& record,std::uint32_t owner,
                                     std::uint32_t address,std::uint32_t size) {
        PreservedNativePointerMap map{{owner},address,size,{}};
        for(const auto& pointer:record.at("pointers")) {
            const auto field=pointer.at(0).get<std::uint32_t>(),target=pointer.at(1).get<std::uint32_t>();
            if(field>size || 4>size-field || target>=size)
                throw FormatError("Preserved native pointer map escapes its verified allocation");
            map.pointers.emplace_back(field,target);
        }
        preserved_native_pointer_maps_.push_back(std::move(map));
    };
    for(const auto& record:records) {
        const auto handle=bind(record.at("group"),record.at("name"));
        const Tag original=tags_.at(handle&0xffffu);
        const auto [address,size]=append(record);auto& t=tags_.at(handle&0xffffu);t.address=address;t.size=size;
        if(original.address&&original.size)retained_tag_allocations_.push_back(original);
        retain_pointer_map(record,handle,address,size);
    }
    // Uplift's complete NoAlpha PC setup differs from ordinary lightmapping
    // only in its authored impl0 RGB write mask. Qualify every original setup
    // and material bank before publishing independent native metadata owners.
    // VS/PS instructions remain the authenticated existing TU programs.
    {
        const auto* no_alpha_stem=query_tag({.group=TagGroup{"stem"},
            .name="shaders\\shader_templates\\opaque\\tex_bump_no_alpha",.require_address=true});
        const auto* no_alpha_pass=query_tag({.group=TagGroup{"spas"},
            .name="shaders\\shader_passes\\lightmap\\lightmap_tex_detail_no_alpha",.require_address=true});
        const auto* normal_stem=query_tag({.group=TagGroup{"stem"},
            .name="shaders\\shader_templates\\opaque\\tex_bump",.require_address=true});
        const auto* normal_pass=query_tag({.group=TagGroup{"spas"},
            .name="shaders\\shader_passes\\lightmap\\lightmap_tex_detail",.require_address=true});
        const Json* stem_record=nullptr;const Json* pass_record=nullptr;
        for(const auto& record:records) {
            if(record.at("group")=="stem"&&record.at("name")=="shaders\\shader_templates\\opaque\\tex_bump")stem_record=&record;
            if(record.at("group")=="spas"&&record.at("name")=="shaders\\shader_passes\\lightmap\\lightmap_tex_detail")pass_record=&record;
        }
        if(no_alpha_stem&&no_alpha_pass&&normal_stem&&normal_pass&&stem_record&&pass_record) {
            const Tag source_stem=*no_alpha_stem,source_pass=*no_alpha_pass,base_stem=*normal_stem,base_pass=*normal_pass;
            const auto stem_payload=read(payload_path(stem_record->at("payload")),16u*1024u*1024u);
            const auto pass_payload=read(payload_path(pass_record->at("payload")),16u*1024u*1024u);
            const MccShaderRead source_read=[&](auto address,auto size) {
                return Bytes(storage_).slice(metadata_offset(address,size),size);
            };
            const MccShaderFind source_find=[&](auto handle)->const Tag* {return find_tag({handle});};
            const MccShaderNamedFind source_named=[&](std::string_view group,std::string_view name)->const Tag* {
                return query_tag({.group=TagGroup::from_string(group),.name=std::string(name),.require_address=true});
            };
            auto owners=project_mcc_no_alpha_native_owners(source_stem,source_pass,base_stem,base_pass,
                stem_payload,pass_payload,source_read,source_find,source_named);
            if(owners) {
                std::vector<std::pair<Tag,MccOwnedShaderProjection>> pending;
                const auto candidates=tags_;
                for(const auto& original:candidates) {
                    if(original.group!="shad"||!original.address||restored.contains(original.datum))continue;
                    auto projected=project_mcc_no_alpha_material(original,source_stem,source_pass,base_stem,stem_payload,
                        source_read,source_find,[&](auto handle,auto frame){return frame<bitmap_count(handle);},
                        [&](auto id){return string_id(id);});
                    if(projected)pending.emplace_back(original,std::move(*projected));
                }
                if(!pending.empty()) {
                    const auto publish=[&](const Tag& original,MccOwnedShaderProjection&& projected) {
                        const auto address=next_address();
                        if(address+projected.bytes.size()>0x100000000ull)throw FormatError("NoAlpha native allocation overflow");
                        for(const auto& [field,target]:projected.pointers)word(projected.bytes,field,std::uint32_t(address+target));
                        storage_.resize(std::size_t(std::int64_t(address)-metadata_magic_));
                        storage_.insert(storage_.end(),projected.bytes.begin(),projected.bytes.end());
                        auto& owned=tags_.at(original.datum&0xffffu);
                        if(owned.datum!=original.datum)throw FormatError("NoAlpha source owner salt changed");
                        owned.address=std::uint32_t(address);owned.size=std::uint32_t(projected.bytes.size());
                        if(original.size)retained_tag_allocations_.push_back(original);
                        preserved_native_pointer_maps_.push_back({{owned.datum},owned.address,owned.size,std::move(projected.pointers)});
                        restored.insert(owned.datum);
                    };
                    publish(source_pass,std::move(owners->pass));publish(source_stem,std::move(owners->stem));
                    for(auto& [original,projection]:pending)publish(original,std::move(projection));
                }
            }
        }
    }
    // Unmatched donor materials can reuse this exact retained template only
    // after their data has been projected into its original cooked contract.
    // Every child bank is copied; donor banks may be shared by other tags.
    for(const auto& record:records) {
        const auto template_name=record.at("name").get<std::string>();
        if(record.at("group")!="stem" ||
           (template_name!="shaders\\shader_templates\\opaque\\tex_bump" &&
            template_name!="shaders\\shader_templates\\opaque\\tex_bump_active_camo" &&
            template_name!="shaders\\shader_templates\\opaque\\tex_bump_dprs_env" &&
            template_name!="shaders\\shader_templates\\opaque\\tex_bump_env" &&
            template_name!="shaders\\shader_templates\\opaque\\tex_bump_env_no_detail" &&
            template_name!="shaders\\shader_templates\\opaque\\tex_bump_illum_bloom" &&
            template_name!=intro_env_illum_variant.name &&
            template_name!=two_alpha_env_illum_variant.name))continue;
        const auto* stem=query_tag({.group=TagGroup::from_string("stem"),.name=template_name,.require_address=true});
        if(!stem)continue;
        const Tag native_stem=*stem;
        const auto profile_name=record.at("payload").get<std::string>();
        const auto profile=read(payload_path(profile_name),16u*1024u*1024u);
        std::optional<Tag> detail_owner;std::optional<Texture> detail_frame;
        if(template_name=="shaders\\shader_templates\\opaque\\tex_bump_env_no_detail") {
            bool ambiguous=false;
            for(const auto& tag:select_tags({.group=TagGroup::from_string("bitm"),
                .name="shaders\\default_bitmaps\\bitmaps\\default_detail",.require_address=true,.minimum_size=76})) {
                if(detail_owner){ambiguous=true;break;}detail_owner=tag;
            }
            if(ambiguous || !detail_owner)continue;
            try{detail_frame=load_bitmap(detail_owner->datum,0);}catch(const FormatError&){continue;}
        }
        const auto candidates=tags_;
        for(const auto& original:candidates) {
            if(original.group!="shad" || !original.address || restored.contains(original.datum))continue;
            const MccShaderRead source_read=[&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                };
            const MccShaderFind source_find=[&](std::uint32_t handle)->const Tag* {return find_tag({handle});};
            const MccShaderBitmapFrame source_frame=[&](std::uint32_t handle,std::uint32_t frame) {return frame<bitmap_count(handle);};
            auto projected=template_name==two_alpha_env_illum_variant.name?
                project_mcc_transparent_two_alpha_env_illum(original,native_stem,profile_name,profile,
                    source_read,source_find,source_frame):
                project_mcc_unanimated_tex_bump(original,native_stem,profile_name,profile,
                    source_read,source_find,source_frame,
                    {detail_owner?&*detail_owner:nullptr,detail_frame?&*detail_frame:nullptr});
            if(!projected)continue;
            const auto va=next_address();
            if(va+projected->bytes.size()>0x100000000ull)
                throw FormatError("Canonical tex_bump shader allocation overflow");
            for(const auto& [field,target]:projected->pointers)
                word(projected->bytes,field,std::uint32_t(va+target));
            storage_.resize(std::size_t(std::int64_t(va)-metadata_magic_));
            storage_.insert(storage_.end(),projected->bytes.begin(),projected->bytes.end());
            auto& owned=tags_.at(original.datum&0xffffu);
            if(owned.datum!=original.datum)throw FormatError("Canonical tex_bump full datum changed");
            owned.address=std::uint32_t(va);owned.size=std::uint32_t(projected->bytes.size());
            if(original.size)retained_tag_allocations_.push_back(original);
            preserved_native_pointer_maps_.push_back({{owned.datum},owned.address,owned.size,std::move(projected->pointers)});
            restored.insert(owned.datum);
        }
    }
    // Additional PC-only world data must match the complete paired MCC
    // compiler remap graph before reaching an authenticated native template.
    if(additional_world_programs)for(const auto& record:records) {
        if(record.at("group")!="stem")continue;
        const auto* current=query_tag({.group=TagGroup{"stem"},
            .name=record.at("name").get<std::string>(),.require_address=true});
        if(!current)continue;
        const auto source=source_world_templates.find(current->datum);
        if(source==source_world_templates.end())continue;
        const Tag native_stem=*current;
        const auto filename=record.at("payload").get<std::string>();
        const auto payload=read(payload_path(filename),16u*1024u*1024u);
        const auto candidates=tags_;
        for(const auto& original:candidates) {
            if(original.group!="shad"||!original.address||restored.contains(original.datum))continue;
            auto projected=project_mcc_unanimated_world_cooked(original,source->second,native_stem,filename,payload,
                [&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                },[&](std::uint32_t handle)->const Tag* {return find_tag({handle});},
                [&](std::uint32_t handle,std::uint32_t frame) {return frame<bitmap_count(handle);});
            if(!projected)projected=project_mcc_character_shared(original,source->second,native_stem,filename,payload,
                [&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                },[&](std::uint32_t handle)->const Tag* {return find_tag({handle});},
                [&](std::uint32_t handle,std::uint32_t frame) {return frame<bitmap_count(handle);},
                [&](std::uint32_t id){return string_id(id);});
            if(!projected)projected=project_mcc_animated_clock_world(original,source->second,native_stem,filename,payload,
                [&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                },[&](std::uint32_t handle)->const Tag* {return find_tag({handle});},
                [&](std::uint32_t handle,std::uint32_t frame) {return frame<bitmap_count(handle);});
            if(!projected)projected=project_mcc_clock_illum_no_specular(original,source->second,native_stem,filename,payload,
                [&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                },[&](std::uint32_t handle)->const Tag* {return find_tag({handle});},
                [&](std::uint32_t handle,std::uint32_t frame) {return frame<bitmap_count(handle);});
            if(!projected)projected=project_mcc_clock_illum_three_channel(original,source->second,native_stem,filename,payload,
                [&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                },[&](std::uint32_t handle)->const Tag* {return find_tag({handle});},
                [&](std::uint32_t handle,std::uint32_t frame) {return frame<bitmap_count(handle);});
            if(!projected)projected=project_mcc_world_default_roles(original,source->second,native_stem,filename,payload,
                [&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                },[&](std::uint32_t handle)->const Tag* {return find_tag({handle});},
                [&](std::uint32_t handle,std::uint32_t frame) {return frame<bitmap_count(handle);},
                [&](std::string_view name)->std::optional<MccWorldDefaultBitmap> {
                    const auto* bitmap=query_tag({.group=tag_groups::bitmap,.name=std::string(name),.require_address=true});
                    if(!bitmap)return {};
                    return MccWorldDefaultBitmap{*bitmap,load_bitmap(bitmap->datum,0)};
                });
            if(!projected)continue;
            const auto address=next_address();
            if(address+projected->bytes.size()>0x100000000ull)
                throw FormatError("Native world cooked allocation overflow");
            for(const auto& [field,target]:projected->pointers)
                word(projected->bytes,field,std::uint32_t(address+target));
            storage_.resize(std::size_t(std::int64_t(address)-metadata_magic_));
            storage_.insert(storage_.end(),projected->bytes.begin(),projected->bytes.end());
            auto& owned=tags_.at(original.datum&0xffffu);
            if(owned.datum!=original.datum)throw FormatError("Native world cooked full datum changed");
            owned.address=std::uint32_t(address);owned.size=std::uint32_t(projected->bytes.size());
            retained_tag_allocations_.push_back(original);
            preserved_native_pointer_maps_.push_back({{owned.datum},owned.address,owned.size,std::move(projected->pointers)});
            restored.insert(owned.datum);
        }
    }
    // The MCC-only grey instance uses the same retained HUD program as its
    // original red/green peers. Project its authenticated parameter metadata;
    // do not replace its authored constants, curves, or any shader program.
    for(const auto& record:records) {
        if(record.at("group")!="stem" ||
           record.at("name")!="shaders\\shader_templates\\hud\\widget_simple_flash")continue;
        const Json* pass_record=nullptr;
        for(const auto& candidate:records)
            if(candidate.at("group")=="spas" &&
               candidate.at("name")=="shaders\\shader_passes\\hud\\widget_simple_flash")pass_record=&candidate;
        const auto* stem=query_tag({.group=TagGroup{"stem"},
            .name="shaders\\shader_templates\\hud\\widget_simple_flash",.require_address=true});
        const auto* pass=query_tag({.group=TagGroup{"spas"},
            .name="shaders\\shader_passes\\hud\\widget_simple_flash",.require_address=true});
        if(!pass_record || !stem || !pass)continue;
        const Tag native_stem=*stem,native_pass=*pass;
        const auto template_payload=read(payload_path(record.at("payload").get<std::string>()),16u*1024u*1024u);
        const auto pass_payload=read(payload_path(pass_record->at("payload").get<std::string>()),16u*1024u*1024u);
        const auto candidates=tags_;
        for(const auto& original:candidates) {
            if(original.group!="shad" || !original.address || restored.contains(original.datum))continue;
            auto projected=project_mcc_grey_widget_flash(original,
                {native_stem,native_pass,template_payload,pass_payload},
                {.source_read=[&](std::uint32_t address,std::size_t size) {
                    return Bytes(storage_).slice(metadata_offset(address,size),size);
                },.native_read=[&](std::uint32_t address,std::size_t size) {
                    if(!((address==native_stem.address && size==native_stem.size) ||
                         (address==native_pass.address && size==native_pass.size)))
                        throw FormatError("HUD native closure read lost its complete typed owner");
                    return Bytes(storage_).slice(TagAddressSpace(Bytes(storage_),metadata_magic_).offset(address,size),size);
                },.resolve=[&](TagHandle handle)->std::optional<Tag> {
                    const auto* tag=find_tag(handle);
                    return tag?std::optional<Tag>{*tag}:std::nullopt;
                },.string_id=[&](std::uint32_t id){return string_id(id);},.directory=tags_});
            if(!projected)continue;
            const auto va=next_address();
            if(va+projected->bytes.size()>0x100000000ull)
                throw FormatError("Canonical HUD shader allocation overflow");
            for(const auto& [field,target]:projected->pointers)
                word(projected->bytes,field,std::uint32_t(va+target));
            storage_.resize(std::size_t(std::int64_t(va)-metadata_magic_));
            storage_.insert(storage_.end(),projected->bytes.begin(),projected->bytes.end());
            auto& owned=tags_.at(original.datum&0xffffu);
            if(owned.datum!=original.datum)throw FormatError("Canonical HUD shader full datum changed");
            owned.address=std::uint32_t(va);owned.size=std::uint32_t(projected->bytes.size());
            retained_tag_allocations_.push_back(original);
            preserved_native_pointer_maps_.push_back({{owned.datum},owned.address,owned.size,std::move(projected->pointers)});
            restored.insert(owned.datum);
        }
    }
    // PC maps retain their exact HUD and finite paired sky closures.
    // Keep their globals, world lighting and StringID bank unchanged.
    if(!hud_only) {
    const auto* globals=query_tag({.handle=globals_tag_.is_null()?std::nullopt:std::optional<TagHandle>{globals_tag_},.group=tag_groups::globals,.require_address=true});
    if(!globals)throw FormatError("Native shader rasterizer globals owner unavailable");
    const Tag retained_globals=*globals;
    const auto handle=globals->datum;
    const auto va=next_address();
    auto global_bytes=clone_globals(Bytes(storage_).slice(metadata_offset(globals->address,globals->size),globals->size),
        globals->address,std::uint32_t(va),[&](std::uint32_t pointer,std::size_t size){return Bytes(storage_).slice(metadata_offset(pointer,size),size);});
    const auto raster_offset=(global_bytes.size()+15)&~std::size_t(15);
    const auto raster=prepare(bank.at("rasterizer"),va+raster_offset);
    if(raster_offset>maximum || raster.size()>maximum-raster_offset || va+raster_offset+raster.size()>0x100000000ull)
        throw FormatError("Excessive preserved rasterizer globals allocation");
    global_bytes.resize(raster_offset);global_bytes.insert(global_bytes.end(),raster.begin(),raster.end());
    word(global_bytes,0x108,1);word(global_bytes,0x10c,std::uint32_t(va+raster_offset));
    storage_.resize(std::size_t(std::int64_t(va)-metadata_magic_));storage_.insert(storage_.end(),global_bytes.begin(),global_bytes.end());
    tags_[handle&0xffffu].address=std::uint32_t(va);tags_[handle&0xffffu].size=std::uint32_t(global_bytes.size());
    retained_tag_allocations_.push_back(retained_globals);
    retain_pointer_map(bank.at("rasterizer"),handle,std::uint32_t(va+raster_offset),std::uint32_t(raster.size()));
    const auto text=storage_.size();storage_.insert(storage_.end(),strings.begin(),strings.end());
    const auto table=storage_.size();storage_.resize(table+offsets.size()*4);
    for(std::size_t i=0;i<offsets.size();++i)word(storage_,table+i*4,offsets[i]);
    word(storage_,48,std::uint32_t(offsets.size()));word(storage_,52,std::uint32_t(text));
    word(storage_,56,std::uint32_t(strings.size()));word(storage_,60,std::uint32_t(table));
    }
    std::vector<std::pair<std::uint32_t,std::uint32_t>> retained_regions;
    for(const auto& allocation:retained_tag_allocations_)
        retained_regions.emplace_back(allocation.address,allocation.size);
    vista_metadata_regions_=tag_metadata_regions(tags_,retained_regions);
    mcc_native_shaders_.resize(tags_.size());
    for(auto handle:restored)mcc_native_shaders_[handle&0xffffu]=true;
    // Publish trusted labels only after every complete record, relocation,
    // typed reference and native flag in the authenticated batch is installed.
    std::map<std::uint32_t,ShaderSemanticOwner> staged_semantics;
    for(const auto& [identity,semantic]:vehicle_semantics) {
        const auto entry=identities.find(identity);
        if(entry==identities.end() || !restored.contains(entry->second))
            throw FormatError("Vehicle semantic label has no installed authenticated owner");
        const auto& actual=tags_.at(entry->second&0xffffu);
        if(actual.datum!=entry->second || actual.group!=identity.first || actual.name!=identity.second ||
           !actual.address || !actual.size || !is_preserved_native_shader(actual) ||
           !staged_semantics.emplace(actual.datum,ShaderSemanticOwner{actual.group,actual.name,semantic,
               actual.address,actual.size}).second)
            throw FormatError("Vehicle semantic label differs from its final native storage identity");
    }
    if(!shader_semantic_names_.empty())throw FormatError("Native shader semantic metadata was already published");
    shader_semantic_names_.swap(staged_semantics);
    if(!hud_only)function_tables_=owned_tables;
}
}

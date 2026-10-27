#include "halo2/mcc_sky_geometry.hpp"
#include "halo2/math.hpp"
#include "halo2/appearance.hpp"
#include "halo2/sky_input_identity.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <zlib.h>
#include <algorithm>
#include <bit>
#include <limits>
#include <memory>
#include <set>

namespace halo2 {
namespace {
using Json=nlohmann::json;
void need(bool condition,const char *message) {if(!condition)throw FormatError(message);}
struct Writer {
    std::vector<std::byte> bytes;
    void word(std::uint32_t value) {for(unsigned i=0;i<4;++i)bytes.push_back(std::byte((value>>(i*8))&255));}
    void number(float value) {word(std::bit_cast<std::uint32_t>(value));}
    void point(Vec3 p) {number(p.x);number(p.y);number(p.z);}
    void point(Vec2 p) {number(p.x);number(p.y);}
    void vector(ShaderVector p) {for(float value:p)number(value);}
    void count(std::size_t size) {need(size<=1000000,"Sky geometry count exceeds its finite contract");word(std::uint32_t(size));}
};
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    std::uint32_t word() {
        need(at<=bytes.size() && bytes.size()-at>=4,"Truncated original sky geometry");
        std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned>(bytes[at++]))<<(i*8);return value;
    }
    float number() {const auto value=std::bit_cast<float>(word());need(std::isfinite(value),"Nonfinite original sky attribute");return value;}
    Vec3 point() {return {number(),number(),number()};}
    Vec2 uv() {return {number(),number()};}
    ShaderVector vector() {return {number(),number(),number(),number()};}
    std::size_t count(std::size_t maximum=1000000) {auto value=word();need(value<=maximum,"Original sky count exceeds its finite contract");return value;}
    std::uint16_t half() {const auto value=word();need(value<=65535,"Original sky half word overflows");return std::uint16_t(value);}
};
std::vector<std::byte> hex(std::string_view encoded) {
    need(encoded.size()%2==0 && encoded.size()<=64*1024*1024,"Invalid encoded sky geometry extent");
    const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw FormatError("Noncanonical sky geometry hex");};
    std::vector<std::byte> out;out.reserve(encoded.size()/2);
    for(std::size_t i=0;i<encoded.size();i+=2)out.push_back(std::byte((digit(encoded[i])<<4)|digit(encoded[i+1])));return out;
}
const Json& catalog();
#include "mcc_sky_material_relocations.inc"
std::vector<std::byte> source(const Json& entry) {
    const Json* storage=&entry;
    if(!entry.contains("zlib_hex")) {
        const auto identity=entry.at("sha256").get<std::string>();const auto& bank=catalog().at("geometry");
        need(bank.contains(identity),"Original sky geometry has no authenticated source state");storage=&bank.at(identity);
        need(storage->at("bytes")==entry.at("bytes")&&storage->at("sha256")==entry.at("sha256"),"Original sky geometry source reference differs");
    }
    const auto compressed=hex(storage->at("zlib_hex").get<std::string>());
    const auto size=entry.at("bytes").get<std::size_t>();need(size>0&&size<=64*1024*1024,"Original sky geometry has an invalid extent");
    std::vector<std::byte> bytes(size);uLongf expanded=size;
    need(uncompress(reinterpret_cast<Bytef*>(bytes.data()),&expanded,reinterpret_cast<const Bytef*>(compressed.data()),compressed.size())==Z_OK&&expanded==size,
        "Original sky geometry cannot be expanded");
    need(mcc_sky_geometry_sha256(bytes)==entry.at("sha256").get<std::string>(),"Original sky geometry source digest differs");return bytes;
}
const Json& catalog() {
    static const Json value=Json::parse(mcc_sky_geometry_profiles());
    need(value.at("schema")=="H2_ORIGINAL_SKY_GEOMETRY_V1","Unsupported original sky geometry catalog");return value;
}
}
std::string_view mcc_sky_geometry_profiles() {
    return
#include "mcc_sky_geometry_profiles.inc"
    ;
}
std::string mcc_sky_geometry_sha256(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> hash{};unsigned length{};
    need(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&length,EVP_sha256(),nullptr)==1&&length==hash.size(),"Cannot authenticate sky geometry");
    constexpr char digits[]="0123456789abcdef";std::string value;
    for(auto b:hash){value+=digits[b>>4];value+=digits[b&15];}return value;
}
bool mcc_sky_material_relocation_matches(std::string_view model_input,
    std::uint32_t shader,std::string_view name,std::string_view original_payload_sha256,
    std::span<const std::byte> payload) {
    static const Json policy=Json::parse(sky_material_relocation_policy);
    need(policy.at("schema")=="H2_MCC_SKY_MATERIAL_RELOCATIONS_V1","Unsupported derived sky material input receipt");
    const auto actual=mcc_sky_geometry_sha256(payload);
    for(const auto& receipt:policy.at("materials")) {
        if(receipt.at("model_input_sha256")!=model_input || receipt.at("shader")!=shader ||
           receipt.at("name")!=name || receipt.at("before_payload_sha256")!=original_payload_sha256 ||
           receipt.at("after_payload_sha256")!=actual)continue;
        const auto& source=receipt.at("source_record");
        need(source.at("group")=="shad"&&source.at("name")==name&&source.at("bytes")==payload.size(),
            "Derived sky material receipt lacks its complete original owner");
        std::set<std::size_t> declared,changed;
        for(const auto& pointer:source.at("pointers"))
            need(declared.insert(pointer.at(0).get<std::size_t>()).second,"Duplicate original sky pointer relocation");
        std::vector<std::byte> before(payload.begin(),payload.end());
        const auto delta=receipt.at("allocation_delta").get<std::uint32_t>();
        need(delta>0&&delta<=16u*1024u*1024u,"Derived sky allocation shift escapes its finite source receipt");
        for(const auto& word:receipt.at("pointer_words")) {
            const auto at=word.at("offset").get<std::size_t>();
            const auto old=word.at("old").get<std::uint32_t>(),now=word.at("new").get<std::uint32_t>();
            need(at<=payload.size()&&payload.size()-at>=4&&declared.contains(at)&&changed.insert(at).second&&
                 std::uint64_t(old)+delta==now&&Bytes(payload).u32(at)==now,
                "Derived sky material escaped its exact original pointer before/after words");
            for(unsigned byte=0;byte<4;++byte)before[at+byte]=std::byte((old>>(byte*8))&255);
        }
        need(changed==declared,"Derived sky material has an incomplete original relocation receipt");
        return mcc_sky_geometry_sha256(before)==original_payload_sha256;
    }
    return false;
}
std::string mcc_sky_geometry_input_sha256(const Cache& cache,std::uint32_t model) {
    Writer value;const auto* tag=cache.find_tag({model},tag_groups::render_model);need(tag,"Sky projection has no typed model owner");
    value.word(model);value.word(tag->address);value.word(tag->size);
    const auto append=[&](std::span<const std::byte> bytes){need(bytes.size()<=64*1024*1024,"Sky model resource is outside its finite contract");value.word(std::uint32_t(bytes.size()));value.bytes.insert(value.bytes.end(),bytes.begin(),bytes.end());};
    const auto payload=cache.tag_payload({model});append(payload);const Bytes root(payload);
    // MCC may forward blocks into a retained allocation outside tag->size.
    // Authenticate every metadata block consumed by the native model decoder,
    // including all LODs, rather than trusting allocation contiguity.
    const auto block=[&](const Bytes& owner,std::size_t field,unsigned stride,unsigned maximum) {
        const auto count=owner.u32(field),address=owner.u32(field+4);need(count<=maximum,"Sky model metadata block exceeds source limits");
        value.word(count);value.word(address);const auto data=count?cache.read_tag_bytes(address,std::size_t(count)*stride):std::span<const std::byte>{};append(data);return data;
    };
    block(root,0x14,0x38,64);
    const auto regions=block(root,0x1c,16,16);for(std::size_t at=0;at<regions.size();at+=16)block(Bytes(regions.subspan(at,16)),8,16,65536);
    const auto sections=block(root,0x24,0x5c,65536);for(std::size_t at=0;at<sections.size();at+=0x5c)block(Bytes(sections.subspan(at,0x5c)),0x48,16,128);
    const auto groups=block(root,0x34,12,256);for(std::size_t at=0;at<groups.size();at+=12)block(Bytes(groups.subspan(at,12)),4,16,256);
    block(root,0x48,0x60,255);block(root,0x60,32,65536);
    need(root.u32(0x74)<=1,"Sky projection requires every authored PRT resource owner");
    block(root,0x74,0x58,1);
    const auto resources=cache.load_model_resources(model);value.count(resources.size());
    for(const auto& resource:resources)append(resource);
    return mcc_sky_geometry_sha256(value.bytes);
}
std::vector<std::byte> encode_mcc_sky_geometry(const RenderModel& model) {
    need(model.mesh.lightmaps.empty()&&model.mesh.resident_material_lightmaps.empty()&&!model.mesh.clusters&&!model.mesh.instances,
        "Original sky geometry has an unexported BSP/lightmap owner");
    Writer out;out.word(0x31475953);out.word(model.checksum);out.word(model.animation);
    out.count(model.bones.size());for(const auto& bone:model.bones) {
        out.word(bone.name);out.word(std::bit_cast<std::uint32_t>(std::int32_t(bone.parent)));
        out.point(bone.bind.translation);for(auto f:bone.bind.rotation)out.number(f);out.number(bone.bind.scale);
        for(auto f:bone.inverse)out.number(f);out.number(bone.inverse_scale);
    }
    out.count(model.compound_nodes.size());for(const auto& node:model.compound_nodes) {for(auto n:node.nodes)out.word(n);for(auto f:node.weights)out.number(f);}
    out.count(model.mesh.vertices.size());for(const auto& v:model.mesh.vertices) {
        out.point(v.position);out.point(v.color);out.point(v.uv);out.word(v.material);out.point(v.lightmap_uv);
        out.point(v.normal);out.point(v.tangent);out.point(v.binormal);out.point(v.incident);
        out.word(v.lightmap);out.word(std::bit_cast<std::uint32_t>(v.bsp_cluster));out.number(v.prt_cluster);out.vector(v.prt_weights);
        out.word(v.source_section);out.word(v.source_part);out.word(v.source_stipple_level);out.point(v.source_local_position);out.number(v.source_color_alpha);
        out.word(v.source_part_type);out.word(v.source_part_flags);
    }
    out.count(model.skin.size());for(const auto& skin:model.skin) {for(auto n:skin.bones)out.word(n);for(auto f:skin.weights)out.number(f);}
    out.count(model.vertex_origins.size());for(const auto& origin:model.vertex_origins) {out.word(origin.section);out.word(origin.vertex);out.word(origin.part);out.word(origin.vertex_class);}
    return std::move(out.bytes);
}
RenderModel decode_mcc_sky_geometry(std::span<const std::byte> bytes) {
    Reader in{bytes};need(in.word()==0x31475953,"Unsupported original sky geometry record");RenderModel out;
    out.checksum=in.word();out.animation=in.word();out.bones.resize(in.count(255));
    for(auto& bone:out.bones) {
        bone.name=in.word();bone.parent=std::bit_cast<std::int32_t>(in.word());bone.bind.translation=in.point();
        for(auto& f:bone.bind.rotation)f=in.number();bone.bind.scale=in.number();for(auto& f:bone.inverse)f=in.number();bone.inverse_scale=in.number();
        need(bone.parent<int(out.bones.size()),"Original sky parent escapes model hierarchy");
    }
    out.compound_nodes.resize(in.count(65535));for(auto& node:out.compound_nodes) {for(auto& n:node.nodes){const auto value=in.word();need(value<=255,"Original sky compound node overflows");n=std::uint8_t(value);}for(auto& f:node.weights)f=in.number();}
    out.mesh.vertices.resize(in.count());for(auto& v:out.mesh.vertices) {
        v.position=in.point();v.color=in.point();v.uv=in.uv();v.material=in.word();v.lightmap_uv=in.uv();
        v.normal=in.point();v.tangent=in.point();v.binormal=in.point();v.incident=in.point();
        v.lightmap=in.word();v.bsp_cluster=std::bit_cast<std::int32_t>(in.word());v.prt_cluster=in.number();v.prt_weights=in.vector();
        v.source_section=in.word();v.source_part=in.word();v.source_stipple_level=in.word();v.source_local_position=in.point();v.source_color_alpha=in.number();
        v.source_part_type=in.half();v.source_part_flags=in.half();
    }
    out.skin.resize(in.count());for(auto& skin:out.skin) {for(auto& n:skin.bones)n=in.half();for(auto& f:skin.weights)f=in.number();}
    out.vertex_origins.resize(in.count());for(auto& origin:out.vertex_origins) {origin.section=in.half();origin.vertex=in.half();origin.part=in.half();const auto value=in.word();need(value<=6,"Original sky vertex class overflows");origin.vertex_class=std::uint8_t(value);}
    need(in.at==bytes.size()&&out.mesh.vertices.size()==out.skin.size()&&out.mesh.vertices.size()==out.vertex_origins.size()&&out.mesh.vertices.size()%3==0,"Original sky geometry record has inconsistent banks");
    if(!out.mesh.vertices.empty()) {
        out.mesh.minimum=out.mesh.maximum=out.mesh.vertices.front().position;
        for(const auto& v:out.mesh.vertices){out.mesh.minimum=minimum(out.mesh.minimum,v.position);out.mesh.maximum=maximum(out.mesh.maximum,v.position);}
    }
    return out;
}
bool restore_mcc_sky_geometry(const Cache& cache,std::uint32_t model,RenderModel& output,
    bool textures,std::optional<unsigned> geometry_lod,std::optional<std::span<const unsigned>> resident_sections) {
    if(!cache.is_mcc())return false;
    // A model name is diagnostic text, never its selector. Only actual typed
    // sky references may enter this finite original-asset projection.
    bool sky=false;
    for(const auto& tag:cache.select_tags({.group=tag_groups::sky})) {
        const auto bytes=cache.tag_payload(tag.handle());if(bytes.size()>=8&&Bytes(bytes).u32(4)==model){sky=true;break;}
    }
    if(!sky)return false;
    const auto input=mcc_sky_geometry_input_sha256(cache,model);
    const Json* profile=nullptr;
    for(const auto& candidate:catalog().at("profiles"))if(candidate.at("input_sha256")==input){need(!profile,"Ambiguous original sky geometry identity");profile=&candidate;}
    if(!profile)return false;
    return restore_mcc_sky_geometry_profile(cache,model,output,textures,profile->dump(),geometry_lod,resident_sections);
}
bool restore_mcc_sky_geometry_profile(const Cache& cache,std::uint32_t model,RenderModel& output,
    bool textures,std::string_view profile_json,std::optional<unsigned> geometry_lod,
    std::optional<std::span<const unsigned>> resident_sections) {
    need(cache.is_mcc(),"Original sky projection requires an actual MCC input");
    const auto profile_storage=Json::parse(profile_json);const auto* profile=&profile_storage;
    if(profile->at("input_sha256")!=mcc_sky_geometry_input_sha256(cache,model))return false;
    const auto map_text=profile->at("materials").dump();
    need(mcc_sky_geometry_sha256(std::as_bytes(std::span(map_text)))==profile->at("material_mapping_sha256").get<std::string>(),
        "Sky canonical material mapping digest differs");
    need(!geometry_lod||*geometry_lod<=5,"Original sky geometry LOD escapes source bank");
    const auto& selection=geometry_lod?profile->at("lods").at(*geometry_lod):profile->at("preview");
    if(resident_sections) {
        const auto expected=selection.at("mcc_sections").get<std::vector<unsigned>>();
        need(std::ranges::equal(*resident_sections,expected),"Original sky resident sections lack an exact source projection");
    }
    auto restored=decode_mcc_sky_geometry(source(selection));restored.name=output.name;
    // load_sky rejects graph-bearing skies. Bind-only pose_sky never resolves
    // these literal provenance StringIDs in the imported map's namespace.
    need(restored.animation==0xffffffffu,"Original sky projection cannot introduce an animation owner");
    need(profile->at("original_material_count").get<std::size_t>()==profile->at("materials").size(),
        "Sky canonical source material extent differs");
    std::vector<Material> materials;materials.reserve(profile->at("materials").size());
    const auto payload=cache.tag_payload({model});const Bytes root(payload);
    const auto count=root.u32(0x60),address=root.u32(0x64);need(count+1==output.mesh.materials.size(),"Sky material bank differs from its exact model input");
    Mesh prepared;
    if(textures){prepared.materials=output.mesh.materials;prepare_forward_materials(cache,prepared);}
    for(const auto& mapping:profile->at("materials")) {
        need(mapping.at("original_semantic_sha256")==mapping.at("mcc_semantic_sha256"),
            "Sky canonical material mapping has no exact original semantic authority");
        const auto index=mapping.at("mcc_index").get<unsigned>();need(index<output.mesh.materials.size(),"Sky material projection escapes its actual MCC bank");
        if(index<count) {
            const auto record=cache.read_tag_bytes(address+index*32,32);const auto handle=Bytes(record).u32(12);
            need(handle==mapping.at("mcc_shader").get<std::uint32_t>(),"Sky shader mapping has another salted owner");
            const auto* shader=cache.find_tag({handle},tag_groups::shader);need(shader&&shader->name==mapping.at("name").get<std::string>(),"Sky shader mapping has another typed owner");
            const auto material_payload=cache.tag_payload({handle});
            const auto expected_payload=mapping.at("mcc_shader_sha256").get<std::string>();
            need(mcc_sky_geometry_sha256(material_payload)==expected_payload ||
                 mcc_sky_material_relocation_matches(profile->at("input_sha256").get<std::string>(),handle,shader->name,expected_payload,material_payload),
                 "Sky shader projection input digest differs");
        } else need(mapping.at("name")=="","Sky default material projection is not its authored NONE slot");
        if(textures)need(sky_audit::semantic_digest(prepared.materials[index],cache)==mapping.at("mcc_semantic_sha256").get<std::string>(),
            "Sky material/texture semantics differ from the authenticated projection input");
        materials.push_back(output.mesh.materials[index]);
    }
    for(const auto& v:restored.mesh.vertices)need(v.material<materials.size(),"Original sky geometry escapes its canonical source material bank");
    restored.mesh.materials=std::move(materials);restored.mesh.material_warnings=std::move(output.mesh.material_warnings);
    output=std::move(restored);return true;
}
} // namespace halo2

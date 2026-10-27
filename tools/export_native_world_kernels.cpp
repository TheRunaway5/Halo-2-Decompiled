// Explicit offline source maintenance. Normal builds consume the checked-in
// registry and do not open original executables, original maps or MCC caches.
#include "halo2/appearance.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/model.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_render_policy.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/water.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/pstencil/detail.hpp"
#include "halo2/pstencil/environment.hpp"
#include "halo2/pstencil/specular_mask.hpp"
#include "halo2/pstencil/light_response_2003.hpp"
#include "halo2/pstencil/normal_accumulation_2003.hpp"
#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include "halo2/sky_reference_inventory.hpp"
#include "native_particle_fragment_inventory.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
using json=nlohmann::json;
std::string hex(std::span<const std::byte> data) {
    std::ostringstream out;out<<std::hex<<std::setfill('0');
    for(auto value:data)out<<std::setw(2)<<std::to_integer<unsigned>(value);
    return out.str();
}
std::string sha256(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    if(!input)throw FormatError("Cannot hash source map: "+path.string());
    const auto context=std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    if(!context || EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)!=1)throw FormatError("Cannot initialize SHA256");
    std::array<char,65536> buffer;
    while(input){input.read(buffer.data(),buffer.size());if(EVP_DigestUpdate(context.get(),buffer.data(),input.gcount())!=1)throw FormatError("Cannot hash source bytes");}
    if(!input.eof())throw FormatError("Cannot read complete source map");
    std::array<std::byte,32> digest;unsigned size{};
    if(EVP_DigestFinal_ex(context.get(),reinterpret_cast<unsigned char*>(digest.data()),&size)!=1 || size!=digest.size())throw FormatError("Cannot finish SHA256");
    return hex(digest);
}
bool dynamic_color(unsigned index){return (index>=10 && index<=25)||index==43||index==44;}
struct AliasEvidence {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> digest{EVP_MD_CTX_new(),EVP_MD_CTX_free};
    std::set<std::string> roles;
    std::map<std::string,std::uint64_t> maps;
    std::uint64_t count{};
    AliasEvidence() {
        if(!digest || EVP_DigestInit_ex(digest.get(),EVP_sha256(),nullptr)!=1)
            throw FormatError("Cannot initialize complete native source-owner digest");
    }
    bool add(const json& alias,bool cpu=false) {
        // Every source owner contributes to the complete digest and map count.
        // Retain a concrete representative for every distinct runtime routing
        // signature used by project_native_material_fragments.py, regardless
        // of which map introduced that role. Repeated map/pass/switch aliases
        // then cannot multiply the in-memory compiler catalog without bound.
        const auto text=alias.dump()+"\n";
        if(EVP_DigestUpdate(digest.get(),text.data(),text.size())!=1)
            throw FormatError("Cannot hash complete native source-owner evidence");
        ++count;
        ++maps[alias.value("source","")+":"+alias.value("map_sha256","")+":"+alias.value("map","")];
        json role=json::array();
        for(const auto* name:{"kind","family","also_family","operation","vertex_shader","implementation","vertex_class","stage","source_controls"})
            role.push_back(alias.contains(name)?alias.at(name):json(nullptr));
        const auto name=alias.value("template_name","");
        role.push_back(name.ends_with("\\emblem_overlay_simple"));
        if(cpu) {
            role.push_back(alias.value("name",""));
            role.push_back(alias.value("classification",0u));
        }
        return roles.insert(role.dump()).second;
    }
    void finish(json& row) {
        std::array<std::byte,32> value;unsigned size{};
        if(EVP_DigestFinal_ex(digest.get(),reinterpret_cast<unsigned char*>(value.data()),&size)!=1 || size!=value.size())
            throw FormatError("Cannot finish complete native source-owner digest");
        row["source_owner_evidence"]={{"count",count},{"sha256",hex(value)},
            {"encoding","SHA256 of every canonical JSON source-owner record plus LF in deterministic export order"},
            {"map_counts",maps},{"retained_routing_signatures",roles.size()},
            {"retention","One exact representative per complete runtime-family routing signature; every source owner hashed and counted"}};
    }
};
struct Registry {
    json entries=json::array(),maps=json::array(),rejected=json::array();
    json fragment_programs=json::array(),cpu_vertex_programs=json::array(),metadata_rejected=json::array(),non_world_inputs=json::array();
    std::map<std::string,unsigned> keys,fragment_keys,cpu_keys;
    std::vector<AliasEvidence> entry_evidence,fragment_evidence,cpu_evidence;
    void finish_evidence() {
        for(unsigned i=0;i<entries.size();++i)entry_evidence[i].finish(entries[i]);
        for(unsigned i=0;i<fragment_programs.size();++i)fragment_evidence[i].finish(fragment_programs[i]);
        for(unsigned i=0;i<cpu_vertex_programs.size();++i)cpu_evidence[i].finish(cpu_vertex_programs[i]);
    }
    static void alias(json& rows,std::vector<AliasEvidence>& evidence,unsigned index,const json& value,bool cpu=false) {
        if(evidence[index].add(value,cpu))rows[index]["aliases"].push_back(value);
    }
    void add_cpu_program(std::span<const std::byte> bytes,const json& alias) {
        const auto decoded=decode_vertex_program(bytes);const auto key=hex(bytes);
        if(const auto found=cpu_keys.find(key);found!=cpu_keys.end()) {
            Registry::alias(cpu_vertex_programs,cpu_evidence,found->second,alias,true);return;
        }
        json row={{"vertex_program_hex",key},{"vertex_instructions",json::array()},
            {"relative_constants",decoded.relative_constants},{"aliases",json::array()}};
        for(unsigned i=0;i<decoded.count;++i)row["vertex_instructions"].push_back(decoded.instructions[i]);
        const auto index=unsigned(cpu_vertex_programs.size());
        cpu_keys.emplace(key,index);cpu_vertex_programs.push_back(std::move(row));cpu_evidence.emplace_back();
        Registry::alias(cpu_vertex_programs,cpu_evidence,index,alias,true);
    }
    void add_fragment(const PixelProgram& program,const json& alias) {
        auto controls=program.words;
        for(unsigned i=0;i<controls.size();++i)if(dynamic_color(i))controls[i]=0;
        const auto key=json(controls).dump();
        if(const auto found=fragment_keys.find(key);found!=fragment_keys.end()) {
            Registry::alias(fragment_programs,fragment_evidence,found->second,alias);return;
        }
        const auto index=unsigned(fragment_programs.size());
        fragment_keys.emplace(key,index);fragment_programs.push_back({{"controls",controls},{"aliases",json::array()}});
        fragment_evidence.emplace_back();Registry::alias(fragment_programs,fragment_evidence,index,alias);
    }
    void add_program(std::span<const std::byte> vertex_program,const PixelProgram& program,
        const std::optional<SourceVertexDeclaration>& declaration,SourceWorldVertexInputTransform transform,
        bool extended,unsigned forward_vertex,const json& alias) {
        const auto decoded=decode_vertex_program(vertex_program);
        if(!decoded.count)throw FormatError("Material has no vertex instructions");
        auto controls=program.words;
        for(unsigned i=0;i<controls.size();++i)if(dynamic_color(i))controls[i]=0;
        const auto packed=pack_source_vertex_declaration(declaration,transform);
        json row={{"vertex_program_hex",hex(vertex_program)}, {"controls",controls},
            {"packed_declaration",packed},{"extended_vertex_bank",extended}};
        const auto key=row.dump();
        if(const auto found=keys.find(key);found!=keys.end()){
            Registry::alias(entries,entry_evidence,found->second,alias);return;
        }
        row["vertex_instructions"]=json::array();
        for(unsigned i=0;i<decoded.count;++i)row["vertex_instructions"].push_back(decoded.instructions[i]);
        row["semantics"]=json::array();
        for(unsigned i=0;i<16;++i)row["semantics"].push_back((packed[i/4]>>(8*(i%4)))&255);
        row["forward_vertex"]=forward_vertex;
        row["relative_constants"]=decoded.relative_constants;
        row["declaration"]=declaration?json{{"class_index",declaration->class_index},
            {"semantics",declaration->semantics}}:json(nullptr);
        row["aliases"]=json::array();
        const auto index=unsigned(entries.size());keys.emplace(key,index);entries.push_back(std::move(row));
        entry_evidence.emplace_back();Registry::alias(entries,entry_evidence,index,alias);
    }
    void add(const Material& material,const json& alias) {
        if(!material.forward_pass || material.forward_pass_index>=material.passes.size() || material.vertex_program.empty())
            throw FormatError("Material lacks its actual authored forward program: "+material.name);
        if(!texture_program_supported(material.forward_program))
            throw FormatError("Material has an unsupported authored texture operation: "+material.name);
        add_fragment(material.forward_program,alias);
        add_program(material.vertex_program,material.forward_program,material.vertex_declaration,
            material.world_vertex_input_transform,bool(material.vertex_bank),material.forward_vertex,alias);
    }
};

// 0001BA00 selects each fragment's permutation by an ordinal switch index,
// not by the permutation's separate remap-filter value. Enumerate that finite
// ordinal domain, preserving dependencies between fragments sharing a switch.
// Dynamic colors are data, absent from the normalized executable identity.
struct ProgramVariant {
    PixelProgram program;
    std::array<std::int32_t,8> switches;
    std::uint64_t valid_assignments{};
    std::string assignment_sha256;
    json provenance() const {
        return {{"valid_assignments",valid_assignments},{"sha256",assignment_sha256},
            {"encoding","Each valid assignment in ordered switch-domain traversal: eight int32 LE switches followed by sixty uint32 LE assembled program words"}};
    }
};
std::vector<ProgramVariant> variants(
    const Material& material,const MaterialPass& pass) {
    std::map<unsigned,std::set<std::int32_t>> domains;
    for(const auto& fragment:pass.fragments) {
        if(fragment.switch_index<0)continue;
        if(unsigned(fragment.switch_index)>=material.shader_switches.size())
            throw FormatError("Native catalog shader switch escapes source bank");
        auto& domain=domains[unsigned(fragment.switch_index)];
        domain.insert(material.shader_switches[unsigned(fragment.switch_index)]);
        if(fragment.permutations.first()>pass.permutations.size() ||
           fragment.permutations.count()>pass.permutations.size()-fragment.permutations.first())
            throw FormatError("Native catalog permutation escapes complete pass");
        for(std::size_t i=0;i<fragment.permutations.count();++i)domain.insert(std::int32_t(i));
    }
    std::size_t count=1;
    for(const auto& [_,values]:domains) {
        if(values.size()>65536/count)throw FormatError("Native catalog switch domain exceeds explicit 65536-program bound");
        count*=values.size();
    }
    // The assembler's switch domain can contain many assignments that produce
    // the same executable controls. Preserve their exact count and ordered
    // source digest, but compile and attach map provenance only once for that
    // complete native identity. Otherwise millions of redundant JSON aliases
    // can consume the host's memory before the all-map census is written.
    std::vector<ProgramVariant> output;
    using Digest=std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>;
    std::vector<Digest> digests;
    std::map<std::array<std::uint32_t,60>,std::size_t> identities;
    std::vector<std::pair<unsigned,std::vector<std::int32_t>>> ordered;
    for(const auto& [index,values]:domains)ordered.push_back({index,{values.begin(),values.end()}});
    auto switches=material.shader_switches;
    const auto visit=[&](auto&& self,std::size_t at)->void {
        if(at<ordered.size()) {
            const auto& [index,values]=ordered[at];
            for(const auto value:values){switches[index]=value;self(self,at+1);}
            return;
        }
        try {
            const auto program=assemble_pixel_program(pass.definition,pass.fragments,
                pass.permutations,pass.combiners,switches);
            auto controls=program.words;
            for(unsigned i=0;i<controls.size();++i)if(dynamic_color(i))controls[i]=0;
            const auto [identity,inserted]=identities.try_emplace(controls,output.size());
            if(inserted) {
                Digest context(EVP_MD_CTX_new(),EVP_MD_CTX_free);
                if(!context || EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)!=1)
                    throw std::runtime_error("Cannot initialize native variant source digest");
                digests.push_back(std::move(context));output.push_back({program,switches,0,{}});
            }
            const auto ordinal=identity->second;
            std::array<std::byte,(8+60)*4> source{};
            const auto word=[&](unsigned at,std::uint32_t value) {
                for(unsigned byte=0;byte<4;++byte)source[at*4+byte]=std::byte(value>>(byte*8));
            };
            for(unsigned i=0;i<8;++i)word(i,std::bit_cast<std::uint32_t>(switches[i]));
            for(unsigned i=0;i<60;++i)word(8+i,program.words[i]);
            if(EVP_DigestUpdate(digests[ordinal].get(),source.data(),source.size())!=1)
                throw std::runtime_error("Cannot hash native variant source assignment");
            ++output[ordinal].valid_assignments;
        } catch(const FormatError&) {
            // An original material's initial switch can be outside the valid
            // ordinal domain. Shared switches also constrain one another's
            // ranges. The original assembler rejects those assignments.
        }
    };
    visit(visit,0);
    if(output.empty())throw FormatError("Native catalog pass has no valid source permutation");
    for(unsigned i=0;i<output.size();++i) {
        std::array<std::byte,32> digest;unsigned size{};
        if(EVP_DigestFinal_ex(digests[i].get(),reinterpret_cast<unsigned char*>(digest.data()),&size)!=1 || size!=digest.size())
            throw FormatError("Cannot finish native variant source digest");
        output[i].assignment_sha256=hex(digest);
    }
    return output;
}

void export_all_materials(Registry& out,const Cache& cache,json& map) {
    const auto before_entries=out.entries.size(),before_fragments=out.fragment_programs.size(),
        before_cpu=out.cpu_vertex_programs.size(),before_errors=out.metadata_rejected.size();
    unsigned materials=0,passes=0;
    std::map<std::uint32_t,json> hud_shaders;
    const auto hud_reference=[&](std::uint32_t shader,json owner) {
        if(shader==0xffffffffu)return;
        const auto typed=cache.query_tag({.handle=TagHandle{shader},.group=tag_groups::shader,.require_address=true});
        if(!typed)throw FormatError("Native HUD shader reference lacks its typed shad owner");
        hud_shaders[shader].push_back(std::move(owner));
    };
    for(const auto& tag:cache.tags())try {
        if(!tag.address || (tag.group!="nhdt" && tag.group!="hudg" && tag.group!="mulg"))continue;
        const Bytes root(cache.tag_payload(tag.handle()));
        if(tag.group=="nhdt" && root.size()>=0x28) {
            for(unsigned text=0;text<2;++text) {
                const unsigned offset=text?0x10:8,stride=text?0x54:0x64;
                const auto count=root.u32(offset);if(count>65536)throw FormatError("Native HUD widget count escapes source owner");
                const auto rows=Bytes(count?cache.read_tag_bytes(root.u32(offset+4),std::size_t(count)*stride):std::span<const std::byte>{});
                for(unsigned i=0;i<count;++i)hud_reference(rows.u32(std::size_t(i)*stride+(text?0x24:0x2c)),
                    {{"group","nhdt"},{"owner",tag.datum},{"widget",i},{"text",bool(text)}});
            }
        } else if(tag.group=="hudg" && root.size()>=0x418) {
            for(unsigned bank=0;bank<2;++bank) {
                const unsigned offset=bank?0x410:0x408,stride=bank?24:36;
                const auto count=root.u32(offset);if(count>64)throw FormatError("Native waypoint count escapes source owner");
                const auto rows=Bytes(count?cache.read_tag_bytes(root.u32(offset+4),std::size_t(count)*stride):std::span<const std::byte>{});
                for(unsigned i=0;i<count;++i)hud_reference(rows.u32(std::size_t(i)*stride+12),
                    {{"group","hudg"},{"owner",tag.datum},{"bank",bank},{"row",i}});
            }
        } else if(tag.group=="mulg" && root.size()>=16 && root.u32(8)) {
            if(root.u32(8)!=1)throw FormatError("Native scoreboard runtime owner is ambiguous");
            const Bytes runtime(cache.read_tag_bytes(root.u32(12),1384));
            for(const unsigned offset:{0x54cu,0x55cu})hud_reference(runtime.u32(offset),
                {{"group","mulg"},{"owner",tag.datum},{"runtime_offset",offset}});
        }
    }catch(const std::exception& error){out.metadata_rejected.push_back({{"kind","hud-typed-reference"},
        {"map",map.at("map")},{"source",map.at("source")},{"datum",tag.datum},{"group",tag.group},{"error",error.what()}});}
    for(const auto& [datum,owners]:hud_shaders)try {
        const auto material=cache.load_material(datum,true,false);
        if(material.passes.empty())throw FormatError("Typed HUD shader lacks original compiled pass metadata");
        for(unsigned index=0;index<material.passes.size();++index) {
            const auto& pass=material.passes[index];
            for(const auto& variant:variants(material,pass))
                out.add_fragment(variant.program,{{"kind","hud-material-pass"},{"family","hud"},{"map",map.at("map")},
                    {"map_sha256",map.at("sha256")},{"source",map.at("source")},{"datum",datum},
                    {"name",material.name},{"template_name",material.template_name},{"pass",index},
                    {"pass_name",pass.name},{"implementation",pass.implementation},{"switches",variant.switches},
                    {"source_variant_assignments",variant.provenance()},{"typed_owners",owners}});
        }
    }catch(const std::exception& error){out.metadata_rejected.push_back({{"kind","hud-material-pass"},
        {"map",map.at("map")},{"source",map.at("source")},{"datum",datum},{"error",error.what()}});}
    map["hud_shader_owners"]=hud_shaders.size();
    map["particle_fragment_census"]=emit_native_particle_fragment_inventory(cache,
        [&](const PixelProgram& program,json alias) {
            alias["map"]=map.at("map");alias["map_sha256"]=map.at("sha256");alias["source"]=map.at("source");
            out.add_fragment(program,alias);
        });
    if(map.at("map")=="mainmenu.map")try {
        const auto program=cache.load_menu_scene_filter();
        out.add_fragment(program,{{"kind","menu-scene-filter"},{"family","bloom"},
            {"map",map.at("map")},{"map_sha256",map.at("sha256")},{"source",map.at("source")},
            {"operation","Cache::load_menu_scene_filter"}});
    }catch(const std::exception& error){out.metadata_rejected.push_back({{"kind","menu-scene-filter"},
        {"map",map.at("map")},{"source",map.at("source")},{"error",error.what()}});}
    for(const auto& tag:cache.tags()) {
        if(tag.group!="shad" || !tag.address || tag.size<8)continue;
        json alias={{"kind","material-pass"},{"map",map.at("map")},{"map_sha256",map.at("sha256")},
            {"source",map.at("source")},{"name",tag.name},{"datum",tag.datum}};
        try {
            const auto material=cache.load_material(tag.datum,false,false);++materials;
            alias["template_name"]=material.template_name;
            if(material.passes.empty()) {
                alias["error"]="Shader metadata has no original admitted pass program";
                alias["rendering_gaps"]=material.rendering_gaps;out.metadata_rejected.push_back(alias);continue;
            }
            for(unsigned index=0;index<material.passes.size();++index) {
                const auto& pass=material.passes[index];
                alias.erase("switches");alias.erase("source_variant_assignments");
                alias["pass"]=index;alias["pass_name"]=pass.name;
                alias["template_pass"]=pass.template_pass;alias["implementation"]=pass.implementation;
                alias["vertex_shader"]=pass.vertex_shader;
                alias["vertex_declaration"]=pass.vertex_declaration?json{{"class_index",pass.vertex_declaration->class_index},
                    {"semantics",pass.vertex_declaration->semantics}}:json(nullptr);
                const auto programs=variants(material,pass);++passes;
                const bool world_inputs=!pass.vertex_declaration || source_world_vertex_declaration_supported(*pass.vertex_declaration);
                if(!pass.vertex_code.empty() && !world_inputs) {
                    auto item=alias;item["declaration_class"]=pass.vertex_declaration->class_index;
                    item["semantics"]=pass.vertex_declaration->semantics;item["native_control_identities"]=programs.size();
                    out.non_world_inputs.push_back(std::move(item));
                }
                for(const auto& variant:programs) {
                    const auto& program=variant.program;
                    alias["switches"]=variant.switches;
                    alias["source_variant_assignments"]=variant.provenance();
                    out.add_fragment(program,alias);
                    if(pass.vertex_code.empty())continue; // Procedural UI/effect geometry owns its own native VS.
                    if(!world_inputs)continue;
                    const auto decoded=decode_vertex_program(pass.vertex_code);
                    bool prt=decoded.relative_constants;
                    if(pass.vertex_declaration)for(const auto semantic:pass.vertex_declaration->semantics)
                        prt=prt || semantic==19 || semantic==20;
                    std::vector<SourceWorldVertexInputTransform> transforms{SourceWorldVertexInputTransform::Preserve};
                    if(pass.vertex_declaration) {
                        const auto& semantics=pass.vertex_declaration->semantics;
                        if(std::find(semantics.begin(),semantics.end(),4)!=semantics.end())
                            transforms.push_back(SourceWorldVertexInputTransform::NormalizeSkinNormal);
                        if(semantics==std::vector<std::uint16_t>{0,4,5,6})
                            transforms.push_back(SourceWorldVertexInputTransform::OffsetLocalPosition);
                    }
                    for(const auto transform:transforms)for(const bool extended:{false,true}) {
                        if(prt && !extended)continue;
                        out.add_program(pass.vertex_code,program,pass.vertex_declaration,transform,extended,0,alias);
                    }
                }
            }
        }catch(const std::exception& error){alias["error"]=error.what();out.metadata_rejected.push_back(std::move(alias));}
    }
    // Sprite particles embed shader parameters in prt3 rather than referencing
    // a shad owner. Their complete control words still come from a resident
    // original spas implementation. Inventory those directly, independently
    // of texture loading and independently of the world material binder.
    for(const auto& tag:cache.tags()) {
        if(tag.group!="spas" || !tag.address || tag.size<36 ||
           (cache.is_mcc() && !cache.is_preserved_native_shader({tag.datum})))continue;
        json alias={{"kind","raw-pass-implementation"},{"group","spas"},{"map",map.at("map")},
            {"map_sha256",map.at("sha256")},{"source",map.at("source")},{"name",tag.name},{"datum",tag.datum}};
        try {
            // Xbox caches deduplicate processed blocks across complete spas
            // allocations. Cache::load_material follows these actual cache
            // addresses too; confining them to the current tag's payload
            // incorrectly rejects legitimate lightmap/illumination aliases.
            struct Block {std::uint32_t count{},address{};std::span<const std::byte> bytes;};
            const auto block=[&](const Bytes& parent,std::size_t offset,std::size_t stride) {
                Block result;result.count=parent.u32(offset);result.address=parent.u32(offset+4);
                if(result.count>65536 || (result.count && !result.address))
                    throw FormatError("Native raw pass block has invalid source ownership");
                if(result.count)result.bytes=cache.read_tag_bytes(result.address,std::size_t(result.count)*stride);
                return result;
            };
            const auto processed=block(Bytes(cache.tag_payload(tag.handle())),0x1c,88);
            if(!processed.count)continue;
            const Bytes definition(processed.bytes);
            const auto implementations=block(definition,0,306);
            const auto fragments=block(definition,32,4);
            const auto permutations=block(definition,40,6);
            const auto combiners=block(definition,48,32);
            const Bytes bytes(implementations.bytes),fragment_bytes(fragments.bytes),
                permutation_bytes(permutations.bytes),combiner_bytes(combiners.bytes);
            for(unsigned implementation=0;implementation<implementations.count;++implementation) {
                const auto at=implementation*306;
                MaterialPass pass;pass.name=tag.name;pass.implementation=implementation;
                for(unsigned word=0;word<60;++word)pass.definition.words[word]=bytes.u32(at+6+word*4);
                const ShaderRange range{bytes.u16(at+246)};
                if(range.first()>fragments.count || range.count()>fragments.count-range.first())
                    throw FormatError("Native raw pass fragment range escapes original owner");
                for(unsigned i=0;i<range.count();++i) {
                    const auto row=(range.first()+i)*4;
                    pass.fragments.push_back({std::bit_cast<std::int16_t>(fragment_bytes.u16(row)),{fragment_bytes.u16(row+2)}});
                }
                for(unsigned i=0;i<permutations.count;++i) {
                    const auto row=i*6;
                    pass.permutations.push_back({std::bit_cast<std::int16_t>(permutation_bytes.u16(row)),permutation_bytes.u16(row+2),{permutation_bytes.u16(row+4)}});
                }
                for(unsigned i=0;i<combiners.count;++i) {
                    ShaderCombiner combiner;
                    for(unsigned word=0;word<8;++word)combiner.words[word]=combiner_bytes.u32(i*32+word*4);
                    pass.combiners.push_back(combiner);
                }
                alias["implementation"]=implementation;
                if(const auto* vertex=cache.find_tag({bytes.u32(at+256)},tag_groups::vertex_shader)) {
                    alias["vertex_shader"]=vertex->name;
                    alias["vertex_datum"]=vertex->datum;
                    alias["vertex_class_count"]=Bytes(cache.tag_payload({vertex->datum})).u32(4);
                }
                for(const auto& variant:variants(Material{},pass)) {
                    alias["switches"]=variant.switches;alias["source_variant_assignments"]=variant.provenance();
                    out.add_fragment(variant.program,alias);
                }
            }
        }catch(const std::exception& error){alias["error"]=error.what();out.metadata_rejected.push_back(std::move(alias));}
    }
    for(const auto& tag:cache.tags()) {
        if(tag.group!="vrtx" || !tag.address || tag.size<12)continue;
        json alias={{"kind","raw-vertex-class"},{"map",map.at("map")},{"map_sha256",map.at("sha256")},
            {"source",map.at("source")},{"name",tag.name},{"datum",tag.datum}};
        if(cache.is_mcc() && !cache.is_preserved_native_shader({tag.datum})) {
            alias["reason"]="Unpreserved PC vertex metadata is not an original Xbox shader program";
            alias["inventory_role"]="non-native-input";out.non_world_inputs.push_back(std::move(alias));continue;
        }
        try {
            const Bytes payload(cache.tag_payload({tag.datum}));
            if(payload.u16(0)!=1)continue; // Original-format programs only.
            const auto count=payload.u32(4);
            if(count>65536)throw FormatError("Native catalog vertex class count escapes source extent");
            for(unsigned classification=0;classification<count;++classification) {
                alias["classification"]=classification;
                const auto program=cache.load_vertex_shader_code(tag.datum,classification);
                if(!program.empty())out.add_cpu_program(program,alias);
            }
        }catch(const std::exception& error){alias["error"]=error.what();out.metadata_rejected.push_back(std::move(alias));}
    }
    map["native_shader_census"]={{"shader_metadata_owners",materials},{"source_passes",passes},
        {"world_kernel_identities_added",out.entries.size()-before_entries},
        {"fragment_control_identities_added",out.fragment_programs.size()-before_fragments},
        {"raw_vertex_identities_added",out.cpu_vertex_programs.size()-before_cpu},
        {"metadata_rejections",out.metadata_rejected.size()-before_errors}};
    std::cout<<map.at("map")<<": "<<materials<<" metadata owners, "<<passes<<" complete passes\n";
}

void export_world_adapters(Registry& out) {
    out.add_fragment({},{{"kind","native-fixed-procedural"},{"family","world-fixed"},
        {"operation","Original no-program world material branch"}});
    // This adaptation belongs to the world/pStencil conduit. Its base
    // controls need an admitted complete world tuple, rather than merely an
    // unrelated raw HUD/particle spas record. Retain only the small unique
    // control set while appending adapters; do not copy all map aliases.
    std::set<std::array<std::uint32_t,60>> authored;
    for(const auto& source:out.entries)authored.insert(source.at("controls").get<std::array<std::uint32_t,60>>());
    for(const auto& controls:authored) {
        PixelProgram program;program.words=controls;
        for(unsigned stage=0;stage<8;++stage)try {
            const auto changed=pstencil::neutral_detail_response(program,stage);
            out.add_fragment(changed,{{"kind","native-source-adaptation"},{"family","world"},
                {"operation","pstencil::neutral_detail_response"},{"stage",stage},{"source_controls",program.words}});
        }catch(const FormatError&){}
    }
    out.add_fragment(pstencil::adapt_environment_response({}),{{"kind","native-source-adaptation"},
        {"family","world"},{"operation","pstencil::adapt_environment_response"}});
    out.add_fragment(pstencil::normal_accumulation_2003(),{{"kind","native-source-adaptation"},
        {"family","pstencil-normal"},{"operation","pstencil::normal_accumulation_2003"}});
    out.add_fragment(pstencil::texture_accumulation_2003(),{{"kind","native-source-adaptation"},
        {"family","pstencil-texture"},{"operation","pstencil::texture_accumulation_2003"}});
    for(unsigned geometry=0;geometry<=3;++geometry)
        for(unsigned diffuse=0;diffuse<=3;++diffuse)for(unsigned specular=0;specular<=3;++specular)
            for(int quality=-1;quality<=2;++quality)for(const bool dull:{false,true}) {
                const auto program=pstencil::light_utility_2003(geometry,diffuse,specular,quality,dull);
                out.add_fragment(program,{{"kind","native-source-constructor"},
                    {"family","pstencil-light-utility"},{"also_family","pstencil-normal-utility"},
                    {"operation","pstencil::light_utility_2003"},{"geometry",geometry},{"diffuse",diffuse},
                    {"specular",specular},{"quality",quality},{"dull",dull}});
            }
    for(unsigned geometry=0;geometry<=3;++geometry)for(unsigned mask=0;mask<=3;++mask)
        for(unsigned shift=0;shift<=2;++shift)
            out.add_fragment(pstencil::dependent_light_2003(geometry,mask,shift,0,0),
                {{"kind","native-source-constructor"},{"family","pstencil-dependent-light"},
                    {"operation","pstencil::dependent_light_2003"},{"geometry",geometry},{"mask",mask},{"shift",shift}});
}
void export_map(Registry& out,const std::filesystem::path& path,const std::filesystem::path& prepared,
                const std::shared_ptr<const OriginalData>& original,bool all_materials,
                const std::filesystem::path& shared={},std::string source={}) {
    const bool imported=!prepared.empty();
    std::cout<<"Enumerating "<<path.string()<<std::endl;
    const auto map_hash=sha256(path);
    if(source.empty())source=imported?"prepared-mcc":"original";
    json map={{"map",path.filename().string()},{"path",path.string()},{"sha256",map_hash},{"bytes",std::filesystem::file_size(path)},
        {"source",source},{"prepared_root",prepared.string()},{"shared_directory",shared.empty()?path.parent_path().string():shared.string()},
        {"sky_materials",0},{"water_materials",0},
        {"window_materials",0}};
    try {
        Cache cache(path,shared.empty()?path.parent_path():shared,original,prepared);
        if(all_materials)export_all_materials(out,cache,map);
        if(cache.scenario_tag().is_null() || cache.bsps().empty()){map["status"]="no-scenario-bsp";out.maps.push_back(map);return;}
        std::set<std::pair<unsigned,int>> skies;
        for(unsigned bsp=0;bsp<cache.bsps().size();++bsp) {
            const auto atmosphere=cache.load_bsp_atmosphere(bsp);
            for(const auto& cluster:atmosphere.clusters) {
                int sky=cluster.visible_sky;
                if(sky<0 && (atmosphere.scenario_flags&2u) && !atmosphere.skies.empty())sky=0;
                if(sky>=0)skies.emplace(bsp,sky);
            }
        }
        for(const auto& [bsp,sky]:skies) {
            const auto definition=cache.load_sky(unsigned(sky));if(!definition)continue;
            auto model=cache.load_model(definition->model,true);prepare_forward_materials(cache,model.mesh);
            std::set<unsigned> used;
            for(const auto& vertex:model.mesh.vertices)used.insert(vertex.material);
            for(auto index:used) {
                if(index>=model.mesh.materials.size())throw FormatError("Sky geometry has a foreign material ordinal");
                const auto& material=model.mesh.materials[index];
                json alias={{"kind","sky"},{"map",path.filename().string()},{"map_sha256",map_hash},
                    {"source",source},{"bsp",bsp},{"sky",sky},
                    {"name",material.name},{"datum",material.source_shader_datum}};
                try {
                    const auto layer=source_material_layer(material);
                    if(!layer || *layer!=15)throw FormatError("Sky lacks authored dedicated layer15");
                    out.add(material,alias);map["sky_materials"]=map["sky_materials"].get<unsigned>()+1;
                } catch(const std::exception& error) {
                    if(!imported)throw;
                    alias["error"]=error.what();out.rejected.push_back(std::move(alias));
                }
            }
        }
        if(path.stem()=="elongation") {
            // The actual BSP's authored pane owners are part of the complete
            // window flow, independently of the dedicated sky model. Compile
            // their actual material tuple; no shader-name donor lookup occurs.
            const auto bsp=cache.load_bsp(0,true);
            for(unsigned slot:{24u,27u,30u,35u,52u,55u,68u,75u}) {
                const auto& material=bsp.materials.at(slot);
                if(!material.forward_pass || material.forward_raster.depth_write!=0 ||
                   material.source_shader_datum==0xffffffffu ||
                   (imported && !cache.is_preserved_native_shader({material.source_shader_datum})))
                    throw FormatError("Actual Elongation window lacks its authenticated non-writing native pass");
                out.add(material,{{"kind","window-glass"},{"map",path.filename().string()},
                    {"map_sha256",map_hash},{"source",source},
                    {"bsp",0},{"slot",slot},{"name",material.name},{"datum",material.source_shader_datum}});
                map["window_materials"]=map["window_materials"].get<unsigned>()+1;
            }
        }
        for(const auto& tag:cache.tags()) {
            if(tag.group!="shad" || !tag.address || tag.size<8)continue;
            const auto* stem=cache.find_tag({Bytes(cache.tag_payload(tag.handle())).u32(4)},TagGroup::from_string("stem"));
            if(!stem || (!stem->name.starts_with("shaders\\shader_templates\\water\\") &&
                stem->name!="shaders\\shader_templates\\transparent\\waves"))continue;
            json alias={{"kind","water"},{"map",path.filename().string()},{"map_sha256",map_hash},
                {"source",source},{"name",tag.name},{"datum",tag.datum}};
            try {
                const auto authored=cache.load_material(tag.datum);Material material;
                const auto bind=[&](const auto& prepare){auto copy=authored;if(!prepare(copy))return false;material=std::move(copy);return true;};
                if(!(bind([&](auto& value){return prepare_water_material(cache,value,false);}) ||
                     bind([&](auto& value){return prepare_water_material(cache,value,true);}) ||
                     bind([&](auto& value){return prepare_world_material(cache,value,true);}) ||
                     bind([&](auto& value){return prepare_world_material(cache,value,false);})))
                    throw FormatError("Water has no admitted actual source pass");
                out.add(material,alias);map["water_materials"]=map["water_materials"].get<unsigned>()+1;
            } catch(const std::exception& error) {
                if(!imported)throw;
                alias["error"]=error.what();out.rejected.push_back(std::move(alias));
            }
        }
        map["status"]="enumerated";
    }catch(const std::exception& error){map["status"]="failed";map["error"]=error.what();out.maps.push_back(map);if(!imported)throw;return;}
    out.maps.push_back(map);
    std::cout<<path.filename().string()<<": "<<map["sky_materials"]<<" sky, "<<map["water_materials"]
        <<" water and "<<map["window_materials"]<<" window material bindings\n";
}
}
int main(int argc,char** argv)try {
    std::filesystem::path root,prepared,output,image;
    bool all_materials=false;
    for(int i=1;i<argc;++i){const std::string option=argv[i];
        if(option=="--all-materials" && !all_materials){all_materials=true;continue;}
        if(i+1>=argc)throw FormatError("Missing export option value");
        auto value=std::filesystem::path(argv[++i]);
        if(option=="--root" && root.empty())root=value;
        else if(option=="--prepared-root" && prepared.empty())prepared=value;
        else if(option=="--output" && output.empty())output=value;
        else if(option=="--xbe" && image.empty())image=value;
        else throw FormatError("Unknown or duplicate export option: "+option);
    }
    if(root.empty() || output.empty())throw FormatError("usage: export_native_world_kernels --root project --output registry.json [--xbe TU1.5.xbe] [--prepared-root cache]");
    std::shared_ptr<const OriginalData> original;
    if(!image.empty()){original=OriginalData::load_xbe(image);if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Offline source image must be authenticated TU1.5");}
    Registry registry;json inventories=json::array();std::set<std::filesystem::path> exported;
    for(const auto& directory:{root/"assets/disc/maps",root/"assets/map-pack/maps"}){
        std::vector<std::filesystem::path> paths;
        for(const auto& item:std::filesystem::directory_iterator(directory))if(item.is_regular_file()&&item.path().extension()==".map")paths.push_back(item.path());
        std::sort(paths.begin(),paths.end());for(const auto& path:paths) {
            if(!exported.insert(std::filesystem::weakly_canonical(path)).second)continue;
            export_map(registry,path,{},original,all_materials,root/"assets/disc/maps",
                directory==root/"assets/disc/maps"?"original-disc":"original-map-pack");
        }
    }
    if(!prepared.empty()){
        for(const auto& reference:original_sky_reference_inputs(root,prepared))
            if(exported.insert(std::filesystem::weakly_canonical(reference.path)).second)
                export_map(registry,reference.path,{},original,all_materials,root/"assets/disc/maps","original-preserved");
        const auto owned=[&](const std::filesystem::path& owner,std::string source) {
            const auto index_path=owner/"native-shaders/map-owners.json";
            const auto index=sky_reference_detail::read_json(index_path);
            if(index.at("schema")!="H2_MCC_NATIVE_MAP_OWNERS_V1")throw FormatError("Native compiler map owner has another schema");
            std::set<std::filesystem::path> declared,actual;
            std::vector<std::pair<std::filesystem::path,std::filesystem::path>> inputs;
            const auto relative=[](const json& value,bool is_map) {
                const std::filesystem::path path=value.get<std::string>();
                if(path.empty() || path.is_absolute() || path!=path.lexically_normal() || (is_map && path.extension()!=".map"))
                    throw FormatError("Native compiler input path is not a normalized owned relative path");
                for(const auto& component:path)if(component=="..")throw FormatError("Native compiler input escapes its declared owner");
                return path;
            };
            for(const auto& row:index.at("maps")) {
                const auto path=relative(row.at("map"),true),shared=relative(row.at("shared_directory"),false);
                if(!declared.insert(path).second)throw FormatError("Native compiler map owner is duplicated");
                inputs.push_back({path,shared});
            }
            for(auto iterator=std::filesystem::recursive_directory_iterator(owner);iterator!=std::filesystem::recursive_directory_iterator{};++iterator) {
                const auto path=iterator->path().lexically_relative(owner);
                if(source=="prepared-mcc" && iterator->is_directory() && path=="custom-maps") {iterator.disable_recursion_pending();continue;}
                if(iterator->is_regular_file() && path.extension()==".map")actual.insert(path);
            }
            json item={{"prepared_root",owner.string()},{"source",source},{"owner_index_sha256",sha256(index_path)},
                {"declared_maps",declared.size()},{"actual_maps",actual.size()},{"complete",!declared.empty() && actual==declared}};
            inventories.push_back(item);
            if(!item.at("complete").get<bool>())throw FormatError("Native compiler complete map inventory has missing or unlisted files");
            std::sort(inputs.begin(),inputs.end());
            for(const auto& [path,shared]:inputs) {
                if(!exported.insert(std::filesystem::weakly_canonical(owner/path)).second)
                    throw FormatError("Native compiler input has duplicate declared owners");
                export_map(registry,owner/path,owner,{},all_materials,owner/shared,source);
            }
        };
        owned(prepared,"prepared-mcc");
        if(std::filesystem::is_directory(prepared/"custom-maps")) {
            std::vector<std::filesystem::path> custom;
            for(const auto& entry:std::filesystem::directory_iterator(prepared/"custom-maps"))if(entry.is_directory())custom.push_back(entry.path());
            std::sort(custom.begin(),custom.end());
            for(const auto& owner:custom) {
                if(std::filesystem::is_regular_file(owner/"native-shaders/map-owners.json"))owned(owner,"prepared-mcc-custom");
                else for(const auto& entry:std::filesystem::recursive_directory_iterator(owner))
                    if(entry.is_regular_file() && entry.path().extension()==".map")
                        throw FormatError("Native compiler custom map has no declared owner index");
            }
        }
    }
    if(all_materials)export_world_adapters(registry);
    if(registry.entries.empty())throw FormatError("No actual sky/water native kernel programs were found");
    registry.finish_evidence();
    const auto entries=registry.entries.size(),rejected=registry.rejected.size();
    const json report={{"schema","HALO2_NATIVE_WORLD_KERNEL_PROGRAMS_V1"},{"abi",1},{"target","vulkan1.3"},
        {"source_authority","Original authored shader programs; original executable is an offline reference only"},
        {"dynamic_color_words",{10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,43,44}},
        {"entries",std::move(registry.entries)},{"maps",std::move(registry.maps)},{"rejected",std::move(registry.rejected)},
        {"fragment_programs",std::move(registry.fragment_programs)},{"all_materials",all_materials},
        {"cpu_vertex_programs",std::move(registry.cpu_vertex_programs)},{"prepared_inventories",std::move(inventories)},
        {"metadata_rejected",std::move(registry.metadata_rejected)},{"non_world_inputs",std::move(registry.non_world_inputs)}};
    std::filesystem::create_directories(output.parent_path());std::ofstream file(output);
    if(!file || !(file<<std::setw(2)<<report<<'\n'))throw FormatError("Cannot write native world registry");
    std::cout<<entries<<" exact native kernel identities, "<<rejected<<" explicit unadmitted MCC bindings\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

#pragma once
#include "halo2/cache.hpp"
#include "halo2/native_weather_vertex.hpp"
#include "halo2/source_weather_assets.hpp"
#include "halo2/source_weather_sprite_profile.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

namespace halo2::renderer_test {
namespace weather_inventory_detail {
using json=nlohmann::json;
inline std::string sha256(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> value{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),value.data(),&size,EVP_sha256(),nullptr)!=1||size!=value.size())
        throw FormatError("Cannot hash complete weather source payload");
    std::ostringstream text;text<<std::hex<<std::setfill('0');
    for(auto byte:value)text<<std::setw(2)<<unsigned(byte);
    return text.str();
}
inline json tag_context(const Cache& cache,const Tag& tag) {
    return {{"datum",tag.datum},{"group",tag.group},{"name",tag.name},
        {"address",tag.address},{"bytes",tag.size},
        {"preserved_native_allocation",cache.is_preserved_native_shader(tag)}};
}
} // namespace weather_inventory_detail

// Authored metadata admission only. In particular, the decoded cold particle
// arrays are never initialized, published, advanced or submitted as live rows.
// The caller keeps this report even when passed is false and continues its map
// cohort; every discovered owner/palette is attempted independently.
inline nlohmann::json native_renderer_weather_inventory(const Cache& cache) {
    using namespace weather_inventory_detail;
    json out={{"schema","HALO2_NATIVE_RENDERER_WEATHER_INVENTORY_V1"},
        {"passed",true},{"census_passed",true},{"status","passed"},{"errors",json::array()},
        {"required_errors",json::array()},{"census_errors",json::array()},
        {"definitions",json::array()},{"bsp_palettes",json::array()},{"textures",json::array()},
        {"cache",cache.path().string()},{"shared_directory",cache.shared_directory().string()},
        {"scenario_datum",cache.scenario_tag().value},{"globals_datum",cache.globals_tag().value},
        {"mcc",cache.is_mcc()},{"vista",cache.is_vista()},
        {"scope","Typed authored weather/BSP/resource/kernel admission; no live particles, camera, visibility, simulation, residency or GPU draw claim"}};
    const auto error=[&](const std::string& context,const std::string& message,bool required=true) {
        const json row={{"context",context},{"message",message},{"required_for_bsp_clusters",required}};
        out["errors"].push_back(row);out[required?"required_errors":"census_errors"].push_back(row);
        if(required)out["passed"]=false;else out["census_passed"]=false;
    };
    std::set<std::uint32_t> definitions,required_definitions;
    std::map<std::uint32_t,json> references,textures;
    std::size_t directory_tags{},resident_tags{},unresident_tags{},palette_references{},required_palette_references{},particle_systems{},plate_systems{};
    for(const auto& tag:cache.select_tags({.group=tag_groups::weather_system})) {
        ++directory_tags;
        if(tag.address){++resident_tags;definitions.insert(tag.datum);}
        else {++unresident_tags;auto row=tag_context(cache,tag);
            row["status"]="authored-unresident-directory-row";out["definitions"].push_back(std::move(row));}
    }
    for(std::size_t bsp=0;bsp<cache.bsps().size();++bsp) {
        json row={{"bsp",bsp},{"bsp_datum",cache.bsps()[bsp].datum},{"name",cache.bsps()[bsp].name},
            {"status","failed"},{"palette",json::array()}};
        try {
            const auto authored=cache.load_source_weather_bsp(bsp);
            row["cluster_count"]=authored.cluster_palette.size();
            row["cluster_palette_sha256"]=sha256(std::as_bytes(std::span(authored.cluster_palette)));
            row["exclusion_polyhedra"]=authored.polyhedra.size();
            row["unassigned_clusters"]=std::count(authored.cluster_palette.begin(),authored.cluster_palette.end(),std::int16_t(-1));
            for(std::size_t index=0;index<authored.palette.size();++index) {
                const auto& palette=authored.palette[index];
                json item={{"index",index},{"name",palette.name},{"datum",palette.datum},
                    {"authored_sha256",sha256(palette.authored)},
                    {"assigned_clusters",std::count(authored.cluster_palette.begin(),authored.cluster_palette.end(),std::int16_t(index))}};
                const bool required=item["assigned_clusters"].get<std::size_t>()!=0;
                item["required_for_bsp_clusters"]=required;
                if(palette.datum==0xffffffffu)item["status"]="authored-null-weather-reference";
                else {
                    ++palette_references;definitions.insert(palette.datum);
                    if(required){++required_palette_references;required_definitions.insert(palette.datum);}
                    if(!references.contains(palette.datum))references[palette.datum]=json::array();
                    references[palette.datum].push_back({{"bsp",bsp},{"palette",index},{"assigned_clusters",item["assigned_clusters"]}});
                    const auto* tag=cache.query_tag({.handle=TagHandle{palette.datum},.group=tag_groups::weather_system,.require_address=true});
                    item["status"]=tag?"typed-resident-weather-reference":"failed";
                    if(!tag)error("bsp:"+std::to_string(bsp)+":palette:"+std::to_string(index),
                        "Authored weather palette has no complete typed resident weat owner",required);
                }
                row["palette"].push_back(std::move(item));
            }
            row["status"]=authored.palette.empty()?"authored-no-weather-palette":"passed";
        }catch(const std::exception& e){row["error"]=e.what();error("bsp:"+std::to_string(bsp),e.what());}
        out["bsp_palettes"].push_back(std::move(row));
    }
    const auto texture=[&](std::uint32_t datum) -> const json& {
        if(const auto found=textures.find(datum);found!=textures.end())return found->second;
        json row={{"datum",datum},{"frame",0},{"status","failed"}};
        try {
            const auto* tag=cache.query_tag({.handle=TagHandle{datum},.group=tag_groups::bitmap,.require_address=true});
            if(!tag)throw FormatError("Weather texture has no typed resident bitm owner");
            row.update(tag_context(cache,*tag));row["bitmap_count"]=cache.bitmap_count(datum);
            // This is the same actual subresource used by the live weather
            // texture owner, not a synthetic mip chain or fallback binding.
            const auto image=cache.load_bitmap(datum,0);
            row["width"]=image.width;row["height"]=image.height;row["depth"]=image.depth;
            row["layers"]=image.layers;row["format"]=image.format;row["mips"]=image.mips.size();
            row["decoded_bytes"]=image.pixels.size();row["decoded_sha256"]=sha256(image.pixels);
            row["point_binding_admitted"]=source_weather_sprite_resource_profile(image.width,image.height,
                unsigned(image.mips.size()),image.format,image.layers,image.depth);
            row["plate_layer_binding_admitted"]=image.format==16&&image.layers==1&&!image.depth;
            row["status"]="passed";
        }catch(const std::exception& e){row["error"]=e.what();}
        return textures.emplace(datum,std::move(row)).first->second;
    };
    for(const auto datum:definitions) {
        const bool required=required_definitions.contains(datum);
        const auto previous_errors=out["errors"].size();
        json row={{"datum",datum},{"status","failed"},{"particles",json::array()},{"plates",json::array()},
            {"required_for_bsp_clusters",required},
            {"programs",json::array()},{"bsp_references",references.contains(datum)?references.at(datum):json::array()}};
        try {
            const auto* tag=cache.query_tag({.handle=TagHandle{datum},.group=tag_groups::weather_system,.require_address=true});
            if(tag)row.update(tag_context(cache,*tag));
            row["metadata_decoder"]="Cache::load_source_weather_definition";
            // Imported ownership is provenance, not proof that an unmarked
            // weather layout is incompatible. The actual decoder validates
            // the root, blocks, cold resources and global program owners.
            // Unsupported MCC metadata is a decoder failure, never absence.
            const auto definition=cache.load_source_weather_definition(datum);
            row["original_weather_decoder_admitted"]=true;
            row["authored_sha256"]=sha256(definition.authored);
            row["particle_system_count"]=definition.particles.size();row["plate_count"]=definition.plates.size();
            particle_systems+=definition.particles.size();plate_systems+=definition.plates.size();
            for(unsigned slot=0;slot<2;++slot) {
                const bool needed=slot==0?!definition.plates.empty():!definition.particles.empty();
                json program={{"global_slot",21+slot},{"classification",0},{"datum",definition.vertex_datums[slot]},
                    {"bytes",definition.vertex_programs[slot].size()},{"sha256",sha256(definition.vertex_programs[slot])},
                    {"needed_for_authored_producers",needed},{"status","authored-unused-producer"}};
                if(const auto* vertex=cache.query_tag({.handle=TagHandle{definition.vertex_datums[slot]},.group=tag_groups::vertex_shader,.require_address=true}))
                    program["source_owner"]=tag_context(cache,*vertex);
                if(needed)try {
                    if(slot==0)native_weather::admit_plate(definition.vertex_programs[slot]);
                    else native_weather::admit_point(definition.vertex_programs[slot]);
                    program["status"]="complete-native-kernel-identity-admitted";
                }catch(const std::exception& e){program["status"]="failed";program["error"]=e.what();error("weat:"+std::to_string(datum)+":VS"+std::to_string(21+slot),e.what(),required);}
                row["programs"].push_back(std::move(program));
            }
            for(std::size_t index=0;index<definition.particles.size();++index) {
                const auto& particles=definition.particles[index];const bool rain=particles.type==2;
                const auto pixel=source_weather_pixel_program(rain);
                json item={{"index",index},{"type",particles.type},{"native_path",rain?"rain-line":"point-sprite"},
                    {"authored_sha256",sha256(particles.authored)},{"maximum_particles",particles.maximum_particles},
                    {"cold_render_rows",particles.render.size()},{"cold_state_rows",particles.state.size()},
                    {"resource_storage_bytes",particles.resource_storage.size()},{"resource_storage_sha256",sha256(particles.resource_storage)},
                    {"resource_descriptors",particles.resources.size()},{"sprite_datum",particles.sprites},
                    {"fragment_program_sha256",sha256(std::as_bytes(std::span(pixel.words)))},
                    {"sprite_consumed_by_path",!rain},{"line_width",particles.line_width}};
                if(!rain) {
                    const auto& binding=texture(particles.sprites);
                    item["native_texture_binding_admitted"]=binding.value("point_binding_admitted",false);
                    if(!item["native_texture_binding_admitted"].get<bool>())error("weat:"+std::to_string(datum)+":particle:"+std::to_string(index),
                        binding.value("error",std::string("Authored point sprite requires 2D BC3: 32x32/four mips or 64x64/five mips")),required);
                }
                row["particles"].push_back(std::move(item));
            }
            for(std::size_t index=0;index<definition.plates.size();++index) {
                const auto& plate=definition.plates[index];
                json item={{"index",index},{"authored_sha256",sha256(plate.authored)},{"texture_datums",plate.textures},
                    {"scene_depth_alias","Current caller-owned D24 alias; not manufactured by metadata inventory"},
                    {"native_texture_bindings_admitted",true}};
                for(unsigned layer=0;layer<3;++layer) {
                    const auto& binding=texture(plate.textures[layer]);
                    if(!binding.value("plate_layer_binding_admitted",false)) {
                        item["native_texture_bindings_admitted"]=false;error("weat:"+std::to_string(datum)+":plate:"+std::to_string(index)+":layer:"+std::to_string(layer),
                            binding.value("error",std::string("Authored weather plate layer lacks the admitted2D/BC3 source profile")),required);
                    }
                }
                row["plates"].push_back(std::move(item));
            }
            row["status"]=previous_errors==out["errors"].size()?"decoded-and-native-identity-checked":"failed";
        }catch(const std::exception& e){row["error"]=e.what();error("weat:"+std::to_string(datum),e.what(),required);}
        row["passed"]=previous_errors==out["errors"].size();
        out["definitions"].push_back(std::move(row));
    }
    for(auto& [datum,row]:textures)out["textures"].push_back(std::move(row));
    out["directory_weather_tags"]=directory_tags;out["resident_weather_tags"]=resident_tags;
    out["unresident_weather_tags"]=unresident_tags;out["palette_references"]=palette_references;
    out["required_palette_references"]=required_palette_references;out["required_definitions"]=required_definitions.size();
    out["decoded_particle_systems"]=particle_systems;out["decoded_plates"]=plate_systems;
    if(!out["passed"].get<bool>())out["status"]="failed";
    else if(!directory_tags&&!palette_references)out["status"]="authored-no-weather";
    else if(!resident_tags&&!palette_references)out["status"]="authored-no-resident-weather";
    else if(required_definitions.empty())out["status"]="authored-no-cluster-weather";
    return out;
}
} // namespace halo2::renderer_test

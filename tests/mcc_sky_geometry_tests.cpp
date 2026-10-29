#include "halo2/mcc_sky_geometry.hpp"
#include "halo2/sky_input_identity.hpp"
#include "halo2/sky_reference_inventory.hpp"
#include "halo2/appearance.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <memory>

using namespace halo2;using Json=nlohmann::json;namespace fs=std::filesystem;
namespace {
void need(bool value,const char* message){if(!value)throw FormatError(message);}
template<class F>void rejects(F operation,const char* message) {
    try {operation();}catch(const FormatError&){return;}throw FormatError(message);
}
void codec() {
    RenderModel source;source.checksum=0xfedcba98;source.animation=0x12345678;
    ModelBone bone;bone.name=0x543210ff;bone.parent=-1;bone.bind.translation={-0.f,1.25f,-27.5f};
    bone.bind.rotation={-0.f,.125f,-.25f,1};bone.bind.scale=-0.f;bone.inverse_scale=2;
    for(unsigned i=0;i<bone.inverse.size();++i)bone.inverse[i]=float(i)-6.5f;source.bones.push_back(bone);
    source.compound_nodes.push_back({{0,255,3,17},{-0.f,.5f,.125f}});
    Vertex v;v.position={-0.f,4,-7};v.color={.1f,-0.f,.4f};v.uv={-0.f,.75f};v.material=3;v.lightmap_uv={-.25f,-0.f};
    v.normal={.5f,-0.f,-1};v.tangent={1,0,.25f};v.binormal={-0.f,-.5f,1};v.incident={-.75f,0,1};
    v.lightmap=123;v.bsp_cluster=-7;v.prt_cluster=-0.f;v.prt_weights={.25f,-0.f,-.5f,1};
    v.source_section=0xfffedcba;v.source_part=0x789abcde;v.source_stipple_level=7;v.source_local_position={-0.f,-3,.25f};v.source_color_alpha=-0.f;
    v.source_part_type=5;v.source_part_flags=0xa55a;
    source.mesh.vertices={v,v,v};source.skin.resize(3,{{0,3,128,65535},{-0.f,.25f,.5f,.25f}});
    source.vertex_origins.resize(3,{65535,123,0xffff,6});
    const auto encoded=encode_mcc_sky_geometry(source);auto decoded=decode_mcc_sky_geometry(encoded);
    need(encoded==encode_mcc_sky_geometry(decoded),"Sky geometry codec changed a literal attribute, node, weight or owner word");
    need(std::bit_cast<std::uint32_t>(decoded.mesh.vertices[0].position.x)==0x80000000u&&
        std::bit_cast<std::uint32_t>(decoded.skin[0].weights[0])==0x80000000u&&
        decoded.mesh.vertices[0].source_part_flags==0xa55a&&decoded.vertex_origins[0].part==0xffff,
        "Sky geometry codec lost signed zero or raw source part provenance");
    for(std::size_t n=0;n<encoded.size();n+=4)rejects([&]{(void)decode_mcc_sky_geometry(std::span(encoded).first(n));},"Truncated sky geometry was admitted");
    auto trailing=encoded;trailing.push_back(std::byte{});rejects([&]{(void)decode_mcc_sky_geometry(trailing);},"Trailing sky geometry was admitted");
    auto bad=encoded;bad[0]=std::byte{};rejects([&]{(void)decode_mcc_sky_geometry(bad);},"Another geometry schema was admitted");
    source.mesh.vertices[0].uv.x=std::numeric_limits<float>::infinity();
    rejects([&]{(void)decode_mcc_sky_geometry(encode_mcc_sky_geometry(source));},"Nonfinite original sky attribute was admitted");
}
fs::path maps(const fs::path& root) {
    for(const auto& dir:{root/"maps",root/"content/halo2/maps",root/"content/halo2/h2_maps_win64_dx11",root/"halo2/maps",root/"halo2/h2_maps_win64_dx11",root/"h2_maps_win64_dx11"})if(fs::is_directory(dir))return dir;
    return root;
}
RenderModel input(const Cache& cache,std::uint32_t model,bool textures) {
    RenderModel out;out.name=cache.find_tag({model})->name;const Bytes root(cache.tag_payload({model}));const auto count=root.u32(0x60),address=root.u32(0x64);
    out.mesh.materials.resize(count+1);
    if(textures)for(unsigned i=0;i<count;++i)try {
        const auto handle=Bytes(cache.read_tag_bytes(address+i*32,32)).u32(12);out.mesh.materials[i]=cache.load_material(handle);
        prepare_model_environment(cache,out.mesh.materials[i]);
    }catch(const FormatError&) {}
    return out;
}
}
int main(int argc,char** argv) {
    try {
        codec();need(argc==3,"Sky source projection requires repository and prepared MCC roots");
        std::string selected=argv[2];if(selected.empty())if(const char* value=std::getenv("HALO2_SKY_MCC_ROOT"))selected=value;
        if(selected.empty()){std::cout<<"codec signed-zero and malformed-record controls passed; prepared sky assets unavailable\n";return 77;}
        const fs::path root=argv[1],prepared=selected;need(fs::is_directory(prepared),"Explicit prepared sky geometry root is unavailable");
        const auto target_dir=maps(prepared);const auto catalog=Json::parse(mcc_sky_geometry_profiles());
        need(!catalog.at("profiles").empty(),"Finite sky source catalog contains no authenticated profiles");
        unsigned profiles{},lods{},negative{};std::map<fs::path,std::string> source_hashes;
        for(const auto& profile:catalog.at("profiles")) {
            auto original=root/"assets/disc/maps"/profile.at("original_map").get<std::string>();if(!fs::is_regular_file(original))original=root/"assets/map-pack/maps"/profile.at("original_map").get<std::string>();
            if(!fs::is_regular_file(original)&&profile.contains("original_path"))original=profile.at("original_path").get<std::string>();
            if(profile.contains("original_file_sha256")) {
                if(!source_hashes.contains(original))source_hashes[original]=sky_reference_detail::file_sha256(original);
                need(source_hashes.at(original)==profile.at("original_file_sha256").get<std::string>(),"Complete original sky map source digest differs");
            }
            const auto target_file=profile.value("mcc_map",original.filename().string());
            const bool pc_geometry=profile.value("geometry_source",std::string{"originalXbox"})=="originalPC11081";
            Cache source(original,pc_geometry?original.parent_path():root/"assets/disc/maps");
            Cache target(target_dir/target_file,target_dir,{},prepared);
            if(pc_geometry)need(source.is_vista()&&!source.is_mcc()&&source.build()=="11081.07.04.30.0934.main",
                "PC-only sky geometry reference is not its exact original PC11081 schema");
            if(profile.value("binding_authority",std::string{})=="authenticated-Blastacular-source-pair")
                validate_sky_reference_pair(source,target,{original,fs::path(target_file).stem().string()});
            const auto source_model=profile.at("original_model_datum").get<std::uint32_t>(),target_model=profile.at("mcc_model").get<std::uint32_t>();
            // PC-only references contribute geometry, never D3D shader data.
            const auto expected=source.load_model(source_model,false);const auto actual=target.load_model(target_model,true);
            need(encode_mcc_sky_geometry(expected)==encode_mcc_sky_geometry(actual),"Native preview sky projection differs from original source words");
            need(expected.mesh.materials.size()==actual.mesh.materials.size(),"Canonical source sky material extent differs");
            std::unique_ptr<Cache> material_cache;const Cache* material_owner=&source;
            if(profile.contains("native_material_path")) {
                auto path=root/"assets/disc/maps"/profile.at("native_material_map").get<std::string>();
                if(!fs::is_regular_file(path))path=root/"assets/map-pack/maps"/profile.at("native_material_map").get<std::string>();
                if(!fs::is_regular_file(path))path=profile.at("native_material_path").get<std::string>();
                if(!source_hashes.contains(path))source_hashes[path]=sky_reference_detail::file_sha256(path);
                need(source_hashes.at(path)==profile.at("native_material_file_sha256").get<std::string>(),"Complete Xbox native material source digest differs");
                if(fs::weakly_canonical(path)!=fs::weakly_canonical(original)) {material_cache=std::make_unique<Cache>(path,root/"assets/disc/maps");material_owner=material_cache.get();}
            }
            need(profile.value("native_material_source",std::string{"originalXboxTU15"})=="originalXboxTU15"&&
                !material_owner->is_vista()&&!material_owner->is_mcc(),"Native sky material source must remain the original Xbox authority");
            const auto material_model=profile.value("native_material_model_datum",source_model);
            if(profile.contains("native_material_model_input_sha256"))need(mcc_sky_geometry_input_sha256(*material_owner,material_model)==profile.at("native_material_model_input_sha256").get<std::string>(),
                "Xbox native material model authority differs");
            auto native=input(*material_owner,material_model,true);prepare_forward_materials(*material_owner,native.mesh);
            Mesh actual_materials;actual_materials.materials=actual.mesh.materials;prepare_forward_materials(target,actual_materials);
            for(unsigned slot=0;slot<actual_materials.materials.size();++slot) {
                const auto authority_slot=profile.at("materials").at(slot).value("native_authority_material_index",slot);
                need(authority_slot<native.mesh.materials.size(),"Original sky material authority slot is unavailable");
                const auto expected_semantic=sky_audit::complete_semantic(native.mesh.materials[authority_slot],*material_owner);
                const auto actual_semantic=sky_audit::complete_semantic(actual_materials.materials[slot],target);
                if(sky_audit::semantic_digest(native.mesh.materials[authority_slot],*material_owner)!=
                    sky_audit::semantic_digest(actual_materials.materials[slot],target)) {
                    auto ordinary=material_owner->load_model(material_model,true);prepare_forward_materials(*material_owner,ordinary.mesh);
                    throw FormatError("Native material semantics differ from the independent original Xbox authority: "+
                        Json({{"map",target_file},{"slot",slot},{"authority_slot",authority_slot},
                            {"ordinary_original_matches",authority_slot<ordinary.mesh.materials.size() &&
                                sky_audit::complete_semantic(ordinary.mesh.materials[authority_slot],*material_owner)==actual_semantic},
                            {"differences",Json::diff(expected_semantic,actual_semantic)}}).dump());
                }
            }
            auto restored=input(target,target_model,true);need(restore_mcc_sky_geometry_profile(target,target_model,restored,true,profile.dump()),"Exact sky profile was not admitted");
            need(encode_mcc_sky_geometry(expected)==encode_mcc_sky_geometry(restored),"Explicit native sky projection differs from original source words");
            for(unsigned lod=0;lod<6;++lod) {
                const auto original_sections=profile.at("lods").at(lod).at("original_sections").get<std::vector<unsigned>>();
                const auto mcc_sections=profile.at("lods").at(lod).at("mcc_sections").get<std::vector<unsigned>>();
                const auto wanted=source.load_model_sections(source_model,false,0,lod,original_sections);
                const auto projected=target.load_model_sections(target_model,false,0,lod,mcc_sections);
                need(encode_mcc_sky_geometry(wanted)==encode_mcc_sky_geometry(projected),"Native sky LOD projection differs from original source words");++lods;
            }
            auto changed=profile;changed["input_sha256"]=std::string(64,'0');auto out=input(target,target_model,false);
            need(!restore_mcc_sky_geometry_profile(target,target_model,out,false,changed.dump()),"Wrong complete MCC input identity was admitted");++negative;
            changed=profile;changed["preview"]["sha256"]=std::string(64,'0');out=input(target,target_model,false);
            rejects([&]{restore_mcc_sky_geometry_profile(target,target_model,out,false,changed.dump());},"Mutated original source geometry was admitted");++negative;
            changed=profile;changed["materials"][0]["mcc_index"]=0xfffffffeu;out=input(target,target_model,false);
            rejects([&]{restore_mcc_sky_geometry_profile(target,target_model,out,false,changed.dump());},"Mutated canonical material mapping was admitted");++negative;
            changed=profile;changed["materials"][0]["mcc_semantic_sha256"]=std::string(64,'0');changed["materials"][0]["original_semantic_sha256"]=std::string(64,'0');const auto mapping=changed.at("materials").dump();
            changed["material_mapping_sha256"]=mcc_sky_geometry_sha256(std::as_bytes(std::span(mapping)));out=input(target,target_model,true);
            rejects([&]{restore_mcc_sky_geometry_profile(target,target_model,out,true,changed.dump());},"Mutated actual material semantics were admitted");++negative;
            changed=profile;changed["materials"][0]["mcc_shader"]=0xffffffffu;const auto shader_mapping=changed.at("materials").dump();
            changed["material_mapping_sha256"]=mcc_sky_geometry_sha256(std::as_bytes(std::span(shader_mapping)));out=input(target,target_model,false);
            rejects([&]{restore_mcc_sky_geometry_profile(target,target_model,out,false,changed.dump());},"Mutated actual typed shader owner was admitted");++negative;
            out=input(target,target_model,false);const std::vector<unsigned> bad_sections{0xffffffffu};
            rejects([&]{restore_mcc_sky_geometry_profile(target,target_model,out,false,profile.dump(),5,bad_sections);},"Unauthenticated resident sky section subset was admitted");++negative;
            ++profiles;
        }
        std::cout<<Json({{"profiles",profiles},{"source_exact_lods",lods},{"negative_controls",negative},{"codec_signed_zero_exact",true}}).dump()<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

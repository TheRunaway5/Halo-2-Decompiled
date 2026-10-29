#include "halo2/path_tracing/geometry.hpp"
#include "halo2/path_tracing/scene.hpp"
#include <algorithm>
#include <cfenv>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void need(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F&& function,const char* message) {
    try {function();}catch(const FormatError&){return;}
    throw std::runtime_error(message);
}
}
int main() {
    try {
        std::vector<Vertex> input(4);
        for(auto& vertex:input) {
            vertex.position={1,2,3};vertex.normal={0,0,1};
            vertex.tangent={1,0,0};vertex.binormal={0,1,0};
            vertex.material=27;vertex.lightmap=4;vertex.uv={.25f,.75f};
        }
        auto geometry=std::make_shared<ModelSkinGeometry>();
        geometry->vertices={input[1],input[2]};
        geometry->bindings.resize(2);geometry->source_vertices={0,1};
        geometry->palette.push_back({1,0,0,7,0,1,0,8,0,0,1,9});
        geometry->post_identity_placement=true;
        std::vector<ModelSkinSubmission> submissions{{1,geometry}};
        const auto original=input;
        const auto old_round=std::fegetround();std::fesetround(FE_DOWNWARD);
        const auto output=path_tracing::posed_vertices(input,submissions);
        need(std::fegetround()==FE_DOWNWARD,"Path Tracing geometry changed caller rounding mode");
        std::fesetround(old_round);
        need(output[1].position.x==8 && output[1].position.y==10 && output[1].position.z==12,
            "Path Tracing BVH received local rather than posed positions");
        need(output[2].normal.z==1 && output[1].tangent.x==1 && output[1].binormal.y==1,
            "Path Tracing skin changed the source basis");
        need(output[1].material==27 && output[1].lightmap==4 && output[1].uv.y==.75f,
            "Path Tracing skin lost material metadata");
        need(!std::memcmp(output.data(),input.data(),sizeof(Vertex)) &&
            !std::memcmp(&output[3],&input[3],sizeof(Vertex)),"Path Tracing skin touched undeclared ranges");
        need(!std::memcmp(original.data(),input.data(),input.size()*sizeof(Vertex)),
            "Path Tracing skin mutated its caller's bank");
        const auto copied=path_tracing::posed_vertices(input,{});
        need(!std::memcmp(copied.data(),input.data(),input.size()*sizeof(Vertex)),
            "Already posed geometry changed without a submission");
        submissions.push_back({2,geometry});
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Overlapping deferred owners were accepted");
        submissions={{3,geometry}};
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Out-of-range deferred owner was accepted");
        submissions={{0,{}}};
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Missing deferred owner was accepted");
        submissions={{1,geometry}};geometry->bindings.front().bones[0]=1;
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Out-of-range palette index was accepted");
        std::array<Material,3> materials;
        for(auto& material:materials)
            material.diffuse={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};
        const auto triangle=[](float x,float z,unsigned material) {
            std::vector<Vertex> vertices(3);
            const std::array<Vec3,3> positions{{{x,0,z},{x+1,0,z},{x,1,z}}};
            for(unsigned n=0;n<3;++n) {
                vertices[n].position=positions[n];vertices[n].normal={0,0,1};
                vertices[n].color={1,1,1};vertices[n].material=material;
            }
            return vertices;
        };
        auto primary=triangle(0,0,0);
        const auto legs=triangle(4,2,2),body=triangle(0,2,1);
        primary.insert(primary.end(),legs.begin(),legs.end());const auto primary_before=primary;
        const std::array<path_tracing::MaterialRange,2> replacements{{{1,1},{2,1}}};
        std::vector<Vertex> secondary;
        path_tracing::append_secondary_geometry(secondary,primary,materials,replacements);
        auto missing=path_tracing::build_scene(secondary,materials);path_tracing::Hit hit;
        need(!path_tracing::intersect(missing,{.2f,.2f,.01f},{0,0,1},5,hit),
            "Missing full-body control unexpectedly occluded the source light");
        path_tracing::append_secondary_geometry(secondary,body,materials);
        auto complete=path_tracing::build_scene(secondary,materials);
        need(path_tracing::intersect(complete,{.2f,.2f,.01f},{0,0,1},5,hit) && hit.distance>1.98f && hit.distance<2.f,
            "A local full WORLD body omitted from primary visibility did not cast a ray shadow");
        need(complete.triangles.size()==2,"First-person partial body remained beside its full WORLD caster");
        need(primary.size()==primary_before.size() && !std::memcmp(primary.data(),primary_before.data(),primary.size()*sizeof(Vertex)),
            "Secondary full-body assembly changed first-person primary geometry");
        primary.insert(primary.end(),body.begin(),body.end());secondary.clear();
        path_tracing::append_secondary_geometry(secondary,primary,materials,replacements);
        path_tracing::append_secondary_geometry(secondary,body,materials);
        need(path_tracing::build_scene(secondary,materials).triangles.size()==2,
            "Visible full body was duplicated in the secondary scene");
        auto other_body=triangle(4,2,1);std::vector<Vertex> other_view;
        path_tracing::append_secondary_geometry(other_view,primary,materials,replacements);
        path_tracing::append_secondary_geometry(other_view,other_body,materials);
        const auto other_scene=path_tracing::build_scene(other_view,materials);
        need(!path_tracing::intersect(other_scene,{.2f,.2f,.01f},{0,0,1},5,hit),
            "A second view retained another native player's caster pose");
        const auto valid=secondary;
        for(const auto range:std::array<path_tracing::MaterialRange,3>{{{2,2},{0,0},{0xffffffffu,2}}})
            rejects([&]{path_tracing::append_secondary_geometry(secondary,body,materials,std::span(&range,1));},
                "Invalid body replacement range was accepted");
        auto mixed=body;mixed[2].material=0;
        rejects([&]{path_tracing::append_secondary_geometry(secondary,mixed,materials);},
            "Mixed-material caster was accepted");
        rejects([&]{path_tracing::append_secondary_geometry(secondary,std::span(body).first(2),materials);},
            "Partial body triangle was accepted");
        mixed=body;for(auto& vertex:mixed)vertex.material=3;
        rejects([&]{path_tracing::append_secondary_geometry(secondary,mixed,materials);},
            "Caster outside the resident material bank was accepted");
        need(secondary.size()==valid.size() && !std::memcmp(secondary.data(),valid.data(),valid.size()*sizeof(Vertex)),
            "Rejected secondary geometry partly changed the existing bank");
        // Synthetic ownership seam: the real runtime separately supplies the
        // native body/model/pose provenance. These checks ensure a weapon or
        // visible partial body cannot satisfy its final-BVH body receipt.
        std::array<Material,4> body_materials{materials[0],materials[1],materials[2],materials[0]};
        auto native_torso=triangle(0,2,1),native_legs=triangle(4,2,1),weapon=triangle(8,2,2);
        for(auto& vertex:native_torso)vertex.source_section=17;
        for(auto& vertex:native_legs)vertex.source_section=19;
        for(auto& vertex:weapon)vertex.source_section=25;
        auto native_bank=native_torso;native_bank.insert(native_bank.end(),native_legs.begin(),native_legs.end());
        native_bank.insert(native_bank.end(),weapon.begin(),weapon.end());
        path_tracing::SecondaryBodyOwner body_owner;
        body_owner.root=0x12340007u;body_owner.definition=0xabcd0001u;body_owner.render_model=0xabcd0002u;
        body_owner.source_tick=90;body_owner.lod=4;body_owner.materials={1,1};
        body_owner.first_vertex=0;body_owner.vertex_count=6;body_owner.sections={17,19};
        const std::array<path_tracing::MaterialRange,3> body_replacements{{{1,1},{2,1},{3,1}}};
        const auto counted=[&](const path_tracing::Scene& scene,std::span<const Vertex> bank,
            std::span<const path_tracing::SecondaryBodyOwner> owners) {
            return path_tracing::secondary_body_counts(scene,bank,owners,body_materials,body_replacements);
        };
        const auto only_body=[&](const path_tracing::Scene& scene,std::span<const Vertex> bank) {
            const auto counts=counted(scene,bank,std::span(&body_owner,1));
            need(counts.size()==1 && counts[0].root==body_owner.root && counts[0].definition==body_owner.definition && counts[0].render_model==body_owner.render_model &&
                counts[0].source_tick==90 && counts[0].lod==4,"Final body receipt lost its designated native owner metadata");
            return counts[0].triangles;
        };
        const auto full_scene=path_tracing::build_scene(native_bank,body_materials);
        need(full_scene.triangles.size()==3 && only_body(full_scene,native_bank)==2,
            "Designated full body did not retain its own final-BVH triangles separately from its weapon");
        const auto complete_count=counted(full_scene,native_bank,std::span(&body_owner,1));
        need(complete_count[0].eligible_triangles==2 &&
            complete_count[0].triangles==complete_count[0].eligible_triangles,
            "Complete native body did not satisfy the complete eligible-bank criterion");
        const auto torso_only_count=counted(path_tracing::build_scene(native_torso,body_materials),
            native_bank,std::span(&body_owner,1));
        need(torso_only_count[0].triangles==1 && torso_only_count[0].eligible_triangles==2 &&
            torso_only_count[0].triangles!=torso_only_count[0].eligible_triangles,
            "A same-material exact torso subset falsely satisfied the complete native body criterion");
        need(counted(full_scene,native_bank,{}).empty(),"Ray geometry acquired a body owner without runtime provenance");
        const auto weapon_scene=path_tracing::build_scene(weapon,body_materials);
        need(!weapon_scene.triangles.empty() && only_body(weapon_scene,native_bank)==0,
            "Weapon-only secondary geometry falsely satisfied the full-player-body receipt");
        const auto partial=triangle(0,2,3);
        need(only_body(path_tracing::build_scene(partial,body_materials),native_bank)==0,
            "Visible partial-body geometry falsely satisfied the designated full-body receipt");
        auto displaced=native_bank;
        for(unsigned n=0;n<6;++n)displaced[n].position.x+=.125f;
        need(only_body(path_tracing::build_scene(displaced,body_materials),native_bank)==0,
            "Foreign posed positions with the same material acquired native body ownership");
        auto reversed=native_bank;
        for(unsigned first=0;first<6;first+=3)std::swap(reversed[first],reversed[first+1]);
        need(only_body(path_tracing::build_scene(reversed,body_materials),native_bank)==0,
            "Changed authored triangle order acquired native body ownership");
        auto duplicate=native_bank;duplicate.insert(duplicate.end(),native_bank.begin(),native_bank.begin()+6);
        need(only_body(path_tracing::build_scene(duplicate,body_materials),native_bank)==2,
            "Duplicate final geometry inflated the designated native body bank");
        auto reordered=full_scene;std::reverse(reordered.triangles.begin(),reordered.triangles.end());
        need(only_body(reordered,native_bank)==2,"BVH triangle order changed native body ownership");
        auto faded=native_bank;for(unsigned n=0;n<6;++n)faded[n].source_stipple_level=0;
        need(only_body(path_tracing::build_scene(faded,body_materials),faded)==0,
            "A source-faded body was counted after final BVH filtering");
        auto degenerate=native_bank;
        for(unsigned first=0;first<6;first+=3)degenerate[first+1].position=degenerate[first].position;
        need(only_body(path_tracing::build_scene(degenerate,body_materials),degenerate)==0,
            "A degenerate body was counted after final BVH filtering");
        const auto invalid_body=[&](const path_tracing::SecondaryBodyOwner& invalid) {
            rejects([&]{counted(full_scene,native_bank,std::span(&invalid,1));},
                "Invalid body provenance was admitted into a final-BVH receipt");
        };
        auto invalid=body_owner;invalid.root=~0u;invalid_body(invalid);
        invalid=body_owner;invalid.definition=~0u;invalid_body(invalid);
        invalid=body_owner;invalid.render_model=~0u;invalid_body(invalid);
        invalid=body_owner;invalid.lod=6;invalid_body(invalid);
        invalid=body_owner;invalid.vertex_count=0;invalid_body(invalid);
        invalid=body_owner;invalid.first_vertex=1;invalid_body(invalid);
        invalid=body_owner;invalid.vertex_count=4;invalid_body(invalid);
        invalid=body_owner;invalid.vertex_count=12;invalid_body(invalid);
        invalid=body_owner;invalid.materials={2,1};invalid_body(invalid);
        invalid=body_owner;invalid.materials={~0u,2};invalid_body(invalid);
        invalid=body_owner;invalid.sections={17};invalid_body(invalid);
        invalid=body_owner;invalid.sections={17,17};invalid_body(invalid);
        invalid=body_owner;invalid.sections.clear();invalid_body(invalid);
        const std::array duplicate_roots{body_owner,body_owner};
        rejects([&]{counted(full_scene,native_bank,duplicate_roots);},"Duplicate native body roots were counted");
        auto overlapping=duplicate_roots;overlapping[1].root+=1;
        rejects([&]{counted(full_scene,native_bank,overlapping);},"Overlapping native body banks were counted");
        rejects([&]{path_tracing::secondary_body_counts(full_scene,native_bank,std::span(&body_owner,1),body_materials,{});},
            "A full body without its primary replacement owner was counted");
        std::cout<<"Path Tracing designated-body versus weapon/partial-body receipt checks passed\n";
        std::cout<<"Path Tracing current-pose geometry checks passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}

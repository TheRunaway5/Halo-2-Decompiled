#include "halo2/first_person_body_geometry.hpp"
#include "halo2/model.hpp"
#include "halo2/source_world_pose.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void need(bool value,const char* message){if(!value)throw FormatError(message);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
std::uint32_t unit(const Cache& cache,std::string_view species) {
    const auto name="objects\\characters\\"+std::string(species)+"\\"+std::string(species)+"_mp";
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto& t) {
        return t.group=="bipd" && t.name==name;
    });
    need(tag!=cache.tags().end(),"Actual multiplayer biped definition is absent");return tag->datum;
}
std::uint32_t first_person_model(const Cache& cache,unsigned representation) {
    const auto globals=cache.query_tag({.group=tag_groups::globals,.require_address=true});
    need(globals,"Actual representation globals are absent");
    const auto metadata=cache.load_source_tag_metadata(globals->datum);
    const auto block=read_tag_block_descriptor(Bytes(metadata.bytes_at(metadata.source_address(),0x140)),0x138);
    need(representation<unsigned(block.count),"Actual first-person representation is absent");
    return Bytes(cache.read_tag_bytes(block.address,block.extent(0xbc,256))).u32(representation*0xbc+0xc);
}
struct Summary {
    std::vector<std::string> regions,nodes;
    std::vector<int> parents;
    std::vector<unsigned> sections;
    std::vector<std::uint32_t> palette;
};
Summary inspect(const std::filesystem::path& path,bool mcc) {
    Cache cache(path,path.parent_path());need(cache.is_mcc()==mcc,"Body geometry input has the wrong asset provenance");
    const auto chief=unit(cache,"masterchief"),fp=first_person_model(cache,2);
    const auto geometry=first_person_body_geometry(cache,chief,fp);
    need(geometry.connected_torso && geometry.lod==4 && geometry.render_model!=fp &&
        geometry.sections==std::vector<unsigned>{17,19},"Chief still selects the capped first-person lower body");
    const auto definition=cache.load_initial_model_definition(chief);
    need(geometry.render_model==definition.render_model,"Connected torso did not retain its actual biped render-model owner");
    const auto metadata=cache.load_source_tag_metadata(definition.render_model);
    const Bytes root(metadata.bytes_at(metadata.source_address(),0x50));
    const auto count=root.u32(0x1c);need(count==4,"Chief four-region authored policy changed");
    const Bytes regions(metadata.bytes_at(root.u32(0x20),count*16));
    Summary result;result.sections=geometry.sections;
    const auto active=cache.load_initial_model_selection(chief);
    need(active.resolved(),"Chief native initial permutation selection is unresolved");
    for(unsigned n=0;n<count;++n)result.regions.push_back(cache.string_id(regions.u32(n*16)));
    need(result.regions==std::vector<std::string>{"arms","body","helmet","legs"},"Chief role metadata changed");
    need(source_initial_model_sections(definition,active,4)==std::vector<unsigned>{16,17,18,19},
        "Native LOD4 region/permutation mapping differs from the actual authored sections");
    SourceWorldPoseModel secondary{chief,active.permutations};
    need(source_world_pose_secondary_sections(secondary,chief,definition)==std::vector<unsigned>{16,17,18,19},
        "Replica secondary rays lost the actual full-body LOD4 region choices");
    secondary.permutations[2]=255;
    need(source_world_pose_secondary_sections(secondary,chief,definition)==std::vector<unsigned>{16,17,19},
        "Replica secondary rays replaced an authority-hidden region with initial/preview geometry");
    bool foreign_rejected{};try{(void)source_world_pose_secondary_sections(secondary,chief+1,definition);}
    catch(const FormatError&){foreign_rejected=true;}
    need(foreign_rejected,"Replica secondary rays accepted a different full model owner");
    const auto selected=cache.load_model_sections(geometry.render_model,false,0,geometry.lod,geometry.sections);
    const auto all=cache.load_model(chief,false,0,active,4);
    const auto capped=cache.load_model(fp,false);
    need(selected.bones.size()==33 && capped.bones.size()==33,"Chief WORLD skeleton must retain its33 authored nodes");
    need(!selected.mesh.vertices.empty() && selected.mesh.vertices.size()%3==0,
        "Connected body resource bank lost its authored triangles");
    const auto visible=first_person_body_vertex_selection(cache,selected,"spine1");
    constexpr std::array<std::string_view,10> lower{"pelvis","spine","l_thigh","r_thigh","l_calf","r_calf","l_foot","r_foot","l_toe","r_toe"};
    std::vector<unsigned> expected_visible;std::size_t leg_vertices{},visible_leg_vertices{},abdomen_vertices{};
    for(unsigned first=0;first<selected.skin.size();first+=3) {
        bool keep=true;
        for(unsigned v=0;v<3;++v)for(unsigned influence=0;influence<4;++influence)if(selected.skin[first+v].weights[influence]!=0) {
            const auto node=selected.skin[first+v].bones[influence];
            need(node<selected.bones.size(),"Chief fixture has an unreviewed compound anatomy influence");
            const auto name=cache.string_id(selected.bones[node].name);
            keep=keep&&std::find(lower.begin(),lower.end(),name)!=lower.end();
        }
        if(selected.mesh.vertices[first].source_section==19)leg_vertices+=3;
        if(keep)for(unsigned v=0;v<3;++v) {
            expected_visible.push_back(first+v);
            if(selected.mesh.vertices[first+v].source_section==19)++visible_leg_vertices;
            else if(selected.mesh.vertices[first+v].source_section==17)++abdomen_vertices;
        }
    }
    need(visible==expected_visible && !visible.empty() && visible.size()<selected.mesh.vertices.size(),
        "First-person anatomy policy retained upper chest/head tissue or discarded an authored lower-body triangle");
    need(leg_vertices && visible_leg_vertices==leg_vertices && abdomen_vertices,
        "First-person anatomy policy removed legs or disconnected the lower abdomen");
    need(selected.vertex_origins.size()==selected.mesh.vertices.size() && selected.skin.size()==selected.mesh.vertices.size(),
        "Connected torso lost complete authored geometry/skin/resource provenance");
    std::size_t destination{};std::set<unsigned> sections;
    for(std::size_t n=0;n<all.mesh.vertices.size();++n) {
        const auto section=all.vertex_origins.at(n).section;
        if(section!=17 && section!=19)continue;
        need(destination<selected.mesh.vertices.size(),"Connected torso lost an authored full-model triangle");
        const auto& a=all.vertex_origins[n];const auto& b=selected.vertex_origins[destination];
        need(a.section==b.section && a.vertex==b.vertex && a.part==b.part && a.vertex_class==b.vertex_class,
            "Connected torso invented a resource vertex/part identity");
        need(!std::memcmp(&all.mesh.vertices[n],&selected.mesh.vertices[destination],sizeof(Vertex)) &&
            !std::memcmp(&all.skin[n],&selected.skin[destination],sizeof(SkinBinding)),
            "Connected torso changed authored full-model vertices or skin weights");
        sections.insert(section);++destination;
    }
    need(destination==selected.mesh.vertices.size() && sections==std::set<unsigned>{17,19},
        "Connected torso contains synthetic geometry, arms, helmet or capped FP mesh");
    const auto node_count=root.u32(0x48);need(node_count==selected.bones.size(),"Full-model node metadata disagrees with the draw bank");
    const Bytes nodes(metadata.bytes_at(root.u32(0x4c),node_count*0x60));
    for(unsigned n=0;n<node_count;++n) {
        const auto& bone=selected.bones[n];const auto at=n*0x60;
        need(bone.name==nodes.u32(at) && bone.parent==std::bit_cast<std::int16_t>(nodes.u16(at+4)) &&
            bone.name==capped.bones[n].name && bone.parent==capped.bones[n].parent,
            "Connected torso changed the native WORLD node-order/hierarchy identity");
        result.nodes.push_back(cache.string_id(bone.name));result.parents.push_back(bone.parent);
        need(bits(bone.inverse_scale)==nodes.u32(at+40),"Full-body inverse scale was borrowed from another model");
        result.palette.push_back(bits(bone.inverse_scale));
        for(unsigned lane=0;lane<12;++lane) {
            need(bits(bone.inverse[lane])==nodes.u32(at+44+lane*4),"Full-body inverse palette was borrowed from the capped FP body");
            result.palette.push_back(bits(bone.inverse[lane]));
        }
    }
    need(selected.bones[0].inverse!=capped.bones[0].inverse ||
        bits(selected.bones[0].inverse_scale)!=bits(capped.bones[0].inverse_scale),
        "Fixture no longer distinguishes the full-model root inverse from the capped FP model");
    const auto elite_fp=first_person_model(cache,3);
    const auto elite=first_person_body_geometry(cache,unit(cache,"elite"),elite_fp);
    need(!elite.connected_torso && elite.render_model==elite_fp && elite.sections.empty() && elite.lod==4,
        "Elite combined body/arms region incorrectly received the Chief torso mask");
    const auto absent=first_person_body_geometry(cache,0xffffffffu,0xffffffffu);
    need(absent.render_model==0xffffffffu && !absent.connected_torso && absent.sections.empty(),
        "Absent native FP owner acquired replacement geometry");
    std::cout<<(mcc?"Prepared MCC":"Original")<<" Chief full-model torso/legs: sections17,19 LOD4 nodes="
        <<node_count<<" authored_vertices="<<destination<<" old_capped_vertices="<<capped.mesh.vertices.size()
        <<" visible_lower_torso_vertices="<<visible.size()<<" full_legs="<<visible_leg_vertices
        <<"; own inverse palette and Elite fallback passed\n";
    return result;
}
}
int main(int argc,char** argv)try {
    need(argc==2 || argc==3,"Supply original map and optional prepared MCC map");
    for(int n=1;n<argc;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
    const auto original=inspect(argv[1],false);
    if(argc==3) {
        const auto mcc=inspect(argv[2],true);
        need(original.regions==mcc.regions && original.sections==mcc.sections && original.nodes==mcc.nodes &&
            original.parents==mcc.parents && original.palette==mcc.palette,
            "Prepared MCC connected-body selection or native skeleton/inverse metadata differs from original");
    }
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

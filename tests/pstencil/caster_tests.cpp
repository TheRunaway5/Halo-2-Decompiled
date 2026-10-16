#include "halo2/pstencil/frame.hpp"
#include "halo2/pstencil/geometry.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/math.hpp"
#include "halo2/pstencil/caster.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_transparent_parts.hpp"
#include <algorithm>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
}
int main(int argc,char** argv){try {
    require(argc==2,"supply an actual current Xbox map");
    Cache cache(argv[1]);
    const auto shader=std::ranges::find_if(cache.tags(),[](const auto& tag){
        return tag.group=="vrtx" && tag.name.ends_with("\\lightaccum_sphere_diffuse");
    });require(shader!=cache.tags().end(),"source sphere position program missing");
    const auto bsp=cache.load_source_bsp_admission(0);
    unsigned casting{},noncasting{};
    for(unsigned section=0;section<bsp.clusters.size();++section) {
        if(!bsp.clusters[section].geometry_count)continue;
        const auto geometry=std::make_shared<const MenuWorldSourceSection>(
            load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_cluster,section));
        for(unsigned part=0;part<geometry->parts().size();++part) {
            const auto& authored=geometry->parts()[part];
            if((authored.type!=2 && authored.type!=3) || authored.index_count<3)continue;
            const auto topology=(authored.flags&8u)?MenuWorldSourceTopology::triangle_list:MenuWorldSourceTopology::triangle_strip;
            auto draw=menu_world_source_cluster_draw(geometry,part,topology);
            auto binding=menu_world_source_binding_class(cache,
                {MenuWorldSourceOwnerKind::bsp_cluster,geometry->owner_datum(),section,{}},
                draw,shader->datum,0,{});
            const auto positions=pstencil::world_positions(binding);
            for(unsigned t=0;t<draw.triangle_count();++t) {
                const auto indices=draw.triangle(t);const auto a=positions[indices[0]],b=positions[indices[1]],c=positions[indices[2]];
                const auto normal=cross(Vec3{b.x-a.x,b.y-a.y,b.z-a.z},Vec3{c.x-a.x,c.y-a.y,c.z-a.z});
                if(dot(normal,normal)<1.e-8f)continue;
                EffectLightRawDraw packet{0,binding};packet.light_identity=1;
                packet.point_light_position=Vec3{a.x+normal.x,a.y+normal.y,a.z+normal.z};
                pstencil::Frame frame(1);frame.add(0,packet,0);
                require(frame.lights(0).size()==1 && frame.lights(0)[0].receivers.size()==1,
                    "part classification removed an actual light receiver");
                const auto vertices=frame.lights(0)[0].volume.size();
                if(authored.type==3){require(vertices==0,"2003 noncasting opaque receiver produced a shadow volume");++noncasting;}
                else {require(vertices>0,"2003 casting opaque part lost its shadow volume");++casting;}
                // This bank deliberately has no color receiver. It is the
                // actual full indexed source part, with a copied source pose.
                pstencil::PointCaster caster{binding,1,0,*packet.point_light_position,128,false};
                pstencil::Frame separate(1);separate.add_caster(0,caster);
                require(separate.lights(0)[0].receivers.empty(),"independent caster manufactured a color receiver");
                if(authored.type==2) {
                    require(!separate.lights(0)[0].volume.empty(),"caster without a color draw was omitted");
                    for(const auto& vertex:separate.lights(0)[0].volume)
                        require(vertex.stipple_level==8,"fractional caster lost its original stipple level");
                    const auto expanded=pstencil::posed_triangles(binding,0);
                    pstencil::Frame dynamic(1);dynamic.add(0,packet,0,{},false);
                    const std::array<std::uint8_t,1> no_shadow_shader{0};
                    dynamic.add_casters(0,expanded,no_shadow_shader);
                    require(!dynamic.lights(0)[0].volume.empty(),"authored casting part incorrectly requires shader layer10");
                } else require(separate.lights(0)[0].volume.empty(),"independent noncasting receiver cast a shadow");
                caster.opacity=0;pstencil::Frame transparent(1);transparent.add_caster(0,caster);
                require(transparent.lights(0).empty(),"zero-opacity caster created a shadow batch");
                break;
            }
            if(casting && noncasting)break;
        }
        if(casting && noncasting)break;
    }
    require(casting && noncasting,"actual map does not cover both casting and noncasting opaque parts");
    unsigned models{},model_vertices{};
    for(const auto group:{"bipd","vehi","weap","scen","mach","bloc"}) {
        const auto object=std::ranges::find_if(cache.tags(),[&](const auto& tag){return tag.group==group;});
        if(object==cache.tags().end())continue;
        const auto definition=cache.load_initial_model_definition(object->datum);
        auto model=cache.load_model(object->datum,false);
        std::vector<unsigned> sections;
        for(const auto& origin:model.vertex_origins)
            if(std::ranges::find(sections,unsigned(origin.section))==sections.end())sections.push_back(origin.section);
        const auto metadata=cache.load_source_transparent_model_parts(definition.render_model,sections);
        const auto nodes=pose_model_nodes_world(model,Transform{});
        const auto posed=skin_model_world_nodes(model,nodes);
        require(posed.size()==model.vertex_origins.size(),"posing lost source vertex provenance");
        for(unsigned n=0;n<posed.size();++n) {
            const auto& vertex=posed[n];const auto origin=model.vertex_origins[n];
            const auto section=std::ranges::find_if(metadata.sections,[&](const auto& section){return section.section==origin.section;});
            require(section!=metadata.sections.end() && origin.part<section->parts.size(),"actual model part missing");
            const auto& part=section->parts[origin.part];
            require(vertex.source_part_type==part.type && vertex.source_part_flags==part.flags,
                "current model posing lost literal caster type/flags");++model_vertices;
        }
        ++models;
    }
    require(models>=3 && model_vertices>100,"current object-family metadata coverage is empty");
    std::cout<<"PASS: actual raw opaque casting/noncasting parts retain lighting receivers and obey the 2003 caster predicate\n";
    std::cout<<"PASS: "<<models<<" actual current object families; "<<model_vertices<<" posed vertices retain source part type/flags\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

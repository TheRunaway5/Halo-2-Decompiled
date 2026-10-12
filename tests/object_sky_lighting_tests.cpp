#include "halo2/object_light_query.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/object_light_contacts.hpp"
#include "halo2/posed_collision.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const char* message) {if(!value)throw FormatError(message);}
void print_sample(const ObjectLightmapSample& sample) {
    std::cout<<std::bit_cast<std::uint32_t>(sample.incident.x)<<' '
        <<std::bit_cast<std::uint32_t>(sample.incident.y)<<' '
        <<std::bit_cast<std::uint32_t>(sample.incident.z)<<' '
        <<sample.diffuse_rgb<<' '<<sample.lightmap_rgb;
}
ObjectLightmapSample endpoint_sample() {
    BspSurfaceLighting bank;bank.collision.resize(1);
    auto& collision=bank.collision.front();
    collision.nodes.push_back({0,0x800000,0xffffff});
    collision.planes.push_back({0,0,1,100});
    collision.collision_leaves.push_back({0,1,0});
    collision.references2.push_back({0,0x8000});
    collision.surfaces.push_back({0,0,0,0,-1});
    ObjectSkyLightingDefinition sky;
    ObjectSkyLightingDefinition::Light light;light.pitch=1.5707963705062866f;
    light.radiosity={{1,0,0},1};sky.lights.push_back(light);
    const auto query=query_object_sky_lightmap(bank,&sky,{});
    require(bool(query.sample),"source endpoint sky query failed");
    return *query.sample;
}
void probe_sky() {
    ObjectSkyLightingDefinition sky;unsigned count;int material;
    while(std::cin>>sky.outdoor_ambient.x>>sky.outdoor_ambient.y>>sky.outdoor_ambient.z>>count) {
        sky.lights.clear();
        for(unsigned i=0;i<count;++i) {
            ObjectSkyLightingDefinition::Light light;int present;
            ObjectSkyLightingDefinition::Light::Radiosity radiosity;
            std::cin>>light.yaw>>light.pitch>>present>>radiosity.color.x>>radiosity.color.y
                >>radiosity.color.z>>radiosity.power;
            if(present)light.radiosity=radiosity;
            sky.lights.push_back(light);
        }
        std::cin>>material;require(bool(std::cin),"truncated sky oracle fixture");
        const auto frame=object_sky_light_frame(sky);
        print_sample(object_sky_light_sample(frame,material==-2?std::nullopt:std::optional(std::int16_t(material))));
        std::cout<<' '<<frame.test_collision<<'\n';
    }
    require(std::cin.eof(),"invalid sky oracle fixture");
}
void probe_admission() {
    ObjectLightQueryContext context;unsigned flags,vehicle;int first,second,sky,valid;
    while(std::cin>>valid>>context.object_type>>flags>>context.runtime_flags>>context.biped_state
        >>vehicle>>context.creature_flags>>first>>second>>sky) {
        context.bank_valid=valid;context.definition_flags=std::uint16_t(flags);
        context.vehicle_type=std::uint8_t(vehicle);
        std::vector<unsigned> modes;unsigned sky_calls=0;
        const auto result=source_object_light_query(context,[&](unsigned mode) {
            const int status=modes.empty()?first:second;modes.push_back(mode);
            ObjectLightQueryResult value;value.status=ObjectLightQueryStatus(status);
            if(!status)value.sample={{1,2,3},0x123456u,0xabcdefu};
            return value;
        },[&] {
            ++sky_calls;ObjectLightQueryResult value;value.status=ObjectLightQueryStatus(sky);
            if(!sky)value.sample={{4,5,6},0xff555555u,0x765432u};
            return value;
        });
        std::cout<<result.accepted<<' '<<result.retried<<' '<<result.used_sky<<' '
            <<modes.size();for(auto mode:modes)std::cout<<' '<<mode;
        std::cout<<' '<<sky_calls<<' '<<bool(result.query.sample)<<'\n';
    }
    require(std::cin.eof(),"invalid admission oracle fixture");
}
void print_collision(const ModelCollision& model) {
    std::cout<<model.model<<' '<<model.collision<<' '<<model.materials.size();
    for(const auto& m:model.materials)std::cout<<' '<<m.damage_section<<' '<<m.global_material;
    std::cout<<' '<<model.model_regions.size();
    for(const auto& r:model.model_regions) {
        std::cout<<' '<<unsigned(r.collision)<<' '<<r.permutations.size();
        for(auto p:r.permutations)std::cout<<' '<<unsigned(p);
    }
    std::cout<<' '<<model.authored_inverse.size();
    for(const auto& matrix:model.authored_inverse)for(auto word:std::bit_cast<std::array<std::uint32_t,13>>(matrix))
        std::cout<<' '<<word;
    std::cout<<' '<<model.regions.size();
    for(const auto& region:model.regions) {
        std::cout<<' '<<region.size();
        for(const auto& permutation:region) {
            std::cout<<' '<<permutation.size();
            for(const auto& b:permutation) {
                std::cout<<' '<<b.node<<' '<<b.nodes.size();
                for(const auto& n:b.nodes)std::cout<<' '<<n.plane<<' '<<n.children[0]<<' '<<n.children[1];
                std::cout<<' '<<b.planes.size();
                for(const auto& p:b.planes)for(float f:{p.normal.x,p.normal.y,p.normal.z,p.distance})std::cout<<' '<<std::bit_cast<std::uint32_t>(f);
                std::cout<<' '<<b.leaves.size();
                for(const auto& l:b.leaves)std::cout<<' '<<unsigned(l.flags)<<' '<<unsigned(l.count)<<' '<<l.first;
                std::cout<<' '<<b.references.size();
                for(const auto& r:b.references)std::cout<<' '<<r.plane<<' '<<r.node;
                std::cout<<' '<<b.nodes2.size();
                for(const auto& n:b.nodes2)std::cout<<' '<<std::bit_cast<std::uint32_t>(n.normal.x)<<' '
                    <<std::bit_cast<std::uint32_t>(n.normal.y)<<' '<<std::bit_cast<std::uint32_t>(n.distance)
                    <<' '<<n.children[0]<<' '<<n.children[1];
                std::cout<<' '<<b.surfaces.size();
                for(const auto& s:b.surfaces)std::cout<<' '<<s.plane<<' '<<s.first_edge<<' '<<unsigned(s.flags)<<' '<<unsigned(s.breakable)<<' '<<s.material;
                std::cout<<' '<<b.edges.size();
                for(const auto& e:b.edges)std::cout<<' '<<e.vertices[0]<<' '<<e.vertices[1]<<' '
                    <<e.next[0]<<' '<<e.next[1]<<' '<<e.surfaces[0]<<' '<<e.surfaces[1];
                std::cout<<' '<<b.vertices.size();
                for(auto p:b.vertices)for(float f:{p.x,p.y,p.z})std::cout<<' '<<std::bit_cast<std::uint32_t>(f);
            }
        }
    }
    std::cout<<'\n';
}
}
int main(int argc,char** argv) {try {
    if(argc==2 && std::string(argv[1])=="--sky") {probe_sky();return 0;}
    if(argc==2 && std::string(argv[1])=="--admission") {probe_admission();return 0;}
    if(argc==2 && std::string(argv[1])=="--contact-policy") {
        std::int16_t type,policy;unsigned ready,initial;
        while(std::cin>>type>>policy>>ready>>initial)
            std::cout<<int(source_object_light_contact_action(type,policy,ready))<<' '
                <<source_object_light_collision_flags(initial)<<'\n';
        require(std::cin.eof(),"invalid original contact policy fixture");return 0;
    }
    if(argc==2 && std::string(argv[1])=="--spatial-policy") {
        ObjectLightSpatialState state;unsigned handle,type,unit,biped,query,spatial_type,spatial_flags;
        std::uint32_t object,excluded_a,excluded_b;
        while(std::cin>>state.object_flags>>handle>>type>>unit>>biped>>state.type11_flags_12c
            >>query>>spatial_type>>spatial_flags>>object>>excluded_a>>excluded_b) {
            state.handle_flags=std::uint8_t(handle);state.object_type=std::uint8_t(type);
            state.unit_flags_10a=std::uint8_t(unit);state.biped_flags_348=std::uint8_t(biped);
            std::cout<<source_object_light_spatial_flags(state)<<' '
                <<source_object_light_collision_exclusion_mask(query)<<' '
                <<source_object_light_spatial_contact_admitted(query,std::uint8_t(spatial_type),
                    std::uint16_t(spatial_flags),object,excluded_a,excluded_b)<<'\n';
        }
        require(std::cin.eof(),"invalid source spatial contact fixture");return 0;
    }
    if(argc==2 && std::string(argv[1])=="--render-reference") {
        std::int16_t node;std::uint32_t leaf_count,leaf,plane;unsigned nodes,maps;
        while(std::cin>>node>>leaf_count>>leaf>>plane>>nodes>>maps) {
            ObjectLightRenderLeaves::Section section;
            for(unsigned i=0;i<nodes;++i) {unsigned value;std::cin>>value;section.nodes.push_back(std::uint8_t(value));}
            for(unsigned i=0;i<maps;++i) {
                ObjectLightRenderLeaves::NodeMap map;unsigned leaves,references;
                std::cin>>leaves>>references;
                for(unsigned j=0;j<leaves;++j) {
                    ObjectLightRenderLeaves::Leaf l;std::cin>>l.count>>l.first;map.leaves.push_back(l);
                }
                for(unsigned j=0;j<references;++j) {
                    ObjectLightRenderLeaves::Reference ref;
                    std::cin>>ref.strip>>ref.lightmap_triangle>>ref.collision_node;map.references.push_back(ref);
                }
                section.maps.push_back(std::move(map));
            }
            unsigned plane_count;std::cin>>plane_count;std::vector<std::uint16_t> planes;
            for(unsigned i=0;i<plane_count;++i) {unsigned value;std::cin>>value;planes.push_back(std::uint16_t(value));}
            int accepted;std::cin>>accepted;require(bool(std::cin),"truncated render-reference fixture");
            std::vector<int> attempts;
            const auto selected=source_object_light_render_reference(section,node,leaf_count,leaf,plane,planes,
                [&](const ObjectLightRenderReference& r) {attempts.push_back(r.index);return r.index==accepted;});
            std::cout<<bool(selected);
            if(selected)std::cout<<' '<<selected->index<<' '<<selected->strip<<' '<<selected->lightmap_triangle;
            std::cout<<' '<<attempts.size();for(int index:attempts)std::cout<<' '<<index;
            std::cout<<'\n';
        }
        require(std::cin.eof(),"invalid render-reference fixture");return 0;
    }
    if(argc==4 && std::string(argv[1])=="--render-leaves") {
        const auto bank=Cache(argv[2]).load_object_light_render_leaves(std::stoul(argv[3]));
        std::cout<<bank.sections.size()<<'\n';
        for(const auto& section:bank.sections) {
            std::cout<<section.nodes.size()<<' '<<section.maps.size();
            for(auto node:section.nodes)std::cout<<' '<<unsigned(node);std::cout<<'\n';
            for(const auto& map:section.maps) {
                std::cout<<map.leaves.size()<<' '<<map.references.size();
                for(const auto& l:map.leaves)std::cout<<' '<<l.cluster<<' '<<l.count<<' '<<l.first;
                for(const auto& r:map.references)std::cout<<' '<<r.strip<<' '<<r.lightmap_triangle<<' '<<r.collision_node;
                std::cout<<'\n';
            }
        }
        return 0;
    }
    if(argc==3 && std::string(argv[1])=="--collision-models") {
        const Cache cache(argv[2]);std::uint32_t datum;
        while(std::cin>>datum)print_collision(cache.load_model_collision(datum));
        require(std::cin.eof(),"invalid collision metadata fixture");return 0;
    }
    if(argc==4 && std::string(argv[1])=="--posed-model") {
        const Cache cache(argv[2]);const auto model=cache.load_model_collision(std::stoul(argv[3]));
        const auto original=cache.original_data();unsigned count;
        const auto real=[] {std::uint32_t word;std::cin>>word;return std::bit_cast<float>(word);};
        while(std::cin>>count) {
            std::vector<Transform> nodes;
            for(unsigned i=0;i<count;++i) {
                std::array<float,13> matrix;for(auto& f:matrix)f=real();nodes.push_back(std::bit_cast<Transform>(matrix));
            }
            unsigned active_count;std::cin>>active_count;std::vector<std::uint8_t> active;
            for(unsigned i=0;i<active_count;++i) {unsigned v;std::cin>>v;active.push_back(std::uint8_t(v));}
            unsigned flags;std::cin>>flags;const float previous=real();
            const Vec3 origin{real(),real(),real()},delta{real(),real(),real()};
            require(bool(std::cin),"truncated original posed query fixture");
            const auto hit=posed_collision(model,nodes,active,*original,origin,delta,flags,previous);
            std::cout<<bool(hit);
            if(hit)std::cout<<' '<<std::bit_cast<std::uint32_t>(hit->fraction)<<' '<<hit->packed_bsp<<' '
                <<hit->region<<' '<<hit->node<<' '<<hit->leaf<<' '<<hit->surface<<' '<<hit->signed_plane
                <<' '<<hit->model_material<<' '<<unsigned(hit->flags)<<' '<<unsigned(hit->breakable)
                <<' '<<std::bit_cast<std::uint32_t>(hit->plane.normal.x)
                <<' '<<std::bit_cast<std::uint32_t>(hit->plane.normal.y)
                <<' '<<std::bit_cast<std::uint32_t>(hit->plane.normal.z)
                <<' '<<std::bit_cast<std::uint32_t>(hit->plane.distance)<<' '<<hit->global_material;
            std::cout<<'\n';
        }
        require(std::cin.eof(),"invalid original posed query fixture");return 0;
    }
    if(argc==4 && std::string(argv[1])=="--contact-definition") {
        const auto d=Cache(argv[2]).load_object_light_contact_definition(std::stoul(argv[3]));
        require(bool(d),"missing authored light contact definition");
        std::cout<<d->datum<<' '<<d->model<<' '<<d->render_model<<' '<<d->collision<<' '
            <<d->type<<' '<<d->scenery_policy<<' '<<d->prt_count<<' '
            <<d->section_render_leaves_count<<' '<<d->model_attribution_available()<<'\n';return 0;
    }
    if(argc==3 && std::string(argv[1])=="--contact-definitions") {
        const Cache cache(argv[2]);std::uint32_t datum;
        while(std::cin>>datum) {
            const auto d=cache.load_object_light_contact_definition(datum);
            require(bool(d),"missing authored light contact definition");
            std::cout<<d->datum<<' '<<d->model<<' '<<d->render_model<<' '<<d->collision<<' '
                <<d->type<<' '<<d->scenery_policy<<' '<<d->prt_count<<' '
                <<d->section_render_leaves_count<<' '<<d->model_attribution_available()<<'\n';
        }
        require(std::cin.eof(),"invalid authored contact definition fixture");return 0;
    }
    if(argc==2 && std::string(argv[1])=="--endpoint") {print_sample(endpoint_sample());std::cout<<'\n';return 0;}
    if(argc==2 && std::string(argv[1])=="--biped-initial") {
        std::uint32_t definition;unsigned runtime;
        while(std::cin>>definition>>runtime)std::cout<<source_biped_light_query_initial_state(definition,std::uint16_t(runtime))<<'\n';
        require(std::cin.eof(),"invalid source biped state fixture");return 0;
    }
    if(argc==4 && std::string(argv[1])=="--query") {
        const Cache cache(argv[2]);const auto bank=cache.load_bsp_surface_lighting();
        const auto sky=cache.load_object_sky_lighting(std::stoul(argv[3]));
        Vec3 origin;
        while(std::cin>>origin.x>>origin.y>>origin.z) {
            const auto query=query_object_sky_lightmap(bank,sky?&*sky:nullptr,origin);
            std::cout<<unsigned(query.status)<<' ';
            if(query.sample)print_sample(*query.sample);
            std::cout<<'\n';
        }
        require(std::cin.eof(),"invalid sky map query fixture");return 0;
    }
    if(argc==4 && std::string(argv[1])=="--definition") {
        const auto sky=Cache(argv[2]).load_object_sky_lighting(std::stoul(argv[3]));
        std::cout<<bool(sky);
        if(sky) {
            std::cout<<' '<<sky->datum;
            for(float v:{sky->outdoor_ambient.x,sky->outdoor_ambient.y,sky->outdoor_ambient.z})
                std::cout<<' '<<std::bit_cast<std::uint32_t>(v);
            std::cout<<' '<<sky->lights.size();
            for(const auto& light:sky->lights) {
                std::cout<<' '<<std::bit_cast<std::uint32_t>(light.yaw)<<' '
                    <<std::bit_cast<std::uint32_t>(light.pitch)<<' '<<bool(light.radiosity);
                if(light.radiosity)for(float v:{light.radiosity->color.x,light.radiosity->color.y,
                    light.radiosity->color.z,light.radiosity->power})std::cout<<' '<<std::bit_cast<std::uint32_t>(v);
            }
        }
        std::cout<<'\n';return 0;
    }
    if(argc==4 && std::string(argv[1])=="--availability") {
        std::cout<<Cache(argv[2]).object_sky_lighting_available(std::stoul(argv[3]))<<'\n';return 0;
    }
    require(argc==1,"unknown object sky lighting fixture mode");
    require(source_object_light_collision_flags(true)==0x4480000du,
        "source initial map query flags changed");
    require(source_object_light_collision_flags(false)==0x04800c0du,
        "source retained map query flags changed");
    // Actual paused source SCEN flags: the initial collision pass excludes
    // ordinary scenery, while a retained query can be blocked by its collider.
    ObjectLightSpatialState scenery_state;scenery_state.object_type=6;
    scenery_state.object_flags=0x00800900u;scenery_state.handle_flags=0x41;
    const auto scenery_spatial=source_object_light_spatial_flags(scenery_state);
    require(scenery_spatial==12,"captured source scenery spatial flags changed");
    require(!source_object_light_spatial_contact_admitted(0x4480000du,6,scenery_spatial,42),
        "initial lighting query admitted ordinary scenery collision");
    require(source_object_light_spatial_contact_admitted(0x04800c0du,6,scenery_spatial,42),
        "retained lighting query lost source scenery blocking");
    require(!source_object_light_spatial_contact_admitted(0x04800c0du,6,scenery_spatial,42,42),
        "excluded lighting object collided with itself");
    BspLighting missing_bank;
    require(!source_object_light_query_bank_valid(missing_bank),"missing source group admitted");
    missing_bank.structure_matches=true;
    require(!source_object_light_query_bank_valid(missing_bank),"source NONE bitmap admitted");
    missing_bank.bitmap_datum=0;
    require(source_object_light_query_bank_valid(missing_bank),"source bank guard incorrectly depends on bucket residency");
    require(endpoint_sample().lightmap_rgb==0xff0000u,"original sky vector endpoint contact rejected");
    // Status4 is a native evidence boundary, never original success. A
    // successful second map query is also rejected for ordinary scenery.
    for(unsigned first=0;first<5;++first)for(unsigned second=0;second<5;++second) {
        ObjectLightQueryContext context;context.object_type=6;context.definition_flags=2;
        unsigned count=0,sky_calls=0;
        const auto result=source_object_light_query(context,[&](unsigned) {
            ObjectLightQueryResult out;out.status=ObjectLightQueryStatus(count++?second:first);
            out.sample={{1,2,3},4,5};return out;
        },[&] {++sky_calls;return ObjectLightQueryResult{};});
        require(result.accepted==(first==0),"source scenery admission changed");
        require(bool(result.query.sample)==result.accepted,"rejected query retained a new sample");
        require(!sky_calls,"ordinary scenery manufactured a sky fallback");
    }
    constexpr const char* names[]{"ascension","beavercreek","burial_mounds","coagulation","colossus",
        "cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar",
        "backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
    std::size_t skies=0,lights=0,samples=0,contact_definitions=0,attributed_models=0;
    std::size_t render_sections=0,render_maps=0,render_leaves=0,render_refs=0;
    constexpr const char* object_groups[]{"bipd","vehi","weap","eqip","garb","proj",
        "scen","mach","ctrl","lifi","ssce","bloc","crea"};
    for(std::size_t map=0;map<std::size(names);++map) {
        const auto path=std::filesystem::path(map<12?"assets/disc/maps":"assets/map-pack/maps")/(std::string(names[map])+".map");
        const Cache cache(path);const auto atmosphere=cache.load_bsp_atmosphere(0);
        const auto bank=cache.load_bsp_surface_lighting();
        for(const auto& tag:cache.tags()) {
            if(!tag.address || std::find_if(std::begin(object_groups),std::end(object_groups),
                [&](const char* group){return tag.group==group;})==std::end(object_groups))continue;
            const auto contact=cache.load_object_light_contact_definition(tag.datum);
            require(bool(contact),"authored object contact metadata missing");
            ++contact_definitions;attributed_models+=contact->model_attribution_available();
            if(contact->model_attribution_available()) {
                const auto render=cache.load_object_light_render_leaves(contact->render_model);
                render_sections+=render.sections.size();
                for(const auto& section:render.sections) {
                    render_maps+=section.maps.size();
                    require(section.nodes.size()>=section.maps.size(),"authored render maps escape geometry NodeMap");
                    for(const auto& map:section.maps) {
                        render_leaves+=map.leaves.size();render_refs+=map.references.size();
                    }
                }
            }
            if(contact->type==6)require(!contact->model_attribution_available(),
                "original MP scenery unexpectedly acquired model attribution");
        }
        require(source_object_light_query_bank_valid(bank.lighting),"authored MP source bank fails D1850 guard");
        for(std::size_t i=0;i<atmosphere.skies.size();++i) {
            require(cache.object_sky_lighting_available(i)==bool(atmosphere.skies[i].clear),
                "14B360 render sky availability differs from authored guard");
            const auto sky=cache.load_object_sky_lighting(i);
            if(!sky)continue;
            ++skies;lights+=sky->lights.size();
            for(const auto& spawn:cache.spawns()) {
                const auto result=query_object_sky_lightmap(bank,&*sky,spawn.position);
                require(result.status==ObjectLightQueryStatus::resolved && bool(result.sample),"authored sky query failed");
                const auto& s=*result.sample;
                require(s.diffuse_rgb==0xff555555u && !(s.lightmap_rgb&0xff000000u),"source sky packed colors changed");
                require(std::isfinite(s.incident.x)&&std::isfinite(s.incident.y)&&std::isfinite(s.incident.z),"nonfinite sky sample");
                ++samples;
            }
        }
        require(!cache.object_sky_lighting_available(atmosphere.skies.size()),
            "invalid selected render sky admitted");
        require(query_object_sky_lightmap(bank,nullptr,{}).status==ObjectLightQueryStatus::miss,"unavailable render sky manufactured a sample");
    }
    require(contact_definitions==3296 && attributed_models==13,
        "source MP collision-to-render admission inventory changed");
    require(render_sections==50 && render_maps==64 && render_leaves==8845 && render_refs==26493,
        "source MP collision-to-render leaf inventory changed");
    std::cout<<contact_definitions<<" source object definitions and "<<attributed_models
        <<" models with authored collision-to-render lighting maps checked\n";
    std::cout<<skies<<" authored sky references, "<<lights<<" lights and "<<samples<<" static sky queries checked across21 maps\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

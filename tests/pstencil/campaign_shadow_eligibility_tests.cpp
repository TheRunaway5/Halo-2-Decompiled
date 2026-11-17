#include "halo2/pstencil/campaign.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/content.hpp"
#include "halo2/original_data.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <unistd.h>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
}
int main(int argc,char** argv)try {
    require(argc==3,"Supply TU1.5 gameplay image and opening cache");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto content=cache.load_content(0,false);
    auto path=(std::filesystem::temp_directory_path()/"pstencil-shadow-eligibility-XXXXXX").string();
    require(::mkdtemp(path.data())!=nullptr,"Shadow eligibility storage unavailable");
    struct Cleanup {std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    PhysicsLiveWorldTu15 world(cache,argv[1],path);
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678u);
    pstencil::CampaignShadows shadows;
    const Vec3 sun{0,.951057f,.309017f};
    std::size_t accepted_vertices{};
    for(const auto suffix:{"\\hc_cin_volume","\\prophet_mercy","\\regret_holo","\\elite"}) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& row) {
            return (row.group=="scen" || row.group=="bipd") && row.name.ends_with(suffix);
        });
        require(tag!=cache.tags().end(),"Authored cinematic shadow fixture absent");
        const auto placement=std::ranges::find_if(content.objects,[&](const auto& row) {
            return row.definition==tag->datum && !row.model_attachment;
        });
        require(placement!=content.objects.end(),"Authored cinematic shadow placement absent");
        world.preload_render_model(cache.load_initial_model_definition(tag->datum).render_model);
        const auto object=world.create_scenario_object(*placement,SourceScenarioBirth::runtime);
        require(object!=null_datum,"Native cinematic shadow fixture birth declined");
        const auto model=cache.load_model(tag->datum,false,placement->variant);
        const auto& vertices=model.mesh.vertices;
        require(!vertices.empty() && vertices.size()%3==0,"Authored shadow fixture has no triangle geometry");
        const auto body=world.source_object_body(object);
        const auto word=[&](auto at){return Bytes(world.memory().view(at,4)).u32(0);};
        const auto cache_before=word(body+0xccu);
        const bool expected=tag->group=="bipd";
        require(pstencil::native_lightmap_shadow_eligible_tu15(world,object)==expected,
            "Native cinematic family eligibility differs from the authored definition");
        const auto before=shadows.casters().size();
        require(shadows.add(world,object,1,vertices,sun,100)==expected,
            "Sun selection bypassed cinematic family eligibility");
        require(shadows.casters().size()==before+unsigned(expected),"Sun caster count differs from admitted owners");
        require(word(body+0xccu)==cache_before,"Sun family admission changed the native lighting-cache token");
        // The frame may submit an owner as several draw units. Rejected
        // geometry must remain rejected, while accepted units extend one owner.
        require(shadows.add(world,object,1,vertices,sun,200)==expected,
            "Repeated draw unit changed cinematic family admission");
        if(expected) {
            accepted_vertices+=vertices.size()*2;
            require(shadows.casters().back().vertices.size()==vertices.size()*2 &&
                shadows.casters().back().extrusion==200 && shadows.owners().back().object==object,
                "Eligible cinematic draw units lost geometry, extrusion or native owner");
        } else {
            require(!pstencil::native_incident_tu15(world,object,1),"Scenery acquired an ordinary lightmap shadow sample");
            require(shadows.casters().size()==before,"Rejected repeated scenery created a caster");
            require(word(body+0xccu)==cache_before,"Rejected scenery allocated a native lighting-cache token");
        }
        std::cout<<"PASS "<<tag->name<<" authored_vertices="<<vertices.size()
            <<" sun_admitted="<<expected<<'\n';
    }
    require(shadows.attempted()==4 && shadows.casters().size()==3 && accepted_vertices>0,
        "Cinematic shadow family census became vacuous");
    std::cout<<"PASS actual cinematic volume excluded; Mercy, hologram Regret and Elite retained, including repeated draw units\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

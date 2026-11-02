#include "halo2/source_bsp_draw_collector.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace halo2;
namespace {
void require(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
void exercise(bool cluster_ready,bool definition_ready,bool visible_instance) {
    SourceBspAdmissionBsp bsp;bsp.clusters.resize(1);bsp.instances.resize(1);bsp.definitions.resize(1);
    bsp.clusters[0].bounds={-1,1,-1,1,3,5};
    bsp.instances[0].definition=0;bsp.instances[0].center={0,0,4};
    bsp.instances[0].radius=visible_instance?1.f:0.f;
    // Prepared values deliberately disagree with the live owner. The
    // per-unit resource result, after the original alpha gate, owns admission.
    std::vector<SourceBspDrawGeometry> clusters{{0,1,!cluster_ready}};
    std::vector<SourceBspDrawGeometry> definitions{{0,1,!definition_ready}};
    std::vector<SourceBspDrawInstance> instances(1);
    SourceBspAdmission admission;admission.clusters.push_back({0,0,0,0});
    admission.instances.push_back({0,0,0,0});
    std::vector<unsigned> queries;
    SourceBspDrawState state;state.arena.used=1;
    SourceBspDrawWorld world{bsp,clusters,definitions,instances,false};
    world.cluster_resident=[&](unsigned id,bool request) {
        require(id==0,"cluster resource identity changed");
        require(request,"BSP cluster selector was substituted for actual owner-NONE request");
        require(state.arena.used==1 && state.units[0].words[0]>>29==3,
            "cluster resource query moved before lighting or after mask allocation");
        queries.push_back(0);return cluster_ready;
    };
    world.definition_resident=[&](unsigned id,bool request) {
        require(id==0,"instance resource definition changed");
        require(request,"BSP instance lost actual owner-NONE request");
        require(state.arena.used==1u+unsigned(cluster_ready) &&
            state.units[state.draw_count-1].words[0]>>29==3,
            "instance resource query moved before lighting or after mask allocation");
        queries.push_back(1);return definition_ready;
    };
    SourceBspDrawContext context;context.mask_arena_address=0x71000000u;
    context.camera.inverse_view_z={0,0,1,0};context.camera.viewport_height_metric=200;
    source_bsp_collect_draw_units(state,admission,world,context);
    require(queries==(visible_instance?std::vector<unsigned>{0,1}:std::vector<unsigned>{0}),
        "resource requests bypassed alpha gate or changed original unit order");
    const unsigned expected=unsigned(cluster_ready)+unsigned(visible_instance&&definition_ready);
    require(state.draw_count==expected&&state.main_prefix==expected,
        "real unavailable resource did not drop exactly its draw unit");
    require(state.arena.used==1u+expected,"unavailable resource consumed a mask allocation");
    if(!visible_instance)require(admission.instances[0].flags&1u,"zero-alpha instance lost original skipped bit");
    for(unsigned n=0;n<state.draw_count;++n)
        require(state.units[n].words[1]==0xffffffffu,"BSP resource query changed the actual NONE owner");
}
}
int main() {try {
    for(bool cluster:{false,true})for(bool definition:{false,true})
        for(bool visible:{false,true})exercise(cluster,definition,visible);
    std::cout<<"PASS 8 BSP admission profiles: independent selector/request, per-unit results, alpha gate and order\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}

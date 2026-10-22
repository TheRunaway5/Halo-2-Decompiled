#include "halo2/pstencil/point_lights.hpp"
#include "halo2/transform.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <algorithm>
#include <bit>
#include <exception>
#include <iostream>
#include <thread>
using namespace halo2;
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
namespace {
void checkpoint_roundtrip(const Cache &cache,pstencil::PointLights &lights,pstencil::PointLightOwner owner) {
    const auto spatial=cache.load_object_light_spatial_bsp(0);
    const auto geometry=cache.load_source_bsp_admission(0);
    const auto current=lights.lights();require(current.size()==1,"Point-light checkpoint needs its actual live plasma owner");
    const auto cluster=current.front().registration.location.cluster;
    const auto sphere=source_bsp_sphere_clusters(spatial,cluster,owner.root.position,8);
    SourceClusterFrustumSet entries;
    for(const auto id:sphere.clusters){SourceClusterFrustumRanges range;range.source_cluster=id;entries.clusters.push_back(range);}
    require(!entries.clusters.empty(),"Actual Lockout light has no source sphere-query clusters");
    // Explicit diagnostic sphere-query input, not a fabricated main-camera
    // PVS/frustum. The ordered cluster list comes from the real BSP walk.
    SourceEffectLightCollectionContext collection{};collection.visibility.frustums=&entries;
    collection.visibility.query_mode=1;collection.visibility.sphere_center=owner.root.position;collection.visibility.sphere_radius=8;
    collection.containing_cluster=cluster;collection.owners={1,0,0,2,{}};
    SourceEffectLightCandidateContext candidates{};candidates.admission={1,0,0,2,{}};
    candidates.depth_plane={0,0,-1,owner.root.position.z+2};candidates.pixel_scale=240;
    const auto fog=source_bsp_fog_admission(cache.load_bsp_atmosphere(0),{true,true,0xffffffffu,0});
    unsigned visible{},selected{},received{};const EffectLightDefinition *resident{};
    const auto record=[&](unsigned frame){std::vector<std::uint32_t> words;
        const auto word=[&](auto value){words.push_back(std::uint32_t(value));};
        const auto scalar=[&](float value){word(std::bit_cast<std::uint32_t>(value));};
        const auto vector=[&](Vec3 v){scalar(v.x);scalar(v.y);scalar(v.z);};
        const auto candidate=[&](const SourceShadowCandidate &c){word(c.flags);word(c.identity);word(c.owner);scalar(c.requested_fade);scalar(c.coverage);};
        for(const auto &light:lights.lights()){
            word(light.definition);word(light.identity);word(light.effect_owner);vector(light.position);vector(light.forward);vector(light.up);
            scalar(light.radius);scalar(light.specular_radius);scalar(light.state.size);scalar(light.state.control);
            vector(light.state.diffuse);vector(light.state.specular);scalar(light.state.brightness);
            const auto &r=light.registration;word(r.identity);word(r.reference_head);word(r.flags);vector(r.spatial_center);scalar(r.spatial_radius);
            vector(r.cull_center);scalar(r.cull_radius);vector(r.location_center);word(r.location.leaf);word(r.location.cluster);word(r.location.bsp);
            vector(r.position);scalar(r.extension_radius);vector(r.extension_center);
        }
        for(unsigned view=0;view<4;++view){collection.local_player=view;
            const auto result=lights.view(owner.root.position+Vec3{0,0,2},collection,candidates,std::int32_t(frame));
            word(result.epoch);word(result.visible.size());for(const auto id:result.visible)word(id);visible+=result.visible.size();
            word(result.candidates.size());for(const auto &c:result.candidates)candidate(c);
            word(result.selected.size());for(const auto &c:result.selected)candidate(c);selected+=result.selected.size();
            for(const auto &r:result.records){if(!resident)resident=r.definition;require(resident==r.definition,"Point-light checkpoint replaced its resident definition");
                word(r.definition_identity);word(r.runtime.identity);word(r.runtime.flags);word(r.runtime.cluster);
                scalar(r.runtime.overall_fade);vector(r.runtime.spatial_center);scalar(r.runtime.spatial_radius);
                scalar(r.fades.diffuse);scalar(r.fades.specular);scalar(r.fades.secondary);scalar(r.fades.overall);scalar(r.fades.volume);}
            const auto receivers=lights.receivers(result,geometry,fog,nullptr);received+=receivers.size();word(receivers.size());
            for(const auto &receiver:receivers){word(receiver.identity);word(receiver.epoch);scalar(receiver.selected_fade);
                const auto &r=receiver.view;word(r.flags);word(r.pass_mode);word(r.query_mode);word(r.location.leaf);word(r.location.cluster);word(r.location.bsp);
                word(r.visibility.groups.size());word(r.visibility.clusters.size());word(r.visibility.exhausted_limits);
                for(const auto &c:r.visibility.clusters){word(c.source_cluster);for(const auto count:c.counts)word(count);for(const auto first:c.first_indices)word(first);}
                for(const auto ordinal:r.visibility.sorted_cluster_indices)word(ordinal);
                for(const auto bits:r.bsp.marked_clusters)word(bits);
                for(const auto *queue:{&r.bsp.clusters,&r.bsp.instances}){word(queue->size());for(const auto &q:*queue){word(q.id);word(q.flags);word(q.association);word(q.other);}}
                word(r.bsp.associations.size());for(const auto &association:r.bsp.associations)for(const auto bits:association)word(bits);
            }
        }return words;};
    for(unsigned frame=1;frame<=6;++frame)(void)record(frame);
    const auto before_view=lights.checkpoint();auto view_stale=lights.prepare_restore(before_view);
    collection.local_player=0;
    const auto control_view=lights.view(owner.root.position+Vec3{0,0,2},collection,candidates,7);
    require(!lights.restore_ready(view_stale)&&!lights.commit_restore(view_stale),"A later point-light view kept an old preparation eligible");
    auto receiver_stale=lights.prepare_restore(before_view);(void)lights.receivers(control_view,geometry,fog,nullptr);
    require(!lights.restore_ready(receiver_stale)&&!lights.commit_restore(receiver_stale),"A later receiver query kept an old point-light preparation eligible");
    const auto cut=lights.checkpoint();const auto cut_owner=owner;
    require(cut.valid()&&cut.tick()==81&&lights.owns_checkpoint(cut)&&visible&&selected&&received,
        "Point-light owning checkpoint lacks real multi-view query/receiver history");
    const auto continuation=[&]{std::vector<std::uint32_t> out;
        for(unsigned tick=82;tick<=84;++tick){owner.root.position.x+=.004f;lights.synchronize(std::span(&owner,1),tick);
            const auto next=record(tick-70);out.insert(out.end(),next.begin(),next.end());}
        lights.synchronize({},85);owner.object+=0x10000u;lights.synchronize(std::span(&owner,1),86);
        const auto next=record(16);out.insert(out.end(),next.begin(),next.end());return out;};
    const auto expected=continuation();const auto future=lights.lights().at(0).identity;
    {auto cancelled=lights.prepare_restore(cut);require(lights.restore_ready(cancelled),"Actual point-light cut did not prepare");}
    require(lights.lights().at(0).identity==future,"Cancelled point-light restore changed a live identity");
    pstencil::PointLights foreign(cache,0);const auto untouched=foreign.checkpoint();
    require(untouched.valid()&&!untouched.tick()&&!foreign.owns_checkpoint(cut),"Point-light unsynchronized/foreign ownership boundary changed");
    bool rejected{};try{(void)foreign.prepare_restore(cut);}catch(const FormatError&){rejected=true;}require(rejected,"Foreign point-light owner prepared a cut");
    rejected=false;try{(void)lights.prepare_restore({});}catch(const FormatError&){rejected=true;}require(rejected,"Empty point-light checkpoint prepared");
    auto stale=lights.prepare_restore(cut);auto invalid=owner;invalid.object=0xffffffffu;
    rejected=false;try{lights.synchronize(std::span(&invalid,1),87);}catch(const FormatError&){rejected=true;}
    require(rejected&&!lights.restore_ready(stale)&&!lights.commit_restore(stale)&&lights.owns_checkpoint(cut),"Rejected light mutation did not invalidate a preparation");
    rejected=false;try{(void)lights.checkpoint();}catch(const FormatError&){rejected=true;}require(rejected,"Incomplete light synchronization was captured");
    auto prepared=lights.prepare_restore(cut),competing=lights.prepare_restore(cut);
    std::exception_ptr thread_error;std::thread observer([&]{try{
        require(!lights.restore_ready(prepared)&&!lights.commit_restore(prepared)&&!lights.owns_checkpoint(cut),"Point-light restore crossed its executor");
        bool denied{};try{(void)lights.checkpoint();}catch(const FormatError&){denied=true;}require(denied,"Foreign point-light executor captured state");
        denied=false;try{(void)lights.prepare_restore(cut);}catch(const FormatError&){denied=true;}require(denied,"Foreign point-light executor prepared state");
    }catch(...){thread_error=std::current_exception();}});observer.join();if(thread_error)std::rethrow_exception(thread_error);
    require(lights.restore_ready(prepared)&&lights.commit_restore(prepared)&&!lights.commit_restore(prepared)&&!lights.commit_restore(competing)&&
        lights.checkpoint().tick()==cut.tick(),"Point-light exact restoration was stale, consumed twice or lost its successful-sync tick");
    owner=cut_owner;require(continuation()==expected,"Restored point-light datum/spatial allocator, query epoch, queues or four-view selection/receiver history diverged");
    std::cout<<"PASS owning pStencil point-light checkpoint: actual plasma lifetime/spatial salts, four-view query/selection/receiver continuation and allocator reuse; cancellation, stale/foreign/executor and consumed guards\n";
    std::cout<<"LIMIT point-light fixture uses declared sphere collection and no-fog inputs; it does not qualify a live camera, material/GPU publication or native attached-owner callback branch\n";
}
}
int main(int argc,char** argv){try {
    require(argc==2,"Supply current Xbox Lockout cache");Cache cache(argv[1]);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& t) {
        return t.group=="proj" && t.name=="objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade";
    });require(tag!=cache.tags().end(),"authored plasma projectile missing");
    pstencil::PointLights lights(cache,0);require(lights.has(tag->datum),"plasma direct light not admitted");
    auto root=source_object_transform(cache.spawns().at(0).position,{1,0,0},{0,0,1},1,false);
    pstencil::PointLightOwner owner{0xe2710007u,tag->datum,root,{},{}};
    lights.synchronize(std::span(&owner,1),10);const auto born=lights.lights();
    require(born.size()==1,"plasma did not produce exactly its authored glow");
    const auto authored=load_effect_light(cache,born[0].definition);
    const auto expected=source_effect_light_state(authored,1,0,30,cache.original_data()->fade_curve);
    require(born[0].state.diffuse.x==expected.diffuse.x && born[0].state.diffuse.y==expected.diffuse.y &&
        born[0].state.diffuse.z==expected.diffuse.z,"plasma color differs from authored endpoints");
    require(expected.diffuse.z>expected.diffuse.x*3 && expected.diffuse.y>expected.diffuse.x*3,
        "plasma light lost its authored blue/cyan color");
    require(born[0].radius==.8f,"plasma light radius differs from authored value");
    for(unsigned tick=11;tick<80;++tick) {
        owner.root.position.x+=.002f;owner.root.position.y-=.001f;
        lights.synchronize(std::span(&owner,1),tick);
        const auto moving=lights.lights();require(moving.size()==1 && moving[0].identity==born[0].identity,
            "moving plasma glow lost its stable generation");
        require(moving[0].position.x==owner.root.position.x && moving[0].position.y==owner.root.position.y,
            "plasma glow did not follow its current source pose");
        require(moving[0].registration.reference_head!=0xffffffffu,"moving point light lost spatial registration");
    }
    lights.synchronize({},80);require(lights.lights().empty(),"retired plasma grenade left a phantom light");
    owner.object=0xe2720007u;lights.synchronize(std::span(&owner,1),81);
    require(lights.lights().at(0).identity!=born[0].identity,"reused object slot retained stale light generation");
    checkpoint_roundtrip(cache,lights,owner);
    std::cout<<"PASS: authored plasma blue/cyan RGB, radius, 69 moving poses, stable identity, registration, retirement and slot reuse\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#include "halo2/prop_sandbox.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <tuple>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid/active world component continuation accepted");}
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
bool same(const Transform &a,const Transform &b){
    if(!same(a.scale,b.scale) || !same(a.position,b.position))return false;
    for(unsigned i=0;i<3;++i)if(!same(a.basis[i],b.basis[i]))return false;
    return true;
}
bool same(const PropDamageOwner &a,const PropDamageOwner &b){return std::tie(a.player,a.unit,a.designator,a.native_owner)==std::tie(b.player,b.unit,b.designator,b.native_owner);}
bool same(const RechargeState &a,const RechargeState &b){return same(a.body,b.body)&&same(a.shield,b.shield)&&
    std::tie(a.body_stun,a.shield_stun,a.dead,a.paused,a.body_recharging,a.shield_recharging,a.shield_damaged,a.shield_depleted)==
    std::tie(b.body_stun,b.shield_stun,b.dead,b.paused,b.body_recharging,b.shield_recharging,b.shield_damaged,b.shield_depleted);}
bool same(const PropSandboxEvent &a,const PropSandboxEvent &b){return a.handle==b.handle && a.effect==b.effect &&
    a.damage==b.damage && a.remove==b.remove && same(a.owner,b.owner) && same(a.position,b.position) && same(a.forward,b.forward) &&
    same(a.effect_position,b.effect_position) && same(a.effect_forward,b.effect_forward);}
void same(const PropSandbox::AuthorityCheckpoint &a,const PropSandbox::AuthorityCheckpoint &b){
    check(a.version==b.version && a.scene_binding==b.scene_binding && a.definition_binding==b.definition_binding &&
        a.collision_dirty==b.collision_dirty && a.props.size()==b.props.size() && a.pending_events.size()==b.pending_events.size(),"Prop continuation bank shape/binding differs");
    for(const auto &[h,x]:a.props){const auto &y=b.props.at(h);const auto &p=x.state,&q=y.state;
        check(p.handle==q.handle && p.placement==q.placement && p.definition==q.definition && same(p.vitality,q.vitality) &&
            same(p.owner,q.owner) && x.active==y.active && same(x.root,y.root) && x.nodes.size()==y.nodes.size() && p.sections.size()==q.sections.size(),"Prop continuation identity/vitality/pose differs");
        for(unsigned i=0;i<x.nodes.size();++i)check(same(x.nodes[i],y.nodes[i]),"Prop continuation WORLD node differs");
        for(unsigned i=0;i<p.sections.size();++i)check(std::tie(p.sections[i].destroyed,p.sections[i].pending,p.sections[i].damage)==
            std::tie(q.sections[i].destroyed,q.sections[i].pending,q.sections[i].damage),"Prop continuation response countdown differs");
        const auto &r=p.regions,&s=q.regions;
        check(r.looping_effects==s.looping_effects && r.selection.variant_index==s.selection.variant_index && r.selection.states==s.selection.states &&
            r.selection.permutations==s.selection.permutations && r.selection.unresolved_rng_regions==s.selection.unresolved_rng_regions,"Prop continuation region/effect bank differs");
    }
    for(unsigned i=0;i<a.pending_events.size();++i)check(same(a.pending_events[i],b.pending_events[i]),"Prop continuation pending event/attribution differs");
}
bool same(const CollisionTriangle &a,const CollisionTriangle &b){return same(a.a,b.a)&&same(a.b,b.b)&&same(a.c,b.c)&&
    same(a.source_normal,b.source_normal)&&a.material==b.material&&a.flags==b.flags&&
    std::tie(a.source.bsp,a.source.record,a.source.surface,a.source.instance,a.source.plane)==
    std::tie(b.source.bsp,b.source.record,b.source.surface,b.source.instance,b.source.plane);}
void same(std::span<const CollisionTriangle> a,std::span<const CollisionTriangle> b){
    check(a.size()==b.size(),"Continuation triangle count differs");
    for(unsigned i=0;i<a.size();++i)check(same(a[i],b[i]),"Continuation triangle value/order/identity differs");
}
void props(const char *xbe,const char *path) {
    Cache first_cache(path,{},OriginalData::load_xbe(xbe)),second_cache(path,{},OriginalData::load_xbe(xbe));
    auto first=first_cache.load_content(0,true),second=second_cache.load_content(0,true);
    SceneObjects source_scene(first,0,SourceSceneGameContext{2,2}),target_scene(second,0,SourceSceneGameContext{2,2});
    PropSandbox source(first),target(second);source.synchronize(source_scene);target.synchronize(target_scene);
    const auto all=source.states();const auto at=std::ranges::find_if(all,[](const auto &s){return s.definition==0xe6f80583u;});
    check(at!=all.end(),"Real Lockout power module unavailable");const auto h=at->handle;const auto placement=at->placement;
    check(first.prop_sandbox[placement].get()!=second.prop_sandbox[placement].get(),"Prop test did not create independent definition owners");
    const auto w=std::ranges::find_if(first.weapons,[](const auto &row){return row.second.smg.has_value();});
    check(w!=first.weapons.end() && w->second.damage_amount,"Real authored SMG damage unavailable");
    const auto amount=w->second.damage_amount->upper_minimum;
    const PropDamageOwner owner{0xd1230001u,0xd4560002u,2,3}; // Explicit captured native test attribution.
    for(unsigned i=0;i<100 && !source.state(h).vitality.dead;++i)source.damage(h,w->second,amount,owner);
    check(source.state(h).vitality.dead && source.state(h).sections.at(1).pending==48,"Actual authored death/countdown stimulus changed");
    check(source.tick().empty() && source.tick().empty(),"Authored delayed response dispatched prematurely");
    const auto scene=source_scene.authority_checkpoint();const auto saved=source.authority_checkpoint(scene);
    const auto before=target.authority_checkpoint(target_scene.authority_checkpoint());
    {auto staged=target.prepare_authority(saved,scene);rejects([&]{target.tick();});rejects([&]{target.synchronize(target_scene);});
        rejects([&]{target.authority_checkpoint(scene);});rejects([&]{target.prepare_authority(saved,scene);});
        check(!source.commit_authority(std::move(staged)),"Foreign prop component committed");}
    same(before,target.authority_checkpoint(target_scene.authority_checkpoint()));
    auto bad=saved;bad.version=2;rejects([&]{target.prepare_authority(bad,scene);});
    bad=saved;bad.props.at(h).state.definition^=1;rejects([&]{target.prepare_authority(bad,scene);});
    bad=saved;bad.props.at(h).state.handle^=0x10000u;rejects([&]{target.prepare_authority(bad,scene);});
    bad=saved;bad.props.at(h).nodes.clear();rejects([&]{target.prepare_authority(bad,scene);});
    bad=saved;bad.props.at(h).state.sections[1].pending=39;rejects([&]{target.prepare_authority(bad,scene);});
    bad=saved;bad.props.at(h).state.regions.selection.states.clear();rejects([&]{target.prepare_authority(bad,scene);});
    auto wrong_scene=scene;wrong_scene.slots.erase(h);rejects([&]{target.prepare_authority(saved,wrong_scene);});
    auto changed=second;auto changed_definition=std::make_shared<PropSandboxDefinition>(*changed.prop_sandbox[placement]);
    changed_definition->sections[1].responses[0].delay+=.1f;changed.prop_sandbox[placement]=changed_definition;
    PropSandbox other(changed);rejects([&]{other.prepare_authority(saved,scene);});
    auto changed_damage=second;check(!changed_damage.damage_materials.empty(),"Real global material table unavailable");
    changed_damage.damage_materials[0].armor_a^=1;PropSandbox other_damage(changed_damage);
    rejects([&]{other_damage.prepare_authority(saved,scene);});
    auto staged_scene=target_scene.prepare_authority(scene);auto staged=target.prepare_authority(saved,scene);
    check(target_scene.commit_authority(std::move(staged_scene)) && target.commit_authority(std::move(staged)) &&
        !target.commit_authority(std::move(staged)),"Composed scene/prop publication failed or reused token");
    unsigned source_events=0,target_events=0;
    for(unsigned tick=0;tick<12;++tick) {
        const auto x=source.tick(),y=target.tick();check(x.size()==y.size(),"Future prop dispatch tick/order differs");
        for(unsigned i=0;i<x.size();++i){check(same(x[i],y[i]) && same(x[i].owner,owner),"Future actual response lost captured attribution");
            ++source_events;++target_events;if(x[i].remove){source_scene.destroy(x[i].handle);target_scene.destroy(y[i].handle);}}
        source.synchronize(source_scene);target.synchronize(target_scene);
        same(source.authority_checkpoint(source_scene.authority_checkpoint()),target.authority_checkpoint(target_scene.authority_checkpoint()));
        same(source.triangles(),target.triangles());
    }
    check(source_events==1 && target_events==1,"Imported actual delayed response was lost or emitted twice");
    const auto replacement=source_scene.create_placement(placement,100);
    check(replacement==target_scene.create_placement(placement,100) && replacement!=h,"Future prop full-salt recreation diverged");
    source.synchronize(source_scene);target.synchronize(target_scene);
    same(source.authority_checkpoint(source_scene.authority_checkpoint()),target.authority_checkpoint(target_scene.authority_checkpoint()));
    rejects([&]{source.damage(null_datum,w->second,amount);});source.authority_checkpoint(source_scene.authority_checkpoint());
    auto replica=target;replica.apply_regions(replica.region_snapshot());check(replica.replicated(),"Display role was not retained");
    rejects([&]{replica.authority_checkpoint(scene);});rejects([&]{replica.prepare_authority(saved,scene);});
    // Retire all source definition allocations before asking the replacement
    // to use its pointers again. No later source prop operation is admitted.
    first.prop_sandbox.clear();check(!target.triangles().empty(),"Imported props retained source definition pointers");

    // Explicit native definition fixture exercises an immediate queued request.
    // The unchanged cache fixture above separately proves the original delay.
    auto immediate_content=second;auto immediate_definition=std::make_shared<PropSandboxDefinition>(*second.prop_sandbox[placement]);
    immediate_definition->sections[1].responses[0].delay=0;immediate_content.prop_sandbox[placement]=immediate_definition;
    SceneObjects immediate_scene(immediate_content,0,SourceSceneGameContext{2,2});PropSandbox immediate(immediate_content),imported(immediate_content);
    immediate.synchronize(immediate_scene);const auto ih=immediate_scene.objects()[scene.slots.at(h)].handle;
    check(immediate.state(ih).placement==placement,"Immediate fixture lost authored placement");
    for(unsigned i=0;i<100 && !immediate.state(ih).vitality.dead;++i)immediate.damage(ih,w->second,amount,owner);
    const auto immediate_scene_state=immediate_scene.authority_checkpoint();const auto queued=immediate.authority_checkpoint(immediate_scene_state);
    check(queued.pending_events.size()==1,"Immediate fixture did not produce a pending actual native request");
    auto qp=imported.prepare_authority(queued,immediate_scene_state);check(imported.commit_authority(std::move(qp)),"Queued request publication failed");
    unsigned callbacks=0;const auto delivered=imported.tick([&](const auto &event){++callbacks;check(same(event,queued.pending_events[0]),"Imported queued request changed");
        rejects([&]{imported.authority_checkpoint(immediate_scene_state);});});
    check(callbacks==1 && delivered.size()==1 && imported.tick().empty(),"Imported pending event was not consumed once");
    qp=imported.prepare_authority(queued,immediate_scene_state);check(imported.commit_authority(std::move(qp)),"Queued request restore failed");
    rejects([&]{imported.tick([](const auto &){throw FormatError("Declared callback interruption");});});
    rejects([&]{imported.authority_checkpoint(immediate_scene_state);});
    qp=imported.prepare_authority(queued,immediate_scene_state);check(imported.commit_authority(std::move(qp)) && imported.tick().size()==1,"Faulted prop owner did not recover from admitted checkpoint");
    std::cout<<"PASS actual Lockout prop metadata/damage/countdown, twelve future dispatch/collision steps, full-salt recreation; explicit immediate request, callback guard/recovery, immutable admission and replica rejection\n";
}
void scenery(const char *xbe,const char *path) {
    Cache a(path,{},OriginalData::load_xbe(xbe)),b(path,{},OriginalData::load_xbe(xbe));
    auto first=a.load_content(0,true),second=b.load_content(0,true);
    SceneObjects source_scene(first,0,SourceSceneGameContext{2,2}),target_scene(second,0,SourceSceneGameContext{2,2});
    StaticSceneryCollision source(first),target(second);source.synchronize(source_scene);target.synchronize(target_scene);
    const auto initial=source.authority_checkpoint(source_scene.authority_checkpoint());
    check(!initial.owners.empty() && !initial.triangles.empty(),"Real static scenery owner/collision fixture unavailable");
    const auto chosen=std::ranges::find_if(initial.owners,[&](const auto &entry){return
        !first.static_scenery_collision[entry.second.placement]->permutations.empty() &&
        std::ranges::any_of(initial.triangles,[&](const auto &t){return t.source.instance==int(entry.second.placement);});});
    check(chosen!=initial.owners.end(),"No concrete authored static collision owner");
    const auto h=chosen->first;const auto placement=chosen->second.placement;
    check(first.static_scenery_collision[placement].get()!=second.static_scenery_collision[placement].get(),"Scenery did not bind independent definition owners");
    auto &object=source_scene.get(h);object.position.x+=.125f;object.solved_transform=chosen->second.root;
    object.solved_transform->position=object.position;
    object.solved_nodes=pose_model_nodes_world(first.static_scenery_collision[placement]->hierarchy,*object.solved_transform);
    check(source.synchronize(source_scene),"Actual scenery pose did not rebuild collision");
    const auto scene=source_scene.authority_checkpoint();const auto saved=source.authority_checkpoint(scene);
    {auto staged=target.prepare_authority(saved,scene);rejects([&]{target.synchronize(target_scene);});rejects([&]{target.authority_checkpoint(scene);});
        check(!source.commit_authority(std::move(staged)),"Foreign scenery publication accepted");}
    same(initial.triangles,target.triangles());
    auto bad=saved;bad.owners.at(h).placement=first.objects.size();rejects([&]{target.prepare_authority(bad,scene);});
    bad=saved;bad.triangles[0].material^=1;rejects([&]{target.prepare_authority(bad,scene);});
    bad=saved;bad.owners.at(h).solved_nodes=std::vector<Transform>{};rejects([&]{target.prepare_authority(bad,scene);});
    auto changed=second;auto d=std::make_shared<StaticSceneryCollisionDefinition>(*changed.static_scenery_collision[placement]);
    d->permutations[0]^=1;changed.static_scenery_collision[placement]=d;StaticSceneryCollision other(changed);
    rejects([&]{other.prepare_authority(saved,scene);});
    auto staged_scene=target_scene.prepare_authority(scene);auto staged=target.prepare_authority(saved,scene);
    check(target_scene.commit_authority(std::move(staged_scene)) && target.commit_authority(std::move(staged)),"Composed scenery publication failed");
    same(source.triangles(),target.triangles());
    for(unsigned tick=0;tick<8;++tick) {
        source_scene.get(h).bsp_hidden=target_scene.get(h).bsp_hidden=(tick&1)!=0;
        source.synchronize(source_scene);target.synchronize(target_scene);same(source.triangles(),target.triangles());
    }
    source_scene.destroy(h);target_scene.destroy(h);source.synchronize(source_scene);target.synchronize(target_scene);same(source.triangles(),target.triangles());
    check(source_scene.create_placement(placement,200)==target_scene.create_placement(placement,200),"Future scenery generation differs");
    source.synchronize(source_scene);target.synchronize(target_scene);same(source.triangles(),target.triangles());
    first.static_scenery_collision.clear();
    const auto current=target_scene.objects()[scene.slots.at(h)].handle;
    target_scene.get(current).position.y+=.25f;
    check(target.synchronize(target_scene) && !target.triangles().empty(),"Imported scenery retained source content pointers");
    std::cout<<"PASS actual static scenery definitions, exact cached triangle order/source identity, solved WORLD nodes, local rebind, malformed/cancelled imports and subsequent hide/delete/recreate\n";
}
}
int main(int argc,char **argv)try {
    std::cout<<std::unitbuf;check(argc==4,"Supply TU1.5 XBE, prop map, scenery map");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    props(argv[1],argv[2]);scenery(argv[1],argv[3]);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

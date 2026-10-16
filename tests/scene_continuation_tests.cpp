#include "halo2/scene.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid or active scene continuation accepted");}
LevelContent content() {
    LevelContent c;c.bsp_count=2;
    for(int i=0;i<3;++i) {
        ScenarioObject p;p.type=6;p.placement=i;p.name=i;p.definition=100+i;
        p.position={float(i),0,0};c.objects.push_back(p);c.object_names.push_back({"object "+std::to_string(i),6,i});
    }
    return c;
}
void same(const SceneObjects &a,const SceneObjects &b) {
    const auto x=a.authority_checkpoint(),y=b.authority_checkpoint();
    check(x.content_binding==y.content_binding && x.bsp==y.bsp && x.slots==y.slots && x.names==y.names &&
        x.objects.size()==y.objects.size() && x.functions.size()==y.functions.size(),"Restored scene lookup/state shape differs");
    for(std::size_t i=0;i<x.objects.size();++i) {
        const auto &p=x.objects[i],&q=y.objects[i];
        check(p.handle==q.handle && p.placement==q.placement && p.bsp==q.bsp && p.created_tick==q.created_tick &&
            p.visible==q.visible && p.script_hidden==q.script_hidden && p.bsp_hidden==q.bsp_hidden &&
            p.inventory_attached==q.inventory_attached && p.cinematic_lod==q.cinematic_lod &&
            p.attachment_parent==q.attachment_parent && p.render_attribution_at_birth==q.render_attribution_at_birth &&
            p.solved_nodes.has_value()==q.solved_nodes.has_value() && p.solved_transform.has_value()==q.solved_transform.has_value() &&
            std::bit_cast<std::uint32_t>(p.position.x)==std::bit_cast<std::uint32_t>(q.position.x),"Restored scene generation/metadata differs");
        if(p.solved_transform)check(p.solved_transform->position.y==q.solved_transform->position.y,"Restored solved root differs");
        if(p.solved_nodes)check(p.solved_nodes->size()==q.solved_nodes->size() &&
            p.solved_nodes->front().position.z==q.solved_nodes->front().position.z,"Restored WORLD hierarchy differs");
    }
    for(const auto &[key,f]:x.functions) {
        const auto &g=y.functions.at(key);check(f.from==g.from && f.to==g.to && f.start==g.start && f.duration==g.duration,
            "Restored interpolation operands differ");
    }
    check(std::ranges::equal(a.object_lifetimes().events(),b.object_lifetimes().events()),"Restored scene lifetime history differs");
}
void owned() {
    auto first=content(),second=first;int source_calls=0,target_calls=0;SceneObjects *source_ptr=nullptr;
    auto resolver=[&](int &count,std::uint32_t definition)->std::optional<ObjectLightContactDefinition>{
        ++count;if(source_ptr)rejects([&]{source_ptr->authority_checkpoint();});
        ObjectLightContactDefinition d;d.datum=definition;return d;
    };
    SceneObjects source(first,0,{},[&](auto d){return resolver(source_calls,d);});
    SceneObjects target(second,0,{},[&](auto d){++target_calls;ObjectLightContactDefinition v;v.datum=d;return v;});
    source_ptr=&source;
    const auto object=source.resolve(0);source.visible(object,false);source.set_function_variable(object,17,0,0,0);
    source.set_function_variable(object,17,1,60,10);source.destroy(source.resolve(1));source.switch_bsp(1);
    auto &o=source.get(object);o.position.x=-0.f;o.cinematic_lod=true;o.inventory_attached=true;o.attachment_parent=source.resolve(2);
    o.solved_transform=Transform{};o.solved_transform->position.y=4;o.solved_nodes=std::vector<Transform>(2);o.solved_nodes->front().position.z=9;
    const auto saved=source.authority_checkpoint();const auto before=target.authority_checkpoint();
    {
        auto staged=target.prepare_authority(saved);
        check(staged.for_owner(target) && !source.commit_authority(std::move(staged)),"Foreign scene accepted publication");
        rejects([&]{target.get(target.resolve(0));});rejects([&]{target.visible(target.resolve(0),true);});
        rejects([&]{target.authority_checkpoint();});rejects([&]{target.prepare_authority(saved);});
    }
    check(target.authority_checkpoint().names==before.names,"Cancellation changed scene");
    auto bad=saved;bad.version=2;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.content_binding[0]^=std::byte{1};rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.slots.begin()->second=999;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.names[0]=null_datum;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.objects[0].solved_nodes->front().scale=std::numeric_limits<float>::infinity();rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.functions.begin()->second.duration=-1;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.objects[0].handle^=0x10000u;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.local_lifetimes->header(object)[3]^=1;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.local_lifetimes.emplace(1);bad.objects.clear();bad.slots.clear();bad.functions.clear();
    std::fill(bad.names.begin(),bad.names.end(),null_datum);rejects([&]{target.prepare_authority(bad);});
    auto publish=target.prepare_authority(saved);check(target.commit_authority(std::move(publish)) &&
        !target.commit_authority(std::move(publish)),"Scene publication failed or token reused");same(source,target);
    const int source_before=source_calls,target_before=target_calls;
    check(source.create(1,false,97)==target.create(1,false,97),"Future scene birth salted identity diverged");
    check(source_calls==source_before+1 && target_calls==target_before+1,"Import replaced local definition callbacks");
    for(int tick=10;tick<100;++tick)check(source.function_variable(object,17,tick)==target.function_variable(object,17,tick),"Future interpolation diverged");
    source.destroy(object);target.destroy(object);same(source,target);
    rejects([&]{source.create_placement(999);});source.authority_checkpoint(); // Rejected precondition does not fault.

    NativeObjectLifetimes a,b;SceneObjects shared(first,0,{}, {},&a),replacement(second,0,{}, {},&b);
    shared.destroy(shared.resolve(1));const auto state=shared.authority_checkpoint();
    auto staged=replacement.prepare_authority(state,a);check(!replacement.commit_authority(std::move(staged)),"Unpublished shared pool accepted");
    b=a;check(replacement.commit_authority(std::move(staged)),"Admitted shared owned bank failed");same(shared,replacement);
    SceneObjects replica(first,0);replica.adopt_source_replicas({},{});rejects([&]{replica.authority_checkpoint();});
    rejects([&]{replica.prepare_authority(state,a);});
    std::cout<<"PASS scene owned continuation: exact metadata/poses/interpolation, shared lifetime ordering, local callbacks, cancellation and malformed admission\n";
}
struct Directory {
    std::filesystem::path path;
    Directory(){auto p=(std::filesystem::temp_directory_path()/"halo2-scene-continuation-XXXXXX").string();check(::mkdtemp(p.data()),"Cannot make native directory");path=p;}
    ~Directory(){std::error_code e;std::filesystem::remove_all(path,e);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t time{};mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return time;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
void native(const char *xbe,const char *map) {
    Cache cache(map,{},OriginalData::load_xbe(xbe));const auto authored=cache.load_content(0,true);
    const auto variant_type=source_game_variant_type(source_game_variant_default(0));
    check(authored.player.datum!=null_datum && !cache.spawns().empty(),"Actual biped definition/spawn unavailable");
    // Explicit native caller placement, backed by a real cache definition and
    // the actual source biped constructor/destructor. This is not a claimed
    // original scenario palette row or proof of every scenario factory type.
    LevelContent first;first.bsp_count=authored.bsp_count;
    ScenarioObject placement;placement.type=0;placement.placement=0;placement.name=0;
    placement.definition=authored.player.datum;placement.position=cache.spawns().front().position;placement.flags=1;
    first.objects.push_back(placement);first.object_names.push_back({"explicit native integration placement",0,0});auto second=first;
    Directory af,bf;auto ac=std::make_shared<Clock>(),bc=std::make_shared<Clock>();ac->time=1000;bc->time=900000;
    MultiplayerObjectRuntime a(xbe,cache,0,af.path,ac),b(xbe,cache,0,bf.path,bc);
    auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,5);
    a.configure_session(variant,1,30,0x12345678);b.configure_session(variant,1,30,0x12345678);
    NativeObjectLifetimes al(a.memory(),a.object_pool()),bl(b.memory(),b.object_pool());
    SceneObjects *ap=nullptr,*bp=nullptr;unsigned abirth=0,bbirth=0,adeath=0,bdeath=0;bool fail_birth=false;
    SceneObjects source(first,0,SourceSceneGameContext{2,variant_type}, {},&al,{[&](const auto &p,SourceScenarioBirth){++abirth;rejects([&]{ap->authority_checkpoint();});
        const auto result=a.create_biped(p.definition,p.position,{1,0,0});if(fail_birth)throw FormatError("Declared outer callback interruption");return result;},
        [&](auto h){++adeath;rejects([&]{ap->authority_checkpoint();});a.destroy_object(h);}});
    SceneObjects target(second,0,SourceSceneGameContext{2,variant_type}, {},&bl,{[&](const auto &p,SourceScenarioBirth){++bbirth;rejects([&]{bp->authority_checkpoint();});return b.create_biped(p.definition,p.position,{1,0,0});},
        [&](auto h){++bdeath;rejects([&]{bp->authority_checkpoint();});b.destroy_object(h);}});ap=&source;bp=&target;
    auto &am=a.memory(),&bm=b.memory();
    auto handle=source.create_placement(0,19);check(handle!=null_datum,"Actual source biped birth failed");
    source.set_function_variable(handle,17,0,0,19);source.set_function_variable(handle,17,1,50,20);
    source.visible(handle,false);
    auto publish=[&] {
        const auto scene=source.authority_checkpoint();const auto life=al.source_authority_checkpoint();
        auto world=b.prepare_authority(a.authority_checkpoint());
        auto identities=bl.prepare_source_authority(life,world.replacement_memory());
        auto staged=target.prepare_authority(scene,life,world.replacement_memory());
        check(!target.commit_authority(std::move(staged)),"Scene accepted source pool before backing publication");
        check(b.commit_authority(std::move(world)),"Source world publication failed");
        check(!target.commit_authority(std::move(staged)),"Scene accepted source pool before full identity rebind");
        check(bl.commit_source_authority(std::move(identities)) && target.commit_authority(std::move(staged)),"Scene source publication failed");
        same(source,target);check(am.checkpoint()==bm.checkpoint(),"Scene publication changed native bank");
    };
    publish();
    for(int tick=0;tick<10;++tick){ac->time+=33;bc->time+=33;a.step_objects();b.step_objects();check(am.checkpoint()==bm.checkpoint(),"Imported source scene future world tick diverged");}
    source.destroy(handle);target.destroy(handle);check(adeath==1 && bdeath==1,"Local source retirement callbacks changed");
    handle=source.create_placement(0,40);check(target.create_placement(0,40)==handle && abirth==2 && bbirth==1,"Future source scene birth diverged or called old host");
    same(source,target);check(am.checkpoint()==bm.checkpoint(),"Future native scene birth/retirement changed complete bank");
    const auto recovery=source.authority_checkpoint();const auto life=al.source_authority_checkpoint();const auto world=a.authority_checkpoint();
    source.destroy(handle);fail_birth=true;rejects([&]{source.create_placement(0,50);});rejects([&]{source.authority_checkpoint();});
    auto staged_world=a.prepare_authority(world);auto staged_life=al.prepare_source_authority(life,staged_world.replacement_memory());
    auto staged_scene=source.prepare_authority(recovery,life,staged_world.replacement_memory());
    check(a.commit_authority(std::move(staged_world)) && al.commit_source_authority(std::move(staged_life)) &&
        source.commit_authority(std::move(staged_scene)),"Admitted scene recovery failed");same(source,target);
    std::cout<<"PASS actual TU1.5 biped birth/retirement under explicit native scene callbacks, composed world/pool/scene import, local callbacks, ten future source ticks and interrupted-callback recovery\n";
}
}
int main(int argc,char **argv)try {
    std::cout<<std::unitbuf;owned();
    if(argc==3){if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;native(argv[1],argv[2]);}
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

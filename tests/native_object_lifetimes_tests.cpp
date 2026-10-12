#include "halo2/multiplayer_network.hpp"
#include <algorithm>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void require(bool value,const char* message) {if(!value)throw FormatError(message);}
template<class F>void rejects(F action,const char* message) {
    try{action();}catch(const FormatError&){return;}throw FormatError(message);
}
bool same_pool(const DatumPool& a,const DatumPool& b) {
    return a.capacity()==b.capacity() && a.stride()==b.stride() && a.cursor()==b.cursor() &&
        a.high_water()==b.high_water() && a.count()==b.count() && a.next_salt()==b.next_salt() &&
        std::ranges::equal(a.bitmap(),b.bitmap()) && std::ranges::equal(a.elements(),b.elements());
}
void reservations() {
    NativeObjectLifetimes bank(2);
    const auto a=bank.reserve(11),b=bank.reserve(5);
    // Literal name-derived salts/raw zero-based indices, independently covered
    // by the original DatumPool fixture. Ledger semantics below are native.
    require(a==0xe26f0000u && b==0xe2700001u,"shared original datum namespace differs");
    require(!bank.owner(a).published && bank.header(a)[3]==11 && bank.owner(a).pool_rank()==0,
        "reservation manufactured construction or lost its actual header rank");
    require(bank.reserve(0)==null_datum && bank.events().size()==2,"failed allocation published an event");
    bank.publish_native(b,77);bank.release(a);
    require(bank.events().back().kind==NativeObjectLifetimeEventKind::release &&
        !bank.events().back().owner.published,"rollback became a completed object destruction");
    const auto c=bank.reserve(0);bank.publish_native(c,9);
    require(c==0xe2710000u && bank.contains(b) && !bank.contains(a),"shared slot reuse accepted an old salt");
    rejects([&]{bank.publish_native(c,9);},"duplicate completed construction was admitted");
    rejects([&]{bank.release(a);},"stale release changed a reallocated owner");
    const auto saved=bank;bank.release(c);bank.release(b);
    require(saved.contains(c) && saved.contains(b) && saved.pool().count()==2,
        "value checkpoint shared mutable live allocation state");
    bank=saved;require(same_pool(bank.pool(),saved.pool()),"restore lost original pool storage");
    bank.release(c);const auto d=bank.reserve(2);bank.publish_native(d,null_datum);
    require(bank.owner(d).published && bank.owner(d).definition==null_datum,
        "unknown definition was guessed at the native storage boundary");
    for(std::size_t i=0;i<bank.events().size();++i)
        require(bank.events()[i].ordinal==i+1,"native mutation order is not monotonic after restore");
}
LevelContent placements() {
    LevelContent content;
    for(int type:{11,6,7}) {
        ScenarioObject object;object.type=type;object.definition=std::uint32_t(100+type);
        object.name=int(content.object_names.size());object.placement=0;
        object.position={float(type),2,3};
        content.object_names.push_back({"shared identity control",type,0});
        content.objects.push_back(object);
    }
    return content;
}
void scene_handoff() {
    const auto content=placements();NativeObjectLifetimes bank(8);
    const auto unit=bank.reserve(0);bank.publish_native(unit,1);
    SceneObjects scene(content,0,{},{},&bank);
    require(&scene.object_lifetimes()==&bank && bank.pool().count()==4,
        "scene construction did not use the preexisting identity owner");
    require(scene.objects().front().placement==1 && (scene.objects().front().handle&65535u)==1,
        "actual scene constructor order/rank was replaced by local slot numbering");
    for(const auto& object:scene.objects())
        require(scene.get(object.handle).placement==object.placement && bank.owner(object.handle).published,
            "shared scene handle does not resolve its separate placement storage");
    const auto first=scene.resolve(1);const auto before=bank.events().size();
    scene.visible(first,false);require(bank.contains(first) && bank.events().size()==before,
        "hidden retained owner was freed or reconstructed");
    const auto next=scene.create(1,true,9);
    require(next!=first && bank.contains(unit) && !bank.contains(first) && scene.get(next).created_tick==9,
        "same-tick replacement aliased another category or lost its actual clock");
    const auto events=bank.events().subspan(before);
    require(events.size()==3 && events[0].kind==NativeObjectLifetimeEventKind::release &&
        events[1].kind==NativeObjectLifetimeEventKind::reserve &&
        events[2].kind==NativeObjectLifetimeEventKind::publish,"replacement lifetime event order differs");
    rejects([&]{scene.get(unit);},"scene lookup accepted a non-scene shared owner");
    const auto saved_bank=bank;const auto saved_scene=scene;
    scene.destroy(next);bank=saved_bank;scene.restore(saved_scene);
    require(scene.resolve(1)==next && same_pool(bank.pool(),saved_bank.pool()),
        "shared checkpoint handoff lost identity storage");
    SceneObjects standalone(content,0);const auto saved=standalone;
    standalone.destroy_all();standalone.restore(saved);
    require(same_pool(standalone.object_lifetimes().pool(),saved.object_lifetimes().pool()),
        "standalone scene checkpoint did not retain its own value pool");
}
void spawn_reservation_rollback() {
    LevelContent content;content.player.datum=1;content.player.height=1;content.player.radius=.2f;
    content.player.vitality.health=100;
    WeaponDefinition weapon;weapon.datum=3;weapon.capacity=20;weapon.initial_rounds=1;
    content.weapons.emplace(3,weapon);
    for(int i=0;i<2047;++i) {
        ScenarioObject object;object.type=6;object.definition=42;object.name=i;object.placement=i;
        content.objects.push_back(object);content.object_names.push_back({"allocation pressure",6,i});
    }
    auto rules=slayer_rules({"Native allocation pressure",0,{}});rules.weapon=3;
    const std::array<Spawn,1> starts{};MultiplayerMatch match(content,0,starts,rules);
    const auto& bank=match.simulation().object_lifetimes();const auto before=bank.events().size();
    rejects([&]{match.join("Unpublished unit");},"starting weapon allocation did not reach shared capacity");
    require(bank.pool().count()==2047 && bank.events().size()==before+2,
        "failed native spawn leaked the unpublished unit reservation");
    const auto events=bank.events().subspan(before);
    require(events[0].kind==NativeObjectLifetimeEventKind::reserve &&
        events[1].kind==NativeObjectLifetimeEventKind::release &&
        events[0].owner.handle==events[1].owner.handle && !events[1].owner.published,
        "real spawn rollback was recorded as a successful native unit construction");
}
void live_match(const char* map,const char* shared,const char* executable) {
    const auto cache=std::make_shared<Cache>(map,shared,OriginalData::load_xbe(executable));
    auto content=cache->load_content(0,true);content.kill_triggers.clear();
    auto rules=slayer_rules({"Shared lifetime",0,{{0x59,5}}});
    rules.weapon=cache->slayer_starting_weapon(5);rules.time_ticks=rules.score_limit=0;
    MultiplayerMatch match(content,0,cache->spawns(),rules);
    const auto& bank=match.simulation().object_lifetimes();
    // Actual native admission includes four ssce/type10 roots as well as the
    // fifteen independently probed BLOCs. This is native successful storage,
    // not original successful allocation chronology.
    require(bank.pool().count()==19 && bank.events().size()==38,"actual Lockout cold lifetime census differs");
    std::set<std::uint16_t> slots;
    const auto admit=[&](std::uint32_t handle,std::uint8_t type,std::uint32_t definition) {
        const auto& owner=bank.owner(handle);
        require(owner.type==type && owner.definition==definition && owner.published,
            "live native category storage lacks its completed lifetime publication");
        require(slots.insert(owner.pool_rank()).second,"two live categories share a low16 identity");
    };
    for(const auto& object:match.simulation().scene().objects())if(object.handle!=null_datum)
        admit(object.handle,std::uint8_t(content.objects.at(object.placement).type),
            content.objects.at(object.placement).definition);
    match.join("Lifetime host");match.configure_weapon_pickups();
    require(bool(cache->original_data()->grenade_constants),"original grenade operands missing");
    match.configure_grenades(cache,*cache->original_data()->grenade_constants);
    std::array<Controls,multiplayer_capacity> controls{};controls[0].grenade=true;
    controls[0].yaw=match.unit(0).yaw;controls[0].pitch=-1.2f;
    std::uint32_t grenade=null_datum;bool prepared{},released{},freed{};
    for(unsigned tick=0;tick<125;++tick) {
        match.step(controls);controls[0].grenade=false;slots.clear();
        for(const auto& object:match.simulation().scene().objects())if(object.handle!=null_datum)
            admit(object.handle,std::uint8_t(content.objects.at(object.placement).type),
                content.objects.at(object.placement).definition);
        const auto& unit=match.unit(0);admit(unit.source_datum,0,unit.definition.datum);
        for(const auto handle:unit.source_inventory.slots)if(handle!=null_datum) {
            const auto* weapon=match.weapon_from_datum(handle);require(weapon,"inventory lost its actual weapon");
            admit(handle,2,weapon->definition);
        }
        for(const auto& pickup:match.weapon_pickups())if(pickup.weapon)admit(pickup.handle,2,pickup.definition);
        for(const auto& projectile:match.grenade_projectiles()) {
            admit(projectile.datum,5,projectile.definition);grenade=projectile.datum;
            prepared|=projectile.parent!=null_datum;released|=projectile.parent==null_datum;
        }
        if(grenade!=null_datum && !bank.contains(grenade))freed=true;
    }
    require(prepared && released && freed,"actual grenade prepare/release/free lifetime did not complete");
    require(!match.weapon_pickups().empty(),"actual world pickup construction was not exercised");
    for(std::size_t i=0;i<bank.events().size();++i)
        require(bank.events()[i].ordinal==i+1,"live event lost its native call-order ordinal");
    // Slot zero must survive actual LAN encoding and decoding. The removal is
    // supplied at the declared prop-replica boundary; no damage event is claimed.
    // This controlled scenario contains one actual authored BLOC row and no
    // collision mirrors. Unmodified Lockout starts with ssce owners at slot0.
    auto network_content=content;
    const auto block=std::find_if(content.objects.begin(),content.objects.end(),
        [](const auto& object){return object.type==11;});
    require(block!=content.objects.end(),"actual BLOC row missing from LAN control");
    network_content.objects={*block};network_content.prop_sandbox.clear();
    network_content.static_scenery_collision.clear();
    MultiplayerMatch network_match(network_content,0,cache->spawns(),rules);
    const auto& network_bank=network_match.simulation().object_lifetimes();
    const auto& first=network_match.simulation().scene().objects().front();
    require((first.handle&65535u)==0,"actual first scene owner did not occupy original slot zero");
    const PropRemoval removal{first.handle,std::uint32_t(first.placement),network_content.objects[first.placement].definition};
    auto invalid=removal;invalid.handle=(invalid.handle&0xffff0000u)|2048u;
    rejects([&]{network_match.replicate_prop_removals(std::span(&invalid,1));},"out-of-pool prop identity admitted");
    invalid.handle=0;
    rejects([&]{network_match.replicate_prop_removals(std::span(&invalid,1));},"salt-zero prop identity admitted");
    network_match.replicate_prop_removals(std::span(&removal,1));
    require(!network_bank.contains(removal.handle),"prop removal retained allocation");
    LanHost host(network_match,0,0x534841524544ull);
    LanClient client("127.0.0.1:"+std::to_string(host.port()),0x534841524544ull,1);
    for(unsigned i=0;i<32 && !client.ready();++i) {
        client.send(std::array<Controls,1>{});host.receive(controls);host.broadcast();client.receive();
    }
    require(client.ready() && client.state().prop_removals.size()==1 &&
        client.state().prop_removals.front().handle==removal.handle,"actual LAN dropped the zero-index prop removal");
    MultiplayerMatch replica(network_content,0,cache->spawns(),rules);
    replica.replicate_prop_removals(client.state().prop_removals);
    require(!replica.simulation().object_lifetimes().contains(removal.handle),
        "replica scene did not retire the received full zero-index owner");
    std::cout<<"Lockout: 19 cold roots, units/inventory/world pickups and grenade lifecycle; controlled authored BLOC/actual UDP slot-zero removal passed\n";
}
}
int main(int argc,char** argv) {try {
    require(argc==4,"lifetime test requires actual map/shared maps/update executable");
    reservations();scene_handoff();spawn_reservation_rollback();live_match(argv[1],argv[2],argv[3]);
    std::cout<<"Native shared identity/rollback/reuse/checkpoint gates passed; complete source registration remains separate\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

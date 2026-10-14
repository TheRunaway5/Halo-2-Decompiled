#include "halo2/source_match_authority.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &e){failed=true;std::cout<<"Rejected: "<<e.what()<<'\n';}
    check(failed,"Invalid or active coupled Match authority accepted");}
struct Directory {
    std::filesystem::path path;
    Directory(){auto name=(std::filesystem::temp_directory_path()/"halo2-source-match-XXXXXX").string();
        check(::mkdtemp(name.data()),"Cannot create private source Match files");path=name;}
    ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t time{};mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return time;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
constexpr SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15},std::byte{0x53}};
bool bits(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool bits(Vec3 a,Vec3 b){return bits(a.x,b.x)&&bits(a.y,b.y)&&bits(a.z,b.z);}
void same(const SourceMatchAuthority::Checkpoint &a,const SourceMatchAuthority::Checkpoint &b) {
    const auto &x=a.match,&y=b.match;
    check(x.admitted_binding==y.admitted_binding&&x.configuration_binding==y.configuration_binding&&x.source_players==y.source_players&&
        x.simulation.source_lifetimes==y.simulation.source_lifetimes&&x.simulation.ticks==y.simulation.ticks&&
        x.simulation.random_state==y.simulation.random_state&&x.simulation.scripts==y.simulation.scripts,
        "Coupled native Match clocks, admission or source lifetime history changed");
    check(a.world.world.bank==b.world.world.bank,"Complete restored source bank differs");
    check(x.retained_stats.players==y.retained_stats.players&&x.retained_stats.pvp==y.retained_stats.pvp&&x.retained_stats.teams==y.retained_stats.teams&&
        x.result_events.events==y.result_events.events&&x.result_events.index==y.result_events.index&&x.result_events.last==y.result_events.last&&
        x.retained_recording.enabled==y.retained_recording.enabled&&x.retained_recording.frozen==y.retained_recording.frozen,
        "Coupled native retained statistics/recording changed");
    for(unsigned i=0;i<16;++i){const auto &p=x.players[i],&q=y.players[i];
        check(p.source_datum==q.source_datum&&p.connected==q.connected&&p.source_current_object==q.source_current_object&&
            p.source_configuration==q.source_configuration&&p.source_profile_request==q.source_profile_request&&p.name==q.name&&
            p.score==q.score&&p.kills==q.kills&&p.deaths==q.deaths&&p.respawn.countdown==q.respawn.countdown,
            "Coupled native player/profile/current-unit state changed");
        const auto &u=(i?x.simulation.actors[i-1]:x.simulation.player).value,&v=(i?y.simulation.actors[i-1]:y.simulation.player).value;
        check(u.source_datum==v.source_datum&&u.source_player==v.source_player&&u.exists==v.exists&&u.vitality.dead==v.vitality.dead&&
            bits(u.body.position,v.body.position)&&bits(u.body.velocity,v.body.velocity)&&bits(u.vitality.body,v.vitality.body)&&bits(u.vitality.shield,v.vitality.shield)&&
            u.weapon.loaded==v.weapon.loaded&&u.weapon.reserve==v.weapon.reserve&&u.source_inventory.slots==v.source_inventory.slots&&
            u.source_inventory.current==v.source_inventory.current&&u.grenades==v.grenades&&u.grenade_action.phase()==v.grenade_action.phase()&&
            u.grenade_action.counter()==v.grenade_action.counter()&&u.grenade_action.held()==v.grenade_action.held(),
            "Coupled native unit/inventory/vitality/grenade projection changed");
    }
}
std::uint32_t word(const SourceRuntimeMemory::Checkpoint &bank,std::uint32_t at) {
    for(const auto &r:bank.regions)if(at>=r.address&&std::uint64_t(at)+4<=std::uint64_t(r.address)+r.bytes.size()){
        std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(r.bytes[at-r.address+i])<<(8*i);return value;}
    throw FormatError("Fixture source word unavailable");
}
void put(SourceRuntimeMemory::Checkpoint &bank,std::uint32_t at,std::uint32_t value) {
    for(auto &r:bank.regions)if(at>=r.address&&std::uint64_t(at)+4<=std::uint64_t(r.address)+r.bytes.size()){
        check(r.access==SourceRuntimeMemory::Access::ReadWrite,"Fixture attempted immutable edit");
        for(unsigned i=0;i<4;++i)r.bytes[at-r.address+i]=std::byte(value>>(8*i));return;}
    throw FormatError("Fixture mutable source word unavailable");
}
void run(const char *xbe,const char *map,bool active,bool departure) {
    Cache ac(map,{},OriginalData::load_xbe(xbe)),bc(map,{},OriginalData::load_xbe(xbe));
    const auto ca=ac.load_content(0,true),cb=bc.load_content(0,true);
    check(ca.original_data->image==OriginalData::Image::TitleUpdate15,"Source Match acceptance requires TU1.5");
    Directory af,bf;auto at=std::make_shared<Clock>(),bt=std::make_shared<Clock>();at->time=1000;bt->time=900000;
    MultiplayerObjectRuntime wa(xbe,ac,0,af.path,at),wb(xbe,bc,0,bf.path,bt);
    UiVariant variant{"Slayer",0,{{0x59,5}}};auto rules=slayer_rules(variant);rules.weapon=ac.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
    wa.configure_session(source_game_variant(variant),1,30,rules.random_seed);wb.configure_session(source_game_variant(variant),1,30,rules.random_seed);
    MultiplayerMatch *ap=nullptr,*bp=nullptr;
    MultiplayerMatch a(ca,0,ac.spawns(),rules,{},[&]{return 100000u+(ap?unsigned(ap->simulation().ticks()/30):0);},&wa);ap=&a;
    MultiplayerMatch b(cb,0,bc.spawns(),rules,{},[&]{return 100000u+(bp?unsigned(bp->simulation().ticks()/30):0);},&wb);bp=&b;
    const std::array<unsigned,4> locals{0,1,2,3};
    for(auto *match:{&a,&b}) {
        const auto &cache=match==&a?ac:bc;match->configure_weapon_pickups();
        match->configure_grenades(std::shared_ptr<const Cache>(&cache,[](const Cache*){}),*cache.original_data()->grenade_constants);
        match->configure_melee_animation(std::shared_ptr<const Cache>(&cache,[](const Cache*){}),*cache.original_data()->melee_constants);
        for(unsigned i=0;i<4;++i){const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
            std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
            const auto name="Source seat "+std::to_string(i);const auto profile=native_selected_player_configuration(name,appearance,-1);
            check(match->join(name,-1,profile)==i,"Source Match four-seat admission failed");}
        const auto fp=cache.load_first_person_binding(rules.weapon,2);match->configure_replicated_first_person(fp);match->configure_local_views(locals,fp);
    }
    SourceMatchAuthority source(a,wa),target(b,wb);
    rejects([&]{SourceMatchAuthority wrong(a,wb);});rejects([&]{a.authority_checkpoint(binding);});
    if(active) {
        const auto initial_ammo=a.unit(0).weapon.loaded;
        for(unsigned tick=0;tick<20;++tick){std::array<Controls,16> delivered{};
            for(unsigned i=0;i<4;++i)delivered[i].yaw=a.unit(i).yaw;
            delivered[0].fire=tick<12;delivered[0].reload=tick==12;delivered[0].grenade=tick>=17;
            at->time+=33;a.step(delivered);
        }
        check(a.unit(0).weapon.loaded<initial_ammo,"Actual source firing input did not consume ammunition before handoff");
        const auto &unit=a.unit(0);const auto &magazine=unit.weapon.source_magazines[0];
        check(unit.grenade_action.held()!=null_datum&&std::ranges::any_of(a.grenade_projectiles(),[&](const auto &p){
            return p.datum==unit.grenade_action.held()&&p.parent==unit.source_datum;}),"Source active cut omitted its actual held grenade owner");
        std::cout<<"Actual source active cut: tick "<<a.simulation().ticks()<<", magazine "<<initial_ammo<<" -> "<<unit.weapon.loaded
            <<", magazine mode/ticks "<<magazine.mode<<'/'<<magazine.ticks<<", grenade phase/counter "
            <<unsigned(unit.grenade_action.phase())<<'/'<<unsigned(unit.grenade_action.counter())
            <<", observed source projectiles "<<a.grenade_projectiles().size()<<'\n';
    }
    if(departure) {
        const auto player=a.player(0);const auto unit=a.unit(0);
        check(a.depart(0)&&!a.depart(0),"Native ordinary departure was not admitted exactly once");
        const auto &left=a.player(0);
        check(left.connected&&left.source_datum==player.source_datum&&left.name==player.name&&
            left.source_profile_request==player.source_profile_request&&left.source_configuration==player.source_configuration&&
            (left.source_flags&3u)==2u&&left.source_current_object==null_datum&&left.source_previous_unit==null_datum&&
            left.kills==player.kills&&left.deaths==player.deaths&&left.score==player.score,
            "Native departure discarded retained profile/results or current source unlink");
        check(a.unit(0).exists&&!a.unit(0).vitality.dead&&a.unit(0).weapon.source_datum==unit.weapon.source_datum,
            "Native departure substituted immediate death or weapon drop");
        const auto pending=source.checkpoint(binding);
        {auto staged=target.prepare(pending,binding);rejects([&]{b.depart(1);});}
        auto transfer=target.prepare(pending,binding);
        check(target.commit(std::move(transfer)),"Pending source departure could not continue on its target authority");
        same(source.checkpoint(binding),target.checkpoint(binding));
        std::array<Controls,16> neutral{};for(unsigned i=0;i<4;++i)neutral[i].yaw=a.unit(i).yaw;
        at->time+=33;bt->time+=33;a.step(neutral);b.step(neutral);
        same(source.checkpoint(binding),target.checkpoint(binding));
        check(a.unit(0).vitality.dead&&!a.finished()&&a.available_player_slots()==12&&
            a.player(0).source_datum==player.source_datum&&a.player(0).deaths==player.deaths&&
            a.player(0).kills==player.kills&&a.player(0).score==player.score,
            "Native original departure death retired its player, ended the match or published a numerical death");
        const auto state=a.state();
        check(state.players[0].connected&&state.players[0].dead&&state.players[0].source_player==player.source_datum&&
            state.players[0].source_unit==null_datum&&state.players[0].source_weapon==null_datum&&
            std::ranges::any_of(state.corpses,[&](const auto &c){return c.object==unit.source_datum&&c.player==player.source_datum;}),
            "Native departure lost its retained replicated result row or original corpse");
        check(!a.depart(0),"Settled native departure succeeded twice");
        std::cout<<"Actual source departure: retained player/profile/results, held-grenade owner, one deferred world tick, no native death/score publication and three remaining players\n";
    }
    const auto initial=source.checkpoint(binding);const auto before=wb.memory().checkpoint();
    {auto cancelled=target.prepare(initial,binding);rejects([&]{b.step({});});rejects([&]{wb.step_objects();});
        rejects([&]{target.checkpoint(binding);});check(!source.commit(std::move(cancelled)),"Foreign coupled authority committed");}
    check(wb.memory().checkpoint()==before,"Cancelled coupled Match import changed its bank");
    auto prepared=target.prepare(initial,binding);check(target.commit(std::move(prepared))&&!target.commit(std::move(prepared)),"Coupled Match/world publication failed or reused token");
    same(source.checkpoint(binding),target.checkpoint(binding));
    std::array<Controls,16> inputs{};
    for(unsigned tick=0;tick<40;++tick){
        at->time+=33;bt->time+=33;
        for(unsigned i=0;i<4;++i){inputs[i].yaw=a.unit(i).yaw;inputs[i].forward=(i&1)?0:.1f;}
        a.step(inputs);b.step(inputs);same(source.checkpoint(binding),target.checkpoint(binding));
    }
    const auto saved=source.checkpoint(binding);check(saved.match.simulation.ticks==initial.match.simulation.ticks+40,"Complete source Match did not advance forty actual ticks");
    auto bad=saved;bad.world=initial.world;rejects([&]{target.prepare(bad,binding);});
    bad=saved;auto &bank=bad.world.world.bank;const auto clock=word(bank,0x51df54u);put(bank,clock+8u,41);
    rejects([&]{target.prepare(bad,binding);});
    bad=saved;bad.match.players[1].source_current_object=null_datum;rejects([&]{target.prepare(bad,binding);});
    bad=saved;(*bad.match.players[0].source_configuration)[0x40]^=1;rejects([&]{target.prepare(bad,binding);});
    bad=saved;bad.match.players[0].source_flags^=2u;rejects([&]{target.prepare(bad,binding);});
    bad=saved;const auto census=word(bad.world.world.bank,0x4f54f0u);
    put(bad.world.world.bank,census,word(bad.world.world.bank,census)^1u);
    rejects([&]{target.prepare(bad,binding);});
    bad=saved;const auto game=word(bad.world.world.bank,0x4f3218u);put(bad.world.world.bank,game+0x18u,rules.random_seed^1u);
    rejects([&]{target.prepare(bad,binding);});
    bad=saved;put(bad.world.world.bank,game+0xcu,1u|(60u<<16));rejects([&]{target.prepare(bad,binding);});
    bad=saved;put(bad.world.world.bank,game+0x13cu,4);rejects([&]{target.prepare(bad,binding);});
    bad=saved;bad.match.simulation.unit_interaction_binding[0]^=std::byte{1};rejects([&]{target.prepare(bad,binding);});
    bad=saved;bad.match.simulation.version=1;rejects([&]{target.prepare(bad,binding);});
    bad=saved;const auto fp=word(bad.world.world.bank,0x4f6498u);
    put(bad.world.world.bank,fp+0x20ccu+4u,null_datum);rejects([&]{target.prepare(bad,binding);});
    bad=saved;bad.match.first_person_views[1].unit=null_datum;rejects([&]{target.prepare(bad,binding);});
    bad=saved;put(bad.world.world.bank,census+0x10u,null_datum);rejects([&]{target.prepare(bad,binding);});
    same(source.checkpoint(binding),target.checkpoint(binding));
    std::cout<<"PASS coupled actual TU1.5 source Match/world: four real player records and local views, full bank/pool/scene/inventory/recording restoration, forty future authoritative input/world ticks, cancellation and cross-cut tick/unit/profile rejection\n";
    if(departure)check(a.player(0).source_flags&2u&&b.player(0).source_flags&2u&&a.player(0).deaths==0&&b.player(0).deaths==0,
        "Retained departure or numerical death suppression was lost after source Match continuation");
}
}
int main(int argc,char **argv)try {
    std::cout<<std::unitbuf;if((argc!=3&&argc!=4)||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    const bool departure=argc==4&&std::string_view(argv[3])=="--departure";
    if(argc==4&&std::string_view(argv[3])!="--active"&&!departure)throw FormatError("Unknown source Match fixture mode");
    run(argv[1],argv[2],argc==4,departure);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

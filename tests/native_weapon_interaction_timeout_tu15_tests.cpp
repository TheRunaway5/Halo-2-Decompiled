#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_interaction_hold.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
using Clock=std::chrono::steady_clock;
void need(bool condition,const char* message){if(!condition)throw FormatError(message);}
std::uint32_t now(){return std::uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count());}
struct Case {const char* name;unsigned delay;bool primary{},unconsumed{},next_tap{};};
Json run(const char* map,const char* maps,const Case& test) {
    MultiplayerOptions options;options.map=map;options.shared_maps=maps;options.host_port=1;
    options.local_players=1;options.starting_weapon=5;options.variant.values.set(89,5);options.mute=true;
    options.session_random_seed=0x105240;
    options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/test.name;
    MultiplayerPreparedWorld world(options);
    need(world.cache->is_mcc(),"Interaction timeout requires prepared MCC inputs");
    world.bind_source_local_views(world.local_slots);
    const NetgameEquipment* pickup=nullptr;
    for(const auto& row:world.content.netgame_equipment) {
        if(row.classification || source_netgame_equipment_selector(row,true,world.rules.source_engine)<0 ||
            row.items.size()!=1 || !world.content.weapons.contains(row.items.front().definition))continue;
        if((row.items.front().definition==world.rules.weapon)==!test.primary){pickup=&row;break;}
    }
    need(pickup,"Interaction timeout lacks an authored weapon pickup");
    LanHost host(world.match,0,789,options.variant);LanClient client(host.join_address(),789,1);
    std::array<Controls,multiplayer_capacity> delivered{};std::array<Controls,1> local{};
    for(unsigned i=0;i<32&&!client.ready();++i){client.send(local);host.receive(delivered);host.broadcast();client.receive();}
    need(client.ready(),"Interaction timeout UDP admission failed");
    const auto slot=client.slots()[0];auto& native=*world.source_objects;
    const auto latch=test.primary?0x10u:0x20u;
    NativeInteractionHold hold;Json commands=Json::array(),stores=Json::array();
    std::optional<Json> previous;std::string phase="pickup";std::uint32_t selector{};
    native.observe_physics_stores_read_only([&](auto entry,const SourcePhysicsContactRecordStore& store) {
        if(!selector || selector<store.address || std::uint64_t(selector)>=std::uint64_t(store.address)+store.written.size())return;
        const auto offset=selector-store.address;
        if(store.before.at(offset)==store.written.at(offset))return;
        need(stores.size()<64,"Interaction selector trace exceeded its native write bound");
        stores.push_back({{"phase",phase},{"entry",entry},{"instruction",store.instruction},
            {"before",std::to_integer<unsigned>(store.before[offset])},
            {"after",std::to_integer<unsigned>(store.written[offset])}});
    });
    struct Observation {MultiplayerObjectRuntime& native;~Observation(){native.observe_physics_stores_read_only({});}} observation{native};
    const auto step=[&](bool send=true,bool receive=true) {
        if(send) {
            const auto [down,duration]=hold.sample(now());
            if(test.primary){local[0].pickup=down;local[0].pickup_milliseconds=duration;}
            else {local[0].dual_wield=down;local[0].dual_wield_milliseconds=duration;}
            client.send(local);hold.consume();
        }
        host.receive(delivered);
        const auto before=world.match.state();const auto unit=before.players[slot].source_unit;
        if(unit!=null_datum) {
            auto position=pickup->position;position.x-=.2f;
            native.tu15_world().set_object_position(unit,position);
            selector=native.source_object_body(unit)+0x212u+(test.primary?0u:1u);
        }
        world.match.step(delivered);host.broadcast();if(receive)client.receive();
        need(client.error().empty(),"Interaction timeout UDP state delivery failed");
        const auto state=world.match.state();const auto& player=state.players[slot];
        const auto word=[&](auto at){return Bytes(native.memory().view(at,4)).u32(0);};
        const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return native.memory().view(at,n);}};
        const auto record=source_raw_datum_lookup(word(0x4f54f4u),player.source_player,storage);
        need(record,"Interaction timeout lost its full native player identity");
        const auto flags=Bytes(native.memory().view(record+2u,2)).u16(0);
        need(flags==player.source_flags,"Published interaction latches differ from their native player record");
        const auto& inventory=world.match.unit(slot).source_inventory;const auto& command=delivered[slot];
        const Json row{{"flags",flags},{"previous_buttons",Bytes(native.memory().view(record+0x3cu,2)).u16(0)},
            {"pickup",command.pickup},{"dual",command.dual_wield},
            {"pickup_ms",command.pickup_milliseconds},{"dual_ms",command.dual_wield_milliseconds},
            {"fire",command.fire},{"forward",command.forward},{"right",command.right},
            {"slots",inventory.slots},{"current",inventory.current},{"requested",inventory.requested},
            {"offhand",player.secondary_weapon.datum}};
        if(!previous || *previous!=row){auto value=row;value["phase"]=phase;value["tick"]=state.tick;commands.push_back(value);previous=row;}
        return row;
    };
    const auto acquired=[&](const Json& row) {
        if(!(row["flags"].get<unsigned>()&latch))return false;
        for(auto item:world.match.unit(slot).source_inventory.slots)
            if(item!=null_datum && native.object_definition(item)==pickup->items.front().definition)return true;
        return false;
    };
    hold.set(true,now());Json state;
    if(test.unconsumed) {
        state=step();need(!acquired(state),"Unconsumed interaction control was already admitted");
    }else {
        bool admitted{};
        for(unsigned i=0;i<90;++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(33));state=step();
            if(acquired(state)){admitted=true;break;}
        }
        need(admitted,"Original native weapon pickup did not commit");
        phase="settle";
        for(unsigned i=0;i<60 && state["current"]!=state["requested"];++i)state=step();
        need(state["current"]==state["requested"],"Native pickup selectors did not reach their requested owners");
    }
    const auto before=state;unsigned expired_samples{};bool unrelated_neutral=true;
    if(test.delay) {
        phase=test.delay==2?"timeout":"fresh_delay";
        if(!test.unconsumed){local[0].fire=true;local[0].forward=1.f;local[0].right=.5f;state=step();}
        const auto started=Clock::now(),until=started+std::chrono::milliseconds(660);
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(33));state=step(test.delay==1,test.delay==1);
            if(test.delay==2 && Clock::now()-started>std::chrono::milliseconds(550)) {
                ++expired_samples;
                unrelated_neutral&=!state["fire"].get<bool>() && state["forward"]==0.f && state["right"]==0.f;
            }
        }while(Clock::now()<until);
    }
    phase="release_completion";local[0].fire=false;local[0].forward=local[0].right=0.f;
    hold.set(false,now());state=step();phase="released";
    for(unsigned i=0;i<5;++i)state=step();
    const bool retained=test.unconsumed?state["offhand"]!=null_datum:
        state["slots"]==before["slots"] && state["current"]==before["current"];
    const bool released=!(state["flags"].get<unsigned>()&latch) &&
        !(state["previous_buttons"].get<unsigned>()&(test.primary?1u:0x10u));
    bool next_tap=true;
    if(test.next_tap && retained && released) {
        phase="new_physical_tap";hold.set(true,now());step();hold.set(false,now());step();
        for(unsigned i=0;i<5;++i)state=step();next_tap=state["offhand"]==null_datum;
    }else if(test.next_tap)next_tap=false;
    const bool passed=retained && released && next_tap && unrelated_neutral && (test.delay!=2 || expired_samples);
    return {{"name",test.name},{"passed",passed},{"inventory_retained",retained},{"native_latch_released",released},
        {"new_physical_tap_works",next_tap},{"expired_samples",expired_samples},{"movement_and_fire_neutral",unrelated_neutral},
        {"commands",commands},{"native_selector_writes",stores}};
}
}
int main(int argc,char** argv)try {
    need(argc==4,"Supply prepared MCC MAP MAPS REPORT");if(!std::filesystem::is_regular_file(argv[1]))return 77;
    need(std::getenv("HALO2_TEST_SCRATCH"),"Run interaction timeout tests in owned storage");
    Json cases=Json::array();bool passed=true;
    for(const Case test:std::array{Case{"continuous",0},Case{"fresh_delay",1},Case{"consumed_timeout",2},
        Case{"unconsumed_timeout",2,false,true},Case{"timeout_then_new_tap",2,false,false,true},Case{"primary_timeout",2,true}}) {
        auto result=run(argv[1],argv[2],test);passed&=result["passed"].get<bool>();cases.push_back(std::move(result));
    }
    const Json report{{"passed",passed},{"cases",cases},{"native_execution",true},
        {"game_returns_supplied",Json::array()},{"rendered_output_parity",false}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    std::cout<<"Native interaction timeout: "<<passed<<", "<<cases.size()<<" actual UDP/native cases\n";
    return passed?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

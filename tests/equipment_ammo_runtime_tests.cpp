#include "halo2/multiplayer_world.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/custom_slayer.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {void check(bool value,const char *why){if(!value)throw FormatError(why);}}
int main(int argc,char **argv)try {
    check(argc==3,"Supply prepared MCC map and bounded ammo report");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"Ammo acceptance requires owned scratch");
    Cache census(argv[1]);std::vector<std::string> names;
    for(const auto &tag:census.tags())if(tag.group=="eqip") {
        const auto meta=census.load_source_tag_metadata(tag.datum);
        if(Bytes(meta.bytes_at(meta.source_address()+0x12c,2)).u16(0)==0)names.push_back(tag.name);
    }
    nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},
        {"stimulus","Authored resident ammo definitions are constructed by original B9960/B9B70 at the genuine unit position. Matching original loadouts provide actual reserve space; fire/reload is used only if required. No inventory result or map placement is assigned."}};
    const auto save=[&]{std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save ammo evidence");};
    unsigned count{},placed_count{};
    for(const auto &name:names) {
        std::cout<<"Ammo contact family: "<<name<<std::endl;
        std::filesystem::path selected=argv[1];unsigned authored_row=~0u;
        // Six ammo families have real scenario collection placements across
        // these maps. The unused Needler resident remains a constructor case.
        for(const auto map:std::array{std::filesystem::path(argv[1]),std::filesystem::path(argv[1]).parent_path()/"turf.map"}) {
            Cache candidate(map);const auto level=candidate.load_content(0,true);
            for(unsigned row=0;row<level.netgame_equipment.size();++row) {
                const auto &items=level.netgame_equipment[row].items;
                if(items.size()!=1)continue;
                const auto tag=candidate.query_tag({.handle=TagHandle{items.front().definition}});
                if(tag && tag->name==name){selected=map;authored_row=row;break;}
            }
            if(authored_row!=~0u)break;
        }
        Cache cache(selected);const auto content=cache.load_content(0,true);
        const auto found=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){return t.name==name && t.group=="eqip";});
        check(found!=cache.tags().end(),"Missing original ammo family in selected map");const auto &tag=*found;
        const auto ammo=cache.load_source_tag_metadata(tag.datum);
        if(Bytes(ammo.bytes_at(ammo.source_address()+0x12c,2)).u16(0)!=0)continue;
        std::uint32_t weapon=null_datum;unsigned magazine{},quantity{},maximum{};
        for(const auto &[datum,definition]:content.weapons) {
            const auto meta=cache.load_source_tag_metadata(datum);
            const Bytes root(meta.bytes_at(meta.source_address()+0x2c0,8));
            for(unsigned m=0;m<root.u32(0) && weapon==null_datum;++m) {
                // MCC canonical roots can retain literal pointers into a
                // separate owned allocation. Read through Cache's address
                // owner rather than assuming the magazine is in the root.
                const auto mag_bytes=cache.read_tag_bytes(root.u32(4)+m*0x5c,0x5c);const Bytes mag(mag_bytes);
                for(unsigned r=0;r<mag.u32(0x54);++r) {
                    const auto ref_bytes=cache.read_tag_bytes(mag.u32(0x58)+r*0xc,0xc);const Bytes ref(ref_bytes);
                    if(ref.u32(8)==tag.datum) {
                        weapon=datum;magazine=m;quantity=ref.u16(0);maximum=mag.u16(0xc);break;
                    }
                }
            }
            if(weapon!=null_datum)break;
        }
        check(weapon!=null_datum && quantity>0 && maximum>0,"Authored ammo lacks a real magazine transfer reference");
        unsigned starting{};const auto choices=cache.multiplayer_weapon_choices();
        for(unsigned option=3;option<=20;++option) {
            std::uint32_t seed{};
            if(source_starting_weapon(null_datum,option,null_datum,choices,seed)==weapon){starting=option;break;}
        }
        check(starting,"Matching ammo weapon lacks an original starting loadout");
        MultiplayerOptions options;options.map=selected;options.source_object_world=true;
        options.local_players=1;options.starting_weapon=starting;
        options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/("ammo-"+std::to_string(tag.datum));
        MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;auto &objects=*prepared.source_objects;
        prepared.bind_source_local_views(prepared.local_slots);
        auto &world=objects.tu15_world();const auto slot=prepared.local_slots.at(0);
        std::array<Controls,multiplayer_capacity> controls{};controls[slot].pitch=1.4f;
        for(unsigned n=0;n<60;++n)match.step(controls);
        const auto unit=match.player(slot).source_current_object;
        check(match.state().players[slot].weapon==weapon,"Original loadout selected a different matching weapon");
        const auto reserve=[&]{check(match.player(slot).source_current_object==unit && !match.state().players[slot].dead &&
                match.state().players[slot].source_weapon!=null_datum,"Ammo fixture lost its original living player/weapon");
            const auto p=objects.object_body(match.state().players[slot].source_weapon)+0x22a+magazine*0x10;
            return Bytes(objects.memory().view(p,2)).u16(0);};
        const auto before_fire=reserve();
        // Authored starting equipment normally already has reserve space.
        // Use an actual shot/reload only if that loadout starts at its cap.
        const auto loaded=match.state().players[slot].loaded;
        if(before_fire>=maximum) {
        for(unsigned n=0;n<180 && match.state().players[slot].loaded==loaded;++n) {
            controls[slot].fire=n%2==0;match.step(controls);
        }
        controls[slot].fire=false;
        for(unsigned n=0;n<30;++n)match.step(controls);
        controls[slot].reload=true;
        for(unsigned n=0;n<300 && reserve()==before_fire;++n)match.step(controls);
        controls[slot].reload=false;
        std::cout<<"Ammo readiness: loaded "<<loaded<<" -> "<<match.state().players[slot].loaded
            <<" reserve "<<before_fire<<" -> "<<reserve()<<" dead="<<match.state().players[slot].dead<<std::endl;
        check(!match.state().players[slot].dead && reserve()<before_fire,"Actual shot/reload did not create ammunition space");
        }
        check(reserve()<maximum,"Authored loadout provides no ammunition space");
        const auto construct=[&] {
            constexpr std::uint32_t request=0x60010000u;
            auto abi=world.prepare_source_call(std::array{tag.datum,null_datum,0u});abi.gp[6]=request;
            world.run_actual(0xb9960u,abi);
            const auto position=match.state().players[slot].position;
            world.write_source_call_input(request+0x1c,std::as_bytes(std::span(&position,1)));
            const auto object=world.call_actual(0xb9b70u,std::array{request});
            check(object!=null_datum && objects.object_definition(object)==tag.datum,"Original ammo constructor failed");
            return object;
        };
        nlohmann::json row={{"ammo",tag.name},{"definition",tag.datum},{"weapon",weapon},{"quantity",quantity},{"maximum",maximum},
            {"starting_reserve",before_fire},{"before_contact",reserve()},{"transfers",nlohmann::json::array()}};
        if(authored_row!=~0u) {
            auto object=objects.netgame_object(authored_row);if(object==null_datum)object=objects.create_netgame_object(authored_row);
            check(object!=null_datum,"Original netgame ammo row failed to spawn");
            std::map<unsigned,std::uint32_t> original_rows;
            for(unsigned r=0;r<content.netgame_equipment.size();++r) {
                const auto &items=content.netgame_equipment[r].items;
                if(items.size()==1 && items.front().definition==tag.datum) {
                    const auto item=objects.netgame_object(r);if(item!=null_datum)original_rows.emplace(r,item);
                }
            }
            const auto safe=match.state().players[slot].position;const auto before=reserve();
            world.set_object_position(unit,objects.object_transform(object).position);bool consumed{};
            for(unsigned n=0;n<60 && !consumed;++n) {
                match.step(controls);const auto live=objects.live_objects();
                consumed=std::none_of(live.begin(),live.end(),[&](const auto&o){return o.object==object;});
                if(reserve()==maximum)break;
            }
            const auto contact=match.state().players[slot].position;
            const auto table=Bytes(objects.memory().view(0x4f0414u,4)).u32(0);
            const auto definition=Bytes(objects.memory().view(table+(weapon&65535u)*16u+8u,4)).u32(0);
            const auto magazines=Bytes(objects.memory().view(definition+0x2c4u,4)).u32(0);
            const auto native_maximum=Bytes(objects.memory().view(magazines+magazine*0x5cu+0xcu,2)).u16(0);
            std::cout<<"Authored ammo contact: "<<nlohmann::json{{"ammo",name},{"row",authored_row},{"consumed",consumed},
                {"before",before},{"after",reserve()},{"native_maximum",native_maximum},
                {"player",{contact.x,contact.y,contact.z}}}.dump();
            if(!consumed){const auto point=objects.object_transform(object).position;
                std::cout<<" item="<<point.x<<','<<point.y<<','<<point.z;}
            std::cout<<std::endl;
            const auto remaining=objects.live_objects();std::vector<unsigned> deleted_rows;
            for(const auto &[r,item]:original_rows)
                if(std::none_of(remaining.begin(),remaining.end(),[&](const auto&o){return o.object==item;}))deleted_rows.push_back(r);
            check(!deleted_rows.empty() && reserve()==std::min(maximum,before+quantity*unsigned(deleted_rows.size())),
                "Actual deleted ammo generations do not match the authored inventory transfer");
            if(!consumed)check(reserve()==maximum,"Non-full ammo contact retained its selected original item");
            const auto contact_row=authored_row;
            // Adjacent same-family rows can fill the reserve before the
            // selected row. Follow an actually deleted generation's timer.
            if(!consumed){authored_row=deleted_rows.front();object=original_rows.at(authored_row);}
            world.set_object_position(unit,safe);
            const auto &placement=content.netgame_equipment[authored_row];
            const auto seconds=placement.respawn_seconds?placement.respawn_seconds:(placement.collection_delay?placement.collection_delay:30);
            std::uint32_t replacement=null_datum;unsigned waited{};
            for(;waited<unsigned(seconds)*30u+300u;++waited) {
                match.step(controls);replacement=objects.netgame_object(authored_row);
                if(replacement!=null_datum && replacement!=object)break;
            }
            check(replacement!=null_datum && replacement!=object,"Authored ammo row did not respawn a new generation");
            row["map"]=selected.filename().string();row["contact_row"]=contact_row;row["authored_row"]=authored_row;row["respawn_wait_frames"]=waited;
            row["deleted_rows"]=deleted_rows;
            row["original_object"]=object;row["replacement"]=replacement;++placed_count;
        }
        for(unsigned n=0;n<32 && reserve()<maximum;++n) {
            const auto before=reserve();const auto object=construct();
            world.set_object_position(unit,objects.object_transform(object).position);
            bool consumed{};
            for(unsigned tick=0;tick<60 && !consumed;++tick) {
                match.step(controls);const auto live=objects.live_objects();
                consumed=std::none_of(live.begin(),live.end(),[&](const auto &o){return o.object==object;});
            }
            const auto after=reserve();
            check(consumed && after==std::min(maximum,before+quantity),"Ordinary ammo contact lost its exact authored transfer/delete");
            row["transfers"].push_back({{"before",before},{"after",after},{"object",object}});
        }
        check(reserve()==maximum,"Authored ammo contacts did not fill the reserve");
        const auto full=construct();world.set_object_position(unit,objects.object_transform(full).position);
        for(unsigned n=0;n<30;++n)match.step(controls);
        const auto live=objects.live_objects();
        check(reserve()==maximum && std::any_of(live.begin(),live.end(),[&](const auto&o){return o.object==full;}),
            "Full reserve consumed an additional ammo box");
        row["full_inventory_retains_item"]=true;row["passed"]=true;report["cases"].push_back(row);save();++count;
        std::cout<<row.dump()<<std::endl;
    }
    check(count==7,"All seven original ammo families must complete matching-weapon contact controls");
    check(placed_count==6,"All six placed ammo families must complete actual netgame respawn controls");
    report["passed"]=true;save();return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

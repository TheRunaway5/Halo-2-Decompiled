#pragma once
#include "halo2/simulation.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/native_campaign_script_world_tu15.hpp"
#include <bit>
#include <nlohmann/json.hpp>

namespace halo2 {
// Bounded observation after ordinary recorded input. The actor positions are
// the published native biped motion; the VM checkpoint is read, never restored.
inline nlohmann::json campaign_input_diagnostics(const Simulation &simulation,
    const ScriptProgram &program,unsigned input_tick) {
    using Json=nlohmann::json;
    const auto vector=[](Vec3 v){return Json::array({v.x,v.y,v.z});};
    Json actors=Json::array(),threads=Json::array();unsigned actor_index{};
    for(const auto &unit:simulation.actors()) {
        if(actor_index>=16)break;
        actors.push_back(Json{{"index",actor_index++},{"object",unit.source_datum},{"exists",unit.exists},
            {"position",vector(unit.body.position)},{"velocity",vector(unit.body.velocity)},
            {"command_active",unit.command.active},{"command_moving",unit.command.moving},
            {"goal",unit.command.goal?vector(*unit.command.goal):Json(nullptr)},
            {"goal_reference",unit.command.goal_reference?Json(*unit.command.goal_reference):Json(nullptr)},
            {"route_points",unit.route.size()},{"waypoint",unit.waypoint}});
        if(unit.exists) {
            try{actors.back()["script_center"]=vector(simulation.object_center(unit.source_datum));}
            catch(const std::exception &error){actors.back()["script_center_error"]=std::string(error.what()).substr(0,256);}
            if(auto *source=simulation.campaign_source())try {
                const auto bank=native_campaign_script_bank(source->objects().tu15_world());
                const auto headers=bank.u32(bank.u32(0x4ecbd0u)+0x44u);
                const auto body=bank.u32(headers+(unit.source_datum&0xffffu)*12u+8u);
                const auto actor=bank.u32(body+0x12cu);
                if(actor!=0xffffffffu) {
                    const auto row=bank.u32(bank.u32(0x5020c8u)+0x44u)+(actor&0xffffu)*0x888u;
                    const auto id=bank.u32(row+0x85cu);
                    Json native{{"actor",actor},{"command",id},{"movement_action",bank.u16(row+0x4acu)},
                        {"movement_state",bank.u16(row+0x504u)},{"movement_active",bank.u8(row+0x50cu)}};
                    const auto action=body+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(bank.u16(body+0x346u))));
                    native["unit_action_mode"]=bank.u16(action+0x36u);
                    if(id!=0xffffffffu) {
                        const auto command=bank.u32(bank.u32(0x50eee0u)+0x44u)+(id&0xffffu)*0xd4u;
                        const auto hs=bank.u32(command+0x38u);
                        native["kind"]=bank.u16(command+4u);native["state"]=bank.u16(command+0x30u);
                        native["script"]=bank.u32(command+0x34u);native["hs_thread"]=hs;
                        native["autonomous_moving"]=bank.u8(command+0x81u);
                        if(hs!=0xffffffffu) {
                            const auto thread=bank.u32(bank.u32(0x505e5cu)+0x44u)+(hs&0xffffu)*0x418u;
                            native["hs_wake"]=bank.u32(thread+8u);native["hs_frame"]=bank.u32(thread+0x10u);
                        }
                    }
                    actors.back()["native_command"]=std::move(native);
                }
            }catch(const std::exception &error){actors.back()["native_command_error"]=std::string(error.what()).substr(0,256);}
        }
    }
    const auto observed=simulation.scripts().authority_checkpoint();
    for(const auto &thread:observed.threads) {
        if(thread.stopped||thread.dormant)continue;
        if(threads.size()>=16)break;
        Json stack=Json::array();
        const auto first=thread.stack.size()>12?thread.stack.size()-12:0;
        for(std::size_t n=first;n<thread.stack.size();++n) {
            const auto &frame=thread.stack[n];Json values=Json::array();
            for(const auto &value:frame.values) {
                if(values.size()>=16)break;
                values.push_back(Json{{"type",value.type},{"bits",value.bits},{"text",value.text.substr(0,128)}});
            }
            Json row{{"id",frame.id},{"name",frame.name},{"next",frame.next},{"wait_state",frame.wait_state},
                {"wait_start",frame.wait_start},{"wait_interval",frame.wait_interval},{"wait_timeout",frame.wait_timeout},
                {"values",std::move(values)}};
            if(frame.next<frame.children.size())row["next_child"]=frame.children[frame.next];
            const auto index=frame.id&65535u;
            if(index<program.nodes.size()) {
                const auto &node=program.nodes[index];row["node"]={{"opcode",node.opcode},{"type",node.type},{"flags",node.flags},{"value",node.value}};
            }
            stack.push_back(std::move(row));
        }
        threads.push_back(Json{{"script",thread.script},{"name",thread.script<program.scripts.size()?program.scripts[thread.script].name:""},
            {"wake",thread.wake},{"faulted",thread.faulted},{"command_wait",thread.command_wait},
            {"actor",thread.actor?Json(*thread.actor):Json(nullptr)},{"stack",std::move(stack)}});
    }
    Json result{{"tick",simulation.ticks()},{"input_tick",input_tick},{"actors",std::move(actors)},
        {"threads",std::move(threads)},{"player_position",vector(simulation.player().body.position)}};
    if(simulation.player().exists) {
        try{result["player_script_center"]=vector(simulation.object_center(0));}
        catch(const std::exception &error){result["player_script_center_error"]=std::string(error.what()).substr(0,256);}
    }
    return result;
}
}

#pragma once
#include "halo2/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace halo2 {
// A finite recording of ordinary player Controls. Entries patch held input
// at their zero-based tick; grenade/weapon-switch/melee/flashlight pulses last
// one accepted tick. Natural map transitions release held input and restore
// authored spawn yaw while the recording's tick index continues.
// No simulation state or script result is writable.
class CampaignInputSchedule {
    std::map<unsigned,nlohmann::json> entries_;
    std::optional<unsigned> pickup_begin_,dual_begin_,last_tick_;
public:
    explicit CampaignInputSchedule(const nlohmann::json &document,unsigned ticks) {
        const auto bad=[](){throw std::invalid_argument("Invalid campaign input schedule");};
        if(!ticks || !document.is_array() || document.size()>100000u)bad();
        const std::set<std::string> real_fields{"yaw","pitch","forward","right","look_stick"};
        const std::set<std::string> buttons{"jump","crouch","fire","reload","grenade","pickup","dual_wield",
            "switch_weapon","fire_secondary","switch_grenade","accept","cancel","melee","flashlight","start","back"};
        for(const auto &row:document) {
            if(!row.is_object() || !row.contains("tick") || !row.at("tick").is_number_integer() || row.size()<2)bad();
            const auto at=row.at("tick").get<std::int64_t>();if(at<0 || std::uint64_t(at)>=ticks || entries_.contains(unsigned(at)))bad();
            for(const auto &[key,value]:row.items()) {
                if(key=="tick")continue;
                if(real_fields.contains(key)) {
                    if(!value.is_number())bad();
                    const auto v=value.get<double>();
                    if(!std::isfinite(v) || std::abs(v)>1000.)bad();
                    if(key=="pitch" && std::abs(v)>1.5)bad();
                    if((key=="forward" || key=="right" || key=="look_stick") && std::abs(v)>1.)bad();
                } else if(!buttons.contains(key) || !value.is_boolean())bad();
            }
            entries_.emplace(unsigned(at),row);
        }
    }
    static CampaignInputSchedule load(const std::filesystem::path &path,unsigned ticks) {
        if(std::filesystem::file_size(path)>1024u*1024u)throw std::invalid_argument("Campaign input schedule exceeds1MiB");
        std::ifstream input(path);if(!input)throw std::invalid_argument("Could not open campaign input schedule");
        return CampaignInputSchedule(nlohmann::json::parse(input),ticks);
    }
    bool apply(unsigned tick,Controls &input) {
        if(last_tick_ && tick<*last_tick_)throw std::invalid_argument("Campaign input ticks must advance");
        last_tick_=tick;
        // Match the rendered campaign loop's per-tick pulse consumption.
        input.grenade=input.switch_weapon=input.melee=input.flashlight=input.switch_grenade=false;
        const auto at=entries_.find(tick);
        if(at!=entries_.end()) {
          const auto &row=at->second;
        const auto real=[&](const char *name,float &field){if(row.contains(name))field=row.at(name).get<float>();};
        const auto button=[&](const char *name,bool &field){if(row.contains(name))field=row.at(name).get<bool>();};
        real("yaw",input.yaw);real("pitch",input.pitch);real("forward",input.forward);real("right",input.right);real("look_stick",input.look_stick);
        button("jump",input.jump);button("crouch",input.crouch);button("fire",input.fire);button("reload",input.reload);button("grenade",input.grenade);
        button("pickup",input.pickup);button("dual_wield",input.dual_wield);button("switch_weapon",input.switch_weapon);button("fire_secondary",input.fire_secondary);
        button("switch_grenade",input.switch_grenade);button("accept",input.accept);button("cancel",input.cancel);button("melee",input.melee);
          button("flashlight",input.flashlight);button("start",input.start);button("back",input.back);
        }
        // The ordinary keyboard/controller owners deliver both the held bit
        // and its elapsed milliseconds. Headless campaign ticks are30Hz.
        const auto held=[&](bool down,std::optional<unsigned>&begin,std::uint16_t &milliseconds) {
            if(!down){begin.reset();milliseconds=0;return;}
            if(!begin)begin=tick;
            milliseconds=std::uint16_t(std::min<std::uint64_t>((std::uint64_t(tick)-*begin)*1000u/30u,65535u));
        };
        held(input.pickup,pickup_begin_,input.pickup_milliseconds);
        held(input.dual_wield,dual_begin_,input.dual_wield_milliseconds);
        return at!=entries_.end();
    }
};
}

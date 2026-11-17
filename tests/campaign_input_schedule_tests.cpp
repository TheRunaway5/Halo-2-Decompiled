#include "app/campaign_input_schedule.hpp"
#include <iostream>
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
}
int main()try {
    using Json=nlohmann::json;using halo2::CampaignInputSchedule;using halo2::Controls;
    CampaignInputSchedule schedule(Json::parse(R"([{"tick":2,"pitch":0.5,"look_stick":1},{"tick":5,"pitch":-0.5,"look_stick":-1,"forward":1,"pickup":true},{"tick":7,"forward":0,"pickup":false}])"),10);
    Controls input;input.yaw=2.4f;
    check(!schedule.apply(0,input)&&input.pitch==0&&input.yaw==2.4f,"Neutral input changed before the recording");
    check(schedule.apply(2,input)&&input.pitch==.5f&&input.look_stick==1&&input.yaw==2.4f,"Look input lost authored spawn yaw");
    check(!schedule.apply(3,input)&&input.pitch==.5f,"Held aim changed between events");
    schedule.apply(5,input);check(input.forward==1&&input.pickup&&input.pitch==-.5f,"Ordinary movement/use fields were not delivered");
    check(input.pickup_milliseconds==0,"A new press inherited held time");
    schedule.apply(6,input);check(input.pickup_milliseconds==33,"A held use button did not advance at30Hz");
    schedule.apply(7,input);check(input.forward==0&&!input.pickup&&input.pickup_milliseconds==0&&input.pitch==-.5f,"Button release or held aim was lost");
    CampaignInputSchedule long_hold(Json::parse(R"([{"tick":0,"pickup":true,"dual_wield":true},{"tick":3001,"pickup":false,"dual_wield":false}])"),3002);
    Controls held;long_hold.apply(0,held);long_hold.apply(3,held);
    check(held.pickup_milliseconds==100&&held.dual_wield_milliseconds==100,"Recorded hold duration changed");
    long_hold.apply(3000,held);check(held.pickup_milliseconds==65535&&held.dual_wield_milliseconds==65535,"Long hold failed to saturate");
    long_hold.apply(3001,held);check(held.pickup_milliseconds==0&&held.dual_wield_milliseconds==0,"Release failed to clear durations");
    CampaignInputSchedule pulses(Json::parse(R"([{"tick":0,"grenade":true,"switch_weapon":true,"melee":true,"flashlight":true,"switch_grenade":true,"pickup":true}])"),2);
    Controls pulse;pulses.apply(0,pulse);
    check(pulse.grenade&&pulse.switch_weapon&&pulse.melee&&pulse.flashlight&&pulse.switch_grenade,"First-tick action pulse was lost");
    pulses.apply(1,pulse);
    check(!pulse.grenade&&!pulse.switch_weapon&&!pulse.melee&&!pulse.flashlight&&!pulse.switch_grenade&&pulse.pickup&&pulse.pickup_milliseconds==33,"Action pulses repeated or held use was cleared");
    unsigned rejected{};
    for(const auto *text:{R"({})",R"([{"tick":10,"pitch":0}])",R"([{"tick":-1,"pitch":0}])",R"([{"tick":1}])",
        R"([{"tick":1,"pitch":0},{"tick":1,"pitch":1}])",R"([{"tick":1,"pitch":2}])",R"([{"tick":1,"forward":2}])",
        R"([{"tick":1,"accept":1}])",R"([{"tick":1,"script":"complete"}])",R"([{"tick":1,"position":[1,2,3]}])",
        R"([{"tick":1,"skip_cinematic":true}])",R"([{"tick":1,"yaw":1e20}])"}) {
        try{CampaignInputSchedule invalid(Json::parse(text),10);throw std::runtime_error("Invalid input schedule accepted");}
        catch(const std::invalid_argument &){++rejected;}
    }
    check(rejected==12,"Missing input rejection");std::cout<<"PASS campaign Controls schedule: held aim, movement/use, held durations, releases and12 invalid-input controls\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

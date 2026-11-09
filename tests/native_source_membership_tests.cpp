#include "network/source_membership_validation.hpp"
#include <iostream>

using namespace halo2;
namespace {
void need(bool value,const char* message){if(!value)throw FormatError(message);}
}
int main()try {
    MultiplayerState pending;pending.source_game_type=2;pending.team_mode=true;
    pending.retained_team_mask=3;pending.source_body_poses_authoritative=true;
    auto& player=pending.players[0];player.connected=true;player.dead=true;
    player.source_player=0xe0000000u;player.source_unit=player.source_weapon=null_datum;
    player.source_membership=-1;player.source_configuration.emplace();
    (*player.source_configuration)[0x7c]=0xffu;
    detail::validate_native_source_membership(pending,player);
    auto ffa=pending;ffa.team_mode=false;ffa.retained_team_mask=0;
    detail::validate_native_source_membership(ffa,ffa.players[0]);
    struct Control {const char* name;void(*change)(MultiplayerState&);};
    const Control controls[]{
        {"unowned WORLD",[](auto& s){s.source_body_poses_authoritative=false;}},
        {"another game kind",[](auto& s){s.source_game_type=1;}},
        {"playing actor",[](auto& s){s.players[0].dead=false;}},
        {"current body",[](auto& s){s.players[0].source_unit=0xe1000000u;}},
        {"primary weapon",[](auto& s){s.players[0].source_weapon=0xe1000001u;}},
        {"offhand weapon",[](auto& s){s.players[0].secondary_weapon.datum=0xe1000002u;}},
        {"live FP packet",[](auto& s){s.players[0].first_person_pose.emplace();}},
        {"missing native profile",[](auto& s){s.players[0].source_configuration.reset();}},
        {"assigned profile",[](auto& s){(*s.players[0].source_configuration)[0x7c]=1;}},
        {"unsupported negative membership",[](auto& s){s.players[0].source_membership=-2;}},
        {"team beyond source range",[](auto& s){s.players[0].source_membership=8;}},
        {"team outside retained mask",[](auto& s){s.players[0].source_membership=2;
            (*s.players[0].source_configuration)[0x7c]=2;}},
        {"assigned profile mismatch",[](auto& s){s.players[0].source_membership=1;}}
    };
    for(const auto& control:controls) {
        auto bad=pending;control.change(bad);bool refused=false;
        try{detail::validate_native_source_membership(bad,bad.players[0]);}
        catch(const FormatError&){refused=true;}
        need(refused,control.name);
    }
    auto assigned=pending;assigned.players[0].source_membership=1;
    (*assigned.players[0].source_configuration)[0x7c]=1;
    detail::validate_native_source_membership(assigned,assigned.players[0]);
    std::cout<<"PASS native NONE participation and13 invalid membership/actor controls\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

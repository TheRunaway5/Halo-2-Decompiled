#include "halo2/native_request_protocol.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>

namespace halo2 {
std::optional<NativeRequest> decode_native_request(std::span<const std::byte> packet,std::uint16_t protocol) {
    const auto *bytes=reinterpret_cast<const std::uint8_t *>(packet.data());
    const auto frame=online_protocol_bridge::decode_native_request_frame(
        rust::Slice<const std::uint8_t>(bytes,packet.size()),protocol);
    if(frame.status!=online_protocol_bridge::DecodeStatus::Ok)return {};
    if(frame.kind==1) {
        if(frame.players.empty()||frame.players.size()>4)return {};
        NativeHelloRequest value;value.nonce=frame.nonce;value.fingerprint=frame.fingerprint;
        value.players.reserve(frame.players.size());
        for(const auto &player:frame.players) {
            if(player.profile.size()!=(player.has_profile?native_request_profile_bytes:0))return {};
            NativeLocalPlayerRequest local;local.team=player.team;
            if(player.has_profile) {
                local.profile.emplace();std::copy(player.profile.begin(),player.profile.end(),local.profile->begin());
            }
            value.players.push_back(std::move(local));
        }
        return value;
    }
    if(frame.kind==2) {
        if(frame.controls.empty()||frame.controls.size()>4)return {};
        NativeInputRequest value;value.nonce=frame.nonce;value.cookie=frame.cookie;value.sequence=frame.sequence;
        value.ack={frame.ack_latest,frame.ack_mask,frame.ack_flags};
        if(frame.has_view)value.view=NativeSnapshotView{frame.view_tick,frame.view_fraction};
        value.controls.reserve(frame.controls.size());
        const auto intent=[](const online_protocol_bridge::NativeIntent &source)->std::optional<NativeVehicleIntent> {
            if(!source.present)return {};
            return NativeVehicleIntent{source.action,source.index,source.object};
        };
        if(frame.motion.size()!=frame.controls.size())return {};
        const auto motion=[](const online_protocol_bridge::NativeMotion &source)->std::optional<NativeMotionReport> {
            if(!source.present)return {};
            return NativeMotionReport{source.grounded,source.crouching,source.baseline_age,source.position,source.velocity};
        };
        for(const auto &source:frame.controls) {
            NativeInputControl control;
            control.forward=source.forward;control.right=source.right;control.yaw=source.yaw;control.pitch=source.pitch;
            control.flags=source.flags;control.zoom=source.zoom;control.zoom_reset_serial=source.zoom_reset_serial;
            control.pickup_milliseconds=source.pickup_milliseconds;control.dual_wield_milliseconds=source.dual_wield_milliseconds;
            control.completed=source.completed;control.presses=source.presses;
            control.vehicle=intent(source.vehicle);control.completed_vehicle=intent(source.completed_vehicle);
            control.motion=motion(frame.motion[value.controls.size()]);
            value.controls.push_back(std::move(control));
        }
        // Flattened newest-first entries, each with one control per view.
        const auto views=value.controls.size();
        if(frame.redundant.size()%views || frame.redundant.size()/views>native_max_redundant_inputs)return {};
        for(std::size_t at=0;at<frame.redundant.size();at+=views) {
            NativeRedundantInput entry;entry.sequence=frame.redundant[at].sequence;
            for(std::size_t view=0;view<views;++view) {
                const auto &source=frame.redundant[at+view];
                if(source.view!=view || source.sequence!=entry.sequence)return {};
                entry.controls.push_back({source.forward,source.right,source.yaw,source.pitch,source.flags,source.zoom});
            }
            value.redundant.push_back(std::move(entry));
        }
        return value;
    }
    if(frame.kind==19)return NativeTeamChangeRequest{frame.nonce,frame.cookie,frame.sequence,frame.team_view,frame.team};
    if(frame.kind==4)return NativeLeaveRequest{frame.nonce,frame.cookie};
    if(frame.kind==6)return NativeSessionQuery{frame.nonce};
    return {};
}
}

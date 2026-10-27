// Declared transport stimuli through the actual production body codec and
// acknowledged stream. Native actor/Cache/pose proof is the packet endpoint test.
#include "../src/network/multiplayer_network.cpp"
#include "halo2/source_execution_calls.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool v,const char *why){if(!v)throw FormatError(why);}
void set(std::span<std::byte> b,unsigned at,std::uint32_t value){for(unsigned n=0;n<4;++n)b[at+n]=std::byte(value>>(8*n));}
Packet encode(const MultiplayerState &s,std::span<const unsigned> ids,unsigned version=native_multiplayer_protocol) {
    Packet p;encode_state(p,s,ids,nullptr,version);return p;
}
MultiplayerState decode(const Packet &p,std::span<const unsigned> ids,unsigned version=native_multiplayer_protocol) {
    Reader r{p};auto s=decode_state(r,ids,version);r.end();return s;
}
}
int main(int argc,char **argv)try {
    need(argc==1 || argc==2,"Supply optional bounded transport report");
    need(native_multiplayer_protocol==117,"Native attachment input requires its explicit native protocol");
    MultiplayerState state;state.tick=8;state.devices.tick=state.tick;state.source_game_type=2;state.source_engine=2;
    state.source_round_lifecycle_available=true;state.round_phase=1;state.round_index=0;state.round_countdown=0;
    for(unsigned n=0;n<2;++n) {
        auto &p=state.players[n];p.connected=true;p.source_player=0x10000u+n;p.source_unit=0x20000u+n;
        p.source_membership=std::int16_t(n);p.source_ffa_identity=std::uint8_t(n);
        p.native_physical_state=n==0?6:1;
        p.first_person_pose.emplace();auto &packet=*p.first_person_pose;auto &s=packet.temporal;
        s.identity={p.source_player,p.source_unit,0x30005u,2};s.generation=1;s.source_tick=7;
        s.clock_controls[2]=std::byte(30);packet.actors.game_kind=2;packet.actors.unit_world_nodes.resize(1);
        packet.actors.single_hand_ik_transition={std::byte(n+1),std::byte(5),std::byte(1)};
        for(auto &h:s.hands) {set(h.control,4,null_datum);for(unsigned c=0;c<3;++c)set(h.control,8+c*0x20,null_datum);}
    }
    const std::array ids0{0u},ids1{1u};
    const auto complete=encode(state,ids0);const auto received=decode(complete,ids0);
    need(received.players[0].first_person_pose.has_value() && !received.players[1].first_person_pose,
        "FP inputs escaped their assigned recipient or disappeared");
    need(received.players[0].native_physical_state==6 && received.players[1].native_physical_state==1 &&
        !received.players[0].melee_action.active(),
        "Snapshot lost native Sword momentum after its melee activity was cancelled");
    need(encode(received,ids0)==complete,"Native FP body packet did not roundtrip exactly");
    need(received.players[0].first_person_pose->actors.single_hand_ik_transition==
        state.players[0].first_person_pose->actors.single_hand_ik_transition,
        "Native single-hand IK transition inputs did not roundtrip exactly");
    auto no_physical=state;for(auto &p:no_physical.players)p.native_physical_state=0xff;
    for(unsigned version=110;version<=115;++version) {
        need(encode(state,ids0,version)==encode(no_physical,ids0,version),
            "Native physical-state extension changed a legacy body extent");
        const auto legacy=decode(encode(state,ids0,version),ids0,version);
        need(legacy.players[0].native_physical_state==0xff && legacy.players[1].native_physical_state==0xff,
            "Legacy snapshot fabricated ordinary walking permission");
    }
    auto removed=state;for(auto &p:removed.players)p.first_person_pose.reset();
    for(unsigned version=110;version<=114;++version) {
        need(encode(state,ids0,version)==encode(removed,ids0,version),"FP extension changed a legacy body extent");
        need(!decode(encode(state,ids0,version),ids0,version).players[0].first_person_pose,
            "Legacy body fabricated native FP controls");
    }
    unsigned rejected{};const auto reject=[&](auto call){bool failed{};try{call();}catch(const FormatError&){failed=true;}
        need(failed,"Malformed or misassigned native FP inputs were accepted");++rejected;};
    reject([&]{decode(complete,ids1);});
    auto bad=state;bad.players[0].first_person_pose->temporal.identity.player=0x10001u;reject([&]{encode(bad,ids0);});
    bad=state;bad.players[0].first_person_pose->temporal.source_tick=6;reject([&]{encode(bad,ids0);});
    bad=state;bad.players[0].first_person_pose->actors.weapons.resize(5);reject([&]{encode(bad,ids0);});
    auto attached=state;attached.source_body_poses_authoritative=true;
    for(unsigned n=0;n<2;++n) {
        SourceWorldPose pose;pose.object=attached.players[n].source_unit;pose.nodes.resize(1);
        pose.nodes[0].position={float(n)+.25f,0.f,0.f};pose.sphere={{float(n),0.f,0.f},.5f};
        pose.attachment=SourceWorldPoseAttachment{null_datum,std::uint8_t(0xe3+n),{}};
        attached.source_body_poses.push_back(pose);
    }
    attached.source_body_poses[1].attachment=SourceWorldPoseAttachment{
        attached.source_body_poses[0].object,0,attached.source_body_poses[0].nodes[0]};
    const auto attached_bytes=encode(attached,ids0);
    const auto attached_received=decode(attached_bytes,ids0);
    need(encode(attached_received,ids0)==attached_bytes,"New native attachment snapshot changed its exact input bank");
    need(attached_received.source_body_poses[0].attachment->parent_node==0xe3 &&
        attached_received.source_body_poses[1].attachment->parent==attached.players[0].source_unit &&
        attached_received.source_body_poses[1].attachment->parent_frame->position.x==.25f,
        "Snapshot lost the exact native parent/node/frame prefix");
    auto unknown_attachment=attached;for(auto &pose:unknown_attachment.source_body_poses)pose.attachment.reset();
    for(unsigned version=110;version<=116;++version) {
        const auto legacy_bytes=encode(attached,ids0,version);
        need(legacy_bytes==encode(unknown_attachment,ids0,version),"Attachment extension changed a legacy snapshot extent");
        const auto legacy=decode(legacy_bytes,ids0,version);
        need(!legacy.source_body_poses[0].attachment && !legacy.source_body_poses[1].attachment,
            "Legacy snapshot fabricated source parent/node metadata");
    }
    reject([&]{encode(unknown_attachment,ids0);});
    reject([&]{decode(encode(attached,ids0,116),ids0,117);});
    reject([&]{decode(attached_bytes,ids0,116);});
    auto packet=source_encode_first_person_packet_tu15(*state.players[0].first_person_pose,
        state.players[0].first_person_pose->temporal.identity);
    need(std::to_integer<unsigned>(packet[4])==4,"Native FP input codec did not advance its declared schema");
    auto old_packet=packet;old_packet[4]=std::byte(3);
    reject([&]{source_decode_first_person_packet_tu15(old_packet);});
    for(const auto n:{1u,10u,100u})reject([&]{source_decode_first_person_packet_tu15(std::span(packet).first(packet.size()-n));});
    auto changed=state;changed.players[0].first_person_pose->temporal.accepted_action_sequence++;
    need(source_execution_state_result(changed)!=source_execution_state_result(state),"Full native state seal omitted FP controls");
    changed=state;changed.players[0].native_physical_state=1;
    need(source_execution_state_result(changed)!=source_execution_state_result(state),
        "Full native state seal omitted the native physical owner");
    changed=attached;changed.source_body_poses[1].attachment->parent_frame->position.x+=1.f;
    need(source_execution_state_result(changed)!=source_execution_state_result(attached),
        "Full native state seal omitted an exact parent frame");
    changed=attached;changed.source_body_poses[1].attachment->parent_node=1;
    need(source_execution_state_result(changed)!=source_execution_state_result(attached),
        "Full native state seal omitted the actual native parent node");
    SnapshotStreamSender sender(1292,80);SnapshotStreamReceiver receiver(1292);
    SnapshotBodyLayout layout;Packet body;encode_state(body,state,ids0,&layout);
    SnapshotStreamSender::Frame input{body,layout.floats,layout.anchor_offsets,layout.anchor_keys,layout.poses,1,1,1000,{},{},{}};
    const auto output=sender.encode(input);need(output && output->emitted,"Native FP snapshot was not emitted");
    bool delivered{};for(auto it=output->packets.rbegin();it!=output->packets.rend();++it) {
        bool failed{};auto got=receiver.push(*it,1000,failed);need(!failed,"Native FP acknowledged stream rejected an authentic body");
        if(got){need(got->body==body,"Native FP acknowledged stream changed controls");need(receiver.commit(got->sequence),"Native FP receipt did not commit");
            need(decode(got->body,ids0).players[0].first_person_pose.has_value(),"Native FP receipt lost its packet");delivered=true;}
    }need(delivered,"Native FP snapshot fragments never completed");
    SnapshotStreamSender attachment_sender(1292,80);SnapshotStreamReceiver attachment_receiver(1292);
    SnapshotBodyLayout attachment_layout;Packet attachment_body;encode_state(attachment_body,attached,ids0,&attachment_layout);
    const SnapshotStreamSender::Frame attachment_input{attachment_body,attachment_layout.floats,
        attachment_layout.anchor_offsets,attachment_layout.anchor_keys,attachment_layout.poses,1,1,1000,{},{},{}};
    const auto attachment_output=attachment_sender.encode(attachment_input);
    need(attachment_output && attachment_output->emitted,"New native attachment bank was not emitted");
    bool attachment_delivered{};
    for(const auto &part:attachment_output->packets) {
        bool failed{};auto got=attachment_receiver.push(part,1000,failed);
        need(!failed,"Acknowledged stream rejected the admitted parent-frame layout");
        if(!got)continue;
        const auto value=decode(got->body,ids0);
        const auto &prefix=*value.source_body_poses[1].attachment;
        need(prefix.parent==attached.players[0].source_unit && prefix.parent_node==0 &&
            prefix.parent_frame->position.x==.25f && value.source_body_poses[0].attachment->parent_node==0xe3,
            "Acknowledged stream detached the exact prefix from its materialized nodes");
        need(std::abs(value.source_body_poses[1].nodes[0].position.x-1.25f)<.002f,
            "Materialized WORLD root exceeded the declared lossy coordinate tolerance");
        need(attachment_receiver.commit(got->sequence),"Native attachment receipt did not commit");attachment_delivered=true;
    }
    need(attachment_delivered,"Native attachment snapshot fragments never completed");
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run transport test through owned scratch");
    nlohmann::json report{{"passed",true},{"protocol",native_multiplayer_protocol},{"recipient_only",true},
        {"legacy_versions_unchanged",{110,111,112,113,114}},{"physical_state_legacy_unknown",{110,111,112,113,114,115}},
        {"attachment_legacy_unknown",{110,111,112,113,114,115,116}},
        {"attachment_prefix_exact",true},{"attachment_acknowledged_stream",true},{"cancelled_sword_mode6_roundtrip",true},{"malformed_cases_rejected",rejected},
        {"packet_bytes",packet.size()},{"snapshot_fragments",output->packets.size()},
        {"scope","Production snapshot codec and acknowledged stream with declared typed native FP input stimuli"}};
    std::ofstream(argc==2?std::filesystem::path(argv[1]):std::filesystem::path(scratch)/"fp-transport.json")<<report.dump(2)<<'\n';
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

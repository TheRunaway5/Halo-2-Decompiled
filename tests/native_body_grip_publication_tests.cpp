// The same production body-codec/layout seam as the transport regressions.
// Budget mode exercises C++117 -> Rust stream -> C++117 with native inputs.
#include "../src/network/multiplayer_network.cpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/multiplayer_presentation.hpp"
#include "halo2/render_interpolation.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <deque>
#include <limits>

using namespace halo2;
namespace {
void need(bool value,const char *why){if(!value)throw FormatError(why);}
float difference(const Transform&a,const Transform&b){
  const auto x=std::bit_cast<std::array<float,13>>(a),y=std::bit_cast<std::array<float,13>>(b);
  float result{};for(unsigned i=0;i<x.size();++i)result=std::max(result,std::abs(x[i]-y[i]));return result;
}
std::uint32_t render_model(const Cache&cache,std::uint32_t definition){
  const auto object=cache.load_source_tag_metadata(definition);
  const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address()+0x38u,4)).u32(0));
  return Bytes(model.bytes_at(model.source_address()+4u,4)).u32(0);
}
const SourceWorldPose &pose(const MultiplayerState &state,std::uint32_t object) {
  const auto at=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
      [&](const auto &entry){return entry.object==object;});
  need(at!=state.source_body_poses.end(),"Fractional grip lost an actual published owner");return *at;
}
struct FractionalGripProbe {
  struct Parent {std::uint32_t object{};unsigned node{};bool operator==(const Parent&)const=default;};
  struct Endpoint {MultiplayerState state;std::array<Parent,2> parents;};
  std::optional<Endpoint> previous;
  unsigned pairs{},fractional_samples{};
  float endpoint_error{},weapon_error{},weapon_position_error{},body_error{},parent_motion{},parent_rotation{};
  float native_publication_error{},native_object_root_marker_error{};
  float old_independent_weapon_error{},old_independent_weapon_position_error{};
  float old_independent_body_hierarchy_error{};
  nlohmann::json samples=nlohmann::json::array();
  void capture(const MultiplayerState &state,unsigned slot,const RenderModel &body_model,
      const RenderModel &weapon_model,std::array<Parent,2> parents) {
    Endpoint next{state,parents};
    if(!previous){previous=std::move(next);return;}
    const auto &before=previous->state;const auto &a=before.players[slot],&b=state.players[slot];
    if(before.tick+1u!=state.tick || a.source_player!=b.source_player || a.source_unit!=b.source_unit ||
        a.source_weapon!=b.source_weapon || a.secondary_weapon.datum!=b.secondary_weapon.datum ||
        previous->parents!=parents) {previous=std::move(next);return;}
    SourceWorldInterpolation joined;
    capture_multiplayer_body_poses(joined,before,before,true);
    capture_multiplayer_body_poses(joined,before,state);
    const auto &body_before=pose(before,a.source_unit).nodes,&body_after=pose(state,b.source_unit).nodes;
    const SourceWorldInterpolation::ModelProvider models=[&](std::uint32_t owner)->const RenderModel* {
      if(owner==b.source_unit)return &body_model;
      if(owner==b.source_weapon || owner==b.secondary_weapon.datum)return &weapon_model;
      return nullptr;
    };
    ModelNodeInterpolation native_body;native_body.capture(before.tick,body_before);native_body.capture(state.tick,body_after);
    std::array<ModelNodeInterpolation,2> native_weapons;
    const std::array objects{b.source_weapon,b.secondary_weapon.datum};
    for(unsigned hand=0;hand<2;++hand) {
      need(parents[hand].object==b.source_unit && parents[hand].node<body_before.size() &&
          parents[hand].node<body_after.size(),"Fractional grip changed its actual native B93B0 parent/node");
      const auto node=parents[hand].node;
      for(const auto *endpoint:{&before,&state}) {
        const auto &binding=pose(*endpoint,objects[hand]).attachment;
        need(binding && binding->parent==parents[hand].object && binding->parent_node==node && binding->parent_frame,
            "Published held bank lost its actual native parent/node/frame binding");
        const auto &published_parent=pose(*endpoint,b.source_unit).nodes.at(node);
        need(!std::memcmp(&*binding->parent_frame,&published_parent,sizeof(Transform)),
            "Published held frame differs from its SAME final published parent bank");
      }
      parent_motion=std::max(parent_motion,difference(body_before[node],body_after[node]));
      for(unsigned axis=0;axis<3u;++axis)parent_rotation=std::max(parent_rotation,
          length(body_before[node].basis[axis]-body_after[node].basis[axis]));
      native_weapons[hand].capture(before.tick,pose(before,objects[hand]).nodes,body_before[node]);
      native_weapons[hand].capture(state.tick,pose(state,objects[hand]).nodes,body_after[node]);
    }
    for(const auto fraction:{0.f,.25f,.5f,.75f,1.f}) {
      const auto shown_body=joined.sample(b.source_unit,fraction,models);
      const auto old_body=joined.sample(b.source_unit,fraction);
      const auto hierarchical_body=native_body.sample(body_model,fraction);
      need(shown_body.size()==hierarchical_body.size(),"Fractional body lost its admitted hierarchy");
      for(unsigned node=0;node<shown_body.size();++node)
        body_error=std::max(body_error,difference(shown_body[node],hierarchical_body[node]));
      for(unsigned node=0;node<old_body.size();++node)
        old_independent_body_hierarchy_error=std::max(old_independent_body_hierarchy_error,
            difference(old_body[node],hierarchical_body[node]));
      for(unsigned hand=0;hand<2;++hand) {
        const auto shown=joined.sample(objects[hand],fraction,models);
        const auto old_independent=joined.sample(objects[hand],fraction);
        // Isolate attachment drift from body-hierarchy differences: both
        // paths use the exact SAME shown body hand. This is the existing host
        // renderer's attachment-preserving interpolation, with actual native
        // B93B0 parent/node inputs and independently published weapon banks.
        const auto expected=native_weapons[hand].sample(weapon_model,fraction,shown_body[parents[hand].node]);
        const auto old_expected=native_weapons[hand].sample(weapon_model,fraction,old_body[parents[hand].node]);
        need(shown.size()==expected.size(),"Fractional held bank lost its admitted model order");
        float component{},position{};
        for(unsigned node=0;node<shown.size();++node) {
          component=std::max(component,difference(shown[node],expected[node]));
          position=std::max(position,length(shown[node].position-expected[node].position));
        }
        if(fraction==0.f || fraction==1.f)endpoint_error=std::max(endpoint_error,component);
        else {
          for(unsigned node=0;node<old_independent.size();++node) {
            old_independent_weapon_error=std::max(old_independent_weapon_error,difference(old_independent[node],old_expected[node]));
            old_independent_weapon_position_error=std::max(old_independent_weapon_position_error,
                length(old_independent[node].position-old_expected[node].position));
          }
          ++fractional_samples;weapon_error=std::max(weapon_error,component);
          weapon_position_error=std::max(weapon_position_error,position);
          samples.push_back({{"previous_tick",before.tick},{"tick",state.tick},{"fraction",fraction},{"hand",hand},
              {"object",objects[hand]},{"actual_native_parent",parents[hand].object},
              {"actual_native_parent_node",parents[hand].node},{"component_difference",component},
              {"position_difference",position}});
        }
      }
    }
    ++pairs;previous=std::move(next);
  }
  nlohmann::json report()const {
    const bool stimulus=parent_rotation>1e-4f;
    return {{"scope","Actual consecutive native MCC endpoints through joined WORLD interpolation versus the existing host attachment-preserving sampler"},
        {"midpoint_reference","Port presentation policy with actual B93B0 parent/node frames; not an Xbox midpoint framebuffer oracle"},
        {"pairs",pairs},{"fractional_samples",fractional_samples},{"endpoint_component_error",endpoint_error},
        {"native_publication_component_error",native_publication_error},
        {"native_object_root_marker_component_error",native_object_root_marker_error},
        {"weapon_component_error",weapon_error},{"weapon_position_error_world_units",weapon_position_error},
        {"old_independent_weapon_component_error",old_independent_weapon_error},
        {"old_independent_weapon_position_error_world_units",old_independent_weapon_position_error},
        {"old_independent_policy_rejected",old_independent_weapon_error>1e-4f},
        {"body_hierarchy_component_difference",body_error},{"actual_parent_frame_motion",parent_motion},
        {"old_independent_body_hierarchy_component_difference",old_independent_body_hierarchy_error},
        {"actual_parent_frame_rotation",parent_rotation},
        {"native_parent_motion_stimulus",stimulus},{"weapon_node_zero_is_not_object_root",true},
        {"samples",samples},{"passed",pairs>=24u && fractional_samples>=144u && stimulus &&
            native_publication_error<=1e-6f && native_object_root_marker_error<=2e-5f &&
            endpoint_error<=1e-4f && weapon_error<=1e-4f && old_independent_weapon_error>1e-4f}};
  }
};
// The production presentation owner cuts both carried weapons when inventory
// changes and cuts the life/attachment family at player or teleport boundaries.
// The independent ModelNodeInterpolation reference must use that same completed
// endpoint lifetime, rather than bridge the genuine native pickup discontinuity.
// Each cut is also tested against the old, deliberately continuous counterfactual.
bool budget_reference_discontinuity(const MultiplayerState &previous,const MultiplayerState &current,
    std::uint32_t owner) {
  if(previous.source_body_poses_authoritative!=current.source_body_poses_authoritative)return true;
  for(unsigned slot=0;slot<current.players.size();++slot) {
    const auto &a=previous.players[slot],&b=current.players[slot];
    const bool weapon=owner==a.source_weapon || owner==b.source_weapon ||
        owner==a.secondary_weapon.datum || owner==b.secondary_weapon.datum;
    if(weapon && (a.source_weapon!=b.source_weapon || a.secondary_weapon.datum!=b.secondary_weapon.datum))return true;
    const bool same_lifetime=a.connected==b.connected && a.dead==b.dead && a.source_player==b.source_player &&
        a.source_unit==b.source_unit && a.source_parent==b.source_parent && a.source_seat==b.source_seat &&
        a.source_teleport_serial==b.source_teleport_serial && length(a.position-b.position)<5;
    if(!same_lifetime && (weapon || owner==a.source_unit || owner==b.source_unit))return true;
  }
  return false;
}
struct NativeBudgetProbe {
  // pose.rs: positions are rounded at1/1024WU; rigid rotations keep three
  // quaternion components at1/4096 and accept native orthogonality within.002.
  static constexpr float coordinate_bound=.0011f,basis_bound=.003f;
  struct Original {std::uint32_t sequence{},tick{};Transform codec_node_zero;SourceWorldPoseAttachment binding;bool available{};};
  std::string profile,error;
  std::unique_ptr<SnapshotStreamSender> sender;
  SnapshotStreamReceiver receiver{1292};
  std::map<std::uint32_t,std::deque<Original>> originals;
  std::map<std::uint32_t,RenderModel> models;
  std::optional<MultiplayerState> previous;
  unsigned inputs{},delivered{},dropped{},fragments{},provenance{},older_frames{},fractional{},binding_changes{},births{};
  unsigned sequence{};
  float max_root_coordinate{},max_root_basis{},max_policy_error{},max_current_articulation_error{};
  unsigned continuous_policy_samples{},cut_policy_samples{},wrong_continuous_cut_samples{};
  float max_continuous_policy_error{},max_cut_policy_error{},max_wrong_continuous_cut_error{};
  std::uint32_t initial_budget_bps{},last_budget_bps{};
  nlohmann::json adaptive_budget_transitions=nlohmann::json::array();
  explicit NativeBudgetProbe(std::string name):profile(std::move(name)) {
    const auto *prior=std::getenv("H2_NET_PROFILE");const std::optional<std::string> saved=prior?std::optional(std::string(prior)):std::nullopt;
    const auto set=[](const char *value) {
#if defined(_WIN32)
      need(_putenv_s("H2_NET_PROFILE",value)==0,"Declare actual finite native stream profile");
#else
      need(setenv("H2_NET_PROFILE",value,1)==0,"Declare actual finite native stream profile");
#endif
    };
    set(profile.c_str());sender=std::make_unique<SnapshotStreamSender>(1292,80);
    if(saved)set(saved->c_str());
    else {
#if defined(_WIN32)
      set("");
#else
      need(unsetenv("H2_NET_PROFILE")==0,"Restore stream profile after finite sender birth");
#endif
    }
    need(sender->stats().budget_bps==(profile=="original"?30720u:262144u),"Actual Rust sender did not select its finite profile");
    initial_budget_bps=last_budget_bps=sender->stats().budget_bps;
  }
  static bool binding_equal(const SourceWorldPoseAttachment &a,const SourceWorldPoseAttachment &b) {
    return a.parent==b.parent && a.parent_node==b.parent_node &&
        a.parent_frame.has_value()==b.parent_frame.has_value() &&
        (!a.parent_frame || !std::memcmp(&*a.parent_frame,&*b.parent_frame,sizeof(Transform)));
  }
  void receive(const MultiplayerState &source,const SnapshotStreamReceiver::Delivery &delivery,
      const Packet &encoded,std::span<const unsigned> ids,MultiplayerPreparedWorld &world) {
    Reader reader{delivery.body};auto value=decode_state(reader,ids,native_multiplayer_protocol);reader.end();
    auto generic=value;generic.source_body_poses=source.source_body_poses;Packet generic_bytes;
    encode_state(generic_bytes,generic,ids,nullptr,native_multiplayer_protocol);
    need(generic_bytes==encoded,"Finite native stream changed non-pose gameplay or FP inputs");
    need(value.source_body_poses.size()==source.source_body_poses.size(),"Finite117 stream omitted an actual native WORLD owner");
    for(const auto &materialized:value.source_body_poses) {
      const auto &actual=pose(source,materialized.object);
      need(actual.attachment && materialized.attachment && actual.nodes.size()==materialized.nodes.size(),
          "Finite native bank lost its source-qualified binding or node layout");
      need(actual.attachment->parent==materialized.attachment->parent &&
          actual.attachment->parent_node==materialized.attachment->parent_node,
          "Finite stream retained an old native parent/salt/node binding");
      bool matched{};unsigned newest{};float matched_position{},matched_basis{};
      for(const auto &input:originals.at(materialized.object)) {
        if(!input.available)continue;
        if(!binding_equal(input.binding,*materialized.attachment))continue;
        const auto &a=input.codec_node_zero,&b=materialized.nodes.front();float position{},basis{};
        for(unsigned axis=0;axis<3;++axis) {
          position=std::max(position,std::abs(std::bit_cast<std::array<float,13>>(a)[10u+axis]-
              std::bit_cast<std::array<float,13>>(b)[10u+axis]));
          for(unsigned component=0;component<3;++component)basis=std::max(basis,
              std::abs(std::bit_cast<std::array<float,13>>(a)[1u+axis*3u+component]-
                  std::bit_cast<std::array<float,13>>(b)[1u+axis*3u+component]));
        }
        if(position<=coordinate_bound && basis<=basis_bound &&
            std::bit_cast<std::uint32_t>(a.scale)==std::bit_cast<std::uint32_t>(b.scale) && input.sequence>=newest) {
          newest=input.sequence;matched_position=position;matched_basis=basis;matched=true;
        }
      }
      need(matched,"Materialized codec node-zero and exact parent frame have no matching genuine native input tuple");
      ++provenance;older_frames+=newest<inputs;max_root_coordinate=std::max(max_root_coordinate,matched_position);
      max_root_basis=std::max(max_root_basis,matched_basis);
      for(unsigned node=0;node<actual.nodes.size();++node)
        max_current_articulation_error=std::max(max_current_articulation_error,
            length(actual.nodes[node].position-materialized.nodes[node].position));
    }
    const SourceWorldInterpolation::ModelProvider hierarchy=[&](std::uint32_t owner)->const RenderModel* {
      std::uint32_t definition=null_datum;
      for(const auto &player:value.players)if(player.connected) {
        if(owner==player.source_unit)definition=world.content.player_representations.at(
            source_session_player_representation(player.source_configuration,value.source_game_type)).biped.datum;
        if(owner==player.source_weapon)definition=player.weapon;
        if(owner==player.secondary_weapon.datum)definition=player.secondary_weapon.definition;
      }
      for(const auto &corpse:value.corpses)if(owner==corpse.object)definition=corpse.definition;
      for(const auto &item:value.world_weapons)if(owner==item.handle)definition=item.definition;
      for(const auto &item:value.world_vehicles)if(owner==item.handle)definition=world.content.objects.at(item.placement).definition;
      for(const auto &item:value.source_scene_roots)if(owner==item.object)definition=item.definition;
      for(const auto &item:value.source_dynamic_objects)if(owner==item.object)definition=item.definition;
      if(definition==null_datum)return nullptr;
      auto found=models.find(definition);if(found==models.end())found=models.emplace(definition,world.cache->load_model_hierarchy(definition)).first;
      return &found->second;
    };
    if(previous && previous->tick+1u==value.tick) {
      SourceWorldInterpolation history;capture_multiplayer_body_poses(history,*previous,*previous,true);
      capture_multiplayer_body_poses(history,*previous,value);
      for(const auto &now:value.source_body_poses)if(now.attachment && now.attachment->parent!=null_datum) {
        const auto old=std::find_if(previous->source_body_poses.begin(),previous->source_body_poses.end(),
            [&](const auto &p){return p.object==now.object;});
        if(old==previous->source_body_poses.end() || !old->attachment ||
            old->attachment->parent!=now.attachment->parent || old->attachment->parent_node!=now.attachment->parent_node)continue;
        const auto *model=hierarchy(now.object);need(model,"Actual budgeted child has no admitted model hierarchy");
        const bool pair=!budget_reference_discontinuity(*previous,value,now.object) &&
            old->nodes.size()==now.nodes.size() &&
            old->attachment->parent_frame.has_value()==now.attachment->parent_frame.has_value();
        ModelNodeInterpolation reference;
        if(pair)reference.capture(previous->tick,old->nodes,old->attachment->parent_frame);
        reference.capture(value.tick,now.nodes,now.attachment->parent_frame);
        ModelNodeInterpolation wrong_continuous;
        if(!pair) {
          wrong_continuous.capture(previous->tick,old->nodes,old->attachment->parent_frame);
          wrong_continuous.capture(value.tick,now.nodes,now.attachment->parent_frame);
        }
        for(const auto fraction:{.25f,.5f,.75f}) {
          const auto parent_nodes=history.sample(now.attachment->parent,fraction,hierarchy);
          need(now.attachment->parent_node<parent_nodes.size(),"Budgeted child lost its actual shown parent node");
          const auto expected=reference.sample(*model,fraction,parent_nodes[now.attachment->parent_node]);
          const auto shown=history.sample(now.object,fraction,hierarchy);
          need(shown.size()==expected.size(),"Budgeted child changed its model-order bank");
          float policy_error{};
          for(unsigned node=0;node<shown.size();++node)policy_error=std::max(policy_error,difference(shown[node],expected[node]));
          max_policy_error=std::max(max_policy_error,policy_error);
          if(pair) {
            ++continuous_policy_samples;max_continuous_policy_error=std::max(max_continuous_policy_error,policy_error);
          } else {
            ++cut_policy_samples;max_cut_policy_error=std::max(max_cut_policy_error,policy_error);
            const auto wrong=wrong_continuous.sample(*model,fraction,parent_nodes[now.attachment->parent_node]);
            float counterfactual_error{};
            for(unsigned node=0;node<shown.size();++node)counterfactual_error=std::max(counterfactual_error,difference(shown[node],wrong.at(node)));
            max_wrong_continuous_cut_error=std::max(max_wrong_continuous_cut_error,counterfactual_error);
            wrong_continuous_cut_samples+=counterfactual_error>1e-5f;
          }
          ++fractional;
        }
      }
    }
    previous=std::move(value);++delivered;
  }
  void step(const MultiplayerState &state,unsigned slot,MultiplayerPreparedWorld &world) {
    if(!error.empty())return;
    try {
      ++inputs;
      for(const auto &actual:state.source_body_poses) {
        need(actual.attachment.has_value(),"Genuine native budget input lacks its final parent-frame binding");
        auto &history=originals[actual.object];if(history.empty())++births;
        else binding_changes+=history.back().binding.parent!=actual.attachment->parent ||
            history.back().binding.parent_node!=actual.attachment->parent_node;
        history.push_back({inputs,state.tick,actual.nodes.front(),*actual.attachment});
        while(history.size()>256u)history.pop_front();
      }
      const std::array ids{slot};SnapshotBodyLayout layout;Packet body;encode_state(body,state,ids,&layout,native_multiplayer_protocol);
      need(layout.poses.has_value(),"Actual native snapshot did not expose its production pose-bank layout");
      std::vector<float> viewers;std::vector<std::uint32_t> own,hot;
      const auto &player=state.players[slot];viewers={player.position.x,player.position.y,player.position.z};
      if(player.source_unit!=null_datum)own.push_back(player.source_unit);
      for(const auto &p:state.players)if(p.connected && p.source_unit!=null_datum &&
          (p.melee_action.active() || p.grenade_action.animation_active || (p.source_action_word&(1u<<22))))
        for(const auto object:{p.source_unit,p.source_weapon,p.secondary_weapon.datum})if(object!=null_datum)hot.push_back(object);
      for(const auto &corpse:state.corpses)hot.push_back(corpse.object);
      const auto now=1000u+std::uint64_t(inputs)*1000u/30u;
      const auto output=sender->encode({body,layout.floats,layout.anchor_offsets,layout.anchor_keys,layout.poses,
          state.tick,sequence+1u,now,viewers,own,hot});
      need(output.has_value(),"Finite native stream retired or rejected the genuine C++117 body layout");
      if(output->emitted) {
        ++sequence;
        if(inputs%19u==0u)++dropped; // Declared transport loss, never a producer mutation.
        else {
          for(const auto &actual:state.source_body_poses)originals.at(actual.object).back().available=true;
          for(auto part=output->packets.rbegin();part!=output->packets.rend();++part) {
          ++fragments;bool failed{};auto delivery=receiver.push(*part,now,failed);
          need(!failed,"Finite stream rejected actual native fragments");
          if(delivery) {receive(state,*delivery,body,ids,world);need(receiver.commit(delivery->sequence),"Commit actual finite native receipt");}
          }
        }
      }
      sender->ack(receiver.ack(),now+1u);
      const auto current_budget=sender->stats().budget_bps;
      if(current_budget!=last_budget_bps) {
        need(adaptive_budget_transitions.size()<16u,"Bound actual finite budget transition evidence");
        adaptive_budget_transitions.push_back({{"input",inputs},{"tick",state.tick},{"now_ms",now},
            {"before_bps",last_budget_bps},{"after_bps",current_budget}});
        last_budget_bps=current_budget;
      }
    }catch(const std::exception &failure){error=failure.what();}
  }
  nlohmann::json report()const {
    const auto sent=sender->stats();const auto received=receiver.stats();
    return {{"profile",profile},{"inputs",inputs},{"delivered",delivered},{"declared_lost_messages",dropped},
        {"fragments",fragments},{"budget_bps",sent.budget_bps},{"full_refreshes",sent.full_refreshes},
        {"initial_budget_bps",initial_budget_bps},{"adaptive_budget_transitions",adaptive_budget_transitions},
        {"forced_full_refreshes",sent.forced_refreshes},{"root_updates",sent.root_updates},{"dirty_held_refreshes",sent.held_refreshes},
        {"receiver_held_or_root_materializations",received.held_poses},{"receiver_missing_poses",received.missing_poses},
        {"receiver_failures",received.failures},{"over_budget",sent.over_budget},{"retired",sent.retired},
        {"wire_bytes",sent.wire_bytes},{"emitted_messages",sent.messages},{"reported_lost_messages",sent.lost_messages},
        {"native_input_tuple_proofs",provenance},{"older_native_tuples",older_frames},{"new_native_pose_owners",births},
        {"native_parent_or_node_transitions",binding_changes},{"fractional_policy_samples",fractional},
        {"coordinate_component_bound_world_units",coordinate_bound},{"rotation_component_bound",basis_bound},
        {"parent_frame_prefix_lossless",error.empty() && provenance>0},{"generic_non_pose_exact",error.empty() && delivered>0},
        {"codec_node_zero_is_not_object_root",true},
        {"max_codec_root_coordinate_error",max_root_coordinate},{"max_codec_root_basis_error",max_root_basis},
        {"max_internal_parent_frame_policy_error",max_policy_error},
        {"continuous_policy_samples",continuous_policy_samples},{"cut_policy_samples",cut_policy_samples},
        {"wrong_continuous_cut_samples",wrong_continuous_cut_samples},
        {"max_continuous_policy_error",max_continuous_policy_error},{"max_cut_policy_error",max_cut_policy_error},
        {"max_wrong_continuous_cut_error",max_wrong_continuous_cut_error},
        {"presentation_discontinuity_policy","Independent model reference resets at the actual source player/inventory/teleport lifetime boundary; the old continuous counterfactual must differ"},
        {"max_current_articulation_position_difference_world_units",max_current_articulation_error},
        {"articulation_limit","Budgeted articulated banks may be older; the finite profile is not lossless native-pose parity"},
        {"path_observability","Sender counters classify full/root/dirty deferral globally; receiver held counter also includes roots. No per-entry Rust trace is inferred."},
        {"error",error},{"passed",error.empty() && inputs>=180u && delivered>=150u && provenance && fractional &&
            continuous_policy_samples && cut_policy_samples && wrong_continuous_cut_samples && max_wrong_continuous_cut_error>1e-5f &&
            sent.budget_bps<std::numeric_limits<std::uint32_t>::max() && !sent.retired && !received.failures &&
            !received.missing_poses && max_policy_error<=1e-5f}};
  }
};
nlohmann::json native_budget_case(MultiplayerPreparedWorld &world,unsigned slot,unsigned representation) {
  NativeBudgetProbe original("original"),broadband("broadband");auto &match=world.match;auto &native=*world.source_objects;
  const auto weapon=world.cache->slayer_starting_weapon(5);std::optional<unsigned> dual_tick;unsigned dual_frames{},drops{};
  std::string native_error;
  try {for(unsigned tick=0;tick<240u;++tick) {
    const auto before=match.state();const auto &old=before.players[slot];std::array<Controls,multiplayer_capacity> controls{};
    if(old.source_unit!=null_datum && !dual_tick) {
      const auto pickup=std::find_if(before.world_weapons.begin(),before.world_weapons.end(),[&](const auto &p){return p.definition==weapon;});
      if(pickup!=before.world_weapons.end())native.tu15_world().set_object_position(old.source_unit,pickup->position+Vec3{-.2f,0,0});
      controls[slot].dual_wield=true;controls[slot].dual_wield_milliseconds=1000;
    }
    if(dual_tick) {
      const auto elapsed=tick-*dual_tick;
      controls[slot].pitch=.5f*std::sin(float(elapsed)*.12f);
      if(elapsed>=60u && elapsed<120u) {controls[slot].yaw=.6f*std::sin(float(elapsed)*.15f);controls[slot].forward=.08f;}
      controls[slot].fire=elapsed>=80u && elapsed<100u;
      controls[slot].fire_secondary=elapsed>=120u && elapsed<140u;
      controls[slot].switch_weapon=elapsed==174u;
    }
    match.step(controls);const auto state=match.state();const auto &player=state.players[slot];
    if(player.secondary_weapon.datum!=null_datum){if(!dual_tick)dual_tick=tick;++dual_frames;}
    if(old.secondary_weapon.datum!=null_datum && player.secondary_weapon.datum==null_datum)++drops;
    SourceRuntimeWriteSerial serial(native.memory());original.step(state,slot,world);broadband.step(state,slot,world);
    need(!serial.load(),"Finite stream or fractional renderer wrote native producer inputs");
  }}catch(const std::exception &failure){native_error=failure.what();}
  auto a=original.report(),b=broadband.report();const auto active=[](const nlohmann::json &row) {
    return row["full_refreshes"].get<std::uint64_t>()+row["forced_full_refreshes"].get<std::uint64_t>()>0 &&
        row["root_updates"].get<std::uint64_t>()>0 && row["dirty_held_refreshes"].get<std::uint64_t>()>0;
  };
  const bool paths=active(a)||active(b);
  return {{"representation",representation},{"dual_native_frames",dual_frames},{"native_drop_observed",drops},
      {"profiles",{a,b}},{"actual_full_root_dirty_hold_paths",paths},{"native_error",native_error},
      {"coverage_gap",paths?"":"No finite native-input case observed all full, root and dirty-hold sender paths"},
      {"passed",native_error.empty() && dual_tick && dual_frames>=120u && a["passed"].get<bool>() && b["passed"].get<bool>() && paths}};
}
}
int main(int argc,char**argv)try {
  need(argc==4 || (argc==5 && (std::string_view(argv[4])=="fractional" || std::string_view(argv[4])=="budget")),"Supply prepared MCC map, shared maps, report and optional fractional|budget");
  const bool fractional=argc==5 && std::string_view(argv[4])=="fractional",budget=argc==5 && std::string_view(argv[4])=="budget";
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run through tools/run_test.py");
  nlohmann::json report{{"native",true},{"runtime_emulation",false},{"cases",nlohmann::json::array()}};bool passed=true;
  if(budget)report["scope"]="Genuine native MCC snapshots through production C++117 layout, finite Rust stream and model-order parent-frame sampling; no finite-profile GPU/UDP rendering claim";
  for(unsigned representation:{2u,3u}) {
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=0;
    options.dedicated=true;options.mute=true;options.starting_weapon=5;options.source_object_world=true;
    options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};
    options.source_object_savegame_directory=std::filesystem::path(scratch)/std::to_string(representation);
    MultiplayerPreparedWorld world(options);auto&match=world.match;auto&native=*world.source_objects;
    need(world.cache->is_mcc()&&native.owns_physics()&&match.source_engine_owned(),"Native canonical MCC authority required");
    auto profile=source_default_player_configuration();profile[0x44]=representation==3;
    const auto slot=match.join("Native grip",-1,profile),weapon=world.cache->slayer_starting_weapon(5);
    match.configure_replicated_first_person(world.cache->load_first_person_binding(weapon,representation));
    if(budget) {auto row=native_budget_case(world,slot,representation);passed&=row["passed"].get<bool>();report["cases"].push_back(std::move(row));continue;}
    const auto biped=world.content.player_representations.at(representation).biped.datum;
    const auto model=world.cache->load_model(biped,false);std::array<Controls,multiplayer_capacity> input{};
    const auto weapon_model=world.cache->load_model(weapon,false);FractionalGripProbe interpolation;
    std::optional<unsigned> dual_tick;bool measured=false;
    for(unsigned tick=0;tick<(fractional?180u:100u)&&!measured;++tick){
      const auto prior=match.state();const auto&player=prior.players[slot];
      if(player.source_unit!=null_datum&&!dual_tick){
        const auto pickup=std::find_if(prior.world_weapons.begin(),prior.world_weapons.end(),[&](const auto&w){return w.definition==weapon;});
        if(pickup!=prior.world_weapons.end())native.tu15_world().set_object_position(player.source_unit,pickup->position+Vec3{-.2f,0,0});
        input[slot].dual_wield=true;input[slot].dual_wield_milliseconds=1000;
      }
      if(fractional && dual_tick) {
        const auto elapsed=tick-*dual_tick;
        input[slot].yaw=.65f*std::sin(float(elapsed)*.45f);
        input[slot].pitch=.3f*std::cos(float(elapsed)*.3f);input[slot].forward=.2f;
        input[slot].fire=elapsed>=8u && elapsed<14u;
        input[slot].fire_secondary=elapsed>=16u && elapsed<22u;
      }
      match.step(input);auto state=match.state();const auto&p=state.players[slot];
      if(p.secondary_weapon.datum==null_datum)continue;
      input[slot].dual_wield=false;if(!dual_tick)dual_tick=tick;if(tick-*dual_tick<5)continue;
      const auto&memory=native.memory();const int owner{};std::map<std::uint32_t,std::byte> before;unsigned topology{};
      source_runtime_register_write_observer(memory,&owner,[&](auto at,auto size){const auto bytes=memory.view(at,size);for(unsigned n=0;n<size;++n)before.try_emplace(at+n,bytes[n]);});
      source_runtime_register_topology_observer(memory,&owner,[&](auto,auto){++topology;});
      try{state=match.state();(void)match.state();}
      catch(...){source_runtime_unregister_write_observer(memory,&owner);source_runtime_unregister_topology_observer(memory,&owner);throw;}
      source_runtime_unregister_write_observer(memory,&owner);source_runtime_unregister_topology_observer(memory,&owner);
      unsigned changed{};for(const auto&[at,value]:before)if(memory.view(at,1)[0]!=value)++changed;
      need(!changed&&!topology,"Body pose publication changed retained native source memory or topology");
      const auto publication=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto&pose){return pose.object==p.source_unit;});
      need(publication!=state.source_body_poses.end(),"Source player publication is absent");
      native.preload_render_model(render_model(*world.cache,biped));
      native.preload_render_model(render_model(*world.cache,weapon));native.begin_model_frame();
      native.render_model_view([&](auto&source){
        (void)source.build_world_model(p.source_unit,0,1,false,1.f,1.f);
        (void)source.build_world_model(p.source_weapon,0,1,false,1.f,1.f);
        (void)source.build_world_model(p.secondary_weapon.datum,0,1,false,1.f,1.f);
      });
      const auto completed=native.rendered_world_nodes(p.source_unit);need(completed&&completed->size()==model.bones.size(),"Actual native BF640 output absent");
      need(publication->nodes.size()==completed->size(),"Published model node count differs");
      nlohmann::json changed_nodes=nlohmann::json::array();float error{},position_error{};
      for(unsigned n=0;n<completed->size();++n){const auto delta=difference(publication->nodes[n],completed->at(n));error=std::max(error,delta);
        if(delta>1e-6f){const auto position=length(publication->nodes[n].position-completed->at(n).position);position_error=std::max(position_error,position);
          changed_nodes.push_back({{"node",n},{"name",world.cache->string_id(model.bones[n].name)},{"component_error",delta},{"position_error",position}});}}
      const auto parent_regions=native.object_model_regions(p.source_unit).selection.permutations;
      nlohmann::json hands=nlohmann::json::array();
      std::array<FractionalGripProbe::Parent,2> actual_parents;
      bool attachments_equal=true;
      for(unsigned hand=0;hand<2;++hand) {
        const auto object=hand?p.secondary_weapon.datum:p.source_weapon;
        const auto native_body=native.tu15_world().source_object_body(object);
        const Bytes native_parent(memory.view(native_body+0x14u,5));
        actual_parents[hand]={native_parent.u32(0),std::to_integer<unsigned>(native_parent.slice(4,1).front())};
        const auto definition=native.object_definition(object);
        const auto child_regions=native.object_model_regions(object).selection.permutations;
        const auto attachment=select_weapon_attachment(
            world.cache->load_weapon_attachment(biped,definition,hand),parent_regions,child_regions);
        // Both admitted SMGs use the unnamed-child branch of D0870/B8EE0.
        // Named child markers need their original model-local pose, not a
        // WORLD matrix substituted into that branch.
        need(!attachment.named_child,"Dual-SMG marker fixture unexpectedly selected a named child");
        const auto root=native.object_transform(object);
        const auto parent=compose_transform(completed->at(attachment.parent.node),attachment.parent.local);
        const auto expected=source_object_attachment_frame(
            {root.position,root.basis[0],root.basis[2]},parent,attachment.child.local,false);
        const auto root_error=difference(root,camera_transform(expected.position,expected.forward,expected.up));
        const auto held=native.rendered_world_nodes(object);
        const auto sent=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
            [&](const auto&pose){return pose.object==object;});
        need(held && sent!=state.source_body_poses.end() && held->size()==sent->nodes.size(),
            "Dual weapon lost its rendered or published WORLD bank");
        float held_error{};
        for(unsigned n=0;n<held->size();++n)held_error=std::max(held_error,difference(held->at(n),sent->nodes[n]));
        if(fractional) {
          interpolation.native_publication_error=std::max(interpolation.native_publication_error,held_error);
          interpolation.native_object_root_marker_error=std::max(interpolation.native_object_root_marker_error,root_error);
        }
        const bool same=root_error<=2e-5f && held_error<=1e-6f;
        attachments_equal&=same;
        hands.push_back({{"hand",hand},{"object",object},{"parent_node",attachment.parent.node},
            {"authored_root_max_component_error",root_error},{"published_native_weapon_error",held_error},
            {"same",same}});
      }
      const bool equal=error<=1e-6f && attachments_equal;passed&=equal;
      if(fractional) {
        interpolation.native_publication_error=std::max(interpolation.native_publication_error,error);
        SourceRuntimeWriteSerial serial(memory);
        interpolation.capture(state,slot,model,weapon_model,actual_parents);
        need(!serial.load(),"Fractional presentation sampling changed native authority bytes");
        if(interpolation.pairs<24u)continue;
      }
      measured=true;
      nlohmann::json row{{"representation",representation},{"tick",state.tick},{"published_matches_native_BF640",equal},
        {"max_component_error",error},{"max_position_error",position_error},{"source_bytes_changed",changed},
        {"source_topology_changes",topology},{"nodes",changed_nodes},{"hands",hands}};
      if(fractional) {row["fractional"]=interpolation.report();passed&=row["fractional"]["passed"].get<bool>();}
      report["cases"].push_back(std::move(row));
    }
    need(measured,"Native dual-wield fixture did not reach actual held pair");
  }
  report["passed"]=passed;std::ofstream(argv[3])<<report.dump(2)<<'\n';
  if(!passed){std::cerr<<(budget?"FAIL actual finite native budget path or its required full/root/dirty-hold coverage\n":fractional?"FAIL joined fractional held pose differs from the actual native parent-frame interpolation policy\n":
      "FAIL published player fingers differ from native BF640 completion\n");return 1;}
  std::cout<<(budget?"PASS actual finite native budget and parent-frame inputs for both species\n":
      "PASS native completed body grip publication for both species\n");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

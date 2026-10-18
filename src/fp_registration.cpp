#include "halo2/fp_registration.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
float first_person_ammunition_frame(std::int16_t loaded,std::int16_t available,
    std::int16_t capacity,std::int16_t elapsed,std::int16_t weapon_type,
    std::uint32_t activity,unsigned frames) {
  if(!frames || frames>32767)throw FormatError("Invalid first-person ammunition frame count");
  int frame=std::clamp<int>(loaded,0,int(frames)-1);
  if(weapon_type==2 && (activity==0x0c000064u || activity==0x0b000065u)) {
    frame=loaded;
    if(elapsed>=44) {
      const float fraction=std::min(float(int(elapsed)-44)*std::bit_cast<float>(0x3e4ccccdu),1.f);
      const auto increment=int(float(int(std::min(available,capacity))-int(loaded))*fraction);
      frame=std::bit_cast<std::int16_t>(std::uint16_t(int(loaded)+increment));
    }
  }
  //1C9F50 bounds the channel after the weapon-specific selector.
  return float(std::clamp(frame,0,int(frames)-1));
}

namespace {
void remove(FirstPersonHandBinding &hand) {
  hand.flags &= ~7u;
  hand.weapon=0xffffffffu;
  // Original removal deliberately preserves maps, counts and manager channel.
}
bool map_nodes(std::span<const std::uint32_t> model,std::span<const std::uint32_t> graph,
    std::uint32_t &count,std::array<std::int32_t,64> &indices) {
  if(model.size()>indices.size())throw FormatError("First-person model exceeds source map capacity");
  count=std::uint32_t(model.size());bool success=true;
  for(std::size_t n=0;n<model.size();++n) {
    const auto found=std::find(graph.begin(),graph.end(),model[n]);
    indices[n]=found==graph.end() ? -1 : std::int32_t(found-graph.begin());
    success=success && indices[n]!=-1;
  }
  return success;
}
void rebuild(FirstPersonHandBinding &hand,const FirstPersonViewBinding &view,
    std::uint32_t weapon,const FirstPersonDefinitionLookup &lookup) {
  remove(hand);
  if(view.unit==0xffffffffu || view.representation<0 || weapon==0xffffffffu)return;
  const auto *definition=lookup(weapon,view.representation);
  if(!definition || !definition->record_present || definition->weapon_model==0xffffffffu ||
      definition->graph==0xffffffffu)return;
  hand.graph=definition->graph;
  hand.animation_identifier=0xffffffffu;hand.channel_flags=0;
  hand.channel_options=0;hand.rate=1;hand.frame=0;
  hand.activity=0xffffffffu;hand.channel_delay=0;hand.events=hand.future_events=0;
  if(definition->arms_model!=0xffffffffu &&
      map_nodes(definition->arms_nodes,definition->graph_nodes,hand.arms_node_count,hand.arms_nodes))
    hand.flags|=4;
  if(hand.secondary_hand && !definition->graph_node_flags.empty()) {
    if(definition->graph_node_flags.size()!=definition->graph_nodes.size())
      throw FormatError("Invalid first-person graph node flags");
    for(unsigned i=0;i<hand.arms_node_count;++i)
      if(hand.arms_nodes[i]>=0 && !(definition->graph_node_flags.at(hand.arms_nodes[i])&0x20u))
        hand.arms_nodes[i]=-1;
  }
  if(map_nodes(definition->weapon_nodes,definition->graph_nodes,hand.weapon_node_count,hand.weapon_nodes))
    hand.flags|=2;
  if((hand.flags & 6)==6)hand.weapon=weapon;
  // The recovered SMG calls idle selection even when its arms map is absent.
  if(definition->idle_index!=0xffffffffu) {
    auto idle=definition->idle_identifier==0xffffffffu ? (definition->idle_index<<16)|0xffffu : definition->idle_identifier;
    for(const auto &clip:first_person_clips(hand,*definition))if(clip.activity==0x0400000c)idle=first_person_clip_identifier(clip);
    hand.animation_identifier=idle;
    hand.channel_options=hand.dual_wield?(hand.secondary_hand?0x203f:0x103f):0x3f;
    hand.activity=0x0400000c;
  }
}
}
void release_first_person_binding(FirstPersonViewBinding &view) {
  if(view.unit==0xffffffffu)return;
  view.unit=0xffffffffu;view.representation=-1;
  for(auto &hand:view.hands)remove(hand);
}
void observe_first_person_binding(FirstPersonViewBinding &view,const FirstPersonBindingInput &input,
    const FirstPersonDefinitionLookup &lookup) {
  if(input.player==0xffffffffu)return; // Original1678C1 skips unmapped local views.
  if(view.unit!=input.unit) {
    view.unit=input.unit;view.representation=input.representation;
    for(unsigned h=0;h<2;++h)remove(view.hands[h]);
  }
  // A selector change with an unchanged unit does not update the stored selector.
  for(unsigned h=0;h<2;++h)
    if(input.weapons[h]!=view.hands[h].weapon || view.hands[h].dual_wield!=(input.weapons[1]!=0xffffffffu)) {
      view.hands[h].dual_wield=input.weapons[1]!=0xffffffffu;
      view.hands[h].secondary_hand=h!=0;
      rebuild(view.hands[h],view,input.weapons[h],lookup);
    }
}
std::int32_t first_person_weapon_view(std::span<const FirstPersonViewBinding> views,std::uint32_t weapon) {
  std::int32_t selected=-1;
  for(std::size_t v=0;v<views.size();++v)
    for(const auto &hand:views[v].hands)
      if(hand.weapon==weapon)selected=std::int32_t(v);
  return selected;
}

FirstPersonBindingDefinition Cache::load_first_person_binding(std::uint32_t weapon,std::int32_t selector) const {
  return load_first_person_binding(weapon,selector,[this](auto pointer,auto bytes){return raw_data(pointer,bytes);});
}
FirstPersonBindingDefinition Cache::load_first_person_binding(std::uint32_t weapon,std::int32_t selector,
    const AnimationResourceReader &resources) const {
  FirstPersonBindingDefinition result;result.weapon_definition=weapon;result.representation=selector;
  const Bytes data(cache_bytes());
  const auto root=[&](std::uint32_t id,const char *group,std::size_t size) {
    const auto found=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
    if(!found)throw FormatError("Missing first-person binding tag");
    return metadata_offset(found->address,size);
  };
  const auto records=[&](std::size_t at,std::size_t stride,unsigned maximum) {
    const auto count=std::bit_cast<std::int32_t>(data.u32(at));
    if(count<=0)return std::pair<std::size_t,std::size_t>{0,0};
    if(unsigned(count)>maximum)throw FormatError("First-person binding block exceeds limit");
    return std::pair<std::size_t,std::size_t>{std::size_t(count),metadata_offset(data.u32(at+4),std::size_t(count)*stride)};
  };
  const auto globals=query_tag({.group=tag_groups::globals, .require_address=true});
  if(!globals)return result;
  const auto [representations,representation_base]=records(metadata_offset(globals->address,0x140)+0x138,0xbc,256);
  if(selector<0 || std::size_t(selector)>=representations)return result;
  result.arms_model=data.u32(representation_base+std::size_t(selector)*0xbc+4);
  const auto [count,base]=records(root(weapon,"weap",0x2b0)+0x2a8,0x10,256);
  if(!count)return result;
  const unsigned preferred=selector==1 || selector==3 ? 1 : 0;
  const auto record=base+(count>preferred ? preferred : 0)*0x10;
  result.record_present=true;result.weapon_model=data.u32(record+4);result.graph=data.u32(record+12);
  if(result.graph==0xffffffffu)return result;
  const auto names=[&](std::uint32_t id,const char *group,std::size_t block_offset,std::size_t stride,unsigned maximum) {
    const auto [n,p]=records(root(id,group,block_offset+8)+block_offset,stride,maximum);
    std::vector<std::uint32_t> values;values.reserve(n);
    for(std::size_t i=0;i<n;++i)values.push_back(data.u32(p+i*stride));
    return values;
  };
  if(result.weapon_model!=0xffffffffu)result.weapon_nodes=names(result.weapon_model,"mode",0x48,0x60,64);
  if(result.arms_model!=0xffffffffu)result.arms_nodes=names(result.arms_model,"mode",0x48,0x60,64);
  result.graph_nodes=names(result.graph,"jmad",0xc,0x20,255);
  const auto [graph_node_count,graph_node_base]=records(root(result.graph,"jmad",0x14)+0xc,0x20,255);
  for(std::size_t i=0;i<graph_node_count;++i)
    result.graph_node_flags.push_back(std::uint8_t(data.u16(graph_node_base+i*0x20+10)));
  const auto idle=load_action(result.graph,"any","any","any","idle",resources);
  result.idle_index=idle.source_index;
  result.idle_identifier=idle.source_identifier;
  result.continuous_fire_animation=(data.u32(root(weapon,"weap",0x130)+0x12c)&(1u<<17))!=0;
  for(bool dual:{false,true})
  for(const auto activity:std::array<std::uint32_t,36>{
      0x0400000c,0x06000006,0x05000024,0x0c000064,0x0b000065,0x0d000021,
      0x0e000607,0x0e000608,0x0e000609,0x0e00060a,
      0x0c000073,0x0c000075,0x0c000077,0x11000074,0x11000076,
      0x0a0005bb,0x0b0005b2,0x130005bc,0x140005b3,0x0500000a,
      0x0b00006d,0x0a000066,0x08000071,0x0c0005ae,0x150005af,0x140005b0,0x0b0005b1,0x08000025,
      0x06000007,0x0900006a,0x0900006b,0x0b000067,0x08000063,0x09000062,0x1000006c,0x1000006e}) {
    // E95E0/EFE30 ->16A356 ->1680B3 selects the literal primary throw
    // activity0D000021. Resolve that StringID in the authored action table;
    // neither the body throw nor a guessed FP clip chooses the animation.
    AnimationClip selected;
    //1CF470 retains the ordered class/type fallbacks for dual actions too.
    try {selected=load_unit_weapon_action(result.graph,0x030000d9,
        dual?0x0400054b:0x030000d9,0x030000d9,activity,false,resources);}
    catch(const FormatError &) {if(activity==0x0400000c && !dual)throw;continue;}
    // The source channel retains the manager graph plus its packed selector.
    // Definitions and event references belong to the selected inherited graph.
    const auto graph_root=root(selected.source_graph,"jmad",0x64);
    const auto [clip_count,clip_base]=records(graph_root+0x2c,0x6c,10000);
    const auto [sound_count,sound_base]=records(graph_root+0x14,12,10000);
    const auto [effect_count,effect_base]=records(graph_root+0x1c,12,10000);
    if(selected.source_index>=clip_count || selected.source_identifier==0xffffffffu)
      throw FormatError("First-person clock has no valid original packed definition");
    const auto at=clip_base+selected.source_index*0x6c;
    FirstPersonClockClip clip;
    clip.index=selected.source_index;clip.activity=activity;
    clip.inheritance=std::uint16_t(selected.source_identifier);
    clip.frames=std::bit_cast<std::int16_t>(data.u16(at+0x14));
    clip.loop_frame=std::bit_cast<std::int16_t>(data.u16(at+0x2e));
    for(unsigned type=0;type<3;++type) {
      const auto [n,base]=records(at+0x4c+type*8,type==1?8:4,10000);
      for(std::size_t i=0;i<n;++i) {
        const auto p=base+i*(type==1?8:4);
        const auto index=std::bit_cast<std::int16_t>(data.u16(p));
        FirstPersonAnimationEvent event;
        event.type=std::uint16_t(type);event.frame=std::bit_cast<std::int16_t>(data.u16(p+2));
        event.marker=0x0600008a; // Original1CAE40 timing/effect callback marker.
        if(!type)event.id=std::uint32_t(std::int32_t(index));
        else {
          if(index==-1)continue;
          const auto count=type==1?sound_count:effect_count;
          const auto references=type==1?sound_base:effect_base;
          if(index<0 || std::size_t(index)>=count)throw FormatError("Invalid FP event reference");
          event.tag=data.u32(references+std::size_t(index)*12+4);
          event.reference_flags=data.u16(references+std::size_t(index)*12+8);
          if(type==1)event.marker=data.u32(p+4);
          if(event.tag==0xffffffffu)continue;
        }
        clip.events.push_back(event);
      }
    }
    (dual?result.dual_clock_clips:result.clock_clips).push_back(std::move(clip));
  }
  //169965 asks the manager for action0A000069 with current mode/class.
  // The original ordered fallback searches the overlay bank, not base actions.
  for(unsigned dual=0;dual<2;++dual) {
    try {result.ammunition[dual]=load_unit_weapon_action(result.graph,0x030000d9u,
        dual?0x0400054bu:0x030000d9u,0x030000d9u,0x0a000069u,true,resources);}
    catch(const FormatError &e) {
      if(std::string(e.what())!="authored unit weapon action missing: "+string_id(0x0a000069u))throw;
    }
  }
  return result;
}
}

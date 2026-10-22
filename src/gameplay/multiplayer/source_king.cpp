#include "halo2/source_king.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
bool source_king_activation_publication(std::uint8_t mode,std::uint32_t recipient,
    std::span<std::byte,0x24> storage) noexcept {
  if(mode==4)return false;
  const auto word=[&](unsigned at,std::uint32_t value){
    for(unsigned i=0;i<4;++i)storage[at+i]=std::byte((value>>(i*8))&255u);
  };
  word(0,6);word(4,0);word(8,recipient);
  for(unsigned at=0xc;at<=0x18;at+=4)word(at,0xffffffffu);
  word(0x1c,0);storage[0x20]=storage[0x21]=std::byte{0xff};return true;
}
namespace {
void markers_supported(std::span<const SourceKingMarker> markers) {
  // Original loops use a signed16 scenario index. Larger inputs cannot
  // complete that original owner and are not admitted as native geometry.
  if(markers.size()>32767)throw std::runtime_error("King scenario exceeds original signed marker index");
}
std::int16_t narrow(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
void geometry(SourceKingState& state,std::span<const SourceKingMarker> markers,std::int16_t type) {
  const auto count=state.marker_count;
  if(count<4 || count>16 || (count&1))
    throw std::runtime_error("King spline requires the original even control count4..16");
  std::array<std::array<float,3>,16> points{};
  for(int i=0;i<count;++i) {
    const auto index=state.marker_indices[i];
    if(index<0 || std::size_t(index)>=markers.size())
      throw std::runtime_error("King spline references an unavailable authored marker");
    points[i]=markers[index].position;
  }
  unsigned output=0;
  for(int segment=0;segment<count/2;++segment) {
    const int previous=segment==0 ? count-1 : segment*2-1;
    const int next=segment*2+2==count ? 0 : segment*2+2;
    std::array<std::array<float,3>,4> controls{};
    for(unsigned axis=0;axis<3;++axis) {
      controls[0][axis]=(points[segment*2][axis]+points[previous][axis])*0.5f;
      controls[1][axis]=points[segment*2][axis];
      controls[2][axis]=points[segment*2+1][axis];
      controls[3][axis]=(points[next][axis]+points[segment*2+1][axis])*0.5f;
    }
    const auto samples=state.segment_samples[segment];
    if(samples<0 || output+std::uint32_t(samples)>32)
      throw std::runtime_error("King spline sample bank is unavailable");
    for(int sample=0;sample<samples;++sample) {
      const float t=float(sample)/float(samples),t2=t*t,t3=t2*t;
      constexpr float coefficients[4][4]={{-1,3,-3,1},{3,-6,3,0},{-3,3,0,0},{1,0,0,0}};
      std::array<float,4> weights{};
      for(unsigned i=0;i<4;++i)
        weights[i]=((coefficients[i][0]*t3+coefficients[i][2]*t)+t2*coefficients[i][1])+coefficients[i][3];
      for(unsigned axis=0;axis<2;++axis)
        state.polygon[output][axis]=((controls[3][axis]*weights[3]+controls[2][axis]*weights[2])+
            controls[1][axis]*weights[1])+controls[0][axis]*weights[0];
      ++output;
    }
  }
  state.perimeter=0;
  for(unsigned i=0;i<32;++i) {
    const auto& previous=state.polygon[(i+31)%32];
    const double dx=double(previous[0])-double(state.polygon[i][0]);
    const double dy=double(previous[1])-double(state.polygon[i][1]);
    // The original x87 control word027F selects53-bit intermediates. Its
    // perimeter is narrowed only at the final store of each edge sum.
    state.perimeter=float(std::sqrt(dy*dy+dx*dx)+double(state.perimeter));
  }
  state.center={};
  for(int i=0;i<count;++i)for(unsigned axis=0;axis<3;++axis)
    state.center[axis]=points[i][axis]+state.center[axis];
  const float inverse=1.0f/float(count);
  for(auto& component:state.center)component=component*inverse;
  state.radius=0;
  for(int i=0;i<count;++i) {
    const float dx=points[i][0]-state.center[0],dy=points[i][1]-state.center[1];
    const float squared=dy*dy+dx*dx;
    state.radius=state.radius>squared ? state.radius : squared;
  }
  state.radius=float(std::sqrt(double(state.radius)));
  state.minimum_z=state.maximum_z=state.center[2];
  for(int i=0;i<count;++i) {
    if(state.minimum_z>points[i][2])state.minimum_z=points[i][2];
    if(state.maximum_z<=points[i][2])state.maximum_z=points[i][2];
  }
  unsigned height_count=0;
  for(const auto& marker:markers)if(marker.type==type && marker.channel==1 && height_count<8) {
    const float z=marker.position[2];
    state.minimum_z=state.minimum_z>z ? z : state.minimum_z;
    state.maximum_z=state.maximum_z>z ? state.maximum_z : z;
    ++height_count;
  }
  state.minimum_z=state.minimum_z-0.1f;
  state.maximum_z=state.maximum_z+0.8f;
}
}
void source_king_initialize(SourceKingState& state,SourceKingHills& hills,
    std::span<const SourceKingMarker> markers) {
  markers_supported(markers);state={};hills.count=0;
  for(const auto& marker:markers)if(marker.type>=11 && marker.type<=18) {
    const auto index=std::int16_t(marker.type-11);
    if(std::find(hills.indices.begin(),hills.indices.begin()+hills.count,index)==hills.indices.begin()+hills.count)
      hills.indices[hills.count++]=index;
  }
  for(auto& point:state.default_points)point={1,1,1};
}
void source_king_build_hill(SourceKingState& state,std::span<const SourceKingMarker> markers,std::int32_t hill) {
  markers_supported(markers);
  if(hill==-1){state.marker_count=0;return;}
  const auto type=narrow(std::uint32_t(hill)+11u);int count=0;
  for(unsigned i=0;i<markers.size();++i)if(markers[i].type==type && markers[i].channel==0 && count<16)
    state.marker_indices[count++]=std::int32_t(i);
  if(count>3) {
    state.marker_count=count-(count&1);
    const auto segments=state.marker_count/2;
    for(int i=0;i<segments;++i)state.segment_samples[i]=((i+1)*32)/segments-(i*32)/segments;
  }
  if(state.marker_count>3 && !(state.marker_count&1))geometry(state,markers,type);
}
bool source_king_trait(std::int16_t inside,std::int32_t selector,std::uint32_t hill,std::uint32_t global) {
  const bool in_hill=inside>0;
  unsigned bit;
  switch(selector) {
    case 1:bit=in_hill ? 4 : 2;break;
    case 2:bit=in_hill ? 2 : 12;break;
    case 3:bit=in_hill ? 3 : 13;break;
    default:return false;
  }
  return ((in_hill ? hill : global)&(std::uint32_t(1)<<bit))!=0;
}
void source_king_death_statistics(const SourceKingPlayer& victim,const SourceKingPlayer* attacker,
    std::uint8_t betrayal,const std::function<void(std::uint32_t,int)>& add) {
  if(betrayal || !attacker || attacker->datum==victim.datum)return;
  const auto award=[&](int stat) {
    if(!add)throw std::runtime_error("King retained-stat owner unavailable");
    add(attacker->datum,stat);
  };
  if(victim.inside_ticks>0)award(28);
  if(attacker->inside_ticks>0)award(27);
}
void source_king_round_start(SourceKingState& state,std::span<const SourceKingMarker> markers,
    std::int16_t seconds,std::int16_t rate) {
  state.hill=0;state.move_ticks=std::int32_t(seconds)*std::int32_t(rate);
  source_king_build_hill(state,markers,0);
}
std::int32_t source_king_select_hill(const SourceKingHills& hills,std::int32_t current,std::uint32_t& seed) {
  seed=seed*0x19660du+0x3c6ef35fu;
  if(hills.count<1)return -1;
  if(hills.count>8)throw std::runtime_error("King group bank exceeds the original eight authored types");
  const auto first=narrow(((seed>>16)*std::uint32_t(hills.count))>>16);
  for(int i=0;i<hills.count;++i) {
    const auto index=(i+first)%hills.count;
    if(current!=hills.indices[index])return hills.indices[index];
  }
  return -1;
}
bool source_king_contains(const SourceKingState& state,const SourceKingPlayer& player,const SourceKingPosition& position) {
  if(player.datum==0xffffffffu || state.marker_count<4 || (state.marker_count&1) || player.current_unit==0xffffffffu)return false;
  if(!position)throw std::runtime_error("Original King current-unit world-position owner is unavailable");
  const auto point=position(player.current_unit);
  if(!(point[2]>=state.minimum_z && state.maximum_z>=point[2]))return false;
  const float dx=state.center[0]-point[0],dy=state.center[1]-point[1];
  if(!(state.radius*state.radius>dx*dx+dy*dy))return false;
  unsigned intersections=0;
  for(unsigned i=0;i<32;++i) {
    const auto& a=state.polygon[(i+31)%32];const auto& b=state.polygon[i];
    const double original_determinant=(double(b[1])-double(a[1]))*-100.0;
    if(!(std::abs(original_determinant)>double(0.0001f)))continue;
    const float determinant=float(original_determinant);
    const float y=a[1]-point[1],inverse=1.0f/determinant;
    const float segment=(y*100.0f)*inverse;
    const float ray=((b[0]-a[0])*y-(b[1]-a[1])*(a[0]-point[0]))*inverse;
    if(segment>=0 && segment<1 && ray>=0 && ray<1)++intersections;
  }
  return (intersections&1)!=0;
}
void source_king_update_player(const SourceKingState& state,SourceKingPlayer& player,
    bool over,std::uint8_t mode,const SourceKingPosition& position) {
  if(player.current_unit!=0xffffffffu && !over && mode!=4 && source_king_contains(state,player,position))
    player.inside_ticks=narrow(std::uint32_t(std::int32_t(player.inside_ticks))+1u);
  else player.inside_ticks=0;
}
void source_king_score(SourceKingState& state,const SourceKingContext& context,
    std::span<SourceKingPlayer> players,const SourceKingScoringServices& services) {
  std::uint32_t player_mask=0,team_mask=0;
  for(const auto& player:players)if(player.current_unit!=0xffffffffu && player.inside_ticks!=0) {
    player_mask|=1u<<(std::uint8_t(player.datum)&31);
    if(context.engine_present && context.team_mode && player.membership!=-1)
      team_mask|=1u<<(std::uint8_t(player.membership)&31);
  }
  if(context.flags&1) {
    const auto mask=context.engine_present && context.team_mode ? team_mask : player_mask;
    if(!mask || (mask&(mask-1)))return;
  }
  if(!context.engine_present || context.lifecycle!=1 || (context.raw_mode!=4 && context.status!=1))return;
  const auto deliver=[&](SourceKingPlayer& player,bool record) {
    if(!services.score)throw std::runtime_error("Original King score getter is unavailable");
    const auto previous=services.score(player.datum);
    if(record) {
      if(!services.record)throw std::runtime_error("Original King recording-event owner is unavailable");
      services.record(player.datum,4,0xffffffffu);
    }
    if(!services.award)throw std::runtime_error("Original King shared score award owner is unavailable");
    services.award(player.datum,1);
    const auto next=services.score(player.datum);
    if(previous!=next) {
      if(!services.score_changed)throw std::runtime_error("Original King score-change publication owner is unavailable");
      services.score_changed(player.datum,next);
    }
  };
  const auto divides=[&](std::int32_t value) {
    if(context.tick_rate==0 || (value==(-2147483647-1) && context.tick_rate==-1))
      throw std::runtime_error("Original King signed tick division is unavailable");
    return value%context.tick_rate==0;
  };
  if(context.team_mode) {
    for(unsigned team=0;team<8;++team)
      state.team_ticks[team]=(team_mask&(1u<<team)) ?
          std::bit_cast<std::int32_t>(std::uint32_t(state.team_ticks[team])+1u) : 0;
    for(int team=0;team<8;++team)if((team_mask&(1u<<team)) && divides(state.team_ticks[team])) {
      SourceKingPlayer* last=nullptr;
      for(auto& player:players)if(player.current_unit!=0xffffffffu && player.membership==team && player.inside_ticks!=0) {
        bool selected=true;
        if(!(context.flags&2))for(const auto& other:players)
          if(other.current_unit!=0xffffffffu && other.datum!=player.datum && other.membership==player.membership && other.inside_ticks!=0 &&
              (other.inside_ticks>player.inside_ticks ||
               (other.inside_ticks==player.inside_ticks && std::uint8_t(other.datum)>std::uint8_t(player.datum))))selected=false;
        if(selected){deliver(player,false);last=&player;}
      }
      if(last)last->inside_ticks=1;
    }
  } else {
    for(auto& player:players)if(player.current_unit!=0xffffffffu && player.inside_ticks!=0 && divides(player.inside_ticks))
      deliver(player,true);
  }
}
void source_king_world_update(SourceKingState& state,const SourceKingHills& hills,const SourceKingContext& context,
    std::span<const SourceKingMarker> markers,std::span<SourceKingPlayer> players,std::uint32_t& seed,
    const SourceKingWorldServices& services) {
  if(context.game_over || context.raw_mode==4)return;
  const auto notify=[&](const SourceKingNotice& event) {
    if(!services.notification)throw std::runtime_error("Original King engine-event owner is unavailable");
    services.notification(event);
  };
  const auto dirty=[&](std::uint32_t flags) {
    if(context.engine_present && context.match_replication_entity!=0xffffffffu) {
      if(!services.dirty_entity)throw std::runtime_error("Original King match-entity publication owner is unavailable");
      services.dirty_entity(context.match_replication_entity,flags);
    }
  };
  std::uint16_t occupants=0;
  for(const auto& player:players)if(player.current_unit!=0xffffffffu && player.inside_ticks!=0)
    occupants|=std::uint16_t(1u<<(std::uint8_t(player.datum)&31));
  if(occupants!=state.occupants) {
    const auto team_mask=[&](std::uint16_t mask) {
      std::uint32_t teams=0;
      for(const auto& player:players)if((mask&(1u<<(std::uint8_t(player.datum)&31))) && player.membership!=-1)
        teams|=1u<<(std::uint8_t(player.membership)&31);
      return std::uint16_t(teams);
    };
    const auto previous=team_mask(state.occupants),next=team_mask(occupants);
    if(previous!=next && next!=0) {
      const bool single=(next&(next-1))==0;
      const bool contested=!single && previous!=0 && (previous&(previous-1))==0 && std::uint16_t(next^previous)!=0;
      if(single || contested) {
        SourceKingNotice event;event.type=(context.engine_present && context.team_mode ? 4 : 0)+(single ? 1 : 2);
        const auto selection=single ? next : std::uint16_t(next^previous);
        for(const auto& player:players)
          if(player.current_unit!=0xffffffffu && player.inside_ticks!=0 && player.membership!=-1 &&
              (selection&(1u<<(std::uint8_t(player.membership)&31)))) {
            event.player=player.datum;event.membership=player.membership;break;
          }
        notify(event);
      }
    }
    state.occupants=occupants;dirty(0x40);
  }
  source_king_score(state,context,players,services.scoring);
  if(context.move_seconds!=0) {
    state.move_ticks=std::bit_cast<std::int32_t>(std::uint32_t(state.move_ticks)-1u);
    if(state.move_ticks<=0) {
      state.move_ticks=std::int32_t(context.move_seconds)*std::int32_t(context.tick_rate);
      auto selected=source_king_select_hill(hills,state.hill,seed);
      while(selected!=-1) {
        source_king_build_hill(state,markers,selected);
        if(state.marker_count>3 && !(state.marker_count&1))break;
        selected=source_king_select_hill(hills,state.hill,seed);
      }
      if(selected!=-1 && state.hill!=selected) {
        notify(SourceKingNotice{4});state.hill=selected;
      }
      dirty(0x20);
    }
  }
}
} // namespace halo2

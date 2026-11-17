// Native TU1.5 campaign AI script builtins over the live actor/squad/group
// banks: the 27CAA0/27CB30 reference iterator, its 20D070/20D0D0 group and
// 20CFF0/20D030 squad-member walks, the 27C880 actor resolver, and the
// evaluator owners built on them. No original instruction is executed.
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "native_campaign_script_fp_tu15.hpp"
#include <algorithm>
#include <bit>
#include <memory>
#include <vector>

namespace halo2::native_campaign_script_tu15 {
namespace {
using namespace native_campaign_script_fp;
using Bank=NativeCampaignScriptBank;
constexpr std::uint32_t ai_globals=0x5020a8u,actor_pool=0x5020c8u,squad_pool=0x52be7cu,group_pool=0x52be80u;
constexpr std::uint32_t actor_stride=0x888u,squad_stride=0x98u,group_stride=0x38u;
std::uint32_t sign16(std::uint16_t value){return std::uint32_t(std::int32_t(std::int16_t(value)));}
bool ai_enabled(const Bank& bank){return bank.u8(bank.u32(ai_globals)+1u)!=0;}
std::uint32_t squad_row(const Bank& bank,std::uint32_t index) {
  return bank.u32(bank.u32(squad_pool)+0x44u)+(index&0xffffu)*squad_stride;
}
std::uint32_t actor_row(const Bank& bank,std::uint32_t index) {
  return bank.u32(bank.u32(actor_pool)+0x44u)+(index&0xffffu)*actor_stride;
}
std::uint32_t object_row_body(const Bank& bank,std::uint32_t object) {
  return bank.u32(bank.u32(bank.u32(0x4ecbd0u)+0x44u)+(object&0xffffu)*12u+8u);
}

// 20D070/20D0D0: post-order walk of an encounter-group subtree, yielding the
// squads chained from each group's +8 through squad +0x74.
struct GroupWalk {
  std::uint32_t current{},root{},squad{none},next{none},previous{none};
  void descend(const Bank& bank) {
    for(auto child=bank.u32(current+4u);child!=none;child=bank.u32(current+4u))
      current=bank.u32(bank.u32(group_pool)+0x44u)+(child&0xffffu)*group_stride;
  }
  void start(const Bank& bank,std::uint32_t index) {
    current=root=bank.u32(bank.u32(group_pool)+0x44u)+(index&0xffffu)*group_stride;
    previous=none;squad=none;
    descend(bank);
    next=bank.u32(current+8u);
  }
  std::uint32_t advance(const Bank& bank) {
    if(!current)return 0;
    while(next==none) {
      if(current==root){current=0;return 0;}
      const auto sibling=bank.u32(current+0xcu);
      if(sibling==none)
        current=bank.u32(bank.u32(group_pool)+0x44u)+(bank.u32(current+0x10u)&0xffffu)*group_stride;
      else {
        current=bank.u32(bank.u32(group_pool)+0x44u)+(sibling&0xffffu)*group_stride;
        descend(bank);
      }
      next=bank.u32(current+8u);
      if(!current)return 0;
    }
    const auto index=next;
    const auto row=squad_row(bank,index);
    previous=squad;squad=index;
    next=sign16(bank.u16(row+0x74u));
    return row;
  }
};

// 20CFF0/20D030: squad members through actor +0x20 (squad NONE walks the
// global unassigned chain at AI globals +0x14). Disabled AI leaves the walk
// unstarted and yields nothing.
struct MemberWalk {
  std::uint32_t squad{none},actor{none},next{none};
  void start(const Bank& bank,std::uint32_t index) {
    if(!ai_enabled(bank))return;
    squad=index;actor=none;
    next=index==none?bank.u32(bank.u32(ai_globals)+0x14u):bank.u32(squad_row(bank,index)+0x68u);
  }
  std::uint32_t advance(const Bank& bank) {
    if(!ai_enabled(bank) || next==none)return 0;
    const auto row=actor_row(bank,next);
    actor=next;next=bank.u32(row+0x20u);
    return row;
  }
};

// 27CAA0/27CB30: actors of a squad (tag 0), encounter group (tag 1), or one
// resolved actor/starting location (tags 2/3).
class ActorWalk {
public:
  ActorWalk(const Bank& bank,std::uint32_t reference):bank_(bank) {
    const auto tag=(reference>>30)&3u;
    if(tag==1u) {
      group_=reference&0xffffu;
      groups_.start(bank,group_);
      groups_.advance(bank);
      squad_=groups_.squad;
      members_.start(bank,squad_);
      actor_=none;
    } else if(tag==0u) {
      group_=none;squad_=reference&0xffffu;actor_=none;
      members_.start(bank,squad_);
    } else {
      squad_=group_=none;
      actor_=ai_actor_index(bank,reference);
      single_=true;
    }
  }
  std::uint32_t next() {
    if(squad_!=none) {
      while(true) {
        const auto row=members_.advance(bank_);
        actor_=members_.actor;
        if(row)return row;
        actor_=none;
        if(group_==none)return 0;
        const auto squad=groups_.advance(bank_);
        squad_=groups_.squad;
        if(!squad)return 0;
        members_.start(bank_,squad_);
        if(squad_==none)break;
      }
    }
    if(!single_)return 0;
    single_=false;
    return actor_==none?0:actor_row(bank_,actor_);
  }
  std::uint32_t datum()const{return actor_;}
private:
  const Bank& bank_;
  std::uint32_t squad_{none},group_{none},actor_{none};
  bool single_{};
  MemberWalk members_;
  GroupWalk groups_;
};

// 27CA60 and its callers' loop: each squad of a squad/group reference.
template<class Visit> void for_each_squad(const Bank& bank,std::uint32_t reference,bool require_ai,Visit visit) {
  const auto tag=reference>>30;
  GroupWalk groups;
  auto group=none,single=none;
  if(tag==1u){group=reference&0xffffu;groups.start(bank,group);}
  else if(tag==0u)single=reference&0xffffu;
  while(true) {
    std::uint32_t row,index;
    if(group==none) {
      if(single==none)return;
      index=single;single=none;row=squad_row(bank,index);
    } else {
      row=groups.advance(bank);index=groups.squad;
    }
    if(!row)return;
    if(require_ai && !ai_enabled(bank))continue;
    visit(row,index);
  }
}
}

std::uint32_t ai_actor_index(const Bank& bank,std::uint32_t reference) {
  const auto tag=reference>>30;
  if(tag==2u) {
    const auto index=reference&0xffffu;
    const auto pool=bank.u32(actor_pool);
    return (std::uint32_t(bank.u16(bank.u32(pool+0x44u)+bank.u32(pool+0x24u)*index))<<16)|index;
  }
  if(tag!=3u)return none;
  const auto squad=(reference>>16)&0x3fffu;
  const auto scenario=bank.u32(0x4ecc20u);
  if(std::int32_t(squad)>=std::int32_t(bank.u32(scenario+0x160u)))return none;
  const auto definition=bank.u32(scenario+0x164u)+squad*0x74u;
  const auto location=std::int32_t(std::int16_t(reference));
  if(location<0 || location>=std::int32_t(bank.u32(definition+0x48u)))return none;
  const auto start=bank.u32(definition+0x4cu)+std::uint32_t(location)*0x64u;
  MemberWalk members;
  members.start(bank,squad);
  for(auto row=members.advance(bank);row;row=members.advance(bank))
    if(bank.u32(row+0x38u)==bank.u32(start))return members.actor;
  return none;
}

std::uint32_t ai_vehicle_get_from_starting_location(const Bank& bank,std::uint32_t reference) {
  if((reference&0xc0000000u)!=0xc0000000u)return none;
  const auto squad=(reference>>16)&0x3fffu;
  const auto scenario=bank.u32(0x4ecc20u);
  if(std::int32_t(squad)>=std::int32_t(bank.u32(scenario+0x160u)))return none;
  const auto definition=bank.u32(scenario+0x164u)+squad*0x74u;
  const auto location=std::int32_t(std::int16_t(reference));
  if(location<0 || location>=std::int32_t(bank.u32(definition+0x48u)))return none;
  const auto start=bank.u32(definition+0x4cu)+std::uint32_t(location)*0x64u;
  auto vehicle=bank.u32(squad_row(bank,squad)+0x70u);
  if(vehicle==none)return none;
  const auto name=bank.u32(start);
  do {
    const auto body=object_row_body(bank,vehicle);
    if(bank.u32(body+0x3a8u)==name)return vehicle;
    vehicle=bank.u32(body+0x3a4u);
  } while(vehicle!=none);
  return none;
}

std::uint32_t ai_vehicle_get(const Bank& bank,std::uint32_t reference) {
  const auto tag=reference>>30;
  if(tag!=2u && tag!=3u)return none;
  const auto actor=ai_actor_index(bank,reference);
  if(actor==none)return none;
  const auto pool=bank.u32(actor_pool);
  const auto index=actor&0xffffu;
  if(std::int32_t(index)>=std::int32_t(bank.u32(pool+0x38u)))return none;
  const auto row=bank.u32(pool+0x44u)+bank.u32(pool+0x24u)*index;
  const auto salt=bank.u16(row);
  if(!salt || std::int16_t(salt)!=std::int16_t(actor>>16))return none;
  const auto parent=bank.u32(object_row_body(bank,bank.u32(row+0x18u))+0x14u);
  if(parent==none)return none;
  return bank.u8(object_row_body(bank,parent)+0xaau)==1u?parent:none;
}

std::uint32_t object_get_ai(const Bank& bank,std::uint32_t object) {
  constexpr std::uint32_t no_ai=0xc3e703e7u;
  if(object==none)return no_ai;
  const auto body=object_row_body(bank,object);
  if(!((1u<<(bank.u8(body+0xaau)&31u))&3u))return no_ai;
  const auto actor=bank.u32(body+0x12cu);
  return actor==none?no_ai:((actor&0xffffu)|0x80000000u);
}

std::uint16_t ai_spawn_count(const Bank& bank,std::uint32_t reference) {
  std::uint16_t count=0;
  for_each_squad(bank,reference,false,[&](std::uint32_t row,std::uint32_t){count=std::uint16_t(count+bank.u16(row+0x24u));});
  return count;
}

std::uint16_t ai_combat_status(const Bank& bank,std::uint32_t reference) {
  std::int16_t status=0;
  ActorWalk actors(bank,reference);
  for(auto row=actors.next();row;row=actors.next()) {
    const auto value=std::int16_t(bank.u16(row+0x86u));
    if(value>status)status=value;
  }
  return std::uint16_t(status);
}

void ai_disposable(Bank& bank,std::uint32_t reference,std::uint8_t disposable) {
  if(reference==none)return;
  ActorWalk actors(bank,reference);
  for(auto row=actors.next();row;row=actors.next()) {
    bank.store8(row+0xcu,disposable);
    if(!disposable)continue;
    const auto prop=bank.u32(row+0x7cu);
    if(prop!=none)bank.store8(bank.u32(bank.u32(0x50eef8u)+0x44u)+(prop&0xffffu)*0x50u+0x3cu,1);
  }
}

void ai_set_blind(Bank& bank,std::uint32_t reference,std::uint8_t blind) {
  if(reference==none)return;
  if((reference>>30)>=2u) {
    const auto actor=ai_actor_index(bank,reference);
    if(actor!=none)bank.store8(actor_row(bank,actor)+0x228u,blind);
    return;
  }
  for_each_squad(bank,reference,true,[&](std::uint32_t,std::uint32_t index) {
    const auto flags=squad_row(bank,index)+2u;
    bank.store8(flags,std::uint8_t(blind?bank.u8(flags)|1u:bank.u8(flags)&0xfeu));
  });
}

void ai_set_deaf(Bank& bank,std::uint32_t reference,std::uint8_t deaf) {
  if(reference==none)return;
  for_each_squad(bank,reference,true,[&](std::uint32_t,std::uint32_t index) {
    const auto flags=squad_row(bank,index)+2u;
    bank.store8(flags,std::uint8_t(deaf?bank.u8(flags)|2u:bank.u8(flags)&0xfdu));
  });
}

void ai_suppress_combat(Bank& bank,std::uint32_t reference,std::uint8_t suppress) {
  ActorWalk actors(bank,reference);
  for(auto row=actors.next();row;row=actors.next())bank.store8(row+0x223u,suppress);
}

// 27EE70 (disregard, bit 0x80) and 27EF80 (prefer target, bit 0x100).
void ai_target_preference(Bank& bank,std::span<const std::uint32_t> objects,std::uint8_t enable,bool prefer) {
  const std::uint32_t bit=prefer?0x100u:0x80u;
  const auto pool=bank.u32(0x4ecbd0u);
  for(const auto object:objects) {
    if(object==none)break;
    const auto index=object&0xffffu;
    if(std::int32_t(index)>=std::int32_t(bank.u32(pool+0x38u)))continue;
    const auto row=bank.u32(pool+0x44u)+bank.u32(pool+0x24u)*index;
    const auto salt=bank.u16(row);
    if(!salt || std::int16_t(salt)!=std::int16_t(object>>16))continue;
    if(!((1u<<(bank.u8(row+3u)&31u))&3u))continue;
    const auto body=bank.u32(row+8u);
    if(!body)continue;
    const auto flags=bank.u32(body+0x134u);
    bank.store32(body+0x134u,enable?flags|bit:flags&~bit);
  }
}

// 27DA40: for each member of a squad (tag 0 only), walk its prop's knowledge
// chain (0x50EEF8 +14 -> 0x50EEF4 0xC4 rows via +14) for records naming the
// object, then every perception entry (0x50EEF0 0x3C rows via +34) held by a
// squad-mate: +1C = 3, +1E = FISTP(CVTSI2SS(ticks/s) * 5.0). TU1.5 keeps the
// visited-prop mask in one 32-bit stack word, so prop indices >= 32 alias its
// frame temporaries; native keeps the intended full mask (identical below 32).
void ai_magically_see_object(Bank& bank,std::uint32_t reference,std::uint32_t object) {
  if(reference&0xc0000000u)return;
  const auto squad=reference&0xffffu;
  const auto& fp=bank.floating_point();
  std::vector<std::uint32_t> visited(2048);
  MemberWalk members;
  members.start(bank,squad);
  while(ai_enabled(bank) && members.next!=none) {
    const auto actor=actor_row(bank,members.next);
    members.next=bank.u32(actor+0x20u);
    const auto prop=bank.u32(actor+0x7cu);
    if(prop==none)continue;
    const auto index=prop&0xffffu;
    if(visited[index>>5]&(1u<<(index&31u)))continue;
    visited[index>>5]|=1u<<(index&31u);
    auto knowledge=bank.u32(bank.u32(bank.u32(0x50eef8u)+0x44u)+index*0x50u+0x14u);
    while(knowledge!=none) {
      const auto row=bank.u32(bank.u32(0x50eef4u)+0x44u)+(knowledge&0xffffu)*0xc4u;
      if(bank.u32(row+8u)==object) {
        for(auto entry=bank.u32(row+0x18u);entry!=none;) {
          const auto perception=bank.u32(bank.u32(0x50eef0u)+0x44u)+(entry&0xffffu)*0x3cu;
          if(bank.u32(actor_row(bank,bank.u32(perception+4u))+0x30u)==squad) {
            bank.store16(perception+0x1cu,3);
            const auto ticks=cvtsi2ss(std::int16_t(bank.u16(bank.u32(0x51df54u)+2u)),fp.mxcsr);
            bank.store16(perception+0x1eu,std::uint16_t(fistp32(mulss(ticks,5.f,fp.mxcsr),fp.x87_control_word)));
          }
          entry=bank.u32(perception+0x34u);
        }
      }
      knowledge=bank.u32(row+0x14u);
    }
  }
}

// 262590..26264B: before an actor's command-script step the runner publishes
// ai_current_actor (0x50EF00 = index | 0x80000000) and ai_current_squad
// (0x50EF04 = actor +30 index, or 0xC3E703E7); afterwards only the actor
// global is cleared. The native VM names command threads by unit object, so
// the actor is that unit's +12C (absent: the actor global stays cleared).
void ai_command_context(Bank& bank,std::optional<std::uint32_t> unit) {
  if(!unit){bank.store32(0x50ef00u,none);return;}
  const auto reference=object_get_ai(bank,*unit);
  if((reference>>30)!=2u){bank.store32(0x50ef00u,none);return;}
  const auto index=reference&0xffffu;
  bank.store32(0x50ef00u,index|0x80000000u);
  const auto squad=bank.u32(actor_row(bank,index)+0x30u);
  bank.store32(0x50ef04u,squad==none?0xc3e703e7u:(squad&0xffffu));
}

// External globals with original backing (xbox index -> address/type):
// 797/798 the command context above (world bank). 805..821 are the AI
// enumeration shorts in TU1.5 .data at 47C244 (combat status 0..9, movement
// 0..6); the retained world image does not map that block, so they carry the
// authenticated XBE values as constants.
std::optional<NativeCampaignBackedGlobal> backed_global(std::uint32_t index) {
  if(index==797u)return NativeCampaignBackedGlobal{0x50ef04u,19,{}};
  if(index==798u)return NativeCampaignBackedGlobal{0x50ef00u,19,{}};
  static constexpr std::array<std::int16_t,17> enumeration{0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6};
  if(index>=805u && index<=821u)
    return NativeCampaignBackedGlobal{0x47c244u+2u*(index-805u),7,std::uint16_t(enumeration[index-805u])};
  return {};
}

// 27CFF0: every walked actor's unit (or each member of its swarm chain) is
// pinned: model damage-info flag byte bit 7 (or object +10A bit 14) and
// 0.0002 >= vitality +EC by COMISS (DAZ-aware, unordered fails).
bool ai_vitality_pinned(const Bank& bank,std::uint32_t reference) {
  const auto mxcsr=bank.floating_point().mxcsr;
  const auto pinned=[&](std::uint32_t body) {
    const auto tags=bank.u32(0x4f0414u);
    const auto definition=bank.u32(tags+(bank.u32(body)&0xffffu)*16u+8u);
    const auto model=bank.u32(definition+0x38u);
    std::uint32_t info=0;
    if(model!=none) {
      const auto row=bank.u32(tags+(model&0xffffu)*16u+8u);
      if(std::int32_t(bank.u32(row+0x60u))>0)info=bank.u32(row+0x64u);
    }
    if(!((bank.u16(body+0x10au)>>14)&1u) && (!info || !(bank.u8(info)&0x80u)))return false;
    return std::bit_cast<float>(0x3951b717u)>=sse_compared(bank.u32(body+0xecu),mxcsr);
  };
  ActorWalk actors(bank,reference);
  for(auto row=actors.next();row;row=actors.next()) {
    const auto unit=bank.u32(row+0x18u);
    if(unit!=none) {
      if(!pinned(object_row_body(bank,unit)))return false;
      continue;
    }
    const auto swarm=bank.u32(row+0x1cu);
    if(swarm==none)continue;
    auto object=bank.u32(bank.u32(bank.u32(0x510fa8u)+0x44u)+(swarm&0xffffu)*0x34u+0x18u);
    while(object!=none) {
      const auto body=object_row_body(bank,object);
      auto next=none;
      if(!bank.u32(body+0x134u)) {
        const auto extension=body+std::uint32_t(std::int32_t(std::int16_t(bank.u16(body+0x13au))));
        if(extension)next=bank.u32(extension+0xcu);
      }
      if(!pinned(body))return false;
      object=next;
    }
  }
  return true;
}

// 1ED690: the character tag's (and its parents', via +8) 0x3C equipment row
// whose +4 word names the unit's current type; 0 when absent.
std::uint32_t character_equipment_row(const Bank& bank,std::uint32_t character,std::uint16_t type) {
  for(auto tag=character;tag!=none;) {
    const auto definition=bank.u32(bank.u32(0x4f0414u)+(tag&0xffffu)*16u+8u);
    const auto count=std::int32_t(bank.u32(definition+0xdcu));
    for(std::int16_t n=0;n<count;++n) {
      const auto row=bank.u32(definition+0xe0u)+std::uint32_t(n)*0x3cu;
      if(bank.u16(row+4u)==type)return row;
    }
    tag=bank.u32(definition+8u);
  }
  return 0;
}

// 27F5C0: each walked actor's unit gets vitality flags +EC/+F0 = (+E4/+E8 > 0)
// and, when its character names the current type (+23C), a primary-RNG count
// in [+34, +36] that only ever raises +23E[type]; +23D/+23C take that type.
void ai_renew(Bank& bank,std::uint32_t reference) {
  const auto mxcsr=bank.floating_point().mxcsr;
  ActorWalk actors(bank,reference);
  for(auto row=actors.next();row;row=actors.next()) {
    const auto unit=bank.u32(row+0x18u);
    if(unit==none)continue;
    std::uint32_t equipment=0;
    const auto current=std::int8_t(bank.u8(object_row_body(bank,unit)+0x23cu));
    if(current!=-1)equipment=character_equipment_row(bank,bank.u32(row+0x54u),std::uint16_t(std::int16_t(current)));
    const auto body=object_row_body(bank,unit);
    bank.store32(body+0xecu,sse_compared(bank.u32(body+0xe4u),mxcsr)>0.f?0x3f800000u:0u);
    bank.store32(body+0xf0u,sse_compared(bank.u32(body+0xe8u),mxcsr)>0.f?0x3f800000u:0u);
    if(!equipment)continue;
    const auto seed=bank.u32(0x4f3cd8u);
    const auto state=bank.u32(seed)*0x19660du+0x3c6ef35fu;
    bank.store32(seed,state);
    const auto low=std::int16_t(bank.u16(equipment+0x34u)),high=std::int16_t(bank.u16(equipment+0x36u));
    const auto span=std::int32_t(std::int16_t(std::uint16_t(high+1)))-std::int32_t(low);
    // 27F6C4 IMUL keeps the low 32 bits; SHR then adds the low word of +34.
    const auto product=std::uint32_t(span)*(state>>16u);
    const auto count=std::int16_t(std::uint16_t((product>>16u)+std::uint32_t(std::uint16_t(low))));
    const auto type_now=std::int8_t(bank.u8(body+0x23cu));
    const std::int16_t held=type_now==-1?0:std::int8_t(bank.u8(body+0x23eu+std::uint32_t(std::int32_t(type_now))));
    if(held>=count)continue;
    const auto type=std::int16_t(bank.u16(equipment+4u));
    const auto slot=body+0x23eu+std::uint32_t(std::int32_t(type));
    bank.store8(slot,std::uint8_t(bank.u8(slot)+std::uint8_t(std::uint8_t(count)-std::uint8_t(held))));
    bank.store8(body+0x23du,std::uint8_t(type));
    bank.store8(body+0x23cu,std::uint8_t(type));
  }
}

// 27EAB0's iterator (27CAA0 then 27CB30 per actor) for the factory tracks.
std::function<std::uint32_t()> ai_reference_actors(const Bank& bank,std::uint32_t reference) {
  auto walk=std::make_shared<ActorWalk>(bank,reference);
  return [walk]{return walk->next();};
}
std::function<AiReferenceActor()> ai_reference_actor_datums(const Bank& bank,std::uint32_t reference) {
  auto walk=std::make_shared<ActorWalk>(bank,reference);
  return [walk]{const auto row=walk->next();return AiReferenceActor{row,walk->datum()};};
}

namespace {
// BCEE0 with flags 0: live rows whose type bit is in the mask; a row with a
// null body ends the walk exactly like the original's zero return.
struct ObjectIterator {
  std::uint32_t mask{};std::int16_t index{};std::uint32_t handle{none};
  std::uint32_t next(const Bank& bank) {
    const auto pool=bank.u32(0x4ecbd0u);
    auto address=bank.u32(pool+0x44u)+std::uint32_t(std::int32_t(index))*12u;
    for(auto current=std::int32_t(index);current<std::int32_t(bank.u32(pool+0x38u));address+=12u) {
      const auto salt=std::int16_t(bank.u16(address));
      const auto candidate=(std::uint32_t(std::int32_t(salt))<<16u)|std::uint32_t(current);
      index=std::int16_t(index+1);
      if(salt && (mask&(1u<<(bank.u8(address+3u)&31u)))) {
        handle=candidate;
        return bank.u32(address+8u);
      }
      current=std::int32_t(index);
    }
    return 0;
  }
};
void collect_seats(const Bank& bank,std::uint32_t object,std::vector<VehicleSeat>& seats,std::int16_t maximum) {
  const auto rows=bank.u32(bank.u32(0x4ecbd0u)+0x44u);
  const auto record=rows+(object&0xffffu)*12u;
  const auto body=bank.u32(record+8u);
  if(std::int16_t(seats.size())>=maximum)return;
  if(((1u<<(bank.u8(record+3u)&31u))&3u) && !((bank.u8(body+0x10au)>>2)&1u)) {
    const auto definition=bank.u32(bank.u32(0x4f0414u)+(bank.u32(body)&0xffffu)*16u+8u);
    for(std::int32_t seat=0;seat<std::int32_t(bank.u32(definition+0x1c8u));++seat) {
      if(std::int16_t(seats.size())>=maximum)return;
      seats.push_back({object,std::uint16_t(seat),bank.u32(definition+0x1ccu)+std::uint32_t(seat)*0xb0u});
    }
  }
  if(std::int16_t(seats.size())>=maximum)return;
  for(auto child=bank.u32(body+0x10u);child!=none;) {
    const auto child_body=bank.u32(bank.u32(bank.u32(0x4ecbd0u)+0x44u)+(child&0xffffu)*12u+8u);
    if(((1u<<(bank.u8(child_body+0xaau)&31u))&3u) && ((bank.u32(child_body+4u)>>26u)&1u))
      collect_seats(bank,child,seats,maximum);
    child=bank.u32(child_body+0xcu);
  }
}
}

// CAF20: seats of a unit and, recursively, of its seat-bearing unit children
// (object +4 bit 26), at most 0x40 entries in original order.
std::vector<VehicleSeat> vehicle_seats(const Bank& bank,std::uint32_t object) {
  std::vector<VehicleSeat> seats;
  collect_seats(bank,object,seats,0x40);
  return seats;
}

// 11D240: the unit sits in a seat (optionally named) of the vehicle tree.
bool vehicle_test_seat(const Bank& bank,std::uint32_t vehicle,std::uint32_t seat_name,std::uint32_t unit) {
  if(vehicle==none || unit==none)return false;
  const auto body=object_row_body(bank,unit);
  if(!object_try_and_get_and_verify_type(bank,vehicle,3u))return false;
  for(const auto& seat:vehicle_seats(bank,vehicle)) {
    if(seat_name!=none && bank.u32(seat.row+4u)!=seat_name)continue;
    if(bank.u32(body+0x14u)==seat.object && bank.u16(body+0x1fcu)==seat.seat)return true;
  }
  return false;
}

// 11D310: some list member is the first unit (BCEE0 order) occupying a
// matching seat; every seat entry is examined.
bool vehicle_test_seat_list(const Bank& bank,std::uint32_t vehicle,std::uint32_t seat_name,
    std::span<const std::uint32_t> members) {
  if(vehicle==none)return false;
  bool found=false;
  for(const auto& seat:vehicle_seats(bank,vehicle)) {
    if(seat_name!=none && bank.u32(seat.row+4u)!=seat_name)continue;
    ObjectIterator units{3u};
    std::uint32_t occupant=none;
    for(auto body=units.next(bank);body;body=units.next(bank))
      if(bank.u32(body+0x14u)==seat.object && std::int16_t(bank.u16(body+0x1fcu))==std::int16_t(seat.seat)) {
        occupant=units.handle;break;
      }
    if(occupant==none)continue;
    for(const auto member:members) {
      if(member==none)break;
      if(member==occupant){found=true;break;}
    }
  }
  return found;
}

// 27FCD0: reserve (+3B0 = -1) or release (0) every vehicle in the seat tree.
bool ai_vehicle_reserve(Bank& bank,std::uint32_t vehicle,std::uint8_t reserve) {
  if(vehicle==none)return false;
  auto previous=none;
  for(const auto& seat:vehicle_seats(bank,vehicle)) {
    if(seat.object==previous)continue;
    const auto record=bank.u32(bank.u32(0x4ecbd0u)+0x44u)+(seat.object&0xffffu)*12u;
    if(bank.u8(record+3u)==1u)bank.store32(bank.u32(record+8u)+0x3b0u,reserve?none:0u);
    previous=seat.object;
  }
  return true;
}

// 27FBC0: toggle the first named vehicle seat's bit in +3B0.
bool ai_vehicle_reserve_seat(Bank& bank,std::uint32_t vehicle,std::uint32_t seat_name,std::uint8_t reserve) {
  if(vehicle==none)return false;
  for(const auto& seat:vehicle_seats(bank,vehicle)) {
    if(bank.u32(seat.row+4u)!=seat_name)continue;
    const auto record=bank.u32(bank.u32(0x4ecbd0u)+0x44u)+(seat.object&0xffffu)*12u;
    if(bank.u8(record+3u)!=1u)continue;
    const auto body=bank.u32(record+8u);
    const auto bit=1u<<(seat.seat&31u);
    const auto flags=bank.u32(body+0x3b0u);
    bank.store32(body+0x3b0u,reserve?flags|bit:flags&~bit);
    return true;
  }
  return false;
}

// 14E7B0: normalize a new player's starting-profile record (campaign kind 1
// takes the first scenario profile representation; kind 2 maps 1/3 -> 3,
// else 2), then clamp representation, appearance, team/colour and character
// bytes to their original ranges and drop an unavailable representation.
void player_profile_normalize(Bank& bank,std::uint32_t record) {
  const auto options=bank.u32(0x4f3218u);
  const auto kind=bank.u32(options+8u);
  const auto scenario=bank.u32(0x4ecc20u),globals=bank.u32(0x4ecc1cu);
  const auto signed8=[&](std::uint32_t at){return std::int32_t(std::int8_t(bank.u8(at)));};
  if(kind==1u) {
    std::uint8_t representation=0;
    const auto count=std::int32_t(bank.u32(scenario+0x100u));
    auto row=bank.u32(scenario+0x104u);
    for(std::int32_t n=0;n<count;++n,row+=0x34u)
      if(bank.u16(row+0x2cu)!=0xffffu){representation=std::uint8_t(bank.u16(row+0x2cu));break;}
    bank.store8(record+0x7cu,1);
    bank.store8(record+0x44u,representation);
  } else if(kind==2u) {
    const auto value=signed8(record+0x44u);
    bank.store8(record+0x44u,value==1 || value==3?3:2);
  }
  if(bank.u8(record+0x44u)!=0xffu) {
    auto value=signed8(record+0x44u);
    if(value<0)value=0;
    else if(const auto last=std::int32_t(bank.u32(globals+0x138u))-1;value>last)value=last;
    bank.store8(record+0x44u,std::uint8_t(value));
  }
  for(const auto at:{record+0x7eu,record+0x7fu})
    if(bank.u8(at)!=0xffu)bank.store8(at,std::uint8_t(std::max(signed8(at),0)));
  const auto team=std::int32_t(bank.u32(record+0x84u));
  if(team>=0 && team<0x10) {
    const auto colour=std::int32_t(bank.u32(record+0x8cu));
    bank.store32(record+0x8cu,std::uint32_t(std::clamp(colour,0,0x3fffffff)));
    for(const auto at:{record+0x88u,record+0x8au})
      bank.store16(at,std::uint16_t(std::clamp<std::int32_t>(std::int16_t(bank.u16(at)),0,0x7f)));
  } else {
    bank.store32(record+0x84u,none);
    bank.store32(record+0x8cu,none);
    bank.store16(record+0x8au,0xffffu);
    bank.store16(record+0x88u,0xffffu);
  }
  bank.store8(record+0x7du,std::uint8_t(std::clamp(signed8(record+0x7du),0,3)));
  if(const auto value=signed8(record+0x81u);std::clamp(value,0,7)!=value)bank.store8(record+0x81u,0);
  const auto user=bank.u32(0x4f63b8u);
  if(!bank.u32(0x56dfa0u+bank.u32(user+0xc14u)*4u))return;
  if(!(bank.u8(options+0x184u)&1u))return;
  const auto representation=bank.u8(record+0x7cu);
  if(representation==0xffu)return;
  if(!(bank.u16(user+0xau)&(1u<<(representation&31u))))bank.store8(record+0x7cu,0xff);
}

namespace {
std::uint32_t object_rows(const Bank& bank){return bank.u32(bank.u32(0x4ecbd0u)+0x44u);}
// B9390: mark the object and its parents for update unless already marked.
void mark_for_update(Bank& bank,std::uint32_t object) {
  if(bank.u8(object_rows(bank)+(object&0xffffu)*12u+2u)&2u)return;
  for(auto o=object;o!=none;) {
    const auto at=object_rows(bank)+(o&0xffffu)*12u;
    bank.store8(at+2u,std::uint8_t(bank.u8(at+2u)|2u));
    o=bank.u32(bank.u32(object_rows(bank)+(o&0xffffu)*12u+8u)+0x14u);
  }
}
// B7E50: mode-4 network owner dirty mark for a replicated entry.
void network_dirty(Bank& bank,std::uint32_t entry,std::uint32_t flags) {
  const auto owner=bank.u32(0x4dc04cu);
  if(bank.u32(owner+8u)!=4u)return;
  const auto state=bank.u32(owner+4u);
  const auto index=entry&0x3ffu;
  if(!(bank.u8(bank.u32(state+0x20a4u)+index*8u+0x44u)&4u))return;
  const auto at=state+0x2098u+index*36u+0x20u;
  bank.store32(at,bank.u32(at)|flags);
}
// 6CA40/16EA60 over the player data array (bitmap +48, high water +38).
template<class Visit> void for_each_player(const Bank& bank,Visit visit) {
  const auto players=bank.u32(0x4f54f4u);
  for(std::int32_t index=0;;++index) {
    const auto high=std::int32_t(bank.u32(players+0x38u));
    while(index<high && !(bank.u32(bank.u32(players+0x48u)+std::uint32_t(index>>5)*4u)&(1u<<(index&31))))++index;
    if(index>=high)return;
    const auto row=bank.u32(players+0x44u)+bank.u32(players+0x24u)*std::uint32_t(index);
    if(!row)return;
    visit(row,(std::uint32_t(std::int32_t(std::int16_t(bank.u16(row))))<<16)|std::uint32_t(index));
  }
}
std::int16_t nav_kind(std::uint16_t word){return std::int16_t(std::uint16_t(word<<12))>>12;}
std::optional<std::uint32_t> nav_table(const Bank& bank,std::uint32_t player) {
  const auto user=std::int16_t(bank.u16(bank.u32(bank.u32(0x4f54f4u)+0x44u)+(player&0xffffu)*0x21cu+0x28u));
  if(user==-1)return {};
  return std::uint32_t(std::int32_t(user))*0x30u+bank.u32(0x50eed0u);
}
// 25B5DA: refresh a matching nav point or take the last free slot.
void nav_point_add(Bank& bank,std::uint32_t player,std::uint16_t type,std::uint32_t kind,std::uint32_t object,std::uint32_t offset) {
  if(player==none)return;
  const auto user_table=nav_table(bank,player);
  if(!user_table || object==none || type==0xffffu)return;
  const auto table=*user_table;
  std::int16_t free=-1;
  for(std::int16_t n=0;n<4;++n) {
    const auto entry=table+std::uint32_t(n)*12u;
    const auto current=nav_kind(bank.u16(entry+2u));
    if(current==std::int16_t(kind) && bank.u32(entry+8u)==object) {
      bank.store16(entry,type);bank.store32(entry+4u,offset);return;
    }
    if(current==-1)free=n;
  }
  if(free==-1)return;
  const auto entry=table+std::uint32_t(free)*12u;
  const auto change=std::uint16_t((bank.u8(entry+2u)^std::uint8_t(kind))&0xfu);
  bank.store32(entry+4u,offset);
  bank.store16(entry+2u,std::uint16_t(bank.u16(entry+2u)^change));
  bank.store32(entry+8u,object);
  bank.store16(entry,type);
}
// 25B709: clear the first nav point of this kind and target.
void nav_point_remove(Bank& bank,std::uint32_t player,std::uint32_t kind,std::uint32_t object) {
  if(player==none)return;
  const auto user_table=nav_table(bank,player);
  if(!user_table || object==none)return;
  const auto table=*user_table;
  for(std::int16_t n=0;n<4;++n) {
    const auto entry=table+std::uint32_t(n)*12u;
    if(nav_kind(bank.u16(entry+2u))==std::int16_t(kind) && bank.u32(entry+8u)==object) {
      bank.store8(entry+2u,std::uint8_t(bank.u8(entry+2u)|0xfu));
      bank.store32(entry+8u,none);
      bank.store16(entry,0xffffu);
      return;
    }
  }
}
}

// 107AD0: set or clear a weapon trigger's held bit (+20C rows, +4 bit 0x80).
void weapon_hold_trigger(Bank& bank,std::uint32_t weapon,std::uint32_t trigger,std::uint8_t hold) {
  if(weapon==none)return;
  const auto body=object_row_body(bank,weapon);
  if(std::int32_t(trigger)<0)return;
  const auto definition=bank.u32(bank.u32(0x4f0414u)+(bank.u32(body)&0xffffu)*16u+8u);
  if(std::int32_t(trigger)>=std::int32_t(bank.u32(definition+0x2c8u)))return;
  const auto flags=body+0x20cu+trigger*12u+4u;
  bank.store32(flags,hold?bank.u32(flags)|0x80u:bank.u32(flags)&0xffffff7fu);
  mark_for_update(bank,weapon);
}

// 25B6AF / 25B77A: every player on the team with a local user gets the nav
// point (kind 1 object, 0 flag) added or removed. The x87 FLD/FSTP copy of
// the vertical offset quiets a signaling NaN.
void team_nav_point(Bank& bank,bool activate,std::uint16_t type,std::uint16_t team,std::uint32_t kind,
    std::uint32_t target,std::uint32_t offset) {
  if(activate && (offset&0x7f800000u)==0x7f800000u && (offset&0x7fffffu) && !(offset&0x400000u))offset|=0x400000u;
  for_each_player(bank,[&](std::uint32_t row,std::uint32_t player) {
    if(bank.u16(row+0x28u)==0xffffu)return;
    if(team!=std::uint16_t(std::int16_t(std::int8_t(bank.u8(row+0xc0u)))))return;
    if(activate)nav_point_add(bank,player,type,kind,target,offset);
    else nav_point_remove(bank,player,kind,target);
  });
}

// 11CF10 (+D3340 when disabling): camouflage fade rate 1/(max(s, tick) *
// 0.25) * 0.25 into +2B8, +134 bit 3 (and bit 4 cleared when disabling).
void unit_set_active_camo(Bank& bank,std::uint32_t unit,std::uint8_t enable,std::uint32_t seconds) {
  if(unit==none)return;
  const auto mxcsr=bank.floating_point().mxcsr;
  const auto tick=bank.u32(bank.u32(0x51df54u)+4u);
  auto span=std::bit_cast<float>(seconds);
  if(!(sse_compared(seconds,mxcsr)>sse_compared(tick,mxcsr)))span=std::bit_cast<float>(tick);
  const auto rate=mulss(divss(1.f,mulss(span,0.25f,mxcsr),mxcsr),0.25f,mxcsr);
  const auto body=object_row_body(bank,unit);
  if(enable) {
    bank.store32(body+0x134u,bank.u32(body+0x134u)|8u);
  } else {
    const auto flags=bank.u32(body+0x134u)&0xfffffff7u;
    bank.store32(body+0x134u,flags);
    bank.store32(body+0x134u,flags&0xffffffefu);
  }
  bank.store32(body+0x2b8u,std::bit_cast<std::uint32_t>(rate));
  const auto entry=bank.u32(object_row_body(bank,unit)+0xd4u);
  if(entry!=none)network_dirty(bank,entry,0x800000u);
}

// 2B4280: ragdoll a live biped (+10A bit 6, +134 bit 27) and mark it.
void biped_ragdoll(Bank& bank,std::uint32_t unit) {
  if(unit==none)return;
  const auto body=object_try_and_get_and_verify_type(bank,unit,1u);
  if(!body)return;
  bank.store8(body+0x10au,std::uint8_t(bank.u8(body+0x10au)|0x40u));
  bank.store32(body+0x134u,bank.u32(body+0x134u)|0x8000000u);
  mark_for_update(bank,unit);
}

// 11C900: maximum body/shield vitality and their presence flags per object.
void units_set_maximum_vitality(Bank& bank,std::span<const std::uint32_t> objects,std::uint32_t body_vitality,
    std::uint32_t shield_vitality) {
  const auto mxcsr=bank.floating_point().mxcsr;
  for(const auto object:objects) {
    if(object==none)break;
    const auto body=object_row_body(bank,object);
    if((bank.u8(body+0x10au)>>2)&1u)continue;
    bank.store32(body+0xe4u,body_vitality);
    bank.store32(body+0xe8u,shield_vitality);
    bank.store32(body+0xecu,sse_compared(body_vitality,mxcsr)>0.f?0x3f800000u:0u);
    bank.store32(body+0xf0u,sse_compared(shield_vitality,mxcsr)>0.f?0x3f800000u:0u);
  }
}

// 1518A0: some player's unit flashlight intensity (+264) is exactly 1.0.
bool player_flashlight_on(const Bank& bank) {
  const auto mxcsr=bank.floating_point().mxcsr;
  bool on=false;
  for_each_player(bank,[&](std::uint32_t row,std::uint32_t) {
    if(on)return;
    const auto unit=bank.u32(row+0x2cu);
    if(unit==none)return;
    if(sse_compared(bank.u32(object_row_body(bank,unit)+0x264u),mxcsr)==1.f)on=true;
  });
  return on;
}

// 2B5F80: campaign flavor availability bytes at 5020B4.
// The evaluator returns the raw availability byte as its BOOLEAN.
std::uint8_t ice_cream_flavor_available(const Bank& bank,std::uint32_t flavor) {
  if(std::int32_t(flavor)<0 || std::int32_t(flavor)>=15)return 0;
  if(bank.u32(bank.u32(0x4f3218u)+8u)!=1u)return 0;
  return bank.u8(0x5020b4u+flavor);
}

// 27DBC0 -> 1EB590 per walked actor: camouflage on (+134 bit 3, +2B8 0.25)
// or D3340 off (bits 3 and 4 cleared, +2B8 1.0 * 0.25), then the B7E50 mark.
void ai_set_active_camo(Bank& bank,std::uint32_t reference,std::uint8_t enable) {
  ActorWalk actors(bank,reference);
  for(auto row=actors.next();row;row=actors.next()) {
    const auto unit=bank.u32(row+0x18u);
    if(unit==none)continue;
    const auto body=object_row_body(bank,unit);
    if(enable) {
      bank.store32(body+0x134u,bank.u32(body+0x134u)|8u);
    } else {
      const auto flags=bank.u32(body+0x134u)&0xfffffff7u;
      bank.store32(body+0x134u,flags);
      bank.store32(body+0x134u,flags&0xffffffefu);
    }
    bank.store32(body+0x2b8u,0x3e800000u);
    const auto entry=bank.u32(object_row_body(bank,unit)+0xd4u);
    if(entry!=none)network_dirty(bank,entry,0x800000u);
  }
}

// 27F830 -> 1EC900 per walked actor: a changed berserk byte +225; actors
// without +7 also set/clear unit +134 bit 5 and reset +226.
void ai_berserk(Bank& bank,std::uint32_t reference,std::uint8_t berserk) {
  if(reference==none)return;
  ActorWalk actors(bank,reference);
  for(auto row=actors.next();row;row=actors.next()) {
    if(bank.u8(row+0x225u)==berserk)continue;
    bank.store8(row+0x225u,berserk);
    if(bank.u8(row+7u))continue;
    const auto flags=object_row_body(bank,bank.u32(row+0x18u))+0x134u;
    bank.store32(flags,berserk?bank.u32(flags)|0x20u:bank.u32(flags)&0xffffffdfu);
    bank.store8(row+0x226u,0);
  }
}

// 29F5B0: the live flock (data array at 52D88C) whose scenario definition
// (+354 rows of 0x84, name +80) matches; NONE while AI is disabled.
std::uint32_t flock_find(const Bank& bank,std::uint32_t name) {
  if(!ai_enabled(bank))return none;
  const auto pool=bank.u32(0x52d88cu);
  for(std::int32_t index=0;;++index) {
    const auto high=std::int32_t(bank.u32(pool+0x38u));
    while(index<high && !(bank.u32(bank.u32(pool+0x48u)+std::uint32_t(index>>5)*4u)&(1u<<(index&31))))++index;
    if(index>=high)return none;
    const auto row=bank.u32(pool+0x44u)+bank.u32(pool+0x24u)*std::uint32_t(index);
    if(!row)return none;
    const auto definition=bank.u32(bank.u32(0x4ecc20u)+0x354u)+
        std::uint32_t(std::int32_t(std::int16_t(bank.u16(row+2u))))*0x84u;
    if(bank.u32(definition+0x80u)==name)
      return (std::uint32_t(std::int32_t(std::int16_t(bank.u16(row))))<<16)|std::uint32_t(index);
  }
}
// 2AFF60/2AFFD0: flock +E active byte; the result reports a found flock.
bool flock_set_active(Bank& bank,std::uint32_t name,bool active) {
  const auto flock=flock_find(bank,name);
  if(flock==none)return false;
  bank.store8(bank.u32(bank.u32(0x52d88cu)+0x44u)+(flock&0xffffu)*0x28u+0xeu,active?1u:0u);
  return true;
}

namespace {
// 20D590: the squad's group (+6C) or one of its parent groups (+10) is `group`.
bool squad_in_group(const Bank& bank,std::uint32_t squad,std::uint32_t group) {
  for(auto g=bank.u32(squad_row(bank,squad)+0x6cu);g!=none;
      g=bank.u32(bank.u32(bank.u32(group_pool)+0x44u)+(g&0xffffu)*group_stride+0x10u))
    if(g==group)return true;
  return false;
}
// 27C950 over an actor's +30 squad (tags 2/3 never match by squad).
bool squad_in_reference(const Bank& bank,std::uint32_t reference,std::uint32_t squad) {
  switch(reference>>30) {
  case 0:return squad==(reference&0xffffu);
  case 1:return squad_in_group(bank,squad,reference&0xffffu);
  default:return false;
  }
}
// The actor (index value) belongs to the reference: squad tags through its
// +30 squad (NONE never matches), actor tags through 27C880.
bool actor_in_reference(const Bank& bank,std::uint32_t reference,std::uint32_t actor) {
  if((reference>>30)>=2u)return actor==ai_actor_index(bank,reference);
  const auto squad=bank.u32(actor_row(bank,actor)+0x30u);
  return squad!=none && squad_in_reference(bank,reference,squad);
}
void refresh_perception(Bank& bank,std::uint32_t perception) {
  const auto& fp=bank.floating_point();
  bank.store16(perception+0x1cu,3);
  const auto ticks=cvtsi2ss(std::int16_t(bank.u16(bank.u32(0x51df54u)+2u)),fp.mxcsr);
  bank.store16(perception+0x1eu,std::uint16_t(fistp32(mulss(ticks,5.f,fp.mxcsr),fp.x87_control_word)));
}
}

// 27D7D0: actors of reference A refresh, for every prop knowledge record of a
// member of reference B (+1C), each perception entry held by an A member.
// Same single-word visited-prop caveat as 27DA40 (native keeps a full mask).
void ai_magically_see(Bank& bank,std::uint32_t viewers,std::uint32_t targets) {
  std::vector<std::uint32_t> visited(2048);
  ActorWalk actors(bank,viewers);
  for(auto row=actors.next();row;row=actors.next()) {
    const auto prop=bank.u32(row+0x7cu);
    if(prop==none)continue;
    const auto index=prop&0xffffu;
    if(visited[index>>5]&(1u<<(index&31u)))continue;
    visited[index>>5]|=1u<<(index&31u);
    for(auto knowledge=bank.u32(bank.u32(bank.u32(0x50eef8u)+0x44u)+index*0x50u+0x14u);knowledge!=none;) {
      const auto record=bank.u32(bank.u32(0x50eef4u)+0x44u)+(knowledge&0xffffu)*0xc4u;
      const auto known=bank.u32(record+0x1cu);
      if(known!=none && actor_in_reference(bank,targets,known))
        for(auto entry=bank.u32(record+0x18u);entry!=none;) {
          const auto perception=bank.u32(bank.u32(0x50eef0u)+0x44u)+(entry&0xffffu)*0x3cu;
          if(actor_in_reference(bank,viewers,bank.u32(perception+4u)))refresh_perception(bank,perception);
          entry=bank.u32(perception+0x34u);
        }
      knowledge=bank.u32(record+0x14u);
    }
  }
}

namespace {
// One BAD60 marker record (flag 0) in a private overlay, like the original's
// caller-stack record; returns the found count and the record words.
struct MarkerRecord {std::uint16_t count;std::array<std::uint32_t,0x1c> words;};
MarkerRecord probe_marker(const Bank& bank,std::uint32_t object,std::uint32_t name) {
  NativeCampaignScriptScratch scratch(bank.memory(),0x70u);
  NativeCampaignScriptBank local(scratch,{},bank.floating_point());
  MarkerRecord record{};
  record.count=object_get_markers(local,object,name,1,false,NativeCampaignScriptScratch::base);
  for(unsigned n=0;n<record.words.size();++n)record.words[n]=local.u32(NativeCampaignScriptScratch::base+n*4u);
  return record;
}
#if HALO2_NATIVE_SCRIPT_X86_FP
// 10FF78..10FFAF: 1.0 > sqrt((dz*dz + dx*dx) + dy*dy) in x87 precision.
bool x87_within_unit(const std::array<float,3>& child,const std::array<float,3>& marker,std::uint16_t control) {
  const auto mode=x87_control(control);std::uint16_t saved{};std::uint8_t within{};
  const float one=1.f;
  asm volatile("fnstcw %1\n\tfldcw %2\n\t"
      "flds %3\n\tfsubs %6\n\tflds %4\n\tfsubs %7\n\tflds %5\n\tfsubs %8\n\t"
      "fld %%st(0)\n\tfmul %%st(1), %%st\n\tfld %%st(3)\n\tfmul %%st(4), %%st\n\tfaddp\n\t"
      "fld %%st(2)\n\tfmul %%st(3), %%st\n\tfaddp\n\tfsqrt\n\t"
      "fstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\t"
      "flds %9\n\tfcomip %%st(1), %%st\n\tseta %0\n\tfstp %%st(0)\n\tfldcw %1"
      :"=q"(within),"+m"(saved)
      :"m"(mode),"m"(child[0]),"m"(child[1]),"m"(child[2]),"m"(marker[0]),"m"(marker[1]),"m"(marker[2]),"m"(one)
      :"st","st(1)","st(2)","st(3)","st(4)","cc");
  return within!=0;
}
#else
bool x87_within_unit(const std::array<float,3>& c,const std::array<float,3>& m,std::uint16_t) {
  const long double dx=(long double)c[0]-m[0],dy=(long double)c[1]-m[1],dz=(long double)c[2]-m[2];
  return 1.0L>std::sqrt((dz*dz+dx*dx)+dy*dy);
}
#endif
}

// DAE40: model targets (+68/+6C rows of 0x1C) named `target` on the object
// and its seat-bearing children (+4 bit 26) whose marker is absent.
std::uint16_t object_model_targets_destroyed(const Bank& bank,std::uint32_t root,std::uint32_t target) {
  std::uint16_t destroyed=0;
  if(root==none)return 0;
  const auto pool=bank.u32(0x4ecbd0u);
  for(auto current=root;;) {
    std::uint32_t body=0;
    const auto index=current&0xffffu;
    if(std::int32_t(index)<std::int32_t(bank.u32(pool+0x38u))) {
      const auto row=bank.u32(pool+0x44u)+bank.u32(pool+0x24u)*index;
      const auto salt=bank.u16(row);
      if(salt && std::int16_t(salt)==std::int16_t(current>>16)) {
        body=bank.u32(row+8u);
        if(body && (current==root || ((bank.u32(body+4u)>>26)&1u))) {
          const auto tags=bank.u32(0x4f0414u);
          const auto model=bank.u32(bank.u32(tags+(bank.u32(body)&0xffffu)*16u+8u)+0x38u);
          if(model!=none) {
            const auto definition=bank.u32(tags+(model&0xffffu)*16u+8u);
            const auto count=std::int32_t(bank.u32(definition+0x68u));
            for(std::int32_t n=0;n<count;++n) {
              const auto name=bank.u32(bank.u32(definition+0x6cu)+std::uint32_t(n)*0x1cu);
              if(name==target && !probe_marker(bank,current,name).count)destroyed=std::uint16_t(destroyed+1u);
            }
          }
        }
      }
    }
    current=bank.u32(body+(current==root?0x10u:0xcu));
    if(current==none)return destroyed;
  }
}

// 10FF10: the first child attached at the marker's node within 1.0 of the
// marker's local position; NONE otherwise (the BAD60 count is not checked).
std::uint32_t object_at_marker(const Bank& bank,std::uint32_t object,std::uint32_t marker) {
  if(object==none)return none;
  auto child=bank.u32(object_row_body(bank,object)+0x10u);
  const auto record=probe_marker(bank,object,marker);
  if(child==none)return none;
  const auto node=std::int16_t(record.words[0]&0xffffu);
  const std::array<float,3> at{std::bit_cast<float>(record.words[11]),std::bit_cast<float>(record.words[12]),
      std::bit_cast<float>(record.words[13])};
  do {
    const auto body=object_row_body(bank,child);
    if(std::int16_t(std::int8_t(bank.u8(body+0x18u)))==node) {
      const std::array<float,3> position{std::bit_cast<float>(bank.u32(body+0x64u)),
          std::bit_cast<float>(bank.u32(body+0x68u)),std::bit_cast<float>(bank.u32(body+0x6cu))};
      if(x87_within_unit(position,at,bank.floating_point().x87_control_word))return child;
    }
    child=bank.u32(body+0xcu);
  } while(child!=none);
  return none;
}

namespace {
// 190420: the 0x38-byte row of a sound effect collection tag whose name
// (+0) matches; -1 for NONE or no match.
std::uint32_t effect_collection_row(const Bank& bank,std::uint32_t collection,std::uint32_t name) {
  if(collection==none)return none;
  const auto definition=bank.u32(bank.u32(0x4f0414u)+(collection&0xffffu)*16u+8u);
  const auto count=std::int32_t(bank.u32(definition));
  const auto rows=bank.u32(definition+4u);
  for(std::int32_t n=0;n<count;++n)if(bank.u32(rows+std::uint32_t(n)*0x38u)==name)return std::uint32_t(n);
  return none;
}
}
// 1903A0: a script effect name as a sound effect reference: 0x300012A is the
// null effect (0xC0000000), then the scenario collection (+324, tag 2) and
// the globals' collection ([+C4]+C when +C0 is nonzero, tag 1).
std::uint32_t sound_effect_reference(const Bank& bank,std::uint32_t name) {
  if(!name || name==none)return none;
  if(name==0x300012au)return 0xc0000000u;
  const auto scenario=effect_collection_row(bank,bank.u32(bank.u32(0x4ecc20u)+0x324u),name);
  if(scenario!=none)return (scenario&0x3fffffffu)|0x80000000u;
  const auto globals=bank.u32(0x4ecc1cu);
  if(!bank.u32(globals+0xc0u))return none;
  const auto block=bank.u32(globals+0xc4u);
  if(!block)return none;
  const auto global=effect_collection_row(bank,bank.u32(block+0xcu),name);
  return global==none?none:((global&0x3fffffffu)|0x40000000u);
}
// The collection tag datum a tagged effect reference names (18FE80's
// sources): tag 2 scenario +324, tag 1 globals [+C4]+C; NONE otherwise.
std::uint32_t sound_effect_collection(const Bank& bank,std::uint32_t reference) {
  switch(reference>>30) {
  case 2:return bank.u32(bank.u32(0x4ecc20u)+0x324u);
  case 1: {
    const auto globals=bank.u32(0x4ecc1cu);
    if(!bank.u32(globals+0xc0u))return none;
    const auto block=bank.u32(globals+0xc4u);
    return block?bank.u32(block+0xcu):none;
  }
  default:return none;
  }
}
}

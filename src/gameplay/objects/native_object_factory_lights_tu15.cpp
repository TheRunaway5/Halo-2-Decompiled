// Native TU1.5 light tick (track "lights"): C3430, the update of every live
// light once per game tick, and C5290, light deletion. Oracle cases are in
// tools/native_object_factory_cases_lights.py; see
// docs/native_object_factory_tracks/lights.md.
//
// The light update itself (C2CB0) is the attachments track's `light_update`
// and the reconnection (C4D30) is the placement track's `light_reconnect`.
#include "halo2/native_object_factory_lights_tu15.hpp"
#include "halo2/native_object_factory_attachments_tu15.hpp"
#include "halo2/native_object_factory_placement_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
#include "halo2/native_tu15_object_detachment.hpp"
#include "native_object_factory_tu15_internal.hpp"

namespace halo2::native_object_factory_tu15 {
namespace {
using namespace detail;
namespace fp=native_campaign_script_fp;

constexpr std::uint32_t light_data_array=0x4ecbdcu;  // [0x4ECBDC]: the light data array header
constexpr std::uint32_t light_partition=0x4ecbe0u;   // the lights' cluster partition (1D2F10/1D30F0 root)
constexpr std::uint32_t game_time_block=0x51df54u;   // [0x51DF54]: the game time block
constexpr std::uint32_t light_row_size=0x110u;       // the original indexes light rows with this literal

// A TU1.5 data array header (0x48 bytes) as the light tick reads it.
struct DataArray {
  const M& m;
  std::uint32_t at;
  std::uint32_t row_size()const{return r32(m,at+0x24u);}
  std::uint32_t rows()const{return r32(m,at+0x44u);}
  // The row of index `index` by the header's row size.
  std::uint32_t row(std::uint32_t index)const{return rows()+row_size()*index;}
  // 16EA60: the first live index at or after `first`, or NONE.
  std::uint32_t next_live(std::uint32_t first)const{return native_tu15_data_array_next(m,at,std::int32_t(first));}
  // The datum handle of live index `index`: the row's salt word over the index.
  std::uint32_t datum(std::uint32_t index)const{return std::uint32_t(r16(m,row(index)))<<16|index;}
};
constexpr std::uint32_t light_parent_offset=0x58u;  // a light row's parent object

DataArray lights(const M& m){return {m,r32(m,light_data_array)};}

// The game time block: +2 ticks per second (int16), +8 the current tick.
struct GameTime {
  const M& m;
  std::uint32_t at()const{return r32(m,game_time_block);}
  std::int32_t ticks_per_second()const{return std::int16_t(r16(m,at()+2u));}
  std::uint32_t tick()const{return r32(m,at()+8u);}
};

// A light row (0x110 bytes). The fields the tick reads:
//   +2  state byte: bit 1 lit, bit 3 linked into the light partition,
//       bit 4 timed (deleted when its lifetime has passed)
//   +4  the light definition tag
//   +10 the head of its partition links
//   +14 the tick it was created at
//   +4C the object whose marker carries it, +58 the parent object it
//       follows through a node (either may be NONE)
struct Light {
  M& m;
  std::uint32_t datum;
  std::uint32_t row()const{return r32(m,r32(m,light_data_array)+0x44u)+(datum&0xffffu)*light_row_size;}
  std::uint8_t state()const{return r8(m,row()+2u);}
  bool lit()const{return state()&2u;}
  bool linked()const{return state()&8u;}
  bool timed()const{return state()&0x10u;}
  void clear_linked(){const auto at=row()+2u;w8(m,at,r8(m,at)&0xf7u);}
  std::uint32_t definition()const{return tag_data(m,r32(m,row()+4u));}
  std::uint32_t links()const{return row()+0x10u;}
  std::uint32_t created()const{return r32(m,row()+0x14u);}
  std::uint32_t marker_owner()const{return r32(m,row()+0x4cu);}
  std::uint32_t parent()const{return r32(m,row()+light_parent_offset);}
};

// An object's header row (12 bytes): +2 flags, bit 2 "moved this tick".
bool object_moved(const M& m,std::uint32_t object){return r8(m,object_row(m,object)+2u)&4u;}

// 1D30F0 over the light partition: unlink the light from every cluster it
// is linked into, then clear its "linked" bit.
void light_unlink(M& m,Light& light) {
  native_tu15_remove_spatial_chain(m,light_partition,light.datum,light.links());
  light.clear_linked();
}

// The light's definition +B0: its lifetime in seconds.
std::uint32_t light_lifetime(const M& m,std::uint32_t definition){return r32(m,definition+0xb0u);}

// C3522..C354C: the timed light's age in ticks against its lifetime in
// ticks (CVTSI2SS of both, MULSS by the lifetime, COMISS: strictly older).
bool light_expired(const M& m,const Light& light,std::uint32_t now,std::uint32_t definition,std::uint32_t mxcsr) {
  const auto age=fp::cvtsi2ss(std::int32_t(now-light.created()),mxcsr);
  const auto lifetime=fp::mulss(fp::cvtsi2ss(GameTime{m}.ticks_per_second(),mxcsr),
      f32(light_lifetime(m,definition)),mxcsr);
  return fp::sse_compared(bits(age),mxcsr)>fp::sse_compared(bits(lifetime),mxcsr);
}
}

void light_delete(M& m,std::uint32_t datum,bool update_owner_flag) {
  Light light{m,datum};
  const auto pool=lights(m);  // read once, as the original keeps it in ESI
  if(light.linked() && light.lit())light_unlink(m,light);
  const auto parent=light.parent();
  if(parent!=none && update_owner_flag) {
    // The parent keeps its "has lights" bit (body +4 bit 6) only while
    // another live light names it as its parent.
    const auto body=object_body(m,parent);
    bool shared=false;
    for(auto index=pool.next_live(0);index!=none;index=pool.next_live(index+1u)) {
      if(pool.datum(index)==datum)continue;
      if(r32(m,pool.row(index)+light_parent_offset)==parent){shared=true;break;}
    }
    const auto flags=r32(m,body+4u);
    w32(m,body+4u,shared?flags|0x40u:flags&~0x40u);
  }
  native_tu15_data_array_delete(m,pool.at,datum);
}

void lights_update(M& m,NativeTu15PhysicsFrameFp& fpu) {
  const auto now=GameTime{m}.tick();  // read once at entry
  for(auto index=lights(m).next_live(0);index!=none;index=lights(m).next_live(index+1u)) {
    Light light{m,lights(m).datum(index)};
    // The definition is read before the update (C3498), the row's tag
    // cannot change in between.
    const auto definition=light.definition();
    light_update(m,light.datum,fpu.control_word,fpu.mxcsr);
    // A linked light follows its owner: once the owner (the marker owner,
    // else the parent) moved this tick, a lit light is unlinked and the
    // light is placed and linked again.
    if(light.linked()) {
      auto owner=light.marker_owner();
      if(owner==none)owner=light.parent();
      if(owner!=none && object_moved(m,owner)) {
        if(light.lit())light_unlink(m,light);
        light_reconnect(m,light.datum,fpu);
      }
    }
    if(light.timed() && light_expired(m,light,now,definition,fpu.mxcsr))light_delete(m,light.datum,true);
  }
}
}

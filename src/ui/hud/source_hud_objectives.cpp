#include "halo2/source_hud_objectives.hpp"
#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <vector>
namespace halo2 {
SourceHudObjectives source_hud_objectives(const SourceHudObjectiveInput &in,
    std::span<const SourceHudScoreColor> palette) {
  SourceHudObjectives out;
  if(!in.engine_present || in.player==0xffffffffu)return out;
  const auto slot=in.player&65535u;
  if(slot>=in.players.size())throw FormatError("Objective HUD player outside original pool");
  const auto &local=in.players[slot];
  if(local.membership==-1)return out;
  constexpr float capture_scale=std::bit_cast<float>(0x3c820821u); //1/63,45E06C/46A5A4.
  const auto color=[&](int member) {
    const SourceHudScoreColor gray{0x3f000000u,0x3f000000u,0x3f000000u};
    const SourceHudScoreColor fallback{0x3f800000u,0x3ed1eb85u,0x3f333333u};
    const auto &rgb=member==-1?gray:member>=0 && std::size_t(member)<palette.size()?palette[member]:fallback;
    return pack_object_lighting_rgb(std::bit_cast<Vec3>(rgb));
  };
  if(in.engine==8) {
    if(in.territory_count<0 || in.territory_count>8)throw FormatError("Objective HUD territory count escapes original bank");
    for(int territory=0;territory<in.territory_count;++territory) {
      if(in.territory_indices[territory]==-1)continue;
      unsigned maximum=0;int capturer=-1;
      for(unsigned n=0;n<in.players.size();++n) {
        const auto &p=in.players[n];
        if(!(in.active_players&(1u<<n)) || p.capturing!=territory)continue;
        if(p.capture>maximum){maximum=p.capture;capturer=int(n);}
      }
      auto &meter=out.territories[out.territory_count++];
      const auto owner=in.territory_owners[territory];
      if(owner!=0xffffffffu) {
        if((owner&65535u)>=in.players.size())throw FormatError("Objective HUD territory owner escapes player pool");
        meter.color=color(in.players[owner&65535u].membership);
        meter.fraction=1.f-float(maximum)*capture_scale;
      }else if(capturer!=-1) {
        meter.color=color(in.players[unsigned(capturer)].membership);
        meter.fraction=float(maximum)*capture_scale;
      }
    }
  }
  if(local.unit==0xffffffffu)return out;
  if(in.engine==9 && in.assault_bomb) {
    if(!std::isfinite(in.tick_seconds))throw FormatError("Nonfinite original bomb clock");
    const float fraction=in.bomb_index==-1?0.f:in.bomb_armed?1.f:
        (in.tick_seconds/float(in.bomb_seconds?in.bomb_seconds:1))*float(std::max(int(in.bomb_ticks),0));
    if(fraction>0){out.charge_visible=true;out.charge_fraction=fraction;}
  }else if(in.engine==8 && local.capture && local.capturing!=-1) {
    out.charge_visible=true;out.charge_fraction=float(local.capture)*capture_scale;
  }
  return out;
}
SourceHudObjectives source_hud_objectives_tu15(const SourceRuntimeMemory &m,std::uint32_t player) {
  const auto word=[&](std::uint32_t p) {return Bytes(m.view(p,4)).u32(0);};
  const auto half=[&](std::uint32_t p) {return std::bit_cast<std::int16_t>(Bytes(m.view(p,2)).u16(0));};
  const auto byte=[&](std::uint32_t p) {return std::to_integer<std::uint8_t>(m.view(p,1)[0]);};
  SourceHudObjectiveInput in;in.player=player;
  const auto options=word(0x4f3218u);
  if(!options)return {};
  in.engine=std::bit_cast<std::int32_t>(word(options+0x180));
  if(in.engine!=8 && in.engine!=9)return {};
  const auto pool=word(0x4f54f4u);
  const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return m.view(p,n);}};
  const auto local=source_raw_datum_lookup(pool,player,storage);
  if(!local || std::bit_cast<std::int8_t>(byte(local+0xc0))==-1)return {};
  const auto engine=word(0x4f63b8u);
  if(!engine)return {};
  const auto count=word(pool+0x38),stride=word(pool+0x24),rows=word(pool+0x44),bitmap=word(pool+0x48);
  if(count>16 || stride!=0x21c)throw FormatError("Objective HUD requires original player records");
  for(unsigned n=0;n<count;++n) {
    const auto p=rows+n*stride;
    auto &row=in.players[n];
    row.datum=(std::uint32_t(Bytes(m.view(p,2)).u16(0))<<16)|n;
    row.unit=word(p+0x2c);row.membership=std::bit_cast<std::int8_t>(byte(p+0xc0));
    if(in.engine==8) {
      row.capturing=std::bit_cast<std::int8_t>(byte(engine+0x191+n*8));row.capture=byte(engine+0x192+n*8);
    }
    if((word(bitmap+(n/32)*4)&(1u<<(n%32))) && !(byte(p+2)&2))in.active_players|=std::uint16_t(1u<<n);
  }
  std::vector<SourceHudScoreColor> colors;
  if(in.engine==8) {
    in.territory_count=half(engine+0x210);
    if(in.territory_count<0 || in.territory_count>8)throw FormatError("Original territory HUD bank exceeds eight");
    for(unsigned n=0;n<unsigned(in.territory_count);++n) {
      in.territory_indices[n]=half(engine+0x15c+n*2);in.territory_owners[n]=word(engine+0x16c+n*4);
    }
    const auto globals=word(0x4ecc1cu),tags=word(0x4f0414u);
    const auto palette_datum=word(globals+0x16c);
    if(palette_datum!=0xffffffffu) {
      const auto mulg=word(tags+(palette_datum&65535u)*16+8);
      const auto universal=word(mulg+4);
      const auto size=word(universal+0x10),table=word(universal+0x14);
      if(size>64)throw FormatError("Original team palette exceeds bounded HUD bank");
      for(unsigned n=0;n<size;++n)colors.push_back({word(table+n*12),word(table+n*12+4),word(table+n*12+8)});
    }
  } else if(in.players[player&65535u].unit!=0xffffffffu) {
    const auto objects=word(0x4ecbd0u);
    const auto body=[&](std::uint32_t datum) {
      const auto header=source_raw_datum_lookup(objects,datum,storage);
      if(!header)throw FormatError("Objective HUD requires its actual live unit/weapon");
      return word(header+8);
    };
    const auto unit=body(in.players[player&65535u].unit);
    const auto hand=std::bit_cast<std::int8_t>(byte(unit+0x212));
    if(hand!=-1) {
      if(hand<0 || hand>=4)throw FormatError("Original charge HUD weapon slot outside unit");
      const auto weapon=word(unit+0x218+unsigned(hand)*4);
      if(weapon!=0xffffffffu) {
        const auto item=body(weapon),tags=word(0x4f0414u);
        const auto definition=word(tags+(word(item)&65535u)*16+8);
        in.assault_bomb=half(definition+0x290)==1;
        if(in.assault_bomb) {
          in.bomb_index=half(item+0x17e);in.bomb_seconds=half(options+0x244);
          const auto clock=word(0x51df54u);in.tick_seconds=Bytes(m.view(clock+4,4)).f32(0);
          if(in.bomb_index!=-1) {
            // The shared CTF/Assault bank has nine objective rows. Index8
            // belongs to the neutral bomb; it is separate from team0..7.
            if(in.bomb_index<0 || in.bomb_index>=9)throw FormatError("Original bomb HUD index escapes objective bank");
            const auto bombs=word(0x52d710u);
            in.bomb_armed=(byte(bombs+0x170+unsigned(in.bomb_index))&8)!=0;
            in.bomb_ticks=half(bombs+0x1a0+unsigned(in.bomb_index)*2);
          }
        }
      }
    }
  }
  return source_hud_objectives(in,colors);
}
} // namespace halo2

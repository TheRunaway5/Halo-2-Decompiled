#include "halo2/vista_lobby_roster.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <vector>

namespace halo2 {
namespace {
std::int8_t signed_byte(std::byte value) {
  return std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(value));
}
std::array<std::byte,16> appearance(const VistaLobbyRosterConfiguration& c) {
  std::array<std::byte,16> value;std::copy_n(c.begin()+0x40,16,value.begin());return value;
}
bool campaign(std::int32_t value){return value==0 || value==2 || value==4;}
std::array<std::int16_t,4> slots(std::int16_t pane,unsigned row) {
  if(pane<0 || pane>2)return {-1,-1,-1,-1};
  const auto base=pane==1?24:29;const auto second=pane==1?8:13;
  return {std::int16_t(base+row),std::int16_t(second+row),std::int16_t(5+row),std::int16_t(21+row)};
}
VistaLobbyRosterDescriptor descriptor(const VistaLobbyRosterConfiguration& c,
    std::uint32_t id,const VistaLobbyRosterInput& in,bool is_campaign,
    const VistaPcrAppearanceAssets& assets) {
  VistaLobbyRosterDescriptor out;out.flags=0x103;out.appearance=appearance(c);
  bool terminated=false;
  for(unsigned i=0;i<32;++i) {
    const auto ch=std::to_integer<unsigned>(c[i*2])|(std::to_integer<unsigned>(c[i*2+1])<<8);
    if(!ch){terminated=true;break;}out.name+=char16_t(ch);
  }
  if(!terminated)throw FormatError("Vista lobby roster name is not terminated in final configuration");
  const auto rank=signed_byte(c[0x7e]);
  if(assets.rank_owner_present)for(unsigned i=0;i<assets.rank_ranges.size();++i)
    if(rank>=assets.rank_ranges[i].first && rank<=assets.rank_ranges[i].last) {
      out.rank_range=std::uint16_t(i);out.flags|=0x40;break;
    }
  out.selector=signed_byte(c[0x81]);if(out.selector==4)out.selector=1;
  const auto team=signed_byte(c[0x7c]);
  if(!is_campaign && in.variant_present && (in.variant_flags&1)) {
    if(team>=0 && team<8){out.team=std::uint16_t(team);out.flags|=0x84;}
    else if(team==-1){out.team=0xffff;out.no_team=1;out.flags|=0x84;}
  }
  for(unsigned i=0;i<4;++i)if(id==in.local_player_ids[i]) {
    const auto& edit=in.local_edits[i];
    if(edit.active) {
      out.team=std::bit_cast<std::uint16_t>(edit.team);out.no_team=edit.team==-1;
      out.flags|=0x8c;
      if(edit.team==-1)out.team_string=0x090001c7;
      else if(edit.team>=0 && edit.team<8)out.team_string=0x0500020f+std::uint32_t(edit.team);
      else out.team_string=0;
    }
    break;
  }
  return out;
}
} // namespace
void vista_lobby_roster_update(const VistaLobbyRosterInput& in,
    const VistaPcrAppearanceAssets& assets,VistaLobbyRosterServices& services) {
  if(in.player_ids.size()>16)throw FormatError("Vista lobby roster exceeds original sixteen-row bank");
  const bool is_campaign=campaign(services.current_session_enum());
  std::vector<VistaLobbyRosterDescriptor> rows;
  if(in.session_enum!=-1)for(const auto id:in.player_ids)
    rows.push_back(descriptor(services.player_configuration(id),id,in,is_campaign,assets));
  services.publish(rows);
  for(unsigned row=0;row<rows.size();++row) {
    const auto id=in.player_ids[row];const auto c=services.player_configuration(id);
    std::uint32_t team=0xffffffffu;
    const auto current_team=signed_byte(c[0x7c]);
    if(!is_campaign && in.variant_present && (in.variant_flags&1) && current_team>=0 && current_team<8)
      team=std::uint32_t(current_team);
    services.unused_colors(team,0,appearance(c));
    const auto s=slots(in.pane,row);
    for(unsigned i=0;i<3;++i) {
      const auto child=i<2?services.lookup_text(s[i]):services.lookup_bitmap(s[i]);
      if(child)services.visible(child,false);
    }
    const auto voice=services.lookup_bitmap(s[3]);
    if(voice) {
      std::uint32_t frame=2;
      if(services.voice_available(id))frame=services.voice_active(id)?0u:1u;
      services.frame(voice,frame);services.visible(voice,true);
    }
  }
  for(unsigned row=unsigned(rows.size());row<16;++row) {
    const auto s=slots(in.pane,row);
    services.hide_slot(6,s[0]);services.hide_slot(6,s[1]);
    services.hide_slot(8,s[2]);services.hide_slot(8,s[3]);services.hide_slot(9,std::int32_t(row));
  }
}
VistaLobbyRosterSkinView vista_lobby_roster_skin(const VistaLobbyRosterDescriptor& d,
    const VistaPcrAppearanceAssets& a,const VistaLoadingFloatingPoint& fp) {
  if(!(d.flags&2) || !(d.flags&1) || (d.flags&0x230))
    throw FormatError("Vista lobby skin requires an admitted 6443F0 descriptor");
  VistaLobbyRosterSkinView out;out.name=d.name;
  out.team_string=(d.flags&8)?d.team_string:0xffffffffu;
  const auto team=(d.flags&4)?d.team:std::uint16_t(0xffff);
  const auto no_team=(d.flags&0x80)?d.no_team:std::uint8_t(0);
  const auto rank=(d.flags&0x40)?d.rank_range:std::uint16_t(0xffff);
  const auto selector=(d.flags&0x100)?d.selector:0;
  out.primary_color=vista_pcr_appearance_colors(d.appearance,team,no_team,a)[0];
  out.name_color=vista_pcr_profile_name_color(out.primary_color,fp);
  out.name_visible=out.primary_visible=true;out.secondary_visible=false;
  out.rank_visible=rank!=0xffff;out.emblem_visible=out.rank_visible || !(a.emblem_definition_flags&2);
  out.primary_frame=a.previous_primary_frame;out.rank_frame=a.previous_rank_frame;
  const auto frame=selector<=1?0u:std::uint32_t(selector-1);
  if(a.primary_bitmap_present && std::bit_cast<std::int32_t>(a.primary_page_count)>std::bit_cast<std::int32_t>(frame))
    out.primary_frame=std::uint16_t(frame);
  if(out.rank_visible && a.rank_bitmap_present && std::bit_cast<std::int32_t>(a.rank_page_count)>std::int32_t(rank))
    out.rank_frame=rank;
  return out;
}
} // namespace halo2

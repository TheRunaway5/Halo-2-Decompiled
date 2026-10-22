#include "halo2/vista_lobby_info.hpp"

namespace halo2 {
namespace {
bool admitted(const VistaLobbyInfoSession& s) {
  return s.present && s.state>=3 && s.state<=8;
}
}
std::int32_t vista_lobby_info_session_class(const VistaLobbyInfoSession& s) {
  return admitted(s)?s.session_class:-1;
}
std::uint32_t vista_lobby_info_session_privacy(const VistaLobbyInfoSession& s) {
  return admitted(s) && s.privacy<=3?s.privacy:0;
}
std::int32_t vista_lobby_info_session_count(const VistaLobbyInfoSession& s) {
  return admitted(s)?s.membership_count:0;
}
bool vista_lobby_info_privacy_hidden(std::int32_t session_enum) {
  return session_enum==0 || session_enum==1;
}
std::optional<std::uint32_t> vista_lobby_info_description_id(
    const VistaLobbyInfoDescriptions& d,std::int32_t list,std::int32_t entry) {
  if(!d.root_present || !d.tag_present || list<0 ||
      std::uint32_t(list)>=d.lists.size())return std::nullopt;
  const auto strings=d.lists[std::uint32_t(list)];
  if(entry<0 || std::uint32_t(entry)>=strings.size())return std::nullopt;
  return strings[std::uint32_t(entry)];
}
void vista_lobby_info_description(VistaLobbyInfoText text,std::int32_t list,
    std::int32_t entry,const VistaLobbyInfoServices& api) {
  if(const auto id=api.description_id(text,list,entry))api.string_id(text,*id);
}
void vista_lobby_info_privacy(std::int16_t pane,const VistaLobbyInfoServices& api) {
  const std::int16_t index=pane==0 || pane==2?12:pane==1?7:-1;
  const auto text=api.lookup_text(index);
  if(!text)return;
  if(vista_lobby_info_privacy_hidden(api.session_enum())) {
    api.visible(text,0);
    return;
  }
  api.visible(text,1);
  switch(api.privacy()) {
    case 0:case 1:api.string_id(text,0x04000202);return;
    case 2:api.string_id(text,0x0f000203);return;
    case 3:api.string_id(text,0x06000204);return;
    default:return;
  }
}
void vista_lobby_info_help(std::int16_t pane,const VistaLobbyInfoServices& api) {
  std::int16_t index;
  switch(pane) {
    case 0:index=48;break;
    case 1:index=43;break;
    case 2:index=49;break;
    default:return;
  }
  const auto text=api.lookup_text(index);
  if(!text)return;
  if(api.session_class()!=2 || api.membership_count()>1) {
    api.visible(text,0);
    return;
  }
  api.visible(text,1);
  const auto privacy=api.privacy();
  if(privacy==3)vista_lobby_info_description(text,1,0,api);
  else if(privacy==2)vista_lobby_info_description(text,1,1,api);
  else if(api.session_enum()==6)vista_lobby_info_description(text,1,3,api);
  else api.visible(text,0);
}
} // namespace halo2

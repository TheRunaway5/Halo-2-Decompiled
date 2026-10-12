#include "halo2/source_player_profile.hpp"
#include <algorithm>
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourcePlayerConfiguration& p,unsigned at) {
  T value;std::memcpy(&value,p.data()+at,sizeof(value));return value;
}
template<class T>void write(SourcePlayerConfiguration& p,unsigned at,T value) {
  std::memcpy(p.data()+at,&value,sizeof(value));
}
std::int8_t byte(const SourcePlayerConfiguration& p,unsigned at) {
  return std::bit_cast<std::int8_t>(p[at]);
}
}
SourcePlayerIdentity source_offline_player_identity(const SourceMachineIdentity& machine,
    std::int32_t local_user) {
  std::uint64_t address=0;
  for(unsigned i=0;i<machine.size();++i)address|=std::uint64_t(machine[i])<<(8*i);
  // The original CDQ sign extension is ORed with the shifted48-bit address.
  const auto value=(address<<16)|std::uint64_t(std::int64_t(local_user));
  SourcePlayerIdentity identity{};
  for(unsigned i=0;i<8;++i)identity[i]=std::uint8_t(value>>(8*i));
  identity[11]=0xba;identity[10]=0xd0;
  return identity;
}
SourceFullPlayerProfile source_default_full_player_profile() {
  SourceFullPlayerProfile profile{};
  profile[4]=1;profile[0x102]=3;profile[0x118]=10;profile[0x11b]=10;
  return profile;
}
SourcePlayerConfiguration source_default_player_configuration() {
  SourcePlayerConfiguration profile{};
  profile[0x7e]=profile[0x7f]=0xff;
  std::fill(profile.begin()+0x84,profile.end(),0xff);
  return profile;
}
SourcePlayerConfiguration source_offline_local_configuration(
    const SourceOfflineLocalProfile& local,bool override_four) {
  auto result=local.input_member>=0 && local.input_member<4 && local.input_member_present
      ? local.input_configuration : source_default_player_configuration();
  bool ended=false;
  for(unsigned i=0;i<31;++i) {
    const auto c=ended ? std::uint16_t{} : local.display_name[i];
    ended=ended||c==0;write(result,i*2,c);
  }
  write(result,62,std::uint16_t{});
  std::copy(local.appearance.begin(),local.appearance.end(),result.begin()+0x40);
  if(std::any_of(local.account.begin(),local.account.begin()+8,[](auto v){return v!=0;})) {
    std::copy(local.account.begin(),local.account.end(),result.begin()+0x70);
    std::copy(local.account_data.begin(),local.account_data.end(),result.begin()+0x50);
  }else std::fill(result.begin()+0x50,result.begin()+0x7c,0);
  result[0x7c]=std::uint8_t(source_local_profile_team(local.committed_team));
  result[0x7d]=local.field_200;result[0x80]=local.field_46c;
  result[0x81]=override_four ? 4 : std::uint8_t(local.field_204);
  return result;
}
void source_convert_offline_local_profile(const SourceOfflineLocalProfile& local,
    SourcePlayerIdentity* identity,SourcePlayerConfiguration* configuration,bool override_four) {
  if(identity)*identity=local.identity;
  if(configuration)*configuration=source_offline_local_configuration(local,override_four);
}
void source_sanitize_player_configuration(SourcePlayerConfiguration& profile,
    const SourcePlayerConfigurationContext& context) {
  if(context.game_type==1) {
    std::int16_t selected=0;
    for(const auto value:context.scenario_representations)
      if(value!=-1){selected=value;break;}
    profile[0x7c]=1;profile[0x44]=std::uint8_t(selected);
  } else if(context.game_type==2 &&
      context.representation_policy==SourceProfileRepresentationPolicy::MultiplayerMapping) {
    const auto representation=byte(profile,0x44);
    profile[0x44]=(representation==1||representation==3) ? 3 : 2;
  }
  if(auto representation=byte(profile,0x44);representation!=-1) {
    if(representation<0)representation=0;
    else {
      const auto maximum=std::bit_cast<std::int32_t>(
          std::uint32_t(context.representation_count)-1);
      if(maximum<representation)representation=std::bit_cast<std::int8_t>(std::uint8_t(maximum));
    }
    profile[0x44]=std::uint8_t(representation);
  }
  for(const auto at:{0x7e,0x7f})
    if(byte(profile,at)!=-1 && byte(profile,at)<0)profile[at]=0;
  const auto selector=read<std::int32_t>(profile,0x84);
  if(selector<0||selector>15)std::fill(profile.begin()+0x84,profile.end(),0xff);
  else {
    write(profile,0x8c,std::clamp(read<std::int32_t>(profile,0x8c),0,0x3fffffff));
    for(const auto at:{0x88,0x8a})
      write(profile,at,std::clamp(read<std::int16_t>(profile,at),std::int16_t(0),std::int16_t(127)));
  }
  profile[0x7d]=std::uint8_t(std::clamp(byte(profile,0x7d),std::int8_t(0),std::int8_t(3)));
  if(byte(profile,0x81)<0||byte(profile,0x81)>7)profile[0x81]=0;
  if(context.selected_engine && context.team_mode && profile[0x7c]!=0xff &&
      !(std::uint32_t(context.team_mask)&(1u<<(profile[0x7c]&31))))profile[0x7c]=0xff;
}
} // namespace halo2

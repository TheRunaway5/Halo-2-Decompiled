#include "halo2/native_player_profile.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::u16string utf16(std::string_view input) {
  std::u16string result;
  for(std::size_t i=0;i<input.size();) {
    const auto first=std::uint8_t(input[i++]);
    unsigned value=first,extra=0,minimum=0;
    if(first>=0xc2 && first<=0xdf){value=first&31;extra=1;minimum=0x80;}
    else if(first>=0xe0 && first<=0xef){value=first&15;extra=2;minimum=0x800;}
    else if(first>=0xf0 && first<=0xf4){value=first&7;extra=3;minimum=0x10000;}
    else if(first>=0x80)throw FormatError("Selected profile name has invalid native UTF8");
    if(extra>input.size()-i)throw FormatError("Selected profile name has truncated native UTF8");
    for(unsigned n=0;n<extra;++n) {
      const auto next=std::uint8_t(input[i++]);
      if((next&0xc0)!=0x80)throw FormatError("Selected profile name has invalid native UTF8 continuation");
      value=(value<<6)|(next&63);
    }
    if(value<minimum || value>0x10ffff || (value>=0xd800 && value<=0xdfff))
      throw FormatError("Selected profile name has invalid native Unicode scalar");
    if(value<0x10000)result.push_back(char16_t(value));
    else {value-=0x10000;result.push_back(char16_t(0xd800+(value>>10)));result.push_back(char16_t(0xdc00+(value&1023)));}
  }
  return result;
}
void utf8(std::string& result,unsigned value) {
  if(value<0x80)result.push_back(char(value));
  else if(value<0x800){result.push_back(char(0xc0|(value>>6)));result.push_back(char(0x80|(value&63)));}
  else if(value<0x10000){result.push_back(char(0xe0|(value>>12)));result.push_back(char(0x80|((value>>6)&63)));result.push_back(char(0x80|(value&63)));}
  else {result.push_back(char(0xf0|(value>>18)));result.push_back(char(0x80|((value>>12)&63)));result.push_back(char(0x80|((value>>6)&63)));result.push_back(char(0x80|(value&63)));}
}
}
SourcePlayerConfiguration native_selected_player_configuration(std::string_view name,
    std::span<const std::uint8_t,9> appearance,std::int32_t committed_team) {
  SourceOfflineLocalProfile local;
  //65690 clears the freshD0 input-member record before profile delivery.
  // The complete192380->192931->192BAA default bridge independently freezes
  // these zero configuration bytes. Using the standalone constructor's NONE
  // sentinels here would lose that original owning initialization.
  local.input_member=0;local.input_member_present=1;
  const auto full=source_default_full_player_profile();
  std::copy_n(full.begin()+0x118,16,local.appearance.begin());
  std::copy(appearance.begin(),appearance.end(),local.appearance.begin());
  const auto text=utf16(name);
  std::copy_n(text.begin(),std::min(text.size(),local.display_name.size()),local.display_name.begin());
  local.committed_team=committed_team;
  // The fresh native local profile owns its cleared member configuration.
  // Selected name/appearance/team are actual stored frontend fields. Machine
  // and account identities still require separate native platform owners.
  return source_offline_local_configuration(local);
}
std::u16string source_player_configuration_name(const SourcePlayerConfiguration& profile) {
  std::u16string result;
  for(unsigned i=0;i<32;++i) {
    const auto c=char16_t(unsigned(profile[i*2])|(unsigned(profile[i*2+1])<<8));
    if(!c)break;
    result.push_back(c);
  }
  return result;
}
std::string native_player_configuration_name(const SourcePlayerConfiguration& profile) {
  const auto text=source_player_configuration_name(profile);std::string result;
  for(unsigned i=0;i<text.size();++i) {
    unsigned value=text[i];
    if(value>=0xd800 && value<=0xdbff) {
      if(i+1==text.size() || text[i+1]<0xdc00 || text[i+1]>0xdfff)
        throw FormatError("Selected profile name has unavailable native UTF16 encoding");
      value=0x10000+((value-0xd800)<<10)+(text[++i]-0xdc00);
    }else if(value>=0xdc00 && value<=0xdfff)
      throw FormatError("Selected profile name has unavailable native UTF16 encoding");
    utf8(result,value);
  }
  return result;
}
} // namespace halo2

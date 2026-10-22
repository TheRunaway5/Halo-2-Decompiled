#include "halo2/vista_session_reconciliation.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <vector>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t o) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[o+i]))<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[o+i]=std::byte(v>>(8*i));
}
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
bool local_state(std::span<const std::byte> b){const auto s=get(b,0x73a0);return s>=5 && s<=8;}
std::size_t local_record(std::span<const std::byte> b) {
  const auto i=get(b,0x71c0);if(i>=17)throw FormatError("Vista reconciliation has no retained local membership record");return 0x88+i*0x10c;
}
std::size_t player(std::uint32_t i) {
  if(i>=16)throw FormatError("Vista reconciliation player is outside the retained bank");
  return 0x125c+i*0x128;
}
void validate(VistaSessionReconciliationBanks b) {
  if(b.session.size()<vista_session_source_size || b.local_users.size()<vista_local_user_bank_size ||
      b.controllers.size()<vista_controller_user_bank_size || b.retained.size()<vista_session_reconciliation_bank_size)
    throw FormatError("Vista reconciliation requires complete retained session/user/controller/cache owners");
  if(get(b.session,0x14)>=2)throw FormatError("Vista reconciliation source session slot is outside its two retained caches");
}
bool equal(std::span<const std::byte> a,std::span<const std::byte> b) {return std::ranges::equal(a,b);}
VistaLocalUserIdentity identity(std::span<const std::byte> b,std::size_t o) {
  VistaLocalUserIdentity result;std::copy_n(b.begin()+o,8,result.begin());return result;
}
void revision(std::span<std::byte> b){put(b,0x70,get(b,0x70)+1);put(b,0x7930,get(b,0x7930)+1);}
std::optional<std::size_t> reservation(std::span<const std::byte> b,VistaLocalUserIdentity id) {
  for(std::size_t o=0x7978;o<0x7b78;o+=0x20)
    if(b[o]!=std::byte{} && equal(b.subspan(o+0xa,8),id))return o;
  return {};
}
void erase_player(std::span<std::byte> b,std::uint32_t index) {
  const auto p=player(index);const auto peer=get(b,p+8),user=get(b,p+0xc);
  if(peer>=17 || user>=4)throw FormatError("Vista reconciliation player has no retained peer/user mapping");
  if(local_state(b))if(const auto r=reservation(b,identity(b,p)))b[*r+1]=std::byte{};
  put(b,0x184+peer*0x10c+user*4,0xffffffffu);put(b,0x180+peer*0x10c,get(b,0x180+peer*0x10c)-1);
  put(b,0x1254,get(b,0x1254)-1);put(b,0x1258,get(b,0x1258)&~(1u<<index));
}
void message(std::span<const std::byte> b,unsigned id,std::span<const std::byte> payload,VistaSessionReconciliationServices& service) {
  const auto target=get(b,0x64);if(target>=17)throw FormatError("Vista reconciliation has no retained remote association");
  const auto o=0x71c4+target*0x1c;
  if(b[o+1]!=std::byte{})service.send_session_message(get(b,8),get(b,0x14),get(b,o+4),id,payload);
}
std::vector<std::byte> packet(std::span<const std::byte> b,std::size_t n) {
  std::vector<std::byte> p(n);std::copy_n(b.begin()+0x20,8,p.begin());return p;
}
std::uint8_t choose_team(std::span<const std::byte> b,std::uint16_t mask) {
  std::array<unsigned,8> counts{};
  for(unsigned i=0;i<16;++i)if(get(b,0x1258)&(1u<<i)) {
    const auto team=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(b[player(i)+0x118]));
    if(team==-1 || !(std::uint32_t(mask)&(1u<<(std::uint8_t(team)&31))))continue;
    if(team<0 || team>=8)throw FormatError("Vista team selector consumed a team outside its source scratch bank");
    ++counts[team];
  }
  unsigned occupied{},least=0xffffffffu;
  for(unsigned i=0;i<8;++i)if(counts[i]){++occupied;if(least==0xffffffffu || counts[i]<counts[least])least=i;}
  if(occupied>1)return std::uint8_t(least);
  for(unsigned i=0;i<8;++i)if(!counts[i] && (mask&(1u<<i)))return std::uint8_t(i);
  return 0;
}
std::u16string text(std::span<const std::byte> b) {
  std::u16string result;
  for(unsigned i=0;i<32;++i){const auto c=std::to_integer<unsigned>(b[i*2])|(std::to_integer<unsigned>(b[i*2+1])<<8);if(!c)return result;result+=char16_t(c);}
  throw FormatError("Vista reconciliation consumed an unterminated source player name");
}
void copy_name(std::span<std::byte> out,std::span<const std::byte> in) {
  const auto name=text(in);const auto n=std::min<std::size_t>(31,name.size());
  for(unsigned i=0;i<n;++i)put(out,i*2,name[i],2);
  put(out,n*2,0,2);
}
void unique_name(std::span<std::byte> out,std::span<const std::byte> requested,
    const std::vector<std::span<const std::byte>>& names,VistaSessionReconciliationServices& service) {
  put(out,0,0,2);
  for(unsigned attempt=0;attempt<100;++attempt) {
    if(!attempt && !text(requested).empty())copy_name(out,requested);
    else if(!service.authored_random_name(out.first(64)))return;
    const auto value=text(out);bool duplicate{};
    for(const auto name:names)if(text(name)==value){duplicate=true;break;}
    if(!duplicate)return;
  }
}
unsigned selector(VistaSessionReconciliationBanks owner,unsigned user) {
  const auto u=owner.local_users.subspan(user*0xb8,0xb8);if(u[0]==std::byte{})return 0;
  auto b=owner.session;const auto state=get(b,0x73a0);if(state<3 || state>8)return 1;
  const auto i=get(b,local_record(b)+0xfc+user*4);
  if(i==0xffffffffu)return 2+unsigned(u[0xb4+get(b,0x14)]!=std::byte{});
  const auto p=player(i);if(get(b,p+0x14)==0xffffffffu)return 4;
  if(get(b,p+0x14)!=get(u,0xc) || !equal(b.subspan(p+0x18,0x84),u.subspan(0x10,0x84)) ||
      get(b,p+0x120)!=get(u,0x94) || get(b,p+0x124)!=get(u,0x98))return 5;
  return 6;
}
bool add_player(std::span<std::byte> b,unsigned user,std::span<const std::byte> u,std::uint8_t context,VistaSessionReconciliationServices& service) {
  const auto id=identity(u,1);const auto local=get(b,0x71c0);const auto map=local_record(b)+0xfc+user*4;
  if(local_state(b)) {
    auto index=get(b,map);
    if(index!=0xffffffffu && !equal(b.subspan(player(index),8),id)) {
      erase_player(b,index);revision(b);index=0xffffffffu;
    }
    if(index==0xffffffffu) {
      if(get(b,0x73a0)!=0 && get(b,0x70)!=0xffffffffu)
        for(unsigned i=0;i<16;++i)if((get(b,0x1258)&(1u<<i)) && equal(b.subspan(player(i),8),id))return false;
      const auto admission=get(b,0x7b78);if(admission && !service.identity_admitted(admission,id))return false;
      const auto reserved=reservation(b,id);
      if(!reserved) {
        unsigned pending{};for(std::size_t o=0x7978;o<0x7b78;o+=0x20)pending+=b[o]!=std::byte{} && b[o+1]==std::byte{};
        const auto used=get(b,0x1254)+pending;const auto capacity=get(b,0x4c80);
        if(signed_word(used)>signed_word(capacity) || signed_word(capacity-used)<=0)return false;
      }
      index=0;while(index<16 && (get(b,0x1258)&(1u<<index)))++index;
      if(index==16)throw FormatError("Vista insertion has no owned free source player row");
      const auto p=player(index);std::copy(id.begin(),id.end(),b.begin()+p);
      put(b,p+8,local);put(b,p+0xc,user);put(b,p+0x10,context?1:0,2);put(b,p+0x14,0xffffffffu);
      std::fill_n(b.begin()+p+0x18,0x84,std::byte{});std::fill_n(b.begin()+p+0x9c,0x84,std::byte{});
      put(b,map,index);put(b,local_record(b)+0xf8,get(b,local_record(b)+0xf8)+1);
      put(b,0x1254,get(b,0x1254)+1);put(b,0x1258,get(b,0x1258)|(1u<<index));
      if(reserved)b[*reserved+1]=std::byte{1};
      revision(b);
    }
    const auto p=player(index);put(b,p+0x14,get(u,0xc));
    const auto flags=std::to_integer<unsigned>(b[p+0x10])|(std::to_integer<unsigned>(b[p+0x11])<<8);
    put(b,p+0x10,context?(flags|1):(flags&0xfffe),2);
    std::copy_n(u.begin()+0x10,0x84,b.begin()+p+0x18);put(b,p+0x120,get(u,0x94));put(b,p+0x124,get(u,0x98));return true;
  }
  auto p=packet(b,0xa8);put(p,8,user);std::copy(id.begin(),id.end(),p.begin()+0xc);put(p,0x14,context?1:0,2);
  put(p,0x18,get(u,0xc));std::copy_n(u.begin()+0x10,0x84,p.begin()+0x1c);put(p,0xa0,get(u,0x94));put(p,0xa4,get(u,0x98));
  message(b,0x1d,p,service);return true;
}
void update_or_remove(VistaSessionReconciliationBanks owner,unsigned user,unsigned action,
    VistaSessionReconciliationThresholds limits,VistaSessionReconciliationServices& service) {
  auto b=owner.session;auto u=owner.local_users.subspan(user*0xb8,0xb8);const auto slot=get(b,0x14);
  const auto field=(action==0?0xacu:0xa4u)+slot*4;const auto last=get(u,field);
  if(action==0 && get(b,local_record(b)+0xfc+user*4)==0xffffffffu)return;
  const auto elapsed=service.network_clock()-last;
  if(signed_word(elapsed)<(action==0?limits.remove:limits.update) && last)return;
  bool accepted{};
  if(local_state(b)) {
    const auto index=get(b,local_record(b)+0xfc+user*4);
    if(index!=0xffffffffu) {
      if(action==0)erase_player(b,index);
      else {
        const auto p=player(index);put(b,p+0x14,get(u,0xc));std::copy_n(u.begin()+0x10,0x84,b.begin()+p+0x18);
        put(b,p+0x120,get(u,0x94));put(b,p+0x124,get(u,0x98));
      }
      revision(b);accepted=true;
    }
  } else {
    auto p=packet(b,action==0?0xc:0x9c);put(p,8,user);
    if(action!=0){put(p,0xc,get(u,0xc));std::copy_n(u.begin()+0x10,0x84,p.begin()+0x10);put(p,0x94,get(u,0x94));put(p,0x98,get(u,0x98));}
    message(b,action==0?0x1f:0x20,p,service);accepted=true;
  }
  if(accepted)put(u,field,service.network_clock());
}
}
void VistaSessionReconciliationServices::send_peer_message(const VistaSessionLocalPeerMessage& p) {
  send_session_message(p.transport_owner,p.session_slot,p.channel,0x1a,p.payload);
}
void vista_session_publish_final_players(VistaSessionReconciliationBanks owner,VistaSessionReconciliationServices& service) {
  validate(owner);auto b=owner.session;const auto state=get(b,0x73a0);if(state==6 || state==7 || state==8)return;
  const auto slot=get(b,0x14);auto cached=owner.retained.subspan(0x18+slot*0x130,0x130);
  if(state<3 || state>8){std::fill(cached.begin(),cached.end(),std::byte{});return;}
  const auto variant=b.subspan(0x4e3c,0x130);
  if(equal(cached,variant) && get(owner.retained,0x10+slot*4)==get(b,0x70))return;
  std::vector<std::span<const std::byte>> names;
  for(unsigned i=0;i<16;++i)if(get(b,0x1258)&(1u<<i))names.push_back(b.subspan(player(i)+0x9c,64));
  for(unsigned i=0;i<16;++i)if(get(b,0x1258)&(1u<<i)) {
    const auto p=player(i);const auto current=b.subspan(p+0x18,0x84);const auto previous=b.subspan(p+0x9c,0x84);
    std::array<std::byte,0x84> payload{};payload[0x7e]=payload[0x7f]=std::byte{0xff};
    if(!text(current).empty() && text(previous).empty())unique_name(payload,current,names,service);
    else copy_name(payload,previous);
    std::copy_n(current.begin()+0x40,0x3c,payload.begin()+0x40);
    auto team=std::to_integer<std::uint8_t>(previous[0x7c]);
    if(get(b,0x18)==1) {
      team=std::to_integer<std::uint8_t>(current[0x7c]);
      if(team!=0xff) {
        if(std::to_integer<unsigned>(variant[0x48])&1) {
          const auto allowed=service.primary_allowed_team_mask();
          const auto valid=[&](std::uint8_t value){return value<8 && (allowed&(1u<<value));};
          if(!valid(team)){team=std::to_integer<std::uint8_t>(previous[0x7c]);if(!valid(team))team=choose_team(b,allowed);}
        } else team=std::uint8_t(i);
      }
    }
    payload[0x7c]=std::byte(team);std::copy_n(current.begin()+0x7d,6,payload.begin()+0x7d);
    if(!equal(payload,previous) && state==5){std::copy(payload.begin(),payload.end(),b.begin()+p+0x9c);revision(b);}
  }
  put(owner.retained,0x10+slot*4,get(b,0x70));std::copy(variant.begin(),variant.end(),cached.begin());
}
void vista_session_reconcile_local_users(VistaSessionReconciliationBanks owner,
    VistaSessionReconciliationThresholds limits,const VistaSessionLocalPeerValues& peer,
    VistaSessionReconciliationServices& service) {
  validate(owner);auto b=owner.session;const auto slot=get(b,0x14);const auto generation=get(b,0x7960);
  if(get(owner.retained,slot*4)!=generation) {
    put(owner.retained,8+slot*4,0);
    for(unsigned i=0;i<4;++i){auto u=owner.local_users.subspan(i*0xb8,0xb8);for(unsigned o:{0x9cu,0xa4u,0xacu})put(u,o+slot*4,0);put(u,0xb4+slot,0,1);}
    put(owner.retained,slot*4,generation);
  }
  const auto state=get(b,0x73a0);
  if(state!=3 && state!=5 && state!=7 && state!=8)return;
  if((state==7 || state==8) && b[0x73a4]!=std::byte{})return;
  const auto record=local_record(b);
  if(get(b,0x71c0)==get(b,0x74) && get(b,0x18)==1) {
    const auto language=service.language();(void)service.language();
    if(language!=get(b,0x4c70)) {
      if(local_state(b)){put(b,0x4c70,language);put(b,0x4c60,get(b,0x4c60)+1);}
      else {auto p=packet(b,0x4b4);put(p,0x10,1,1);put(p,0x14,language);message(b,0x22,p,service);}
    }
  }
  auto last=get(owner.retained,8+slot*4);
  (void)vista_session_publish_local_peer(b,last,limits.peer,peer,service);put(owner.retained,8+slot*4,last);
  for(unsigned user=0;user<4;++user) {
    const auto action=selector(owner,user);
    if(action==0 || action==4 || action==5){update_or_remove(owner,user,action,limits,service);continue;}
    if(action!=2)continue;
    auto u=owner.local_users.subspan(user*0xb8,0xb8);const auto field=0x9c+slot*4;const auto clock=get(u,field);
    const auto elapsed=service.network_clock()-clock;
    if(signed_word(get(b,0x84))>signed_word(get(b,0x4c7c)) || signed_word(get(b,0x1254)+1)>signed_word(get(b,0x4c80))) {put(u,0xb4+slot,1,1);continue;}
    if(signed_word(elapsed)<limits.add && clock) {
      const auto pending=0x793c+user*9;
      if(b[pending]==std::byte{} || !equal(b.subspan(pending+1,8),u.subspan(1,8)))continue;
      put(u,0xb4+slot,1,1);continue;
    }
    if(std::to_integer<unsigned>(b[0x4e84])&1) {
      const auto team=choose_team(b,service.allowed_team_mask(b));put(u,0x8c,team,1);
      vista_controller_team_write(owner.controllers,0,std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(team))));
    }
    if(add_player(b,user,u,service.join_context_byte(),service))put(u,field,service.network_clock());
    else put(u,0xb4+slot,1,1);
  }
  (void)record;
  if(local_state(b))vista_session_publish_final_players(owner,service);
}
} // namespace halo2

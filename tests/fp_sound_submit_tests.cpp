#include "halo2/fp_sound_submit.hpp"
#include "halo2/sound_object_request.hpp"
#include "halo2/sound_submit.hpp"
#include "halo2/sound_metadata_store.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {
using namespace halo2;
using Bytes=std::vector<std::byte>;
void check(bool v,const std::string &s){if(!v)throw std::runtime_error(s);}
std::uint32_t hex(std::istringstream &in){std::uint32_t n;check(bool(in>>std::hex>>n),"Incomplete composed fixture");return n;}
Bytes bytes(std::istringstream &in){std::string s;check(bool(in>>s)&&s.size()%2==0,"Invalid literal bytes");Bytes b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;}
template<std::size_t N>std::array<std::byte,N> packet(std::istringstream &in){const auto b=bytes(in);check(b.size()==N,"Literal extent mismatch");std::array<std::byte,N> a;std::copy(b.begin(),b.end(),a.begin());return a;}
std::uint32_t word(std::span<const std::byte> b,std::size_t p,unsigned n=4){check(p+n<=b.size(),"Word bounds");std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[p+i])<<(8*i);return v;}
struct Local{std::uint32_t player,unit;std::int32_t mode;};
struct Object{std::uint8_t type,flags;std::uint32_t parent,associated;};
struct Case {
 std::string name;unsigned tick,view;FirstPersonHandBinding hand;
 FirstPersonSoundState before,after;std::uint16_t bsp,cw;FirstPersonAnimationEvent event;
 std::array<std::uint32_t,4> roots;SoundSubmitState submit_state;
 std::uint32_t sound;std::array<std::byte,20> definition;std::array<std::byte,92> sound_class;
 std::array<Local,4> locals;std::map<std::uint32_t,Object> objects;
 std::map<std::uint32_t,Bytes> memory;FirstPersonRawSoundBacking backing,expected;
 std::array<std::byte,160> at_submit;
 std::vector<FirstPersonRawSoundStore> raw_stores;
 std::vector<SoundObjectRequestStore> object_stores;
};
std::vector<Case> load(const char *path){
 std::ifstream in(path);check(bool(in),"Original composed fixture unavailable");std::string line;
 check(bool(std::getline(in,line))&&line=="FP_RAW_SOUND_COMPOSITION_V1","Wrong fixture schema");std::vector<Case> cases;
 while(std::getline(in,line)){
  std::istringstream row(line);std::string kind;row>>kind;
  if(kind=="CASE"){
   Case c{};row>>c.name>>std::dec>>c.tick;c.hand.weapon=hex(row);c.hand.activity=hex(row);
   c.before={hex(row),std::uint16_t(hex(row))};c.after={hex(row),std::uint16_t(hex(row))};row>>std::dec>>c.view;c.bsp=std::uint16_t(hex(row));c.cw=std::uint16_t(hex(row));const auto e=packet<20>(row);
   c.event={std::uint16_t(word(e,0,2)),std::bit_cast<std::int16_t>(std::uint16_t(word(e,2,2))),word(e,4),word(e,8),word(e,12),word(e,16)};cases.push_back(c);
  }else{
   check(!cases.empty(),"No source case");auto &c=cases.back();
   if(kind=="ROOTS")for(auto &v:c.roots)v=hex(row);
   else if(kind=="SUBMIT_STATE")c.submit_state={hex(row),hex(row),hex(row),hex(row),hex(row),hex(row),hex(row),hex(row)};
   else if(kind=="DEFINITION"){c.sound=hex(row);c.definition=packet<20>(row);c.sound_class=packet<92>(row);}
   else if(kind=="LOCAL"){unsigned v;row>>v;const auto p=hex(row),u=hex(row),m=hex(row);c.locals.at(v)={p,u,std::bit_cast<std::int32_t>(m)};}
   else if(kind=="OBJECT"){const auto d=hex(row),t=hex(row),p=hex(row),f=hex(row),a=hex(row);c.objects.emplace(d,Object{std::uint8_t(t),std::uint8_t(f),p,a});}
   else if(kind=="MEM"){const auto p=hex(row);c.memory.emplace(p,bytes(row));}
   else if(kind=="BACKING"){c.backing.event={packet<40>(row),packet<48>(row)};c.backing.location_tail=packet<20>(row);c.backing.request=packet<160>(row);}
   else if(kind=="AFTER"){c.expected=c.backing;c.expected.event={packet<40>(row),packet<48>(row)};c.expected.request=packet<160>(row);}
   else if(kind=="AT_SUBMIT")c.at_submit=packet<160>(row);
   else if(kind=="RAW_STORE"||kind=="OBJECT_STORE"){
    const auto pc=hex(row);std::size_t p;unsigned n;row>>std::dec>>p>>n;const auto b=hex(row),a=hex(row);
    if(kind=="RAW_STORE")c.raw_stores.push_back({pc,p,n,b,a});else c.object_stores.push_back({pc,p,n,b,a});
   }else check(kind=="END","Unexpected fixture record");
  }
 }return cases;
}
std::span<std::byte> memory(Case &c,std::uint32_t address,std::size_t n){
 auto it=c.memory.upper_bound(address);check(it!=c.memory.begin(),"Missing source backing address "+std::to_string(address));--it;const auto p=std::size_t(address-it->first);
 check(p<=it->second.size()&&n<=it->second.size()-p,"Missing source backing extent "+std::to_string(address)+"/"+std::to_string(n));return std::span(it->second).subspan(p,n);
}
void run(std::vector<Case> cases,const OriginalData &data,const SourceSoundMetadataProviders &metadata){
 check(data.first_person_sound_constants&&data.sound_object_request_constants&&data.sound_submit_operands,"Actual source executable operands unavailable");
 std::size_t count=0,world_calls=0,raw_calls=0,raw_writes=0,world_writes=0;
 for(auto &c:cases){
  check(metadata.definition(c.sound)==c.definition&&metadata.sound_class(std::bit_cast<std::int8_t>(c.definition[2]))==c.sound_class,"Actual Cache metadata differs from original event metadata");
  const auto before_memory=c.memory;auto state=c.before;auto backing=c.backing;
  FirstPersonRawSoundServices service;auto &s=service.owners;
  s.sound_definition=metadata.definition;s.sound_class=metadata.sound_class;
  s.object_header_type=[&](auto d){return c.objects.at(d).type;};
  s.weapon_association=[&](auto d){const auto &o=c.objects.at(d);return SoundWeaponAssociation{o.flags,o.associated};};
  s.object_parent=[&](auto d){return c.objects.at(d).parent;};
  s.local_player=[&](auto v){return c.locals.at(v).player;};
  s.player_current_unit=[&](auto p){for(const auto &l:c.locals)if(l.player==p)return l.unit;throw FormatError("Unowned local player");};
  s.camera_mode=[&](auto v){return c.locals.at(v).mode;};
  SoundObjectRequestServices world;world.memory={c.roots[0],c.roots[1],c.roots[2],c.roots[3],[&](auto a,auto n){return std::span<const std::byte>(memory(c,a,n));}};world.associations=s;
  std::vector<SoundObjectRequestStore> object_stores;
  s.owner_request=[&](auto owner,auto sound,auto callbacks,auto marker,auto location){
   check(owner==c.hand.weapon&&sound==c.sound&&callbacks==data.first_person_sound_constants->ordinary_callbacks,"Actual event world-owner ABI differs");++world_calls;
   return source_sound_object_request(owner,sound,marker,location,*data.sound_object_request_constants,c.cw,world,[&](const auto &q){object_stores.push_back(q);});
  };
  SoundSubmitServices submit;
  constexpr std::uint32_t raw_address=0x70000000;
  submit.memory=[&](auto a,auto n)->std::span<std::byte>{if(a>=raw_address&&std::uint64_t(a-raw_address)+n<=160)return std::span(backing.request).subspan(a-raw_address,n);return memory(c,a,n);};
  service.submit=[&](auto sound,auto request){
   ++raw_calls;check(sound==c.sound&&std::ranges::equal(request,c.at_submit),"Complete raw160 request differs at original submit phase");
   // No admit/promote/create/controller-seed/effect reply is supplied. Whole
   // source21F220 must returnNONE through the actual cold source state.
   return source_sound_submit(c.submit_state,sound,raw_address,data.sound_submit_operands->constants,submit);
  };
  std::vector<FirstPersonRawSoundStore> stores;
  check(source_first_person_sound_raw_event(state,true,c.view,c.hand,c.event,c.bsp,*data.first_person_sound_constants,backing,service,[&](const auto &q){stores.push_back(q);}),"Actual original event unexpectedly rejected");
  check(state==c.after,"Retained FP identifier/activity differs");
  check(backing.event.marker==c.expected.event.marker&&backing.event.location==c.expected.event.location&&backing.location_tail==c.expected.location_tail&&backing.request==c.expected.request,"Complete original caller packets differ");
  check(stores==c.raw_stores,"Ordered raw construction stores differ");check(object_stores==c.object_stores,"Ordered complete world stores differ");check(c.memory==before_memory,"Cold read-only source backing changed");
  raw_writes+=stores.size();world_writes+=object_stores.size();++count;
  // A missing raw owner must remain unavailable after construction, without
  // writing the completed retained hand result.
  auto unavailable=service;unavailable.submit={};auto failed_backing=c.backing;auto failed_state=c.before;bool failed=false;
  try{source_first_person_sound_raw_event(failed_state,true,c.view,c.hand,c.event,c.bsp,*data.first_person_sound_constants,failed_backing,unavailable);}catch(const FormatError &){failed=true;}
  check(failed&&failed_state==c.before,"Missing whole submit fabricated a completed event");
 }
 check(count==3&&raw_calls==3&&world_calls==6,"Incomplete actual authored composition coverage");
 std::cout<<"PASS 3 authored FP events, 3 complete world/raw-submit compositions, "<<world_writes<<" world stores, "<<raw_writes<<" raw stores; full160 caller bytes/marker/location/retained state exact; real Cache/image providers; replayed owner/submit replies=0; cold rejection natural; live world/listener/device admission remains unavailable.\n";
}
}
int main(int argc,char **argv)try{
 check(argc==4,"Usage: fp_sound_submit_tests <fixtures.tsv> <ownedLockout.map> <owned1.1.xbe>");
 auto cache=std::make_shared<halo2::Cache>(argv[2]);const auto data=halo2::OriginalData::load_xbe(argv[3]);run(load(argv[1]),*data,halo2::SoundMetadataStore(cache).providers());
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

#include "halo2/fp_sound_request.hpp"
#if HALO2_FP_SOUND_CACHE_TESTS
#include "halo2/sound_variation.hpp"
#endif
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {
using namespace halo2;
void check(bool value,const std::string &why) {if(!value)throw std::runtime_error(why);}
std::uint32_t hex(std::istringstream &row) {
  std::uint32_t value;check(bool(row>>std::hex>>value),"Incomplete original sound fixture");return value;
}
template<std::size_t N> std::array<std::byte,N> raw(std::istringstream &row) {
  std::string s;check(bool(row>>s)&&s.size()==N*2,"Incomplete original sound packet");
  std::array<std::byte,N> out;
  for(std::size_t i=0;i<N;++i)out[i]=std::byte(std::stoul(s.substr(i*2,2),nullptr,16));
  return out;
}
template<std::size_t N> std::uint32_t word(const std::array<std::byte,N> &data,
                                         std::size_t p,unsigned size=4) {
  std::uint32_t value=0;for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(data[p+i])<<(8*i);
  return value;
}
struct Object {std::uint8_t type,flags;std::uint32_t parent,associated;};
struct Local {std::uint32_t player,unit;std::int32_t mode;};
struct Case {
  std::string name;std::uint32_t weapon,activity;
  FirstPersonAnimationEvent event;
  FirstPersonSoundState before,after;
  std::uint16_t current_bsp;
  FirstPersonSoundBacking backing;
  std::array<std::byte,48> owner_result;
  SourceSoundPlayRequest play;
};
struct Gate {
  std::string name;unsigned view;FirstPersonAnimationEvent event;
  std::uint32_t weapon,activity;FirstPersonSoundState before,after;bool processed;
};
struct Marker {
  std::uint32_t sound,owner;std::array<std::byte,40> before,after;bool weapon;
};
struct Audible {std::uint32_t owner,view;std::uint8_t before,after;};
struct Fixture {
  FirstPersonSoundConstants constants;
  std::uint16_t current_bsp;
  std::map<std::uint32_t,std::array<std::byte,20>> definitions;
  std::map<std::int8_t,std::array<std::byte,92>> classes;
  std::map<std::uint32_t,Object> objects;
  std::array<Local,4> locals;
  std::vector<Case> cases;std::vector<Gate> gates;std::vector<Marker> markers;std::vector<Audible> audible;
};
FirstPersonAnimationEvent event(const std::array<std::byte,20> &data) {
  return {std::uint16_t(word(data,0,2)),std::bit_cast<std::int16_t>(std::uint16_t(word(data,2,2))),
      word(data,4),word(data,8),word(data,12),word(data,16)};
}
FirstPersonSoundState state(const std::array<std::byte,6> &data) {
  return {word(data,0),std::uint16_t(word(data,4,2))};
}
Fixture load(const char *path) {
  std::ifstream file(path);check(bool(file),"Literal original sound fixture unavailable");Fixture f;
  for(std::string line;std::getline(file,line);) {
    if(line.empty()||line[0]=='#')continue;
    std::istringstream row(line);std::string kind;row>>kind;
    if(kind=="CONSTANTS") {
      f.constants.marker_position=raw<12>(row);f.constants.marker_forward=raw<12>(row);
      f.constants.ordinary_callbacks=hex(row);f.constants.weapon_callbacks=hex(row);
    } else if(kind=="CURRENT_BSP") {
      f.current_bsp=std::uint16_t(hex(row));
    } else if(kind=="DEFINITION") {
      auto tag=hex(row);auto data=raw<20>(row);f.definitions.emplace(tag,data);
      f.classes.emplace(std::bit_cast<std::int8_t>(data[2]),raw<92>(row));
    } else if(kind=="LOCAL") {
      unsigned view;row>>view;auto player=hex(row),unit=hex(row),mode=hex(row);
      f.locals.at(view)={player,unit,std::bit_cast<std::int32_t>(mode)};
    } else if(kind=="OBJECT") {
      auto datum=hex(row),type=hex(row),parent=hex(row),flags=hex(row),associated=hex(row);
      f.objects.emplace(datum,Object{std::uint8_t(type),std::uint8_t(flags),parent,associated});
    } else if(kind=="CASE") {
      Case c{};row>>c.name;c.weapon=hex(row);c.activity=hex(row);c.event=event(raw<20>(row));
      c.before={hex(row),std::uint16_t(hex(row))};c.after={hex(row),std::uint16_t(hex(row))};
      c.current_bsp=std::uint16_t(hex(row));
      f.cases.push_back(c);
    } else if(kind=="BACKING") {
      f.cases.back().backing={raw<40>(row),raw<48>(row)};
    } else if(kind=="OWNER_RESULT") {
      f.cases.back().owner_result=raw<48>(row);
    } else if(kind=="PLAY") {
      auto &p=f.cases.back().play;p.sound=f.cases.back().event.tag;
      p.flags=hex(row);p.priority=hex(row);p.location=raw<48>(row);p.owner=hex(row);
      p.callbacks=hex(row);p.marker=raw<40>(row);p.data_size=std::uint16_t(hex(row));p.platform_argument=hex(row);
    } else if(kind=="GATE") {
      Gate c{};row>>c.name>>c.view;c.event=event(raw<20>(row));c.weapon=hex(row);c.activity=hex(row);
      c.before=state(raw<6>(row));c.after=state(raw<6>(row));unsigned p;row>>p;c.processed=p;f.gates.push_back(c);
    } else if(kind=="MARKER") {
      Marker c{};c.sound=hex(row);c.owner=hex(row);c.before=raw<40>(row);c.after=raw<40>(row);
      unsigned weapon;row>>weapon;c.weapon=weapon;f.markers.push_back(c);
    } else if(kind=="AUDIBILITY") {
      Audible c{};c.owner=hex(row);c.before=std::uint8_t(hex(row));c.after=std::uint8_t(hex(row));c.view=hex(row);
      f.audible.push_back(c);
    } else check(kind=="ENDCASE","Unknown original sound fixture row");
  }
  return f;
}
FirstPersonSoundServices services(const Fixture &f) {
  FirstPersonSoundServices s;
  s.sound_definition=[&](auto sound){return f.definitions.at(sound);};
  s.sound_class=[&](auto index){return f.classes.at(index);};
  s.object_header_type=[&](auto owner){return f.objects.at(owner).type;};
  s.weapon_association=[&](auto owner){const auto &o=f.objects.at(owner);return SoundWeaponAssociation{o.flags,o.associated};};
  s.object_parent=[&](auto owner){return f.objects.at(owner).parent;};
  s.local_player=[&](auto view){return f.locals.at(view).player;};
  s.player_current_unit=[&](auto player){
    for(const auto &l:f.locals)if(l.player==player)return l.unit;
    throw std::runtime_error("Unrecorded source player");
  };
  s.camera_mode=[&](auto view){return f.locals.at(view).mode;};
  return s;
}
void same(const SourceSoundPlayRequest &a,const SourceSoundPlayRequest &b,const std::string &name) {
  check(a.sound==b.sound&&a.flags==b.flags&&a.priority==b.priority&&a.owner==b.owner&&
      a.callbacks==b.callbacks&&a.platform_argument==b.platform_argument&&a.data_size==b.data_size&&
      a.location==b.location&&a.marker==b.marker,name+" complete represented play request differs");
}
void run(const Fixture &f) {
  std::size_t owners=0,submits=0;
  for(const auto &c:f.cases) {
    auto s=services(f);auto retained=c.before;auto backing=c.backing;
    std::vector<std::string> order;
    s.owner_request=[&](auto owner,auto sound,auto callbacks,auto marker,auto location) {
      ++owners;order.push_back("owner");
      check(owner==c.weapon&&sound==c.event.tag&&callbacks==c.play.callbacks,"Source owner ABI differs");
      check(std::equal(marker.begin(),marker.end(),c.play.marker.begin()),"Full original marker differs");
      for(std::size_t i=0;i<location.size();++i)
        check(location[i]==c.backing.location[i],"Original owner input differs at byte "+std::to_string(i)+
              " native="+std::to_string(std::to_integer<unsigned>(location[i]))+
              " original="+std::to_string(std::to_integer<unsigned>(c.backing.location[i])));
      std::copy(c.owner_result.begin(),c.owner_result.end(),location.begin());
      return true; // Complete recorded original 18DDE0 returns AL1 in these three cases.
    };
    s.submit=[&](const auto &p) {
      ++submits;order.push_back("submit");same(p,c.play,c.name);
      check(retained==c.before,"Retained hand changed before original submit returned");
      return c.after.identifier; // Original unavailable system returns NONE, never a fabricated voice.
    };
    FirstPersonHandBinding hand;hand.weapon=c.weapon;hand.activity=c.activity;
    check(source_first_person_sound_event(retained,true,0,hand,c.event,c.current_bsp,f.constants,backing,s),"Authored event skipped");
    check(retained==c.after,c.name+" original hand stores differ");
    check(backing.location==c.owner_result,c.name+" original location differs");
    check(order==std::vector<std::string>{"owner","submit"},"Original owning order differs");
  }
  for(const auto &c:f.gates) {
    auto s=services(f);auto retained=c.before;FirstPersonSoundBacking backing{};
    unsigned calls=0;
    s.owner_request=[&](auto owner,auto,auto,auto,auto) {
      ++calls;check(owner==0xffffffffu,"Unrecorded rejected source owner");
      return false; // Actual unbound-view BBCB0 -> 18DDE0 rejects, no scheduler invocation.
    };
    FirstPersonHandBinding hand;hand.weapon=c.weapon;hand.activity=c.activity;
    check(source_first_person_sound_event(retained,true,c.view,hand,c.event,f.current_bsp,f.constants,backing,s)==c.processed,
          c.name+" source gate differs");
    check(retained==c.after,c.name+" source gate hand stores differ");
    check(calls==unsigned(c.processed),c.name+" lower-owner invocation differs");
  }
  for(const auto &c:f.markers) {
    auto s=services(f);auto marker=c.before;
    source_first_person_sound_marker(c.sound,c.owner,f.constants,marker,s);
    check(marker==c.after,"Complete original marker helper bytes differ");
    check(source_sound_uses_weapon_callbacks(c.sound,c.owner,s)==c.weapon,"Original callback table branch differs");
  }
  for(const auto &c:f.audible) {
    auto s=services(f);std::uint32_t view=0xaabbccddu;
    check(source_sound_audibility(c.owner,c.before,s,&view)==c.after&&view==c.view,"Original audibility differs");
  }
  // Native missing-provider controls are separately labeled, not original
  // successful playback fixtures. Missing boundaries leave hand state intact.
  const auto &c=f.cases.at(0);
  for(unsigned missing=0;missing<2;++missing) {
    auto s=services(f);FirstPersonSoundBacking backing=c.backing;auto retained=c.before;
    if(missing)s.owner_request=[&](auto,auto,auto,auto,auto location) {
      std::copy(c.owner_result.begin(),c.owner_result.end(),location.begin());return true;
    };
    FirstPersonHandBinding hand;hand.weapon=c.weapon;hand.activity=c.activity;bool stopped=false;
    try {source_first_person_sound_event(retained,true,0,hand,c.event,c.current_bsp,f.constants,backing,s);}
    catch(const FormatError &){stopped=true;}
    check(stopped&&retained==c.before,"Unavailable lower source provider fabricated completion");
  }
  std::cout<<"PASS original FP sound request: "<<f.cases.size()<<" authored requests, "<<f.gates.size()
      <<" event controls, "<<f.markers.size()<<" full marker controls, "<<f.audible.size()
      <<" audibility controls, "<<owners<<" recorded complete owner leaves, "<<submits
      <<" actual rejected submit leaves; 2 native unavailable-provider controls\n";
}
}
int main(int argc,char **argv) {
  try {
    check(argc==2||argc==3,"Usage: fp_sound_request_tests <literal original fixtures> [owned map]");
    auto fixture=load(argv[1]);run(fixture);
#if HALO2_FP_SOUND_CACHE_TESTS
    if(argc==3) {
      Cache cache(argv[2]);
      for(auto &[datum,definition]:fixture.definitions) {
        auto actual=cache.load_sound_variation(datum);
        check(actual.sound==definition,"Actual Cache sound request definition differs");
        const auto index=std::bit_cast<std::int8_t>(actual.sound[2]);
        check(fixture.classes.at(index)==actual.sound_class,"Actual Cache sound class differs");
        definition=actual.sound;fixture.classes.at(index)=actual.sound_class;
      }
      run(fixture);
      std::cout<<"PASS actual Cache metadata providers: "<<fixture.definitions.size()<<" sound/class definitions\n";
    }
#else
    check(argc==2,"Actual Cache provider test was not compiled");
#endif
  }
  catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

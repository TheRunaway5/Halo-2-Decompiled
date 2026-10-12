#include "halo2/death_camera.hpp"
#include "halo2/observer_motion.hpp"
#include "halo2/observer_world.hpp"
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string&s){
  check(s.size()%2==0,"Complete original byte record");std::vector<std::byte>v;
  for(std::size_t i=0;i<s.size();i+=2)v.push_back(std::byte(word(s.substr(i,2))));return v;
}
template<class T>T decode(const std::string&s){
  const auto v=bytes(s);check(v.size()==sizeof(T),"Original byte record extent");T t;
  std::memcpy(&t,v.data(),sizeof t);return t;
}
void state(const halo2::DeathCameraState&s,const std::string&expected,const std::string&where){
  static_assert(sizeof(s)==64);const auto v=bytes(expected);
  check(v.size()==64,"Original death controller extent");
  const auto*actual=reinterpret_cast<const std::byte*>(&s);
  for(unsigned i=0;i<62;++i)check(actual[i]==v[i],where+" controller byte "+std::to_string(i));
}
}
int main(int argc,char**argv){try{
  if(argc<2||argc>3)throw std::runtime_error("Supply unchanged original death camera and optional observer branch fixtures");
  if(!std::filesystem::exists(argv[1]))return 77;
  std::ifstream input(argv[1]);halo2::DeathCameraConstants c;halo2::SourceObserverConstants observer_constants;
  halo2::SourceObserverWorldConstants world_constants;
  std::size_t constructors=0,frames=0,distances=0,casts=0,observers=0,worlds=0;
  std::size_t attached_cases=0;
  // The original fixture binds local player datum0 to the source-owned unit10001.
  const std::array<halo2::DeathCameraPlayer,1> bank{{{0,0xffffffffu,0x10001u,-1}}};
  for(std::string line;std::getline(input,line);){std::istringstream in(line);std::string kind;in>>kind;
    if(kind=="K"){
      std::array<float*,18>fields{&c.default_fov,&c.random_scale,&c.distance_scale,&c.distance_bias,
        &c.full_turn,&c.pitch_scale,&c.pitch_bias,&c.transition_seconds,&c.watch_seconds,
        &c.attached_seconds,&c.one,&c.half,&c.probe_scale,&c.probe_tolerance,&c.epsilon,
        &c.minimum_coordinate,&c.maximum_coordinate,&c.maximum_distance};
      for(auto*p:fields){std::string value;in>>value;*p=std::bit_cast<float>(word(value));}
    }else if(kind=="G"){
      auto&v=observer_constants.scalar;
      std::array<float*,23>fields{&v.default_fov,&v.packet_minimum,&v.packet_maximum,&v.render_minimum,&v.render_maximum,
        &v.magnified_minimum,&v.magnified_maximum,&v.one,&v.packet_multiplier,&v.transition_seconds,&v.frame_dt_multiplier,
        &v.half,&v.three,&v.five,&v.fifteen,&v.six,&v.ten,&v.twenty,&v.two,&v.four,&v.twelve,&v.seven,&v.acceleration_cap};
      for(auto*p:fields){std::string value;in>>value;*p=std::bit_cast<float>(word(value));}
      std::string numerator,packet_width,motion_width,caps;in>>numerator>>packet_width>>motion_width>>caps;
      v.reciprocal_numerator=std::bit_cast<double>(std::stoull(numerator,nullptr,16));
      observer_constants.packet_width=decode<std::array<std::int16_t,6>>(packet_width);
      observer_constants.motion_width=decode<std::array<std::int16_t,6>>(motion_width);
      observer_constants.acceleration_caps=decode<std::array<float,6>>(caps);
      for(auto*p:{&observer_constants.epsilon,&observer_constants.orthogonal_tolerance,&observer_constants.quarter,
          &observer_constants.pi,&observer_constants.full_turn,&observer_constants.velocity_cubic}){
        std::string value;in>>value;*p=std::bit_cast<float>(word(value));}
    }else if(kind=="H"){
      for(auto*p:{&world_constants.maximum_initial_distance,&world_constants.maximum_distance,&world_constants.aspect,
          &world_constants.margin,&world_constants.vertical_bias_threshold,&world_constants.packet_maximum_seconds}){
        std::string value;in>>value;*p=std::bit_cast<float>(word(value));}
      std::string test,forward,up,zero;in>>test>>forward>>up>>zero;
      world_constants.vertical_bias_test=std::bit_cast<double>(std::stoull(test,nullptr,16));
      world_constants.default_forward=decode<halo2::Vec3>(forward);
      world_constants.default_up=decode<halo2::Vec3>(up);
      world_constants.zero=decode<halo2::Vec3>(zero);
    }else if(kind=="C"){
      std::string seed,position,before,after,rng_after;in>>seed>>position>>before>>after>>rng_after;
      auto rng=word(seed);auto s=halo2::source_death_camera_create(decode<halo2::Vec3>(position),0,0,bank,rng,c);
      state(s,after,"constructor "+seed);check(rng==word(rng_after),"Original three-draw RNG state");++constructors;
    }else if(kind=="F"||kind=="A"){
      std::string seed,frame,dt,pos,up,node,before,after,packet;int rate;
      in>>seed>>frame>>dt>>rate>>pos>>up>>node>>before>>after>>packet;
      unsigned target_type=0,target_flags=0;
      if(kind=="A"){std::string type,flags;in>>type>>flags;target_type=word(type);target_flags=word(flags);}
      auto s=decode<halo2::DeathCameraState>(before);
      halo2::DeathCameraFrame f{std::bit_cast<float>(word(dt)),std::int16_t(rate),
        decode<halo2::Vec3>(pos),decode<halo2::Vec3>(up),false,false,true,true};
      const auto actual=halo2::source_death_camera_packet(s,bank,[&](auto d)->std::optional<halo2::DeathCameraTarget>{
        if(d!=0x10001u)return {};return halo2::DeathCameraTarget{decode<halo2::Vec3>(node),std::uint8_t(target_type),std::uint16_t(target_flags)};},f,c,observer_constants,world_constants);
      state(s,after,"frame "+seed+"/"+frame);
      const auto expected=decode<halo2::DeathCameraPacket>(packet);
      for(unsigned i=0;i<actual.size();++i)check(actual[i]==expected[i],"frame "+seed+"/"+frame+" packet word "+std::to_string(i)+
        " actual="+std::to_string(actual[i])+" expected="+std::to_string(expected[i]));
      ++frames;
      if(kind=="A")++attached_cases;
    }else if(kind=="O"){
      std::string before,packet,adopted,dt,after;in>>before>>packet>>adopted>>dt>>after;
      auto s=decode<halo2::SourceObserverState>(before);auto p=decode<halo2::DeathCameraPacket>(packet);
      const auto compare=[&](const std::string&expected,const std::string&phase){
        const auto e=decode<halo2::SourceObserverState>(expected);
        // Source pointer+04 is a submission token; the typed native call owns
        // its actual packet separately, without a32-bit host pointer alias.
        for(unsigned i=0;i<s.size();++i)if(i!=1)check(s[i]==e[i],
            "Observer "+std::to_string(observers)+" "+phase+" word "+std::to_string(i)+
            " actual="+std::to_string(s[i])+" expected="+std::to_string(e[i]));
      };
      halo2::source_observer_register_adopt(s,p,observer_constants);compare(adopted,"adoption");
      halo2::source_observer_motion(s,std::bit_cast<float>(word(dt)),observer_constants);compare(after,"motion");++observers;
    }else if(kind=="W"){
      std::string before,after,current,saved;int bsp,clusters,player_flags;unsigned count,locations,visibility,fog;
      in>>before>>after>>bsp>>clusters>>current>>player_flags>>saved>>count>>locations>>visibility>>fog;
      auto s=decode<halo2::SourceObserverState>(before);const auto expected=decode<halo2::SourceObserverState>(after);
      halo2::SourceObserverWorldFrame frame{0,bsp,clusters,word(current),std::uint16_t(player_flags),decode<halo2::DeathCameraSavedClusters>(saved)};
      unsigned queries=0,located=0,requested=0;
      halo2::SourceObserverWorldServices services;
      services.fog_side=[&](auto){return bool(fog);};
      services.collision=[&](auto flags,auto origin,auto delta)->std::optional<float>{
        check(queries<count,"No extra world collision query");std::string record;
        check(bool(std::getline(input,record)),"Complete original world queries");std::istringstream q(record);
        std::string kind,ef,eo,ed,fraction;unsigned hit;q>>kind>>ef>>eo>>ed>>hit>>fraction;
        check(kind=="Q"&&flags==word(ef),"Original world collision flags");
        check(std::bit_cast<std::array<std::uint32_t,3>>(origin)==decode<std::array<std::uint32_t,3>>(eo),"Original world collision origin");
        check(std::bit_cast<std::array<std::uint32_t,3>>(delta)==decode<std::array<std::uint32_t,3>>(ed),
          "World "+std::to_string(worlds)+" collision delta "+std::to_string(queries));
        ++queries;return hit?std::optional{std::bit_cast<float>(word(fraction))}:std::nullopt;
      };
      services.locate=[&](auto position){
        check(located<locations,"No extra world location");std::string record;check(bool(std::getline(input,record)),"Complete original location");
        std::istringstream q(record);std::string kind,ep,result;q>>kind>>ep>>result;
        check(kind=="Y"&&std::bit_cast<std::array<std::uint32_t,3>>(position)==decode<std::array<std::uint32_t,3>>(ep),
          "World "+std::to_string(worlds)+" location position");++located;return decode<halo2::SourceObserverLocation>(result);
      };
      services.request_visibility=[&](auto cluster){
        check(requested<visibility,"No extra world visibility request");std::string record;check(bool(std::getline(input,record)),"Complete original visibility");
        std::istringstream q(record);std::string kind;int expected;q>>kind>>expected;
        check(kind=="V"&&cluster==expected,"Original world visibility request");++requested;
      };
      halo2::source_observer_world(s,frame,services,observer_constants,c,world_constants);
      check(queries==count&&located==locations&&requested==visibility,"All original world services consumed");
      for(unsigned i=0;i<s.size();++i)check(s[i]==expected[i],"World "+std::to_string(worlds)+" word "+std::to_string(i)+
          " actual="+std::to_string(s[i])+" expected="+std::to_string(expected[i]));++worlds;
    }else if(kind=="D"){
      std::string forward,up,target,distance,margin,result;unsigned fog,count;
      in>>forward>>up>>target>>distance>>margin>>fog>>result>>count;
      unsigned queried=0;
      const auto actual=halo2::source_death_camera_distance(decode<halo2::Vec3>(forward),
          decode<halo2::Vec3>(up),decode<halo2::Vec3>(target),std::bit_cast<float>(word(distance)),
          std::bit_cast<float>(word(margin)),fog,[&](auto flags,auto origin,auto delta)->std::optional<float>{
        check(queried<count,"No extra native obstruction query");std::string record;
        check(bool(std::getline(input,record)),"Complete original obstruction queries");std::istringstream q(record);
        std::string kind,expected_flags,expected_origin,expected_delta,fraction;unsigned hit;
        q>>kind>>expected_flags>>expected_origin>>expected_delta>>hit>>fraction;
        check(kind=="Q","Ordered original query record");
        check(flags==word(expected_flags),"Original obstruction collision flags");
        check(std::bit_cast<std::array<std::uint32_t,3>>(origin)==
            std::bit_cast<std::array<std::uint32_t,3>>(decode<halo2::Vec3>(expected_origin)),"Original obstruction query origin");
        check(std::bit_cast<std::array<std::uint32_t,3>>(delta)==
            std::bit_cast<std::array<std::uint32_t,3>>(decode<halo2::Vec3>(expected_delta)),
            "Obstruction "+std::to_string(distances)+" query "+std::to_string(queried)+" original delta");
        ++queried;++casts;return hit?std::optional{std::bit_cast<float>(word(fraction))}:std::nullopt;
      },c);
      check(queried==count,"All original obstruction queries consumed");
      check(std::bit_cast<std::uint32_t>(actual)==word(result),"Original obstruction distance "+std::to_string(distances));
      ++distances;
    }else throw std::runtime_error("Unknown original camera fixture record");
  }
  if(attached_cases)check(constructors==0&&frames==68&&attached_cases==68&&distances==0&&observers==0&&worlds==0,
      "Complete independent attached-camera fixture");
  else check(constructors==4&&frames==400&&distances==400&&observers==400&&worlds==400,"Complete independent original camera fixture");
  std::size_t branches=0;
  if(argc==3){
    if(!std::filesystem::exists(argv[2]))return 77;
    std::ifstream branch_input(argv[2]);
    const auto compare=[&](const auto&actual,const auto&expected,const std::string&phase,bool pointer){
      for(unsigned i=0;i<actual.size();++i)if(!pointer||i!=1)check(actual[i]==expected[i],
        "Observer branch "+std::to_string(branches)+" "+phase+" word "+std::to_string(i)+
        " actual="+std::to_string(actual[i])+" expected="+std::to_string(expected[i]));
    };
    for(std::string line;std::getline(branch_input,line);){std::istringstream in(line);std::string kind;in>>kind;
      if(kind=="R"){
        std::string before,after;int bsp;in>>before>>bsp>>after;auto s=decode<halo2::SourceObserverState>(before);
        halo2::source_observer_reset(s,std::int16_t(bsp),observer_constants,world_constants);
        compare(s,decode<halo2::SourceObserverState>(after),"reset",false);
      }else if(kind=="P"){
        std::string before,after;in>>before>>after;auto p=decode<halo2::DeathCameraPacket>(before);
        halo2::source_observer_repair_packet(p,observer_constants,c,world_constants);
        compare(p,decode<halo2::DeathCameraPacket>(after),"repair",false);
      }else if(kind=="T"){
        std::string before,packet,packet_after,adopted,dt,after;in>>before>>packet>>packet_after>>adopted>>dt>>after;
        auto s=decode<halo2::SourceObserverState>(before);auto p=decode<halo2::DeathCameraPacket>(packet);
        halo2::source_observer_repair_packet(p,observer_constants,c,world_constants);
        halo2::source_observer_register_adopt(s,p,observer_constants);
        compare(p,decode<halo2::DeathCameraPacket>(packet_after),"adopted packet",false);
        compare(s,decode<halo2::SourceObserverState>(adopted),"adoption",true);
        halo2::source_observer_motion(s,std::bit_cast<float>(word(dt)),observer_constants);
        compare(s,decode<halo2::SourceObserverState>(after),"motion",true);
      }else throw std::runtime_error("Unknown original observer branch");
      ++branches;
    }
    check(branches==52,"Complete independent observer branch fixture");
  }
  // Salt, dead peers, team filtering, next slot and wraparound lifetime controls.
  const std::array<halo2::DeathCameraPlayer,5> peers{{{0x10000,0xffffffffu,0x20000,0},
    {0x70001,0x30000,0xffffffffu,0},{0x10003,0xffffffffu,0x40000,0},
    {0x50005,0x60000,0xffffffffu,1},{0x90008,0x80000,0xffffffffu,0}}};
  check(halo2::source_death_camera_next_player(peers,0x10000,0x70001,false)==0x50005,"Next occupied live peer");
  check(halo2::source_death_camera_next_player(peers,0x10000,0x70001,true)==0x90008,"Original team restriction");
  check(halo2::source_death_camera_next_player(peers,0x10000,0x90008,false)==0x70001,"Original wrap to first live peer");
  std::cout<<constructors<<" original death camera constructors, "<<frames<<" complete packets, "
      <<observers<<" full observer transitions, "<<worlds<<" world deliveries, "<<distances<<" obstruction passes, "<<casts<<" ordered casts and "<<branches<<" observer branches match\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

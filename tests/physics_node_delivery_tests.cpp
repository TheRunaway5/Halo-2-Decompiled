#include "halo2/physics_node_delivery.hpp"
#include "halo2/death_camera_binding.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v,const std::string&s){if(!v)throw std::runtime_error(s);}
std::uint32_t word(const std::string&s){return std::stoul(s,nullptr,16);}
struct Memory:halo2::SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::byte>data;
 void read(std::uint32_t p,std::span<std::byte>out)const override{
  for(auto&b:out){const auto i=data.find(p++);check(i!=data.end(),"Missing actual physical-node input at "+std::to_string(p-1));b=i->second;}}
 void write(std::uint32_t p,std::span<const std::byte>in)override{
  for(auto b:in){auto i=data.find(p++);check(i!=data.end(),"Missing actual physical-node output at "+std::to_string(p-1));i->second=b;}}
};
}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("Supply actual original physical-node fixture");
 if(!std::filesystem::exists(argv[1]))return 77;
 std::ifstream in(argv[1]);Memory m;std::map<std::uint32_t,std::byte>expected;
 struct Target {std::uint32_t pool,datum,body;std::string node;unsigned type,flags;};
 std::vector<Target> targets;
 std::uint32_t player_pool{};
 std::vector<halo2::DeathCameraPlayer> expected_players;
 struct Selection {std::uint32_t own,watched,result;bool team;};
 std::vector<Selection> selections;
 std::uint32_t component{},mask{},scratch_mask{},scratch_body{};
 for(std::string line;std::getline(in,line);){std::istringstream row(line);std::string kind;row>>kind;
  if(kind=="C"){std::string a,b,c,d;row>>a>>b>>c>>d;component=word(a);mask=word(b);scratch_mask=word(c);scratch_body=word(d);}
  else if(kind=="T"){std::string pool,datum,body,node;unsigned type,flags;row>>pool>>datum>>body>>node>>type>>flags;
   targets.push_back({word(pool),word(datum),word(body),node,type,flags});}
  else if(kind=="B"){std::string pool;row>>pool;player_pool=word(pool);}
  else if(kind=="R"){std::string datum,current,previous;unsigned team;row>>datum>>current>>previous>>team;
   expected_players.push_back({word(datum),word(current),word(previous),std::bit_cast<std::int8_t>(std::uint8_t(team))});}
  else if(kind=="N"){std::string own,watched,result;bool team;row>>own>>watched>>team>>result;
   selections.push_back({word(own),word(watched),word(result),team});}
  else{check(kind=="M"||kind=="A","Recognized original node record");std::string at,raw;row>>at>>raw;auto p=word(at);auto&bank=kind=="M"?m.data:expected;
   for(std::size_t i=0;i<raw.size();i+=2)bank.emplace(p++,std::byte(word(raw.substr(i,2))));}}
 halo2::source_physics_deliver_nodes_1_1(m,component,mask,0,{scratch_mask,scratch_body});
 for(const auto&[p,b]:expected)check(m.data.at(p)==b,"Physical-node output differs at "+std::to_string(p)+
   " actual="+std::to_string(std::to_integer<unsigned>(m.data.at(p)))+" expected="+std::to_string(std::to_integer<unsigned>(b)));
 for(const auto&t:targets){
  const auto actual=halo2::source_death_camera_target(m,t.pool,t.datum);
  check(bool(actual)==bool(t.body),"Original full-datum target admission");
  if(actual){const auto raw=std::bit_cast<std::array<std::byte,12>>(actual->node0);
   for(unsigned i=0;i<12;++i)check(std::to_integer<unsigned>(raw[i])==word(t.node.substr(i*2,2)),"Original physical WORLD node zero");
   check(actual->type==t.type&&actual->biped_flags==t.flags,"Original camera target fields");}
 }
 if(player_pool){const auto players=halo2::source_death_camera_players(m,player_pool);
  check(players.size()==expected_players.size(),"Original occupied player count");
  for(unsigned i=0;i<players.size();++i){const auto&a=players[i];const auto&b=expected_players[i];
   check(a.datum==b.datum&&a.current_unit==b.current_unit&&a.previous_unit==b.previous_unit&&a.team==b.team,"Original occupied player ownership");}
  for(const auto&q:selections)check(halo2::source_death_camera_next_player(players,q.own,q.watched,q.team)==q.result,"Original player selection");
 }
 std::cout<<expected.size()<<" actual original bytes match physical-first node delivery and hierarchy; "
          <<targets.size()<<" full-datum camera lookups and "<<selections.size()<<" player selections match\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

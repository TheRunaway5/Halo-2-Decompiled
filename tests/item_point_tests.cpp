#include "halo2/item_point.hpp"
#include "halo2/first_person_ik.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

using namespace halo2;
namespace {
using Row=std::vector<std::string>;
std::uint32_t word(const std::string &s,unsigned base=16) {return std::stoul(s,nullptr,base);}
float real(const std::string &s) {return std::bit_cast<float>(word(s));}
void check(bool value,const std::string &context) {if(!value)throw FormatError("Original item point differs: "+context);}
std::vector<std::byte> bytes(const std::string &s) {
  check(s.size()%2==0,"hex extent");std::vector<std::byte> r;
  for(unsigned i=0;i<s.size();i+=2)r.push_back(std::byte(word(s.substr(i,2))));return r;
}
template<class T>T object(const std::string &s) {
  const auto raw=bytes(s);check(raw.size()==sizeof(T),"source object extent");
  std::array<std::byte,sizeof(T)> data;std::copy(raw.begin(),raw.end(),data.begin());return std::bit_cast<T>(data);
}
template<class T>void same(const T &value,const std::string &expected,const std::string &context) {
  const auto raw=std::bit_cast<std::array<std::byte,sizeof(T)>>(value);const auto e=bytes(expected);
  check(e.size()==raw.size() && std::equal(raw.begin(),raw.end(),e.begin()),context);
}
Vec3 point(const Row &r,unsigned i) {return {real(r.at(i)),real(r.at(i+1)),real(r.at(i+2))};}
void point_same(Vec3 p,const Row &r,unsigned i,const std::string &context) {
  check(std::bit_cast<std::uint32_t>(p.x)==word(r.at(i)) && std::bit_cast<std::uint32_t>(p.y)==word(r.at(i+1)) && std::bit_cast<std::uint32_t>(p.z)==word(r.at(i+2)),context);
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=4)throw FormatError("Supply project, original item-point fixtures and captured metadata");
    const std::filesystem::path root(argv[1]),map=root/"assets/disc/maps/lockout.map";
    if(!std::filesystem::exists(map) || !std::filesystem::exists(argv[2]) || !std::filesystem::exists(argv[3]))return 77;
    Cache cache(map);const auto terrain=cache.load_raw_terrain_collision(0);
    check(terrain.bsps.size()==1,"explicit ordinary collision bank");
    std::map<unsigned,Row> cases,quaternions,compositions,markers,planes;
    std::map<unsigned,std::vector<Row>> stages;std::map<unsigned,std::map<unsigned,Transform>> nodes;
    std::ifstream input(argv[2]);unsigned reads=0;
    for(std::string line;std::getline(input,line);) {
      std::istringstream stream(line);Row r;for(std::string s;stream>>s;)r.push_back(s);
      if(r.empty())continue;const auto id=word(r.at(1),10);
      if(r[0]=="P")check(cases.emplace(id,r).second,"unique item case");
      else if(r[0]=="N")check(nodes[id].emplace(word(r.at(2),10),object<Transform>(r.at(3))).second,"unique original node");
      else if(r[0]=="Q")quaternions[id]=r;
      else if(r[0]=="C")compositions[id]=r;
      else if(r[0]=="M")markers[id]=r;
      else if(r[0]=="L")planes[id]=r;
      else if(r[0]=="S")stages[id].push_back(r);
      else if(r[0]=="R")++reads;
      else throw FormatError("Unknown item-point fixture row");
    }
    std::ifstream metadata(argv[3]);unsigned definitions=0;
    for(std::string line;std::getline(metadata,line);) {
      std::istringstream stream(line);Row r;for(std::string s;stream>>s;)r.push_back(s);
      if(r.empty())continue;
      const auto d=cache.load_item_point_definition(word(r.at(1)));
      check(d.model==word(r.at(2)) && d.render_model==word(r.at(3)) && d.group==int(word(r.at(4),10)),"actual definition/model/render/group chain");
      same(d.markers.at(0),r.at(5),"authored first ground marker");check(d.markers.size()==1,"original ground marker count");++definitions;
    }
    unsigned fallback=0,projected=0,current_nodes=0,owned_stages=0;
    for(const auto &[id,c]:cases) {
      const auto name=std::to_string(id);check(word(c.at(7),10)==2,"original item header type");
      std::vector<Transform> bank;
      for(const auto &[index,m]:nodes.at(id)) {check(index==bank.size(),"contiguous source node bank");bank.push_back(m);}
      current_nodes+=bank.size();
      ItemPointState state;state.datum=word(c.at(5));state.definition=word(c.at(6));state.flags=word(c.at(8));state.parent=word(c.at(9));
      state.item_flags=std::uint8_t(word(c.at(11)));state.contact_instance=std::bit_cast<std::int16_t>(std::uint16_t(word(c.at(13))));
      state.contact_surface=std::bit_cast<std::int16_t>(std::uint16_t(word(c.at(14))));state.root=point(c,15);state.current_nodes=bank;
      const auto d=cache.load_item_point_definition(state.definition);const auto result=source_item_point(d,state,terrain.bsps[0]);
      point_same(result.point,c,18,name+" final output");check(result.returned==word(c.at(21)),name+" full EAX");
      check(bool(result.marker)==markers.contains(id),name+" marker path");
      if(!result.marker) {++fallback;continue;}
      ++projected;const auto &m=*result.marker;const auto &mrow=markers.at(id);const auto &q=quaternions.at(id),&composition=compositions.at(id),&l=planes.at(id);
      const Bytes raw(d.markers.at(0));
      const auto captured_q=object<std::array<float,4>>(q.at(2));
      const std::array<float,4> authored_q{raw.f32(16),raw.f32(20),raw.f32(24),raw.f32(28)};same(authored_q,q.at(2),name+" actual quaternion operand");
      const auto local=source_marker_transform({},captured_q);same(local.basis,q.at(3),name+" original quaternion basis");
      same(bank.at(unsigned(m.node)),composition.at(2),name+" actual current node input");same(m.local,composition.at(3),name+" source local marker");
      same(m.world,composition.at(4),name+" source composed marker");
      check(m.node==int(word(mrow.at(2),10)),name+" selected node");same(m.local,mrow.at(3),name+" defined marker local");same(m.world,mrow.at(4),name+" defined marker world");
      check(std::bit_cast<std::uint32_t>(m.scalar)==word(mrow.at(5)),name+" marker scalar");
      check(std::uint16_t(m.signed_plane)==word(l.at(2)),name+" active ordinary signed plane");
      same(terrain.bsps[0].planes.at(std::uint16_t(m.signed_plane)&0x7fff),l.at(3),name+" actual raw plane");same(m.plane,l.at(4),name+" emitted plane");
      for(const auto &s:stages.at(id)) {
        if(s.at(3)=="marker_point")same(m.world.position,s.at(4),name+" stage marker point");
        else if(s.at(3)=="plane_buffer")same(m.plane,s.at(4),name+" stage plane");
        else if(s.at(3)=="signed_distance")same(m.signed_distance,s.at(4),name+" stage distance");
        else if(s.at(3)=="projection_shift")same(m.shift,s.at(4),name+" stage shift");
        else if(s.at(3)=="output_xyz")same(result.point,s.at(4),name+" stage output");
        else throw FormatError("Unrecognized defined original stage");++owned_stages;
      }
    }
    check(definitions==2 && cases.size()==208 && fallback==205 && projected==3 && current_nodes==832 && owned_stages==21 && reads==2498,"complete independent original fixture");
    std::cout<<cases.size()<<" original item points/full returns: "<<fallback<<" root / "<<projected<<" marker-plane projections; "<<owned_stages<<" owned stages matched; "<<current_nodes<<" original input nodes retained; "<<reads<<" frozen operand reads audited separately\n";
  }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

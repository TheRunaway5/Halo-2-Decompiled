#include "halo2/pickup_visibility.hpp"
#include "halo2/item_point.hpp"
#include "halo2/terrain_outer.hpp"
#include <bit>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
using namespace halo2;
namespace {
using Row=std::vector<std::string>;
std::uint32_t word(const std::string &s,unsigned base=16) {return std::stoul(s,nullptr,base);}
float real(const std::string &s) {return std::bit_cast<float>(word(s));}
Vec3 point(const Row &r,unsigned i) {return {real(r.at(i)),real(r.at(i+1)),real(r.at(i+2))};}
void check(bool value,const std::string &name) {if(!value)throw FormatError("Original pickup differs: "+name);}
void same_real(float actual,const std::string &expected,const std::string &name) {
  check(std::bit_cast<std::uint32_t>(actual)==word(expected),name);
}
void same_point(Vec3 actual,const Row &r,unsigned i,const std::string &name) {
  same_real(actual.x,r.at(i),name+" x");same_real(actual.y,r.at(i+1),name+" y");same_real(actual.z,r.at(i+2),name+" z");
}
Transform source_node(const std::string &s) {
  check(s.size()==sizeof(Transform)*2,"current node extent");
  std::array<std::byte,sizeof(Transform)> bytes;
  for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(word(s.substr(i*2,2)));
  return std::bit_cast<Transform>(bytes);
}
std::string location(TerrainLocation l) {
  std::ostringstream s;s<<std::hex<<std::setfill('0');
  for(unsigned i=0;i<4;++i)s<<std::setw(2)<<((std::uint32_t(l.leaf)>>(8*i))&255);
  for(auto value:{std::uint16_t(l.cluster),std::uint16_t(l.bsp)})
    for(unsigned i=0;i<2;++i)s<<std::setw(2)<<((value>>(8*i))&255);
  return s.str();
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3 && argc!=4)throw FormatError("Supply project and measured pickup fixture, optionally --item-points");
    const bool composed=argc==4;
    if(composed && std::string(argv[3])!="--item-points")throw FormatError("Unknown pickup comparison mode");
    const std::filesystem::path root(argv[1]);
    const auto map=root/"assets/disc/maps/lockout.map",xbe=root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
    if(!std::filesystem::exists(argv[2]) || !std::filesystem::exists(map) || !std::filesystem::exists(xbe))return 77;
    Cache cache(map);const auto terrain=cache.load_raw_terrain_collision(0);
    check(terrain.bsps.size()==1,"explicit ordinary collision bank");
    const auto original=OriginalData::load_xbe(xbe);
    std::array<std::uint32_t,8> intact;intact.fill(0xffffffffu);
    std::map<unsigned,Row> cases,normalizers;std::map<unsigned,std::vector<Row>> casts;
    std::map<unsigned,Row> item_cases;
    std::map<unsigned,std::map<unsigned,Transform>> item_nodes;
    if(composed) {
      const auto path=root/"analysis/multiplayer/integration/inventory-source/pickup-item-point/fixtures.tsv";
      if(!std::filesystem::exists(path))return 77;
      std::ifstream input(path);
      for(std::string line;std::getline(input,line);) {
        std::istringstream stream(line);Row r;for(std::string s;stream>>s;)r.push_back(s);
        if(r.empty())continue;
        if(r[0]=="P" && r.at(2)!="-1")check(item_cases.emplace(word(r.at(2),10),r).second,"unique correlated item point");
        else if(r[0]=="N")check(item_nodes[word(r.at(1),10)].emplace(word(r.at(2),10),source_node(r.at(3))).second,"unique current item node");
      }
      check(item_cases.size()==207,"complete item point/capsule correlation");
      // The source rejects types before asking FBFA0 for parent/node state.
      ItemPointState unavailable;unavailable.parent=0;
      auto forbidden=[](const PickupVisibilityCast &)->PickupVisibilityHit {throw FormatError("Rejected type emitted a cast");};
      check(source_item_pickup_visibility({}, {},unavailable,terrain.bsps[0],0,1,2,forbidden).visible,"unit type gate order");
      check(source_item_pickup_visibility({}, {},unavailable,terrain.bsps[0],0,0,1,forbidden).visible,"item type gate order");
    }
    std::ifstream file(argv[2]);
    for(std::string line;std::getline(file,line);) {
      std::istringstream stream(line);Row r;for(std::string s;stream>>s;)r.push_back(s);
      if(r.empty())continue;const auto id=word(r.at(1),10);
      if(r[0]=="C")check(cases.emplace(id,r).second,"duplicate case");
      else if(r[0]=="N")check(normalizers.emplace(id,r).second,"duplicate normalizer");
      else if(r[0]=="Q")casts[id].push_back(r);
      else throw FormatError("Unknown pickup fixture row");
    }
    unsigned queries=0,visible=0,normalized=0,clips=0,item_points=0,projections=0;
    for(const auto &[id,c]:cases) {
      const auto name=std::to_string(id);const auto definition=cache.load_pickup_capsule_definition(word(c.at(6)));
      check(definition.flags==word(c.at(7)),name+" authored flags actual="+std::to_string(definition.flags)+" expected="+std::to_string(word(c.at(7))));
      same_real(definition.standing,c.at(8),name+" standing");same_real(definition.crouching,c.at(9),name+" crouching");
      same_real(definition.radius,c.at(10),name+" radius");
      const auto capsule=pickup_capsule(point(c,12),definition,real(c.at(11)));
      same_point(capsule.bottom,c,15,name+" capsule bottom");same_real(capsule.extent,c.at(18),name+" extent");
      same_real(capsule.radius,c.at(19),name+" capsule radius");
      auto top=capsule.bottom;top.z=top.z+capsule.extent;same_point(top,c,23,name+" top");
      unsigned ordinal=0;
      auto query=[&](const PickupVisibilityCast &request) {
          const auto &q=casts.at(id).at(ordinal);const auto cast_name=name+" cast "+std::to_string(ordinal);
          check(word(q.at(2),10)==ordinal++,cast_name+" order");
          check(request.mask==word(q.at(3)) && request.unit==word(q.at(4)) && request.item==word(q.at(5)),cast_name+" mask/ignored identities");
          same_point(request.start,q,6,cast_name+" start");same_point(request.delta,q,9,cast_name+" delta");
          const auto collision=ordinary_terrain_query(terrain,*original,request.start,request.delta,request.mask,0,intact,256,-1);
          check(bool(collision.type)==bool(word(q.at(12),10)),cast_name+" AL");same_real(collision.fraction,q.at(13),cast_name+" fraction");
          same_point(collision.point,q,14,cast_name+" outer point");
          check(location(collision.initial)==q.at(17) && location(collision.final)==q.at(18),cast_name+" locations");
          ++queries;clips+=request.mask==0x24800001;return PickupVisibilityHit{bool(collision.type),collision.point};
        };
      PickupVisibilityResult result;
      if(composed) {
        const auto &p=item_cases.at(id);std::vector<Transform> bank;
        for(const auto &[index,node]:item_nodes.at(word(p.at(1),10))) {
          check(index==bank.size(),name+" contiguous current node bank");bank.push_back(node);
        }
        ItemPointState state;state.datum=word(p.at(5));state.definition=word(p.at(6));state.flags=word(p.at(8));state.parent=word(p.at(9));
        state.item_flags=std::uint8_t(word(p.at(11)));state.contact_instance=std::bit_cast<std::int16_t>(std::uint16_t(word(p.at(13))));
        state.contact_surface=std::bit_cast<std::int16_t>(std::uint16_t(word(p.at(14))));state.root=point(p,15);state.current_nodes=bank;
        check(state.datum==word(c.at(3)) && word(p.at(7),10)==word(c.at(5),10),name+" full item identity/type correlation");
        const auto item_definition=cache.load_item_point_definition(state.definition);
        const auto produced=source_item_point(item_definition,state,terrain.bsps[0]);
        same_point(produced.point,c,20,name+" original item point consumed by visibility");
        ++item_points;projections+=bool(produced.marker);
        result=source_item_pickup_visibility(capsule,item_definition,state,terrain.bsps[0],word(c.at(2)),
            word(c.at(4),10),word(c.at(5),10),query);
      }else result=pickup_visibility(capsule,point(c,20),word(c.at(2)),word(c.at(3)),word(c.at(4),10),word(c.at(5),10),query);
      check(ordinal==casts.at(id).size(),name+" complete cast stream");
      check(result.visible==bool(word(c.at(26),10)),name+" visibility AL");visible+=result.visible;
      check(bool(result.normalizer)==normalizers.contains(id),name+" normalizer admission");
      if(result.normalizer) {
        const auto &n=normalizers.at(id);const auto &actual=*result.normalizer;
        same_point(actual.input,n,2,name+" normalizer input");
        check(std::bit_cast<std::uint64_t>(actual.length)==std::stoull(n.at(5),nullptr,16),name+" original53-bit length");
        same_real(actual.length_spill,n.at(6),name+" length spill");same_point(actual.output,n,7,name+" normalized vector");
        same_real(actual.returned,n.at(10),name+" normalizer return");same_point(result.side,n,11,name+" source chosen side");
        same_real(actual.inverse,n.at(20),name+" source reciprocal");
        same_point(result.positive_target,n,14,name+" positive target");same_point(result.negative_target,n,17,name+" negative target");++normalized;
      }
    }
    check(cases.size()==207 && queries==1513 && normalized==187 && visible==21 && clips==374,"complete measured suite");
    if(composed)check(item_points==207 && projections==2,"complete native item-point/visibility composition");
    std::cout<<cases.size()<<" original capsules/visibility calls; "<<normalized<<" exact normalizers; "<<queries
      <<" ordered native terrain casts; "<<visible<<" visible / "<<cases.size()-visible<<" blocked; "<<clips<<" clear target clips matched\n";
    if(composed)std::cout<<item_points<<" source item points consumed by native visibility: "<<item_points-projections<<" root / "<<projections<<" marker-plane projections; source type gates precede point production\n";
  }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

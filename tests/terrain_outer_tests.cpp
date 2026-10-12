#include "halo2/terrain_outer.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
using namespace halo2;
namespace {
using Bytes=std::vector<std::uint8_t>;
std::uint32_t number(const std::string &s,unsigned base=10) {return std::stoul(s,nullptr,base);}
float real(const std::string &s) {return std::bit_cast<float>(number(s,16));}
Vec3 point(const std::vector<std::string> &r,std::size_t i) {return {real(r.at(i)),real(r.at(i+1)),real(r.at(i+2))};}
Bytes bytes(const std::string &s) {
  if(s.size()%2)throw FormatError("Odd fixture bytes");
  Bytes b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(number(s.substr(i,2),16));return b;
}
void store(Bytes &b,std::size_t offset,std::uint32_t value,unsigned size=4) {
  for(unsigned i=0;i<size;++i)b.at(offset+i)=std::uint8_t(value>>(8*i));
}
void put_point(Bytes &b,std::size_t offset,Vec3 p) {
  for(float v:{p.x,p.y,p.z}) {store(b,offset,std::bit_cast<std::uint32_t>(v));offset+=4;}
}
Bytes location(TerrainLocation l) {
  Bytes b(8);store(b,0,std::uint32_t(l.leaf));store(b,4,std::uint16_t(l.cluster),2);store(b,6,std::uint16_t(l.bsp),2);return b;
}
Bytes plane(CollisionPlane p) {
  Bytes b(16);put_point(b,0,p.normal);store(b,12,std::bit_cast<std::uint32_t>(p.distance));return b;
}
std::uint32_t word(const Bytes &b,std::size_t o,unsigned size=4) {
  std::uint32_t v=0;for(unsigned i=0;i<size;++i)v|=std::uint32_t(b.at(o+i))<<(8*i);return v;
}
void check(bool good,const std::string &name) {if(!good)throw FormatError("Original outer terrain differs: "+name);}
void same_real(float f,const std::string &expected,const std::string &name) {
  check(std::bit_cast<std::uint32_t>(f)==number(expected,16),name);
}
void same_point(Vec3 p,const std::vector<std::string> &r,std::size_t i,const std::string &name) {
  same_real(p.x,r.at(i),name+" x");same_real(p.y,r.at(i+1),name+" y");same_real(p.z,r.at(i+2),name+" z");
}
Bytes outer(const TerrainOuterResult &r) {
  Bytes b(92);store(b,0,r.type);store(b,4,std::bit_cast<std::uint32_t>(r.fraction));put_point(b,8,r.point);
  const auto a=location(r.initial),z=location(r.final);std::copy(a.begin(),a.end(),b.begin()+20);std::copy(z.begin(),z.end(),b.begin()+28);
  for(unsigned i=56;i<68;++i)b[i]=255;
  if(r.type) {
    store(b,36,std::uint16_t(*r.global_material),2);const auto p=plane(*r.plane);std::copy(p.begin(),p.end(),b.begin()+40);
    const auto &h=*r.lower.hit;store(b,76,h.leaf);store(b,80,h.surface);store(b,84,h.signed_plane);
    store(b,88,h.flags,1);store(b,89,h.breakable,1);store(b,90,std::uint16_t(h.material),2);
  }
  return b;
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Supply project and original outer fixture");
    const std::filesystem::path root(argv[1]);
    const auto map=root/"assets/disc/maps/lockout.map";
    const auto xbe=root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
    if(!std::filesystem::exists(argv[2]) || !std::filesystem::exists(map) || !std::filesystem::exists(xbe))return 77;
    const auto terrain=Cache(map).load_raw_terrain_collision(0);const auto original=OriginalData::load_xbe(xbe);
    check(terrain.bsps.size()==1,"BSP import count");const auto &bsp=terrain.bsps.at(0);
    std::array<std::uint32_t,8> intact;intact.fill(0xffffffffu);
    std::unordered_map<unsigned,TerrainOuterResult> results;std::unordered_map<unsigned,unsigned> iterations;
    unsigned queries=0,skipped=0,hits=0,lowers=0,retreat=0,classifiers=0,locators=0,planes=0,leaves=0,materials=0,offsets=0;
    std::uint32_t plane_address=0;std::int16_t active=-1;
    std::ifstream file(argv[2]);
    for(std::string line;std::getline(file,line);) {
      std::istringstream stream(line);std::vector<std::string> r;for(std::string s;stream>>s;)r.push_back(s);
      if(r.empty())continue;const auto &tag=r[0];
      if(tag=="I") {
        active=std::int16_t(std::stoi(r.at(1)));check(number(r.at(8))==terrain.bsps.size(),"BSP count");
        check(number(r.at(9))==terrain.global_materials.size(),"material count");check(number(r.at(10))==terrain.leaf_clusters.size(),"leaf count");
      } else if(tag=="B") {if(r.at(1)=="planes")plane_address=number(r.at(5),16);
      } else if(tag=="M") {check(terrain.global_materials.at(number(r.at(1)))==std::stoi(r.at(2)),"material import");++materials;
      } else if(tag=="J") {check(terrain.leaf_clusters.at(number(r.at(1)))==std::stoi(r.at(2)),"leaf import");++leaves;
      } else if(tag=="Q") {
        const auto id=number(r.at(1));if(!number(r.at(4))) {++skipped;continue;}
        auto result=ordinary_terrain_query(terrain,*original,point(r,9),point(r,12),number(r.at(6),16),active,intact,256,-1);
        check(bool(result.type)==bool(number(r.at(15))),"query "+std::to_string(id)+" return AL");
        const auto actual=outer(result),expected=bytes(r.at(24)),mask=bytes(r.at(23));check(expected.size()==92 && mask.size()==92,"outer size");
        for(unsigned i=0;i<92;++i)if(mask[i]==255)check(actual[i]==expected[i],"query "+std::to_string(id)+" byte "+std::to_string(i));
        hits+=bool(result.type);offsets+=bool(result.normal_offset_point);results.emplace(id,std::move(result));++queries;
      } else if(tag=="K") {
        const auto id=number(r.at(1));if(!results.contains(id))continue;const auto &lower=results.at(id).lower;
        check(number(r.at(4),16)==0 && number(r.at(5),16)==0 && number(r.at(6),16)==0x3f800000 && number(r.at(7),16)==0x7f7fffff,"lower fixed range");
        check(number(r.at(8))==256 && std::stoi(r.at(11))==-1 && number(r.at(12))==0 && std::stoi(r.at(13))==-1,"lower fixed admission");
        check(bool(lower.hit)==bool(number(r.at(10))),"lower hit");
        check(lower.prior_leaf==std::stoi(r.at(14)) && lower.prior_class==std::stoi(r.at(15)) && lower.prior_plane==std::stoi(r.at(16)),"lower final descriptor");
        check(lower.visited_leaves.size()==number(r.at(17)),"lower visited count");
        for(unsigned i=0;i<lower.visited_leaves.size();++i)check(lower.visited_leaves[i]==std::stoi(r.at(21+i)),"lower visited order");
        if(lower.hit) {
          const auto expected=bytes(r.at(18));const auto &h=*lower.hit;
          check(std::bit_cast<std::uint32_t>(h.fraction)==word(expected,0),"lower fraction");
          check(plane_address && std::uint32_t(h.crossing_plane)*16+plane_address==word(expected,4),"lower selected plane");
          check(std::uint32_t(h.leaf)==word(expected,8) && std::uint32_t(h.surface)==word(expected,12) && std::uint32_t(h.signed_plane)==word(expected,16),"lower contact");
          check(h.flags==word(expected,20,1) && h.breakable==word(expected,21,1) && std::uint16_t(h.material)==word(expected,22,2),"lower attributes");
        }
        ++lowers;
      } else if(tag=="R") {
        const auto id=number(r.at(1)),i=number(r.at(2));const auto &row=results.at(id).retreat.at(i);
        same_real(row.step,r.at(7),"retreat step");same_real(row.fraction_before,r.at(8),"retreat before");same_real(row.fraction_after,r.at(9),"retreat after");
        same_point(row.point_before,r,10,"retreat point before");same_point(row.point_after,r,13,"retreat point after");
        check(location(row.location_before)==bytes(r.at(16)) && location(row.location_after)==bytes(r.at(17)),"retreat locations");++iterations[id];++retreat;
      } else if(tag=="C") {
        check(std::uint32_t(terrain_leaf(bsp,point(r,7),number(r.at(5),16)))==number(r.at(10),16),"classifier "+r.at(1));++classifiers;
      } else if(tag=="L") {
        TerrainLocation loc;const auto ret=terrain_location(terrain,point(r,6),std::int16_t(std::stoi(r.at(5))),loc);
        check(std::uint32_t(ret)==number(r.at(9),16) && location(loc)==bytes(r.at(10)),"locator "+r.at(1));++locators;
      } else if(tag=="P") {
        check(plane(terrain_oriented_plane(bsp,std::uint16_t(number(r.at(6),16))))==bytes(r.at(7)),"oriented plane "+r.at(1));++planes;
      } else if(tag!="S")throw FormatError("Unknown original terrain fixture");
    }
    for(const auto &[id,r]:results)check(r.retreat.size()==iterations[id],"complete retreat stream");
    check(queries==1785 && skipped==2 && lowers==1785 && retreat==2122 && classifiers==2645 && locators==2364 && planes==3 && offsets==107 && leaves==2313 && materials==19,"incomplete outer suite");
    std::cout<<queries<<" ordinary outer queries / "<<hits<<" hits / "<<offsets<<" normal offsets; "<<retreat
      <<" retreat iterations; "<<classifiers<<" classifiers / "<<locators<<" locators / "<<planes<<" oriented planes; all leaf/material imports matched\n";
  }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

#include "halo2/sound_object_request.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
void check(bool v,const std::string &why){if(!v)throw std::runtime_error(why);}
std::uint32_t hex(std::istringstream &r){std::uint32_t v;check(bool(r>>std::hex>>v),"Incomplete source sound-owner fixture");return v;}
Buffer bytes(std::istringstream &r){std::string s;check(bool(r>>s)&&s.size()%2==0,"Incomplete source memory");Buffer v;for(std::size_t i=0;i<s.size();i+=2)v.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return v;}
template<std::size_t N> std::array<std::byte,N> packet(std::istringstream &r){const auto v=bytes(r);check(v.size()==N,"Incorrect source packet extent");std::array<std::byte,N> q;std::copy(v.begin(),v.end(),q.begin());return q;}
std::uint32_t word(std::span<const std::byte> v,std::size_t p,unsigned n=4){check(p<=v.size()&&n<=v.size()-p,"Truncated source operand");std::uint32_t q=0;for(unsigned i=0;i<n;++i)q|=std::to_integer<std::uint32_t>(v[p+i])<<(i*8);return q;}
Vec3 vector(std::span<const std::byte> v){return {std::bit_cast<float>(word(v,0)),std::bit_cast<float>(word(v,4)),std::bit_cast<float>(word(v,8))};}
struct Local {std::uint32_t player,unit;std::int32_t mode;};
struct Case {
  unsigned id;std::string name;std::uint32_t owner,sound;std::uint16_t fpcw;bool expected;
  std::array<std::uint32_t,4> roots;std::array<Local,4> locals;
  std::map<std::uint32_t,Buffer> memory;
  std::array<std::byte,20> definition;std::array<std::byte,92> sound_class;
  std::array<std::byte,40> marker;std::array<std::byte,48> before,after;
  std::vector<SoundObjectRequestStore> stores;
  std::vector<std::pair<std::array<std::byte,12>,std::array<std::byte,8>>> points;
};
struct Compression {std::array<std::byte,12> input;std::uint16_t fpcw;std::uint32_t output;};
struct Fixture {SoundObjectRequestConstants constants;std::vector<Case> cases;std::vector<Compression> compress;};
Fixture load(const char *path) {
  std::ifstream in(path);check(bool(in),"Original sound-owner fixture unavailable");Fixture f;
  for(std::string line;std::getline(in,line);) {
    if(line.empty()||line[0]=='#')continue;
    std::istringstream r(line);std::string kind;r>>kind;
    if(kind=="CONSTANTS")f.constants={hex(r),hex(r),hex(r),hex(r),packet<12>(r)};
    else if(kind=="CASE") {Case c{};r>>c.id>>c.name;c.owner=hex(r);c.sound=hex(r);c.fpcw=std::uint16_t(hex(r));unsigned v;r>>std::dec>>v;c.expected=v;f.cases.push_back(std::move(c));}
    else if(kind=="ROOTS") {for(auto &v:f.cases.back().roots)v=hex(r);}
    else if(kind=="LOCAL") {unsigned v;r>>v;auto p=hex(r),u=hex(r),m=hex(r);f.cases.back().locals.at(v)={p,u,std::bit_cast<std::int32_t>(m)};}
    else if(kind=="DEFINITION") {check(hex(r)==f.cases.back().sound,"Definition identity differs");f.cases.back().definition=packet<20>(r);f.cases.back().sound_class=packet<92>(r);}
    else if(kind=="MEM") {const auto p=hex(r);auto v=bytes(r);check(f.cases.back().memory.emplace(p,std::move(v)).second,"Duplicate source backing");}
    else if(kind=="MARKER")f.cases.back().marker=packet<40>(r);
    else if(kind=="BEFORE")f.cases.back().before=packet<48>(r);
    else if(kind=="AFTER")f.cases.back().after=packet<48>(r);
    else if(kind=="STORE") {auto ip=hex(r);std::size_t p;unsigned n;r>>std::dec>>p>>n;auto before=hex(r),after=hex(r);f.cases.back().stores.push_back({ip,p,n,before,after});}
    else if(kind=="POINT") {auto v=packet<12>(r);auto result=packet<8>(r);f.cases.back().points.emplace_back(v,result);}
    else if(kind=="COMPRESS") {auto input=packet<12>(r);auto cw=std::uint16_t(hex(r));f.compress.push_back({input,cw,hex(r)});}
    else check(kind=="ENDCASE","Unknown sound-owner fixture row");
  }
  return f;
}
std::span<const std::byte> read(const Case &c,std::uint32_t p,std::size_t n) {
  auto it=c.memory.upper_bound(p);
  check(it!=c.memory.begin(),"Missing source backing at "+std::to_string(p));--it;
  const auto offset=std::size_t(p-it->first);
  check(offset<=it->second.size()&&n<=it->second.size()-offset,
        "Missing source backing extent at "+std::to_string(p)+" size"+std::to_string(n));
  return std::span<const std::byte>(it->second).subspan(offset,n);
}
SoundObjectRequestServices services(const Case &c) {
  SoundObjectRequestServices s;
  s.memory={c.roots[0],c.roots[1],c.roots[2],c.roots[3],
    [&](auto p,auto n){return read(c,p,n);}};
  s.associations.sound_definition=[&](auto d){check(d==c.sound,"Unrecorded source definition");return c.definition;};
  s.associations.sound_class=[&](auto i){check(i==std::bit_cast<std::int8_t>(c.definition[2]),"Unrecorded source class");return c.sound_class;};
  s.associations.local_player=[&](auto v){return c.locals.at(v).player;};
  s.associations.player_current_unit=[&](auto p){for(const auto &l:c.locals)if(l.player==p)return l.unit;throw std::runtime_error("Unrecorded original player");};
  s.associations.camera_mode=[&](auto v){return c.locals.at(v).mode;};
  return s;
}
void run(const Fixture &f) {
  std::size_t stores=0,positive=0,points=0;
  for(const auto &c:f.cases) {
    auto s=services(c);auto location=c.before;std::vector<SoundObjectRequestStore> observed;
    const auto original_memory=c.memory;
    s.point_location=[&](Vec3 p) {
      check(points<c.points.size(),"Unrecorded source point query");
      const auto &q=c.points[points++];const auto expect=vector(q.first);
      check(std::bit_cast<std::uint32_t>(p.x)==std::bit_cast<std::uint32_t>(expect.x)&&
            std::bit_cast<std::uint32_t>(p.y)==std::bit_cast<std::uint32_t>(expect.y)&&
            std::bit_cast<std::uint32_t>(p.z)==std::bit_cast<std::uint32_t>(expect.z),"Source point-query input differs");
      return q.second;
    };
    const auto result=source_sound_object_request(c.owner,c.sound,c.marker,location,f.constants,c.fpcw,s,
                                                [&](const auto &q){observed.push_back(q);});
    check(result==c.expected,"Original AL differs: "+c.name);
    check(location==c.after,"Full original48-byte location differs: "+c.name);
    check(observed==c.stores,"Ordered original location stores differ: "+c.name);
    check(c.memory==original_memory,"Read-only sound owner modified source memory");
    check(points==c.points.size(),"Missing source point query");points=0;
    positive+=result;stores+=observed.size();
  }
  for(const auto &c:f.compress)check(source_sound_compressed_forward(vector(c.input),f.constants,c.fpcw)==c.output,"Original18A800 output differs");
  // Required ownership can fail only as unavailable, never become an original
  // rejection or a replacement node/velocity. This is a native boundary check.
  auto found=std::find_if(f.cases.begin(),f.cases.end(),[](const auto &c){return c.expected;});
  check(found!=f.cases.end(),"No original positive owner case");
  auto s=services(*found);s.memory.read={};auto location=found->before;bool failed=false;
  try {source_sound_object_request(found->owner,found->sound,found->marker,location,f.constants,found->fpcw,s);}
  catch(const FormatError &){failed=true;}
  check(failed&&location==found->before,"Unavailable object provider fabricated sound admission");
  std::cout<<"Original sound object requests: "<<f.cases.size()<<" cases, "<<positive<<" accepted, "
           <<stores<<" exact ordered stores, "<<f.compress.size()<<" compression controls; missing-owner boundary passed\n";
}
}
int main(int argc,char **argv) {
  if(argc!=2){std::cerr<<"usage: sound_object_request_tests literal-fixtures.tsv\n";return 2;}
  try {run(load(argv[1]));return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

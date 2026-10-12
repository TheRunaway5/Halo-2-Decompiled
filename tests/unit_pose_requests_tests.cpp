#include "halo2/unit_pose_requests.hpp"
#ifdef HALO2_POSE_REQUEST_CACHE
#include "halo2/original_data.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#endif
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
namespace {
using namespace halo2;
using Bytes = std::vector<std::byte>;
using State = std::map<std::string, Bytes>;
void check(bool b, const std::string &s) { if (!b) throw std::runtime_error(s); }
std::uint32_t number(std::istringstream &r) {
  std::uint32_t n; check(bool(r >> std::hex >> n), "Incomplete pose fixture"); return n;
}
Bytes decode(const std::string &s) {
  check(s.size() % 2 == 0, "Incomplete original bytes");
  Bytes b(s.size()/2);
  for (std::size_t i=0;i<b.size();++i)
    b[i]=std::byte(std::stoul(s.substr(i*2,2),nullptr,16));
  return b;
}
std::uint32_t get(std::span<const std::byte> b, std::size_t p, unsigned n=4) {
  check(p<=b.size()&&n<=b.size()-p,"Truncated original field");
  std::uint32_t v=0;
  for(unsigned i=0;i<n;++i) v|=std::uint32_t(std::to_integer<unsigned>(b[p+i]))<<(8*i);
  return v;
}
struct Event {
  std::string kind,buffer;
  std::vector<std::uint32_t> words;
  std::optional<std::uint32_t> result;
  friend bool operator==(const Event &,const Event &)=default;
};
struct Case {
  std::string id;
  std::uint32_t entry,capacity,slot_bytes,unit;
  std::optional<std::uint32_t> result;
  std::array<std::uint32_t,4> args;
  std::map<std::string,std::size_t> locations;
  State before,after;
  std::vector<Event> events;
  Bytes clock;
  std::uint32_t fpcw,mxcsr;
};
struct Tag {std::uint32_t address;Bytes raw;};
struct Fixture {
  std::uint32_t one;
  std::map<std::uint32_t,Tag> tags;
  std::vector<Case> cases;
};
Fixture load(const char *path) {
  std::ifstream in(path);check(bool(in),"No original pose fixture");
  Fixture f{};Case *c=nullptr;
  for(std::string line;std::getline(in,line);) {
    std::istringstream r(line);std::string kind;r>>kind;
    if(kind.empty()||kind[0]=='#')continue;
    if(kind=="TAG") {
      auto d=number(r),a=number(r);std::string raw;r>>raw;
      f.tags.emplace(d,Tag{a,decode(raw)});
    } else if(kind=="ONE")f.one=number(r);
    else if(kind=="CASE") {
      f.cases.emplace_back();c=&f.cases.back();r>>c->id;
      c->entry=number(r);std::string value;r>>value;
      if(value!="-")c->result=std::stoul(value,nullptr,16);
      for(auto &v:c->args)v=number(r);
    } else if(kind=="POOL") {
      c->capacity=number(r);c->slot_bytes=number(r);c->unit=number(r);
    } else if(kind=="LOCATION") {
      std::string name;r>>name;c->locations.emplace(name,number(r));
    } else if(kind=="CLOCK") {
      std::string raw;r>>raw;c->clock=decode(raw);c->fpcw=number(r);c->mxcsr=number(r);
    } else if(kind=="BEFORE"||kind=="AFTER") {
      std::string k,raw;r>>k>>raw;(kind=="BEFORE"?c->before:c->after).emplace(k,decode(raw));
    } else if(kind=="EVENT") {
      Event e;r>>e.kind;
      if(e.kind=="STORE") {
        e.words.push_back(number(r));r>>e.buffer;
        for(unsigned i=0;i<4;++i)e.words.push_back(number(r));
      } else if(e.kind=="ENTER"||e.kind=="EXIT") {
        for(unsigned i=0;i<6;++i)e.words.push_back(number(r));
        if(e.kind=="EXIT") {
          std::string v;r>>v;if(v!="-")e.result=std::stoul(v,nullptr,16);
        }
      } else if(e.kind=="RESOURCE") {
        e.words={number(r),number(r)};
      } else check(false,"Unknown pose event");
      c->events.push_back(std::move(e));
    } else check(kind=="ENDCASE","Unknown pose fixture record");
  }
  return f;
}
SourceAnimationGraphMemory literal(const Fixture &f) {
  return {[&](std::uint32_t d){return f.tags.at(d).address;},
    [&](std::uint32_t p,std::size_t n)->std::span<const std::byte>{
      for(const auto &[d,t]:f.tags) {
        (void)d;
        if(p>=t.address) {
          const auto off=std::uint64_t(p)-t.address;
          if(off<=t.raw.size()&&n<=t.raw.size()-off)
            return std::span<const std::byte>(t.raw).subspan(std::size_t(off),n);
        }
      }
      throw std::runtime_error("Missing literal graph backing");
    }};
}
template<class F>void rejects(F &&fn) {
  bool caught=false;try{fn();}catch(const std::exception&){caught=true;}
  check(caught,"Missing owner became successful");
}
void verify(const Fixture &f,const SourceAnimationGraphMemory &memory,
            std::uint32_t one,const char *label) {
  std::size_t stores=0,resources=0,boundaries=0,positive=0,initializers=0;
  for(const auto &c:f.cases) {
    auto state=c.before;
    std::function<std::span<std::byte>(const std::string&)> visible;
    visible=[&](const std::string &name)->std::span<std::byte>{
      for(const auto &parent:{std::string("K"),std::string("U"),std::string("M")}) {
        auto p=c.locations.find(name+"_"+parent);
        if(p!=c.locations.end())return visible(parent).subspan(p->second,state.at(name).size());
      }
      return state.at(name);
    };
    auto compare=[&](const State &expected){
      for(const auto &[k,b]:expected) {
        auto v=visible(k);check(v.size()==b.size()&&std::equal(v.begin(),v.end(),b.begin()),
                               c.id+" Complete retained buffer differs: "+k);
      }
    };
    compare(c.before);
    check(c.clock.size()==12,"Missing literal clock observation");
    std::vector<Event> events;
    SourceUnitPoseRequestServices s{};s.channels.graphs=memory;s.channels.one_bits=one;
    s.channels.resources.state=[&]{
      auto g=visible("G");return std::array<std::uint8_t,2>{std::uint8_t(get(g,0,1)),std::uint8_t(get(g,1,1))};
    };
    s.channels.resources.request=[&](std::uint32_t d,std::uint8_t demand){
      events.push_back({"RESOURCE",{}, {d,demand},{}});
      SourceAnimationDemandServices lower{};lower.bindings.active_byte=0x518411;
      lower.memory.read=[&](std::uint32_t p,std::size_t n)->std::span<const std::byte>{
        auto g=visible("G");check(p>=0x518410&&std::uint64_t(p)-0x518410<=g.size()&&n<=g.size()-(p-0x518410),
                                 "Actual queue/residency owner unavailable");
        return g.subspan(p-0x518410,n);
      };
      request_source_animation_resource(d,demand,lower);
    };
    // The measured owning chain never calls variant/clamp providers. They are
    // deliberately unavailable here, not recorded successful substitutions.
    auto call=[&](const SourceUnitPoseCall &v){
      Event e{v.returning?"EXIT":"ENTER",{}, {v.instruction,v.entry},v.result};
      e.words.insert(e.words.end(),v.arguments.begin(),v.arguments.end());
      events.push_back(std::move(e));
    };
    const auto kind=c.entry==0x114bd0||c.entry==0x115af0?"U":c.entry==0x1cef10?"C":"P";
    auto store=[&](const UnitAnimationSelectionStore &v){
      events.push_back({"STORE",kind,{v.instruction,std::uint32_t(v.offset),v.bytes,v.before,v.after},{}});
    };
    if(c.entry==0x114bd0||c.entry==0x115af0) {
      check(c.slot_bytes==12&&visible("K").size()==std::size_t(c.capacity)*12,
            "Missing full original header-pool backing");
      UnitConstructorObjectPool pool{visible("K"),c.capacity,
        [&](std::uint16_t index){check(index==(c.unit&65535),"Unowned body requested");return visible("U");},{}};
      if(c.entry==0x114bd0)update_source_unit_pose_requests(c.unit,pool,s,call,store);
      else update_source_unit_posture_channel(c.unit,pool,s,call,store);
    } else if(c.entry==0x1cef10) {
      std::array<std::byte,32> constructed;
      construct_source_unit_pose_channel(constructed,one);
      check(std::equal(constructed.begin(),constructed.end(),c.before.at("C").begin()),
            c.id+" Complete original temporary-channel constructor differs");
      ++initializers;
      check(install_source_unit_alternate_pose_channel(visible("M"),visible("C"),c.args[0],c.args[1],c.args[2],c.args[3],s,call,store)==bool(c.result.value()),
            c.id+" DefinedAL differs");
    } else {
      if(c.entry==0x1cf4c0)lookup_source_unit_pose_request(visible("M"),c.args[0],c.args[1],c.args[2],visible("P"),s,call,store);
      else if(c.entry==0x1cf3c0)lookup_source_unit_alternate_pose(visible("M"),c.args[0],c.args[1],c.args[2],visible("P"),s,call,store);
      else if(c.entry==0x1cf360)lookup_source_unit_resolved_pose(visible("M"),c.args[0],c.args[1],c.args[2],visible("P"),s,call,store);
      else if(c.entry==0x1cfb90)lookup_source_unit_pose_index(visible("M"),c.args[0],visible("P"),s,call,store);
      else check(false,"Unknown complete pose owner");
      check(get(visible("P"),0)==c.result,c.id+" Packed output differs");
      positive+=(get(visible("P"),0)>>16)!=65535;
    }
    compare(c.after);
    if(events!=c.events) {
      std::size_t i=0;while(i<std::min(events.size(),c.events.size())&&events[i]==c.events[i])++i;
      std::ostringstream why;why<<c.id<<" Combined original call/store ordering differs at "<<i
       <<" actual/expected counts "<<events.size()<<"/"<<c.events.size();
      if(i<events.size()){why<<" actual "<<events[i].kind<<":"<<events[i].buffer;for(auto v:events[i].words)why<<" "<<std::hex<<v;}
      if(i<c.events.size()){why<<" expected "<<c.events[i].kind<<":"<<c.events[i].buffer;for(auto v:c.events[i].words)why<<" "<<std::hex<<v;}
      throw std::runtime_error(why.str());
    }
    for(const auto &e:events) {
      stores+=e.kind=="STORE";resources+=e.kind=="RESOURCE";
      boundaries+=e.kind=="ENTER"||e.kind=="EXIT";
    }
  }
  check(stores==273&&resources==720&&boundaries==498&&initializers==15&&positive==24,
        "Owning pose comparison counts differ");
  const auto &c=*std::find_if(f.cases.begin(),f.cases.end(),[](const auto &v){return v.entry==0x1cf4c0&&v.result&&(*v.result>>16)!=65535;});
  SourceUnitPoseRequestServices missing{};missing.channels.graphs=memory;missing.channels.one_bits=one;
  auto m=c.before.at("M"),p=c.before.at("P");
  rejects([&]{lookup_source_unit_pose_request(m,c.args[0],c.args[1],c.args[2],std::span<std::byte>(p).first(3),missing);});
  rejects([&]{lookup_source_unit_pose_request(m,c.args[0],c.args[1],c.args[2],p,missing);});
  rejects([&]{construct_source_unit_pose_channel(std::span<std::byte>(p),one);});
  const auto &owned=*std::find_if(f.cases.begin(),f.cases.end(),[](const auto &v){return v.entry==0x114bd0;});
  auto headers=owned.before.at("K");UnitConstructorObjectPool no_body{headers,owned.capacity,{}, {}};
  rejects([&]{update_source_unit_pose_requests(owned.unit,no_body,missing);});
  std::cout<<label<<" cases="<<f.cases.size()<<" packedPositive="<<positive<<" channelInitializers="<<initializers
    <<" orderedStores="<<stores<<" resourceCalls="<<resources<<" callBoundaries="<<boundaries<<'\n';
}
} // namespace
int main(int argc,char **argv) {
  try {
    check(argc==2||argc==3||argc==4,"Supply fixture, optional Lockout and verified image");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    auto f=load(argv[1]);check(f.cases.size()==63,"Expected63 actual owning cases");
    verify(f,literal(f),f.one,"literal");
#ifdef HALO2_POSE_REQUEST_CACHE
    if(argc>=3) {
      auto cache=std::make_shared<Cache>(argv[2]);UnitAnimationMetadataStore store(cache);auto memory=store.graph_memory();
      for(const auto &[d,t]:f.tags) {
        check(memory.graph_address(d)==t.address,"Actual graph identity differs");auto raw=memory.read(t.address,t.raw.size());
        check(std::equal(raw.begin(),raw.end(),t.raw.begin()),"Actual graph payload differs");
      }
      auto one=f.one;
      if(argc==4) {auto image=OriginalData::load_xbe(argv[3]);check(bool(image->unit_animation_constants),"Verified operands absent");
        one=image->unit_animation_constants->one_bits;check(one==f.one,"Actual imported one_bits differs");}
      verify(f,memory,one,argc==4?"actualCache+image":"actualCache");
    }
#else
    check(argc==2,"Actual Cache mode not compiled");
#endif
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

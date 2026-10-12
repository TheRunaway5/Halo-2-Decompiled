#include "halo2/unit_action_dispatch.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
namespace {
using Bytes = std::vector<std::byte>;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
std::uint32_t number(const std::string &s) { return std::uint32_t(std::stoul(s, nullptr, 16)); }
Bytes bytes(const std::string &s) {
  if (s == "-") return {};
  check(!(s.size() % 2), "Fixture odd byte count"); Bytes b;
  for (std::size_t i = 0; i < s.size(); i += 2) b.push_back(std::byte(number(s.substr(i, 2))));
  return b;
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at, unsigned n = 4) {
  check(at <= b.size() && n <= b.size() - at, "Fixture read bounds"); std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i) v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
void put(std::span<std::byte> b, std::size_t at, std::uint32_t v, unsigned n = 4) {
  check(at <= b.size() && n <= b.size() - at, "Fixture write bounds");
  for (unsigned i = 0; i < n; ++i) b[at + i] = std::byte((v >> (8 * i)) & 255);
}
struct Object { std::uint32_t id; Bytes header, body; };
struct State { std::vector<Object> objects; std::uint32_t player{}; Bytes player_bytes; };
struct Leaf { std::uint32_t target{}, unit{}, type{}, phase{}, result{}; Bytes request; State before, after; };
struct Case {
  std::string mode;
  std::uint32_t entry{}, unit{}, type{}, result{}, allocation_bytes{};
  Bytes request, headers_before, headers_after, allocation_header, allocation_body_header,
      allocation_body, view_before, view_after, clock;
  State before, after; std::vector<Leaf> leaves; std::vector<UnitActionStore> stores;
};
struct Fixture { std::array<Bytes,60> descriptors; std::vector<Case> cases; };
Fixture parse(const std::filesystem::path &p) {
  std::ifstream f(p);check(bool(f), "Missing source action fixture");Fixture data;
  Case *c = nullptr;Leaf *l = nullptr;std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream in(line);std::string token, v;in >> token;
    auto n = [&] { in >> v;return number(v); };
    auto b = [&] { in >> v;return bytes(v); };
    if (token == "DESC") { const auto i=n();check(i<60,"Descriptor index");data.descriptors[i]=b(); }
    else if (token == "CASE" || token == "BOOT" || token == "MARK" || token == "LOCAL") {
      data.cases.emplace_back();c=&data.cases.back();c->mode=token;l=nullptr;
      if(token=="CASE") { c->entry=n();c->unit=n();c->type=n();c->result=n();c->request=b(); }
      else if(token=="BOOT") { c->allocation_bytes=n();c->unit=n(); }
      else if(token=="MARK") c->unit=n();
      else { c->unit=n();c->type=n();c->result=n(); }
    } else {
      check(c!=nullptr,"Fixture row outside case");
      if(token=="HPB") c->headers_before=b();
      else if(token=="HPA") c->headers_after=b();
      else if(token=="HEADER_ALLOC") c->allocation_header=b();
      else if(token=="BODY_ALLOC") { c->allocation_body_header=b();c->allocation_body=b(); }
      else if(token=="VIEWB") c->view_before=b();
      else if(token=="VIEWA") c->view_after=b();
      else if(token=="CLOCK") c->clock=b();
      else if(token=="LEAF") { c->leaves.emplace_back();l=&c->leaves.back();l->target=n();l->unit=n();l->type=n();l->phase=n();l->result=n();l->request=b(); }
      else if(token=="END_LEAF") l=nullptr;
      else if(token=="B" || token=="A" || token=="LB" || token=="LA") {
        State *s=token=="B"?&c->before:token=="A"?&c->after:token=="LB"?&l->before:&l->after;
        const auto id=n();auto h=b();auto body=b();s->objects.push_back({id,std::move(h),std::move(body)});
      } else if(token=="BPLAYER" || token=="APLAYER" || token=="LBPLAYER" || token=="LAPLAYER") {
        State *s=token=="BPLAYER"?&c->before:token=="APLAYER"?&c->after:token=="LBPLAYER"?&l->before:&l->after;
        s->player=n();s->player_bytes=b();
      } else if(token=="STORE") {
        const auto pc=n(),id=n(),tag=n(),at=n(),size=n(),before=n(),after=n();
        check(tag<=3,"Store buffer kind");c->stores.push_back({pc,id,UnitActionBuffer(tag),at,size,before,after});
      } else check(token=="END_CASE","Unknown source action fixture token");
    }
  }
  return data;
}
struct Storage {
  Bytes headers;std::map<std::uint16_t,Object> objects;Bytes player;
  UnitConstructorObjectPool pool() {
    return {headers,std::uint32_t(headers.size()/12),[&](std::uint16_t i)->std::span<std::byte> {return objects.at(i).body;},{}};
  }
  void apply(const State &s, bool initial=false) {
    for(const auto &x:s.objects) {
      const auto i=std::uint16_t(x.id);const auto at=std::size_t(i)*12;
      check(at+12<=headers.size() && x.header.size()==12,"Object fixture header bounds");
      std::copy(x.header.begin(),x.header.end(),headers.begin()+at);
      if(initial) objects[i]=x;
      else { auto &o=objects.at(i);check(o.body.size()==x.body.size(),"Callback would relocate retained source body");std::copy(x.body.begin(),x.body.end(),o.body.begin()); }
    }
    if(!s.player_bytes.empty()) {
      if(initial) player=s.player_bytes;
      else { check(player.size()==s.player_bytes.size(),"Source player fixture size changed");std::copy(s.player_bytes.begin(),s.player_bytes.end(),player.begin()); }
    }
  }
  void compare(const State &s) const {
    for(const auto &x:s.objects) {
      const auto i=std::uint16_t(x.id);const auto at=std::size_t(i)*12;
      check(std::equal(x.header.begin(),x.header.end(),headers.begin()+at),"Full source object header mismatch");
      check(objects.at(i).body==x.body,"Full source object body mismatch");
    }
    if(!s.player_bytes.empty()) check(player==s.player_bytes,"Full source player record mismatch");
  }
};
void run(const Fixture &f) {
  std::size_t count=0, callbacks=0, writes=0;
  for(const auto &c:f.cases) {
    Storage st;st.headers=c.headers_before;st.apply(c.before,true);std::vector<UnitActionStore> stores;
    const UnitActionObserver observe=[&](const auto &s) {stores.push_back(s);};
    if(c.mode=="BOOT") {
      SourceObjectBodyAllocationServices s;s.objects=st.pool();
      s.allocate_header=[&] {const auto at=std::size_t(std::uint16_t(c.unit))*12;std::copy(c.allocation_header.begin(),c.allocation_header.end(),st.headers.begin()+at);return c.unit;};
      s.allocate_body=[&](std::uint32_t id,std::int32_t n) {check(id==c.unit && n==std::int32_t(c.allocation_bytes),"Actual source body allocation operands mismatch");const auto at=std::size_t(std::uint16_t(id))*12;std::copy(c.allocation_body_header.begin(),c.allocation_body_header.end(),st.headers.begin()+at);st.objects[std::uint16_t(id)]={id,c.allocation_body_header,c.allocation_body};return true;};
      check(allocate_source_object_body(std::int16_t(c.allocation_bytes),s,observe)==c.unit,"Source body allocation datum mismatch");
    } else if(c.mode=="MARK") mark_source_object_for_update(c.unit,st.pool(),observe);
    else {
      SourceUnitActionServices s;s.objects=st.pool();std::size_t cursor=0;
      s.descriptor=[&](std::uint32_t type)->std::span<const std::byte> {return f.descriptors.at(type);};
      s.player_body=[&](std::uint32_t id)->std::span<const std::byte> {check(id==c.before.player,"Source action player datum mismatch");return st.player;};
      s.invoke=[&](const UnitActionCall &call) {
        check(cursor<c.leaves.size(),"Unexpected source action callback");const auto &l=c.leaves[cursor++];
        check(call.entry==l.target && call.unit==l.unit && call.type==l.type && unsigned(call.callback)==l.phase,"Original callback identity/order mismatch");
        check(std::equal(call.request.begin(),call.request.end(),l.request.begin(),l.request.end()),"Complete original callback request mismatch");
        st.compare(l.before);st.apply(l.after);++callbacks;return std::uint8_t(l.result);
      };
      if(c.mode=="LOCAL") {
        auto view=c.view_before;s.view_record=[&](std::int16_t i)->std::span<std::byte> {check(i==0,"Original local-user value changed");return view;};
        s.source_tick=[&] {return std::bit_cast<std::int32_t>(read(c.clock,8));};
        notify_source_unit_action(c.unit,c.type,std::uint8_t(c.result),s,observe);
        check(view==c.view_after,"Complete original local view record mismatch");
      } else if(c.entry==0xe7f30) check(dispatch_source_unit_action(c.unit,c.request,s,observe)==c.result,"Original dispatch AL mismatch");
      else if(c.entry==0xe7ef0) check(begin_source_unit_action(c.unit,c.type,s,observe)==c.result,"Original generic begin AL mismatch");
      else if(c.entry==0xe7ff0) interrupt_source_unit_action(c.unit,c.type,s,observe);
      else if(c.entry==0xe7e60) check(unsigned(update_source_unit_actions(c.unit,s,observe))==c.result,"Original update AL mismatch");
      else if(c.entry==0xb8250) mark_source_object_for_update(c.unit,s.objects,observe);
      else if(c.entry==0x1eb150) notify_source_unit_action(c.unit,c.type,0,s,observe);
      else check(false,"Unsupported fixture source action entry");
      check(cursor==c.leaves.size(),"Original callback was omitted");
    }
    st.compare(c.after);if(!c.headers_after.empty())check(st.headers==c.headers_after,"Complete original header pool mismatch");
    if(stores.size()!=c.stores.size())throw std::runtime_error("Original owning store count mismatch at case"+std::to_string(count)+" entry"+std::to_string(c.entry)+" actual"+std::to_string(stores.size())+" expected"+std::to_string(c.stores.size()));
    for(std::size_t i=0;i<stores.size();++i) {
      auto actual=stores[i],expected=c.stores[i];
      // E7EF0 stack scratch has no defined incoming native bytes. Compare its
      // exact write sequence/value/width, but not historical caller-stack data.
      if(actual.buffer==UnitActionBuffer::Request) expected.before=actual.before;
      check(actual==expected,"Original owning store identity/order/value mismatch");
    }
    ++count;writes+=stores.size();
  }
  std::cout<<count<<" complete source cases; "<<callbacks<<" explicit original lower callbacks; "<<writes<<" owning stores\n";
}
template<class F> void rejected(F &&f) {
  bool failed=false;try {f();}catch(const std::runtime_error &) {failed=true;}check(failed,"Unavailable owner did not stop");
}
void boundaries(const Fixture &f) {
  const auto it=std::find_if(f.cases.begin(),f.cases.end(),[](const auto &c){return c.entry==0xe7f30;});check(it!=f.cases.end(),"No source dispatch input");
  const auto &c=*it;Storage st;st.headers=c.headers_before;st.apply(c.before,true);SourceUnitActionServices s;s.objects=st.pool();
  s.descriptor=[&](std::uint32_t type)->std::span<const std::byte>{return f.descriptors.at(type);};
  rejected([&]{dispatch_source_unit_action(c.unit,c.request,s);});
  s.player_body=[&](std::uint32_t)->std::span<const std::byte>{return st.player;};
  s.invoke=[](const UnitActionCall &) {return std::uint8_t{0x7f};};
  check(dispatch_source_unit_action(c.unit,c.request,s)==0x7f,"Defined source callback AL was normalized");
  rejected([&]{interrupt_source_unit_action(c.unit,60,s);});
  auto &b=st.objects.at(std::uint16_t(c.unit)).body;const auto old=read(b,0x346,2);put(b,0x346,0xffff,2);
  rejected([&]{source_unit_action_component(b);});put(b,0x346,old,2);
  rejected([&]{mark_source_object_for_update(0xffffffffu,st.pool());});
  // Required view/clock are not inferred from a native allocated player slot.
  auto p=st.player;put(p,0x28,0,2);s.player_body=[&](std::uint32_t)->std::span<const std::byte>{return p;};
  rejected([&]{notify_source_unit_action(c.before.player,22,1,s);});
  Bytes view(0x1b0);s.view_record=[&](std::int16_t)->std::span<std::byte>{return view;};
  rejected([&]{notify_source_unit_action(c.before.player,22,1,s);});
  // Explicit host robustness cases; these are not additional source fixtures.
  for(unsigned i=0;i<4;++i)put(view,0x150+i*8,std::uint32_t(i==2?-2:i==3?-1:0));
  record_source_unit_action(view,0,7,0x12345678,0xff);
  check(read(view,0x160)==7 && read(view,0x164,2)==0x5678 && read(view,0x166,2)==0xff,"Signed oldest/width host boundary mismatch");
  SourceObjectBodyAllocationServices allocation;allocation.objects=st.pool();allocation.allocate_header=[] {return 0xffffffffu;};
  check(allocate_source_object_body(4,allocation)==0xffffffffu,"Real NONE allocator result altered");
  allocation.allocate_header=[&] {return c.unit;};allocation.allocate_body=[](std::uint32_t,std::int32_t){return false;};bool released=false;
  allocation.release_header=[&](std::uint32_t id){check(id==c.unit,"Failed allocation release identity");released=true;};
  const auto headers=st.headers;check(allocate_source_object_body(4,allocation)==0xffffffffu && released && st.headers==headers,"Failed body allocation store/release mismatch");
  std::cout<<"10 explicit host bounds/provider checks passed\n";
}
} // namespace
int main(int argc,char **argv) {
  if(argc!=2 || !std::filesystem::exists(argv[1]))return 77;
  try {const auto f=parse(argv[1]);run(f);boundaries(f);return 0;}
  catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

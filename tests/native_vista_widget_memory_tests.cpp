#include "halo2/vista_widget_screen_disposal.hpp"
#include "halo2/native_vista_widget_memory.hpp"
#include "halo2/native_vista_widget_managers.hpp"
#include "halo2/native_vista_widget_cache.hpp"
#include "halo2/vista_widget_locale.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using Json=nlohmann::json;
namespace {
const Json* pool{};
std::vector<std::byte> unhex(const Json& value){const auto t=(value.is_number_unsigned()?pool->at(value.get<std::size_t>()):value).get<std::string>();auto d=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid disposal hex");};std::vector<std::byte>b(t.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(d(t[i*2])*16+d(t[i*2+1]));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto c:b){const auto v=std::to_integer<unsigned>(c);s.push_back(d[v>>4]);s.push_back(d[v&15]);}return s;}
struct Fixture {
 const Json& q;NativeVistaWidgetMemory memory;std::shared_ptr<NativeVistaWidgetManagers> managers;Json events=Json::array(),trace=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;std::string failure;bool reentered{};
 explicit Fixture(const Json& row,const std::shared_ptr<NativeVistaWidgetCache>& cache):q(row.at("input")),managers(std::make_shared<NativeVistaWidgetManagers>()) {
  memory.bind_managers(managers);memory.bind_cache(cache);
  for(const auto& b:q.at("banks")) {
   const auto at=b.at(0).get<std::uint32_t>();const auto initial=unhex(b.at(1));
   if(at==NativeVistaWidgetManagers::source_base)memory.write(at,initial);
   else memory.mount(at,initial,NativeVistaWidgetMemory::Access::ReadWrite,"explicit original disposal fixture bank");
  }
 }
 std::span<std::byte> bytes(std::uint32_t at,std::size_t n){return memory.mutable_view(at,n);}
 std::uint32_t read(std::uint32_t at,std::size_t n){std::uint32_t v;try{v=memory.read(at,n);}catch(const FormatError&){failure="memory";trace.push_back(Json::array({"fault","r",at,n}));throw;}trace.push_back(Json::array({"r",at,n,v}));return v;}
 void write(std::uint32_t at,std::uint32_t v,std::size_t n){try{memory.write(at,v,n);}catch(const FormatError&){failure="memory";trace.push_back(Json::array({"fault","w",at,n}));throw;}trace.push_back(Json::array({"w",at,n,n==1?v&255:n==2?v&65535:v}));}
 Json snapshot(){Json out=Json::array();for(const auto& b:q.at("banks"))out.push_back(Json::array({b.at(0),hex(memory.view(b.at(0).get<std::uint32_t>(),unhex(b.at(1)).size()))}));return out;}
 void raw(std::uint32_t at,const Json& value){const auto b=unhex(value);auto output=bytes(at,b.size());std::copy(b.begin(),b.end(),output.begin());}
 template<class... T>void event(const std::string& name,T... values){Json args=Json::array();(args.push_back(values),...);events.push_back(Json{{"name",name},{"args",args},{"banks",snapshot()}});const auto n=++occurrences[name];for(std::size_t i=0;i<q.at("mutations").size();++i){const auto& m=q.at("mutations").at(i);if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=n)continue;applied.insert(i);for(const auto& w:m.at("writes"))raw(w.at(0),w.at(1));}if(q.contains("fail")&&q.at("fail")==Json::array({name,n})){failure="provider";throw FormatError("Declared genuine disposal provider failure");}}
 VistaWidgetScreenDisposalServices services(){VistaWidgetScreenDisposalServices s;s.read=[this](auto at,auto n){return read(at,n);};s.write=[this](auto at,auto v,auto n){write(at,v,n);};s.screen_query=[this](auto target,auto screen){const auto v=q.at(target==0x1800200?"index":"kind").template get<std::uint32_t>();event("query",target,screen.value,v);return v;};s.manager_remove=[this](auto target,auto manager,auto screen){event("remove",target,manager.value,screen.value);if(target!=0x637d9b)throw FormatError("Unbound actual manager virtual20");vista_widget_manager_unlink(manager,screen,services());};s.root=[this](auto child){event("root",child.value);VistaWidgetLocaleServices l;l.read=[this](auto at,auto n){return read(at,n);};return vista_widget_locale_root(child,l);};s.cleanup=[this](auto target,auto screen){event("cleanup",target,screen.value);if(q.value("reentry",false)&&!reentered){reentered=true;vista_widget_screen_dispose({0x1a06000},services());}};s.destructor=[this](auto target,auto screen,auto flag){event("destructor",target,screen.value,flag);};s.release=[this](auto screen){event("release",screen.value);};return s;}
 Json run(){const auto mode=q.at("mode");try{if(mode=="mapping")return vista_widget_manager_mapping(q.at("index"),q.at("kind")).value;const auto s=services();if(mode=="dispose")vista_widget_screen_dispose({q.at("screen")},s);else vista_widget_manager_unlink({q.at("manager")},{q.at("screen")},s);}catch(const FormatError&){if(failure.empty())throw;}return nullptr;}
};
Json expanded(Json events){for(auto& e:events)for(auto& b:e.at("banks"))b.at(1)=hex(unhex(b.at(1)));return events;}
void compare(const Json&a,const Json&b,std::size_t row,const char* label){if(a!=b){std::cerr<<"disposal case "<<row<<' '<<label<<" differs\n";if(std::string(label)=="trace"){std::size_t i{};while(i<a.size()&&i<b.size()&&a.at(i)==b.at(i))++i;std::cerr<<"at "<<i<<" counts "<<a.size()<<'/'<<b.size()<<'\n';if(i<a.size())std::cerr<<"actual "<<a.at(i)<<'\n';if(i<b.size())std::cerr<<"original "<<b.at(i)<<'\n';}throw FormatError("Original disposal comparison failed");}}
std::shared_ptr<NativeVistaWidgetCache> load_cache(const char* path) {
 std::ifstream in(path,std::ios::binary|std::ios::ate);
 if(!in||in.tellg()<0)throw FormatError("Actual Vista registry cache missing");
 std::vector<std::byte> raw(std::size_t(in.tellg()));in.seekg(0);
 if(!in.read(reinterpret_cast<char*>(raw.data()),std::streamsize(raw.size())))throw FormatError("Short actual Vista registry cache");
 std::uint64_t fnv=0xcbf29ce484222325ull;
 for(auto b:raw)fnv=(fnv^std::to_integer<unsigned>(b))*0x100000001b3ull;
 if(raw.size()!=61063680||fnv!=0xfb575e16e1c8e5ceull)throw FormatError("Actual Vista registry cache authority changed");
 auto bank=std::make_shared<const VistaWidgetCacheBank>(raw);
 return std::make_shared<NativeVistaWidgetCache>(std::move(bank),0x3200000u);
}
std::size_t boundaries(const std::shared_ptr<NativeVistaWidgetCache>& cache) {
 using Memory=NativeVistaWidgetMemory;using Access=Memory::Access;std::size_t checks{};
 const auto require=[&](bool okay){++checks;if(!okay)throw FormatError("Native Vista source bank boundary differs");};
 const auto reject=[&](auto&& f){bool caught{};try{f();}catch(const FormatError&){caught=true;}require(caught);};
 Memory m;reject([&]{(void)m.read(NativeVistaWidgetManagers::source_base,4);});
 auto managers=std::make_shared<NativeVistaWidgetManagers>();
 const auto initial=std::vector<std::byte>(managers->bytes(managers->source_base,managers->storage_size).begin(),managers->bytes(managers->source_base,managers->storage_size).end());
 const auto manager_pointer=managers->bytes(managers->source_base,1).data();
 m.bind_managers(*managers);m.bind_cache(*cache);
 require(std::equal(initial.begin(),initial.end(),m.view(managers->source_base,initial.size()).begin()));
 require(m.mutable_view(managers->source_base,1).data()==manager_pointer);
 require(m.mutable_view(cache->source_base(),cache->storage_size()).data()==cache->bytes(cache->source_base(),cache->storage_size()).data());
 reject([&]{(void)m.read(0x87cd50u,4);}); //No loaded-cache global is synthesized.
 managers->write(0xd71900,0xa1b2c3d4,4);require(m.read(0xd71900,4)==0xa1b2c3d4);
 m.write(0xd71900,0x98765432,4);require(managers->read(0xd71900,4)==0x98765432);
 const auto old=cache->read(cache->source_base(),4);const auto original=cache->original().read(cache->source_base(),cache->source_base(),4);
 m.write(cache->source_base(),0x12345678,4);require(cache->read(cache->source_base(),4)==0x12345678);
 require(cache->original().read(cache->source_base(),cache->source_base(),4)==original);
 cache->write(cache->source_base(),0xaabbccdd,4);require(m.read(cache->source_base(),4)==0xaabbccdd);cache->write(cache->source_base(),old,4);
 reject([&]{m.bind_managers(managers);});reject([&]{m.bind_cache(cache);});
 reject([&]{m.bind_managers(std::shared_ptr<NativeVistaWidgetManagers>{});});reject([&]{m.bind_cache(std::shared_ptr<NativeVistaWidgetCache>{});});
 std::array<std::byte,8> input{std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7},std::byte{8}};
 m.mount(0x10000,input,Access::ReadWrite,"explicit caller bytes");const auto held=m.mutable_view(0x10000,8);const auto pointer=held.data();input[0]=std::byte{9};require(m.read(0x10000,1)==1);
 for(unsigned i=0;i<64;++i)m.mount(0x20000+i*16,input,Access::ReadWrite,"separate retained caller bytes");
 require(m.mutable_view(0x10000,8).data()==pointer&&m.mutable_view(managers->source_base,1).data()==manager_pointer);
 reject([&]{m.mount(0x10004,input,Access::ReadWrite,"overlap");});reject([&]{m.mount(0x10000-4,input,Access::ReadWrite,"overlap");});
 reject([&]{m.mount(0xffffffffu,input,Access::ReadWrite,"wrap");});reject([&]{m.mount(0x40000,{},Access::ReadWrite,"empty");});reject([&]{m.mount(0x40000,input,Access::ReadWrite,"");});
 for(const auto n:{0u,3u,8u}){reject([&]{(void)m.read(0x10000,n);});reject([&]{m.write(0x10000,0,n);});}
 m.mount(0x30000,input,Access::ReadOnly,"explicit PE constant bytes");
 reject([&]{m.write(0x30000,0,1);});reject([&]{(void)m.mutable_view(0x30000,1);});require(m.read(0x30000,1)==9);
 m.write(0x10000,0x5a,1);reject([&]{m.write(0x30000,0,4);});require(m.read(0x10000,1)==0x5a); //Earlier source operation survives.
 m.release(0x10000);reject([&]{(void)m.read(0x10000,1);});reject([&]{m.release(0x10000);});reject([&]{m.release(0x20001);});
 m.mount(0x10000,input,Access::ReadWrite,"explicit remounted lifetime");require(m.read(0x10000,1)==9);
 auto borrowed=std::make_shared<std::vector<std::byte>>(input.begin(),input.end());std::weak_ptr<std::vector<std::byte>> weak=borrowed;
 const auto borrowed_pointer=borrowed->data();m.borrow(0x40000,std::span(*borrowed),Access::ReadWrite,borrowed,"actual borrowed mutable owner");borrowed.reset();
 require(!weak.expired()&&m.mutable_view(0x40000,8).data()==borrowed_pointer);m.write(0x40000,0x23,1);require(std::to_integer<unsigned>(weak.lock()->at(0))==0x23);
 m.release(0x40000);require(weak.expired());
 auto constant=std::make_shared<const std::vector<std::byte>>(input.begin(),input.end());
 m.borrow(0x41000,std::span<const std::byte>(*constant),constant,"actual immutable owner");require(m.view(0x41000,8).data()==constant->data());reject([&]{m.write(0x41000,0,1);});
 reject([&]{m.borrow(0x42000,std::span(input),Access::ReadWrite,{},"missing lifetime");});
 reject([&]{m.borrow(0x42000,std::span<const std::byte>(input),{},"missing lifetime");});
 auto shared_managers=std::make_shared<NativeVistaWidgetManagers>();std::weak_ptr<NativeVistaWidgetManagers> weak_managers=shared_managers;
 Memory shared;shared.bind_managers(shared_managers);shared_managers.reset();require(!weak_managers.expired());shared.release(managers->source_base);require(weak_managers.expired());
 auto shared_cache=std::make_shared<NativeVistaWidgetCache>(std::make_shared<const VistaWidgetCacheBank>(cache->original()),0x5000000u);std::weak_ptr<NativeVistaWidgetCache> weak_cache=shared_cache;
 shared.bind_cache(shared_cache);shared_cache.reset();require(!weak_cache.expired());shared.release(0x5000000u);require(weak_cache.expired());
 for(const auto alignment:{std::size_t{1},std::size_t{16},std::size_t{64},std::size_t{4096}}) {
  m.mount_aligned(0x50000,input,alignment,Access::ReadWrite,"explicit aligned pool backing");const auto data=m.mutable_view(0x50000,input.size());
  require(reinterpret_cast<std::uintptr_t>(data.data())%alignment==0);require(std::equal(input.begin(),input.end(),data.begin()));
  const auto aligned_pointer=data.data();m.mount(0x60000,input,Access::ReadWrite,"other stable bank");require(m.mutable_view(0x50000,input.size()).data()==aligned_pointer);m.release(0x60000);m.release(0x50000);
 }
 for(const auto alignment:{0u,3u})reject([&]{m.mount_aligned(0x50000,input,alignment,Access::ReadWrite,"invalid alignment");});
 reject([&]{m.mount_aligned(0x50001,input,64,Access::ReadWrite,"unaligned source identity");});
 m.mount_aligned(0x50000,input,64,Access::ReadOnly,"read-only aligned owner");reject([&]{m.write(0x50000,0,1);});require(m.read(0x50000,1)==9);
 std::array<std::byte,2> left{std::byte{1},std::byte{2}},right{std::byte{3},std::byte{4}};
 auto adjacent=std::make_shared<std::array<std::byte,2>>(right);
 m.mount(0x70000,left,Access::ReadWrite,"adjacent copied owner");m.borrow(0x70002,std::span(*adjacent),Access::ReadWrite,adjacent,"adjacent retained owner");
 require(m.read(0x70000,4)==0x04030201u);m.write(0x70000,0x88776655,4);require(m.read(0x70000,4)==0x88776655&&adjacent->at(0)==std::byte{0x77});
 reject([&]{(void)m.view(0x70000,4);});reject([&]{(void)m.mutable_view(0x70000,4);});
 m.mount(0x71000,left,Access::ReadWrite,"atomic first bank");m.mount(0x71002,right,Access::ReadOnly,"atomic read-only next bank");
 reject([&]{m.write(0x71000,0xffffffff,4);});require(m.read(0x71000,4)==0x04030201u);
 m.mount(0x72000,left,Access::ReadWrite,"atomic missing next bank");reject([&]{m.write(0x72000,0xffffffff,4);});require(m.read(0x72000,2)==0x0201);
 std::array<std::byte,4> output{std::byte{0xaa},std::byte{0xaa},std::byte{0xaa},std::byte{0xaa}};reject([&]{m.read(0x72000,std::span(output));});require(std::all_of(output.begin(),output.end(),[](auto b){return b==std::byte{0xaa};}));
 auto aliased=std::make_shared<std::array<std::byte,8>>(input);m.borrow(0x73000,std::span(*aliased),Access::ReadWrite,aliased,"alias owner A");m.borrow(0x74000,std::span(*aliased),Access::ReadWrite,aliased,"alias owner B");
 m.write(0x74001,m.view(0x73000,7));require(*aliased==std::array<std::byte,8>{input[0],input[0],input[1],input[2],input[3],input[4],input[5],input[6]});
 m.read(0x73001,m.mutable_view(0x74000,7));require(*aliased==std::array<std::byte,8>{input[0],input[1],input[2],input[3],input[4],input[5],input[6],input[6]});
 m.mount(0xfffffffcu,output,Access::ReadWrite,"last source32 bytes");require(m.read(0xffffffffu,1)==0xaa);reject([&]{m.write(0xffffffffu,0,2);});require(m.read(0xffffffffu,1)==0xaa);
 reject([&]{m.borrow(NativeVistaWidgetManagers::source_base,std::span(input),Access::ReadWrite,aliased,"manager authority overlap");});
 reject([&]{m.mount(cache->source_base(),input,Access::ReadWrite,"cache authority overlap");});
 require(m.read(0xd71900,4)==0x98765432&&m.mutable_view(managers->source_base,1).data()==manager_pointer);
 require(m.read(cache->source_base(),4)==old&&m.mutable_view(cache->source_base(),1).data()==cache->bytes(cache->source_base(),1).data());
 reject([&]{(void)m.read(0x42000,1);});
 return checks;
}

}
int main(int argc,char**argv)try{
 if(argc!=3)return 77;
 std::ifstream in(argv[2]);if(!in)return 77;Json doc;in>>doc;pool=&doc.at("byte_banks");std::size_t rows{},checks{};
 auto cache=load_cache(argv[1]);const auto boundary_checks=boundaries(cache);
 for(const auto& row:doc.at("cases")){Fixture f(row,cache);compare(f.run(),row.at("result"),rows,"result");compare(f.failure.empty()?Json(nullptr):Json(f.failure),row.at("failure"),rows,"failure");compare(f.trace,row.at("trace"),rows,"trace");compare(f.events,expanded(row.at("events")),rows,"events");Json banks=row.at("banks");for(auto& b:banks)b.at(1)=hex(unhex(b.at(1)));compare(f.snapshot(),banks,rows,"banks");checks+=5+f.trace.size()+f.events.size()+f.q.at("banks").size();++rows;}
 std::cout<<"PASS "<<rows<<" unchanged original disposal cases through retained native manager/cache/owned banks; "<<checks<<" original checks, "<<boundary_checks<<" native registry ownership/alignment/alias/boundary checks\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

#include "halo2/native_tu15_island_transition.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Invalid fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto b:b){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};
    mutable J reads=J::array();J fault;mutable unsigned fault_count{};
    std::vector<SourcePhysicsContactRecordStore>raw_stores;std::function<void(std::uint32_t,std::size_t)>on_write;
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing transition bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing transition extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
    void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);if(on_write)on_write(p,b.size());}
    void put(std::uint32_t p,std::uint32_t v,unsigned n=4){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8u*i));write(p,std::span(b).first(n));}
    std::uint32_t get(std::uint32_t p,unsigned n=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
};
constexpr std::uint32_t base=0x1000000,world=base+0x100,island=base+0x500,active=base+0x1000,inactive=base+0x1100,world_list=base+0x1200,bodies=base+0x1300,body=base+0x1800,body_list=base+0x2000,listener=base+0x2800;
Memory model(){Memory m;m.banks[base].resize(0x4000);m.banks[0x41d000].resize(0x1000);m.put(world+8,active);m.put(world+0xc,3);m.put(world+0x14,inactive);m.put(world+0x18,1);m.put(world+0x1c,8);m.put(active,island);m.put(active+4,base+0x600);m.put(active+8,base+0x700);m.put(island+0x24,0);m.put(island+0x29,1,1);m.put(world+0x78,world_list);m.put(island+0x3c,bodies);m.put(bodies,body);m.put(body+0x70,body_list);for(unsigned n=0;n<8;++n)m.put(listener+n*0x10,0x41d964);m.put(0x41d968,0x3167a0);m.put(0x41d96c,0x316790);m.put(0x41d95c,0x2e4d60);m.put(0x41d960,0x2e4d60);return m;}
void models(){
    {auto m=model();NativeTu15IslandTransitionMemory s(m);NativeTu15PhysicsFrameFp fp;
     native_tu15_deactivate_physics_island(s,{world},{island},fp,{});
     check(m.get(inactive+4)==island&&m.get(world+0x18)==2&&m.get(active)==base+0x700&&m.get(world+0xc)==2&&m.get(base+0x724)==0&&m.get(island+0x24)==1&&m.get(island+0x29,1)==0,"Append/swap-remove/index/flag transition");}
    {auto m=model();m.put(world+0x1c,1);NativeTu15IslandTransitionMemory s(m);NativeTu15PhysicsFrameFp fp;NativeTu15IslandTransitionServices services;unsigned calls{};
     services.grow_array=[&](auto header,auto stride,auto&context){check(header.identity==world+0x14&&stride==4,"Actual growth header/stride");++calls;m.put(world+0x14,base+0x1500);m.put(world+0x18,2);m.put(world+0x1c,4);context.status=0x4501;context.mxcsr=0x5fa0;};
     native_tu15_deactivate_physics_island(s,{world},{island},fp,services);
     check(calls==1&&m.get(base+0x1508)==island&&m.get(island+0x24)==2&&fp.status==0x4501&&fp.mxcsr==0x5fa0,"Growth publishes actual new backing/count and FP");}
    {auto m=model();m.put(world+0x7c,4);for(unsigned n=0;n<4;++n)m.put(world_list+n*4,listener+n*0x10);NativeTu15IslandTransitionMemory s(m);NativeTu15PhysicsFrameFp fp;NativeTu15IslandTransitionServices services;std::vector<unsigned>seen;
     services.island_deactivated=[&](auto receiver,auto owner,auto subject,auto&context){check(owner.identity==0x316790&&subject.identity==island,"Island notification identity/owner");seen.push_back(receiver.identity);
       if(seen.size()==1){m.put(world+0x78,base+0x1400);m.put(base+0x1400,listener+0x40);m.put(base+0x1404,0);m.put(base+0x1408,listener+0x50);m.put(world+0x7c,3);}context.control_word=0x37f;context.status=0x2801;};
     native_tu15_notify_island_deactivation(s,{world},{island},fp,services);
     check(seen==std::vector<unsigned>{listener+0x30,listener+0x50,listener+0x40}&&m.get(world+0x7c)==2&&m.get(base+0x1404)==listener+0x50&&fp.control_word==0x37f&&fp.status==0x2801,"Reverse callbacks, reloaded list, postcallback null compaction and FP");}
    {auto m=model();m.put(island+0x40,1);m.put(body+0x74,3);m.put(body_list,listener);m.put(body_list+4,0);m.put(body_list+8,listener+0x10);NativeTu15IslandTransitionMemory s(m);NativeTu15PhysicsFrameFp fp;NativeTu15IslandTransitionServices services;std::vector<unsigned>arguments;
     services.body_deactivated=[&](auto,auto owner,auto argument,auto&){check(owner.identity==0x3167a0,"Body listener owner");arguments.push_back(argument.identity);if(arguments.size()==1){m.put(bodies,body+0x100);m.put(island+0x40,2);m.put(bodies+4,body+0x200);m.put(body+0x270,base+0x2300);}};
     native_tu15_notify_island_deactivation(s,{world},{island},fp,services);
     check(arguments==std::vector<unsigned>{body,body+0x100}&&m.get(body+0x74)==2&&m.get(body_list+4)==listener+0x10,"Body argument reload, cached original listener header, cohort growth and compaction");}
    {auto m=model();unsigned changes{};m.on_write=[&](auto address,auto){if(address==active&&++changes==1){m.put(island+0x24,1);m.put(world+8,base+0x1400);m.put(base+0x1404,base+0x800);}};NativeTu15IslandTransitionMemory s(m);NativeTu15PhysicsFrameFp fp;
     native_tu15_deactivate_physics_island(s,{world},{island},fp,{});check(m.get(base+0x824)==1,"Moved island lookup must reread index and active backing after slot publication");}
    {auto m=model();m.put(world+0x7c,1);m.put(world_list,listener);m.put(listener,0x41d958);NativeTu15IslandTransitionMemory s(m);NativeTu15PhysicsFrameFp fp;native_tu15_notify_island_deactivation(s,{world},{island},fp,{});check(m.get(world+0x7c)==1,"Authenticated default listener must execute without a residual");}
    std::cout<<"Transition mutable backing/count/index/listeners, reverse callbacks, stable null compaction, cached body header, real FP transfer and native default listener models passed\n";
}
void oracle(const std::filesystem::path&path){
    check(std::filesystem::exists(path),"Required TU1.5 transition fixture absent");std::ifstream in(path);const auto f=J::parse(in);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-island-transition-physical-operands-stores-full-fp-v1","Required physical original transition authority absent");
    check(f.at("transition_sites")==39&&f.at("notification_sites")==98&&f.at("default_listener_sites")==1,"Authenticated original transition bodies changed");
    std::size_t cases{},writes{},reads{},faults{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" lacks physical original proof");Memory original;
        for(const auto&b:c.at("banks"))check(original.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate transition bank");original.fault=c.value("fault",J{});
        for(bool read_pass:{false,true}){
            auto m=original;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;
            SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&store){stores.push_back(store);};
            NativeTu15IslandTransitionMemory storage(m,observer);NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
            bool boundary{},failed{};NativeTu15IslandTransitionServices services;
            auto stop=[&](const char*kind,std::uint32_t receiver,std::uint32_t owner,std::uint32_t argument){const auto&e=c.at("boundary");check(e!=nullptr&&e.at("kind")==kind&&e.at("receiver")==receiver&&e.at("owner")==owner&&e.at("argument")==argument,name+" actual transition dependency ABI differs");boundary=true;throw std::runtime_error("original-transition-boundary");};
            services.grow_array=[&](auto header,auto stride,auto&){stop("growth",header.identity,0x2e3250,stride);};
            services.island_deactivated=[&](auto listener,auto owner,auto island,auto&){stop("island_listener",listener.identity,owner.identity,island.identity);};
            services.body_deactivated=[&](auto listener,auto owner,auto body,auto&){stop("body_listener",listener.identity,owner.identity,body.identity);};
            try{if(c.at("entry")==0x316cd0)native_tu15_deactivate_physics_island(storage,{c.at("world")},{c.at("island")},fp,services);else native_tu15_notify_island_deactivation(storage,{c.at("world")},{c.at("island")},fp,services);}
            catch(const Fault&){failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="original-transition-boundary")throw;}
            check(failed==fault_case&&boundary==!c.at("boundary").is_null(),name+" actual dependency/read-fault completion differs");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original transition read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();}
            else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered transition store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered transition store differs at "+std::to_string(n));}writes+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained transition bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete transition FP differs");
        }++cases;faults+=!c.value("fault",J{}).is_null();
    }check(cases>0,"Empty required original transition fixture");std::cout<<cases<<" physical original TU1.5 transition cases, "<<writes<<" ordered stores, "<<reads<<" ordered reads and "<<faults<<" read-fault prefixes passed; growth/custom callbacks/live composition remain explicit\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();if(argc==2)oracle(argv[1]);else check(argc==1,"Supply optional required transition fixture");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

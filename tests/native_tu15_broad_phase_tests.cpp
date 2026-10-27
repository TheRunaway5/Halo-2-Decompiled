#include "halo2/native_tu15_broad_phase.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
#include <x86intrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid collision fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};struct Stop{std::string kind;std::uint32_t owner;std::vector<std::uint32_t>arguments;};
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};std::vector<SourcePhysicsContactRecordStore>raw_stores;
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing collision bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing collision extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&fault.value("access","read")=="read"&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(!fault.is_null()&&fault.value("access","read")=="write"&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
 void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8u*i));write(p,b);}
 std::uint32_t get(std::uint32_t p,unsigned n=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
};
std::uint16_t host_status(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
std::uint16_t host_control(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
void models(){
 constexpr std::uint32_t base=0x1000000,p=base+0x100,w=base+0x300,allocator=base+0x600,dispatcher=base+0x700,gravity=base+0x800,first=base+0x1000,second=base+0x1020,arena=base+0x2000;
 Memory m;m.banks[base].resize(0x10000);m.banks[0x48c000].resize(0x1000);m.banks[0x46a000].resize(0x1000);m.put(0x48c938,allocator);m.put(allocator+8,arena);m.put(allocator+0xc,0x5000);m.put(allocator+0x14,arena+0x7000);m.put(p+0x20,w);m.put(w+0x238,1);m.put(w+0xcc,gravity);m.put(gravity+8,0xc0800000);m.put(0x46a0cc,0x3f000000);m.put(p+0x3c,base+0x1800);m.put(p+0x40,0);m.put(dispatcher,base+0x900);m.put(base+0x914,0x12345678);
 NativeTu15BroadPhaseServices s;NativeTu15PhysicsFrameFp fp;std::vector<std::string>order;const auto pending=base+0x5000;const auto old_data=base+0x5200,new_data=base+0x5300;
 s.sweep=[&](auto receiver,auto owner,auto entries,auto bounds,auto count,auto a,auto b,auto&){order.push_back("sweep");check(receiver.identity==dispatcher&&owner.identity==0x12345678&&count==0&&a.identity==first&&b.identity==second,"Real sweep arguments differ");check(entries==arena+32&&bounds==arena+32,"Zero-body native arena blocks differ");m.put(first,old_data);m.put(first+4,1);m.put(first+8,1);m.put(p+0x74,pending);m.put(p+0x78,2);m.put(pending,0x11111111);m.put(pending+4,0x22222222);m.put(pending+8,0x33333333);m.put(pending+12,0x44444444);};
 s.grow_pairs=[&](auto a,auto count,auto stride,auto&){order.push_back("grow");check(a.identity==first&&count==3&&stride==8,"Actual pair capacity growth arguments differ");m.put(first,new_data);m.put(first+8,0x80000003);};
 s.consume_pairs=[&](auto subject,auto a,auto b,auto&){order.push_back("consume");check(subject.identity==p&&a.data==new_data&&a.count==3&&b.count==0,"Actual pair consumer identities/counts differ");check(m.get(new_data+8)==0x11111111&&m.get(new_data+12)==0x22222222&&m.get(new_data+16)==0x33333333&&m.get(new_data+20)==0x44444444&&m.get(p+0x78)==0,"Pending pair append/publication differs");};
 native_tu15_update_broad_phase(m,{p},{dispatcher},{w+0x140},{{first},{second}},fp,0,s);
 check(order==std::vector<std::string>{"sweep","grow","consume"},"Broad-phase continuation order differs");check(m.get(p+0x30,1)==1&&m.get(allocator+8)==arena&&m.get(allocator+0xc)==0x5000,"Temporary pair/bounds/index lifetime differs");
 std::cout<<"Mutable real-owner continuation model covers pair growth, append, consume and native temporary lifetime; not an original callback result\n";
}
void oracle(const std::filesystem::path&path){
 check(std::filesystem::exists(path),"Required original broad-phase fixture missing");std::ifstream input(path);const auto f=J::parse(input);check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-broad-phase-physical-semantic-packets-fp-v1","Required original broad-phase authority absent");check(f.at("body_sites")==J::array({278,49,52}),"Original broad-phase roots changed");
 std::size_t count{},writes{},read_count{},faults{},complete{},prefixes{};
 for(const auto&c:f.at("cases")){
  const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" original not physically verified");Memory original;for(const auto&b:c.at("banks"))original.banks.emplace(b.at("address"),bytes(b.at("before")));original.fault=c.value("fault",J{});
  for(bool read_pass:{false,true}){
   auto m=original;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);};NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15BroadPhaseServices services;
   services.allocate_block=[](auto receiver,auto owner,auto bytes,auto&)->std::uint32_t{throw Stop{"allocate",owner.identity,{receiver.identity,bytes}};};services.release_block=[](auto receiver,auto owner,auto block,auto bytes,auto&){throw Stop{"release",owner.identity,{receiver.identity,block,bytes}};};services.release_owned_pairs=[](auto receiver,auto owner,auto data,auto bytes,auto cls,auto&){throw Stop{"owned",owner.identity,{receiver.identity,data,bytes,cls}};};
   services.shape_bounds=[](auto receiver,auto owner,auto transform,auto margin,auto output,auto&){throw Stop{"shape",owner.identity,{receiver.identity,transform,margin,output}};};services.sweep=[](auto receiver,auto owner,auto entries,auto bounds,auto n,auto first,auto second,auto&){throw Stop{"sweep",owner.identity,{receiver.identity,entries,bounds,n,first.identity,second.identity}};};
   bool failed{},stopped{};Stop stop{};const auto cw=host_control(),sw=host_status();const auto mxcsr=_mm_getcsr();
   try{const auto entry=c.at("entry").get<std::uint32_t>();if(entry==0x3177e0)native_tu15_construct_broad_phase_pairs(m,{c.at("header")},c.at("reserve"),fp,services,observer);else if(entry==0x317870)native_tu15_destroy_broad_phase_pairs(m,{c.at("header")},fp,services,observer);else native_tu15_update_broad_phase(m,{c.at("island")},{c.at("dispatcher")},{c.at("configuration")},{{c.at("first")},{c.at("second")}},fp,0,services,observer);}catch(const Fault&){failed=true;}catch(const Stop&s){stopped=true;stop=s;}
   check(failed==fault_case,name+" memory-fault completion differs");if(!fault_case){const auto&b=c.at("boundary");check(stopped==!b.is_null(),name+" actual remaining-owner completion differs");if(stopped){auto args=b.at("arguments").get<std::vector<std::uint32_t>>();args.insert(args.begin(),b.at("receiver").get<std::uint32_t>());check(stop.kind==b.at("kind").get<std::string>()&&stop.owner==b.at("owner")&&stop.arguments==args,name+" actual remaining-owner ABI differs native "+J(stop.arguments).dump()+" original "+J(args).dump());}}
   check(host_control()==cw&&host_status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host numeric context");
   if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original broad-phase read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}read_count+=m.reads.size();}
   else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" broad-phase store count differs "+std::to_string(stores.size())+"/"+std::to_string(w.size()));for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered broad-phase store differs at "+std::to_string(n));}writes+=stores.size();}
   for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained broad-phase bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete broad-phase numeric state differs native "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original "+end.dump());
  }++count;faults+=!c.value("fault",J{}).is_null();if(c.value("fault",J{}).is_null()){complete+=c.at("boundary").is_null();prefixes+=!c.at("boundary").is_null();}
 }std::cout<<count<<" physical broad-phase/pair-lifetime cases, "<<complete<<" complete, "<<prefixes<<" real owner stops, "<<writes<<" stores, "<<read_count<<" reads and "<<faults<<" memory-fault prefixes passed\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();if(argc==2)oracle(argv[1]);else check(argc==1,"Supply optional required original broad-phase fixture");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

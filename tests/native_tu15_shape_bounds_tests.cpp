#include "halo2/native_tu15_shape_bounds.hpp"
#ifdef HALO2_SHAPE_CENTERED
#include "halo2/native_tu15_centered_shape_bounds.hpp"
#endif
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Prefix{};
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;std::string phase;unsigned ordinal{};mutable unsigned attempts{};unsigned writes{};bool logging{},raw_logging{};mutable J reads=J::array();std::vector<SourcePhysicsContactRecordStore>raw;
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing shape bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing shape extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(phase=="before_read"&&attempts++==ordinal)throw Prefix{};if(logging)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(phase=="before_write"&&writes++==ordinal)throw Prefix{};auto i=at(p,b.size());if(raw_logging)raw.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
struct Host {
 alignas(16)std::array<std::byte,108>outer{},expected{};unsigned mxcsr;
 Host():mxcsr(_mm_getcsr()){asm volatile("fnsave %0":"=m"(outer)::"memory");asm volatile("fld1\n\tfldz\n\tfldpi":::"memory","st","st(1)","st(2)");asm volatile("fnsave %0":"=m"(expected)::"memory");std::fill(expected.begin()+12,expected.begin()+24,std::byte{});asm volatile("frstor %0"::"m"(expected):"memory");}
 ~Host(){asm volatile("frstor %0"::"m"(outer):"memory");_mm_setcsr(mxcsr);}
 void verify(){alignas(16)std::array<std::byte,108>actual{};asm volatile("fnsave %0":"=m"(actual)::"memory");asm volatile("frstor %0"::"m"(actual):"memory");check(actual==expected&&_mm_getcsr()==mxcsr,"Shape bounds leaked108-byte host FP state");}
};
}
int main(int argc,char**argv){try{check(argc==2,"Physical shape bounds fixture required");std::ifstream input(argv[1]);const auto f=J::parse(input);check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("body_sites")==
#ifdef HALO2_SHAPE_CENTERED
J::array({48,13,25,3,48,40,61,15})
#else
J::array({48,13,25,3,48,40})
#endif
,"Shape bounds source authority changed");std::size_t count{},reads{},writes{};
 for(const auto&test:f.at("cases")){const auto name=test.at("name").get<std::string>();Memory before;for(const auto&b:test.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));const auto fault=test.value("fault",J::object());before.phase=fault.value("phase",std::string{});before.ordinal=fault.value("ordinal",0u);
  for(bool reading:{false,true}){if(reading&&before.phase=="after_write")continue;auto m=before;m.logging=reading;m.raw_logging=!reading&&(before.phase=="before_read"||before.phase=="before_write");std::vector<SourcePhysicsContactRecordStore>stores;unsigned observed{};SourcePhysicsContactRecordObserver observer;if(!reading&&!m.raw_logging)observer=[&](const auto&s){stores.push_back(s);if(before.phase=="after_write"&&observed++==before.ordinal)throw Prefix{};};NativeTu15PhysicsFrameFp fp{test.at("before_fp")[0],test.at("before_fp")[1],test.at("before_fp")[2]};NativeTu15ShapeBoundsStats stats;const auto depth=test.at("live_x87").size();
   std::function<void(std::uint32_t,NativeTu15BoundsShape,NativeTu15BoundsTransform,std::uint32_t,NativeTu15BoundsOutput,std::uint32_t)>run;
   run=[&](std::uint32_t owner,NativeTu15BoundsShape shape,NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput out,std::uint32_t sp){NativeTu15ShapeBoundsServices services;services.child_bounds=[&](auto child,auto actual,auto matrix,auto expanded,auto output,auto&state){check(&state==&fp,"Different shape FP dependency");const auto aligned=(sp-4u)&~15u;const auto next=owner==0x2e8870u?aligned-0x60u:owner==0x2e5e80u?aligned-0x20u:sp;run(actual,child,matrix,expanded,output,next);};switch(owner){
#ifdef HALO2_SHAPE_CENTERED
case 0x1d64c0:native_tu15_project_centered_box_bounds(m,transform,shape.identity,test.at("center"),margin,out,fp,observer);break;case 0x307e30:native_tu15_centered_shape_bounds(m,shape,transform,margin,out,fp,depth,observer);break;
#endif
case 0x306690:native_tu15_project_box_bounds(m,transform,shape.identity,margin,out,fp,observer,&stats);break;case 0x306760:native_tu15_convex_box_bounds(m,shape,transform,margin,out,fp,depth,observer,&stats);break;case 0x2e8870:native_tu15_transformed_shape_bounds(m,shape,transform,[&]{return margin;},[&]{return out;},{((sp-4u)&~15u)-0x40u},fp,services,observer,&stats);break;case 0x2e9600:native_tu15_forward_shape_bounds(m,shape,transform,margin,out,fp,services,&stats);break;case 0x2e5e80:native_tu15_motion_shape_bounds(m,shape,transform,margin,out,fp,services,observer,&stats);break;default:throw std::runtime_error("Unconverted actual child bounds owner");}};
   Host host;bool stopped{},alignment{};try{run(test.at("entry"),{test.at("shape")},{test.at("transform")},test.at("margin_bits"),{test.at("output")},test.at("gp")[4]);}catch(const Prefix&){stopped=true;}catch(const std::runtime_error&e){const std::string message=e.what();alignment=message=="Native TU1.5 shape bounds requires actual aligned vector storage"||message=="Native TU1.5 geometry requires aligned vector storage"||message=="Native TU1.5 centered bounds requires aligned source storage"||message=="Native TU1.5 centered bounds requires aligned output storage";if(!alignment)throw;}host.verify();check(stopped==!before.phase.empty()&&alignment==test.value("alignment_fault",false),name+" shape prefix/alignment differs");
   if(reading){J expected=J::array();for(const auto&r:test.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" ordered shape read differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();}
   else{if(m.raw_logging)stores=m.raw;const auto&expected=test.at("writes");check(stores.size()==expected.size(),name+" ordered shape store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check((m.raw_logging||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered shape store differs");}writes+=stores.size();}
   for(const auto&b:test.at("banks")){const auto&actual=m.banks.at(b.at("address"));const auto expected=bytes(b.at("after"));if(actual!=expected){std::size_t n{};while(n<actual.size()&&actual[n]==expected[n])++n;throw std::runtime_error(name+" retained shape bytes differ at "+std::to_string(b.at("address").get<std::uint32_t>()+n));}}const auto&after=test.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" complete shape FP differs native="+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+after.dump());
  }++count;
 }std::cout<<count<<" unchanged original complete native shape chains/prefixes, "<<reads<<" reads "<<writes<<" stores passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

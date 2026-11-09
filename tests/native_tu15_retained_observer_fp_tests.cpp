#include "halo2/native_tu15_retained_store_observer.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/native_tu15_joint_constraint_numeric_internal.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstring>
using namespace halo2;
namespace raw=halo2::native_raw_x87_detail;
using J=nlohmann::json;
namespace {
struct Failure{};
void check(bool condition,const char*why){if(!condition)throw std::runtime_error(why);}
raw::Image image(const J&r){raw::Image result;const auto text=r.at("x87").get<std::string>();check(text.size()==216,"Invalid observer input image");for(unsigned i=0;i<108;++i)result.x87[i]=std::byte(std::stoul(text.substr(i*2,2),nullptr,16));result.mxcsr=r.at("mxcsr");return result;}
raw::Image host(const raw::Image&source,unsigned count){
 auto result=source;result.word(0,std::uint16_t((source.word(0)&~0xc00u)|(((source.word(0)+0x400u)&0xc00u))));
 result.word(4,std::uint16_t((source.word(4)&~0x3800u)|(((source.top()+5)&7u)<<11u)));result.mxcsr=0x1f80|((source.mxcsr^0x6000u)&0xe07fu);
 constexpr unsigned char values[4][10]={{0,0,0,0,0,0,0,0x80,0xff,0x3f},{0,0,0,0,0,0,0,0,0,0x80},{0x45,0x23,1,0,0,0,0,0xc0,0xff,0x7f},{1,0,0,0,0,0,0,0,0,0}};
 std::uint16_t tags=0xffff;
 for(unsigned n=0;n<count;++n){std::memcpy(result.x87.data()+28+n*10,values[n%4],10);const auto shift=((result.top()+n)&7)*2;tags=std::uint16_t((tags&~(3u<<shift))|(raw::value_tag(result.x87.data()+28+n*10)<<shift));}
 result.word(8,tags);return result;
}
void disturb(unsigned mode){
 const std::uint16_t masked=0x077f,invalid=0x037e,divide=0x037b;const std::uint32_t zero{};
 if(mode==1)asm volatile("fninit;fld1;fldz;fdivp;fldcw %0"::"m"(masked):"memory","st","st(1)");
 if(mode==2)asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(invalid):"memory","st","st(1)");
 if(mode==3)asm volatile("fninit;fld1;fdivs %0;fldcw %1"::"m"(zero),"m"(divide):"memory","st");
 if(mode==4)asm volatile("fninit;fstp %%st(0);fldcw %0"::"m"(invalid):"memory","st");
 if(mode)_mm_setcsr(0x8065u);
}

struct WordMemory:SourcePhysicsContactMemory {
 std::array<std::byte,4> bytes{std::byte{0x11},std::byte{0x22},std::byte{0x33},std::byte{0x44}};unsigned reads{},writes{};
 void read(std::uint32_t address,std::span<std::byte>out)const override {check(address==0x61000000u&&out.size()==4,"Numeric read escaped word");++const_cast<WordMemory*>(this)->reads;std::memcpy(out.data(),bytes.data(),4);}
 void write(std::uint32_t address,std::span<const std::byte>in)override {check(address==0x61000000u&&in.size()==4,"Numeric write escaped word");++writes;std::memcpy(bytes.data(),in.data(),4);}
};
unsigned integration(const J&rows){
 unsigned comparisons{};
 for(const auto&row:rows)for(unsigned mode=0;mode<5;++mode)for(bool throwing:{false,true})for(bool spill:{false,true}){
  if(spill&&row.at("count")==0)continue;
  const auto source=image(row.at("before")),captured=host(source,4);auto expected=source;std::uint32_t bits=0xaabbccddu;
  if(spill){raw::restore(source);asm volatile("fstps %0":"=m"(bits)::"memory","st");expected=raw::capture();}
  WordMemory storage;const auto preimage=storage.bytes;unsigned calls{};raw::restore(captured);
  NativeTu15RetainedStoreObserver observer{[&](const auto&event){
   check(raw::equivalent(raw::capture(),captured),"Numeric observer did not acquire captured host");check(event.instruction==0x123456u&&event.address==0x61000000u&&event.before==std::vector<std::byte>(preimage.begin(),preimage.end()),"Numeric observer preimage/protocol changed");
   std::uint32_t actual{};check(event.written.size()==4,"Numeric event has wrong width");std::memcpy(&actual,event.written.data(),4);check(actual==bits,"Numeric observer changed completed bits");++calls;disturb(mode);if(throwing)throw Failure{};
  }};
  raw::restore(source);joint_constraint_numeric::Memory memory(storage,observer.callback());bool caught=false;
  try{if(spill)memory.pop_float(0x123456u,0x61000000u);else memory.word(0x123456u,0x61000000u,bits);}catch(const Failure&){caught=true;}
  check(caught==throwing&&calls==1&&storage.reads==1&&storage.writes==1,"Numeric completed-observer continuation changed");check(raw::equivalent(raw::capture(),expected),"Numeric completed observer changed native post-write FP");std::uint32_t written{};std::memcpy(&written,storage.bytes.data(),4);check(written==bits,"Numeric observer failure lost completed write");++comparisons;
 }
 return comparisons;
}

}
int main(int argc,char**argv)try{
 check(argc==2||argc==3,"Supply observer FP corpus");std::ifstream in(argv[1]);const auto fixture=J::parse(in);const auto caller=raw::capture();unsigned comparisons{},callbacks{};
 SourcePhysicsContactRecordStore event{0x123456u,0x61000000u,{std::byte{0x11}},{std::byte{0x22}}};
 for(const auto&row:fixture.at("cases"))for(unsigned host_count:{0u,1u,4u,8u})for(unsigned mode=0;mode<5;++mode)for(bool throwing:{false,true}){
  const auto source=image(row.at("before")),captured=host(source,host_count);raw::restore(captured);
  NativeTu15RetainedStoreObserver observer{[&](const auto&actual){
   check(raw::equivalent(raw::capture(),captured),"Callback did not acquire captured host FP");
   check(actual.instruction==event.instruction&&actual.address==event.address&&actual.before==event.before&&actual.written==event.written,"Observer event changed");++callbacks;disturb(mode);if(throwing)throw Failure{};
  }};
  raw::restore(source);bool caught=false;try{observer.callback()(event);}catch(const Failure&){caught=true;}
  check(caught==throwing,"Observer exception transport changed");check(raw::equivalent(raw::capture(),source),"Callback changed native payload/tags/TOP/SW/CW/MXCSR");++comparisons;
 }
 const auto integrated=argc==3?integration(fixture.at("cases")):0u;raw::restore(caller);check(comparisons==callbacks,"Observer callback count differs");std::cout<<J{{"passed",true},{"comparisons",comparisons},{"callbacks",callbacks},{"normal_and_throwing",true},{"numeric_memory_transactions",integrated}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

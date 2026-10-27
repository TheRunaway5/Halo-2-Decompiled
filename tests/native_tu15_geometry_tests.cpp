#include "halo2/native_tu15_geometry.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Invalid geometry fixture");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct PrefixStop{};
struct Memory:SourcePhysicsContactMemory{
 std::uint32_t first{};std::vector<std::byte>bank;mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
 std::string phase;unsigned ordinal{};mutable unsigned attempted_reads{};unsigned attempted_writes{};
 void read(std::uint32_t p,std::span<std::byte>b)const override{check(p>=first&&std::uint64_t(p-first)+b.size()<=bank.size(),"Missing geometry memory");std::copy_n(bank.begin()+p-first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{if(phase=="before_read"&&attempted_reads++==ordinal)throw PrefixStop{};reads.emplace_back(p,b.size());read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(phase=="before_write"&&attempted_writes++==ordinal)throw PrefixStop{};check(p>=first&&std::uint64_t(p-first)+b.size()<=bank.size(),"Missing geometry write extent");std::copy(b.begin(),b.end(),bank.begin()+p-first);}
};
void run(unsigned entry,NativeTu15GeometryStorage&s,NativeTu15PhysicsFrameFp&fp){switch(entry){case 0x2e44d0:native_tu15_compose_affine_geometry(s,fp);break;case 0x2e47b0:native_tu15_transform_transposed_direction(s,fp);break;case 0x2e4690:native_tu15_transform_geometry_position(s,fp);break;default:throw std::runtime_error("Unknown geometry owner");}}
struct ValueStorage:NativeTu15GeometryStorage{
 std::array<NativeTu15GeometryVector,4>basis{},source{},destination{};
 NativeTu15GeometryVector basis_column(unsigned n)const override{return basis.at(n);}
 NativeTu15GeometryVector source_column(unsigned n)const override{return source.at(n);}
 NativeTu15GeometryVector destination_column(unsigned n)const override{return destination.at(n);}
 void destination_column(unsigned n,const NativeTu15GeometryVector&v,NativeTu15GeometryPublication)override{destination.at(n)=v;}
};
NativeTu15GeometryVector vector(std::span<const std::byte>b,unsigned offset){NativeTu15GeometryVector v{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)v[n]|=std::to_integer<unsigned>(b[offset+n*4+k])<<(k*8u);return v;}
}
int main(int argc,char**argv){try{
 check(argc==2,"Supply physical original geometry fixture");std::ifstream stream(argv[1]);const auto f=J::parse(stream);check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original geometry image changed");check(f.at("required_backend")=="unchanged-tu15-geometry-physical-operands-fp-v1","Physical geometry authority missing");std::size_t count{},reads{},writes{},prefixes{},alignments{},pure{};
 for(const auto&c:f.at("cases")){
  const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>()&&c.at("live_x87_unchanged").get<bool>(),name+" lacks physical/x87 verification");check(c.at("banks").size()==1,"Unexpected geometry bank count");Memory m;m.first=c.at("banks")[0].at("address");m.bank=bytes(c.at("banks")[0].at("before"));const auto initial=m.bank;const auto fault=c.value("fault",J::object());m.phase=fault.value("phase","");m.ordinal=fault.value("ordinal",0u);
  std::vector<SourcePhysicsContactRecordStore>stores;unsigned observer_ordinal{};NativeTu15GeometryMemory view(m,c.at("destination"),c.at("matrix"),c.at("source"),[&](const auto&s){stores.push_back(s);if(m.phase=="after_write"&&observer_ordinal++==m.ordinal)throw PrefixStop{};});NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
  const auto host_mxcsr=_mm_getcsr();alignas(16)std::array<std::byte,108>host{},expected_host{},actual_host{};
  asm volatile("fnsave %0":"=m"(host)::"memory");asm volatile("fld1\n\tfldz\n\tfldpi":::"memory","st","st(1)","st(2)");asm volatile("fnsave %0":"=m"(expected_host)::"memory");
  // This host clears saved instruction/data-pointer metadata on sched_yield,
  // even when no geometry code runs (separate physical control retained).
  // Install zero metadata before the witness, keeping the complete108-byte
  // equality gate, all live values, tags, CW/SW/TOP and reserved bytes intact.
  std::fill(expected_host.begin()+12,expected_host.begin()+24,std::byte{});asm volatile("frstor %0"::"m"(expected_host):"memory");
  bool stopped{},alignment{};
  try{run(c.at("entry"),view,fp);}catch(const PrefixStop&){stopped=true;}catch(const std::runtime_error&e){alignment=std::string_view(e.what())=="Native TU1.5 geometry requires aligned vector storage";if(!alignment){asm volatile("frstor %0"::"m"(host):"memory");throw;}}
  asm volatile("fnsave %0\n\tfrstor %1":"=m"(actual_host):"m"(host):"memory");if(expected_host!=actual_host)std::cerr<<name<<" host x87 before="<<hex(expected_host)<<" after="<<hex(actual_host)<<'\n';check(expected_host==actual_host,name+" changed unrelated live host x87 state");check(_mm_getcsr()==host_mxcsr,name+" leaked SIMD controls/status");
  const bool completed=c.at("completed");check(completed==(!stopped&&!alignment),name+" completion differs");check(alignment==c.value("alignment_fault",false),name+" source alignment boundary differs");check(m.bank==bytes(c.at("banks")[0].at("after")),name+" retained geometry bytes differ");
  const auto&original_reads=c.at("reads");check(m.reads.size()==original_reads.size(),name+" ordered geometry read count differs");for(std::size_t n=0;n<m.reads.size();++n)check(m.reads[n].first==original_reads[n].at("address")&&m.reads[n].second==original_reads[n].at("bytes"),name+" ordered geometry operand read differs");
  check(stores.size()==c.at("writes").size(),name+" ordered geometry store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=c.at("writes")[n];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered geometry store differs at "+std::to_string(n));}
  const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete geometry FP differs native "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original "+end.dump());
  if(c.value("pure_typed",false)){
   ValueStorage values;const unsigned matrix=c.at("matrix").get<unsigned>()-m.first,source=c.at("source").get<unsigned>()-m.first,destination=c.at("destination").get<unsigned>()-m.first;for(unsigned n=0;n<4;++n){values.basis[n]=vector(initial,matrix+n*16);values.source[n]=vector(initial,source+n*16);values.destination[n]=vector(initial,destination+n*16);}NativeTu15PhysicsFrameFp actual{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};run(c.at("entry"),values,actual);for(unsigned n=0;n<(c.at("entry")==0x2e44d0?4u:1u);++n)check(values.destination[n]==vector(m.bank,destination+n*16),name+" pure typed geometry differs");check(actual.control_word==fp.control_word&&actual.status==fp.status&&actual.mxcsr==fp.mxcsr,name+" pure typed geometry FP differs");++pure;
  }
  ++count;reads+=m.reads.size();writes+=stores.size();prefixes+=stopped;alignments+=alignment;
 }
 check(count&&prefixes&&alignments&&pure,"Geometry acceptance matrix incomplete");std::cout<<count<<" physical original geometry cases, "<<reads<<" ordered reads, "<<writes<<" stores, "<<prefixes<<" rejected/committed prefixes, "<<alignments<<" actual alignment faults, "<<pure<<" pure typed cases; full FP/banks and host x87 preservation passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

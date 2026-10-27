#include "halo2/native_tu15_contact_records.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;using Record=NativeTu15ContactRecord;using Phase=NativeTu15ContactRecordPublication;using Vector=NativeTu15ContactRecordVector;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct PrefixStop{};
struct Memory:SourcePhysicsContactMemory{
 struct Bank{std::uint32_t address;std::vector<std::byte>data;};std::vector<Bank>banks;std::string phase;unsigned ordinal{},write_ordinal{};
 void read(std::uint32_t p,std::span<std::byte>b)const override{for(const auto&bank:banks)if(p>=bank.address&&std::uint64_t(p-bank.address)+b.size()<=bank.data.size()){std::copy_n(bank.data.begin()+p-bank.address,b.size(),b.begin());return;}throw std::runtime_error("Missing contact record input extent");}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(phase=="before_write"&&write_ordinal++==ordinal)throw PrefixStop{};for(auto&bank:banks)if(p>=bank.address&&std::uint64_t(p-bank.address)+b.size()<=bank.data.size()){std::copy(b.begin(),b.end(),bank.data.begin()+p-bank.address);return;}throw std::runtime_error("Missing contact record output extent");}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
};
// The semantic access wrapper distinguishes real source operands from the
// observer's diagnostic preimage reads. Rejected reads are not completed reads.
struct Storage:NativeTu15ContactRecordStorage{
 NativeTu15ContactRecordMemory&memory;std::string phase;unsigned ordinal{};mutable unsigned attempted{};mutable std::vector<std::pair<std::uint32_t,unsigned>>reads;
 explicit Storage(NativeTu15ContactRecordMemory&m):memory(m){}
 template<class F>auto input(std::uint32_t p,unsigned width,F&&f)const{if(phase=="before_read"&&attempted++==ordinal)throw PrefixStop{};auto value=f();reads.emplace_back(p,width);return value;}
 Vector vector(Record r,unsigned n)const override{return input(r.identity+n,16,[&]{return memory.vector(r,n);});}
 std::uint32_t word(Record r,unsigned n)const override{return input(r.identity+n,4,[&]{return memory.word(r,n);});}
 std::uint8_t byte(Record r,unsigned n)const override{return input(r.identity+n,1,[&]{return memory.byte(r,n);});}
 std::uint32_t setting(NativeTu15ContactRecordSettings s,unsigned n)const override{return input(s.identity+n,4,[&]{return memory.setting(s,n);});}
 std::uint32_t comparison_limit()const override{return input(0x41d018,4,[&]{return memory.comparison_limit();});}
 Record data(NativeTu15ContactRecordArray a)const override{return input(a.identity,4,[&]{return memory.data(a);});}
 void vector(Record r,unsigned n,const Vector&v,Phase p)override{memory.vector(r,n,v,p);}
 void word(Record r,unsigned n,std::uint32_t v,Phase p,const std::function<void()>&c={})override{memory.word(r,n,v,p,c);}
 void byte(Record r,unsigned n,std::uint8_t v,Phase p)override{memory.byte(r,n,v,p);}
};
}
int main(int argc,char**argv){try{
 check(argc==2,"Supply physical original contact record fixture");std::ifstream stream(argv[1]);const auto f=J::parse(stream);check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-contact-records-physical-operands-fp-v1","Contact record physical authority missing");std::size_t count{},reads{},writes{},prefixes{},alignments{},swaps{};
 for(const auto&c:f.at("cases")){
  const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>()&&c.at("caller_live_x87_preserved").get<bool>(),name+" missing physical/live x87 authority");Memory memory;for(const auto&b:c.at("banks"))memory.banks.push_back({b.at("address"),bytes(b.at("before"))});const auto fault=c.value("fault",J::object());memory.phase=fault.value("phase","");memory.ordinal=fault.value("ordinal",0u);std::vector<SourcePhysicsContactRecordStore>stores;unsigned observed{};
  NativeTu15ContactRecordMemory adapter(memory,[&](const auto&s){stores.push_back(s);if(memory.phase=="after_write"&&observed++==memory.ordinal)throw PrefixStop{};});Storage storage(adapter);storage.phase=memory.phase;storage.ordinal=memory.ordinal;
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();const unsigned entry=c.at("entry");const auto depth=unsigned(c.at("live_x87").size());std::uint32_t returned{};bool stopped{},alignment{};
  const auto host_mxcsr=_mm_getcsr();alignas(16)std::array<std::byte,108>host{},expected{},actual{};
  asm volatile("fnsave %0":"=m"(host)::"memory");asm volatile("fld1\n\tfldz\n\tfldpi\n\tfnsave %0":"=m"(expected)::"memory","st","st(1)","st(2)");std::fill(expected.begin()+12,expected.begin()+24,std::byte{});asm volatile("frstor %0"::"m"(expected):"memory");
  try{if(entry==0x303ba0)returned=native_tu15_copy_contact_record(storage,{c.at("destination")},{args[0]}).identity;
   else if(entry==0x303800){const auto value=native_tu15_test_contact_record(storage,{args[1]},{args[2]},{args[3]},fp,depth);adapter.filter_result(args[0],value);returned=args[0];}
   else if(entry==0x304130)returned=native_tu15_filter_contact_records(storage,{args[0]},{args[1]},args[2],args[3],{args[4]},fp,depth);
   else throw std::runtime_error("Unknown native contact record owner");
  }catch(const PrefixStop&){stopped=true;}catch(const std::runtime_error&e){alignment=std::string_view(e.what())=="Native TU1.5 contact records require aligned vector storage";if(!alignment){asm volatile("frstor %0"::"m"(host):"memory");throw;}}
  asm volatile("fnsave %0\n\tfrstor %1":"=m"(actual):"m"(host):"memory");check(expected==actual,name+" changed unrelated host x87 state");check(_mm_getcsr()==host_mxcsr,name+" leaked host SIMD controls/status");
  const bool completed=c.at("completed");check(completed==(!stopped&&!alignment),name+" completion differs");check(alignment==c.value("alignment_fault",false),name+" alignment boundary differs");if(completed)check(returned==c.at("boundary_gp")[0],name+" semantic result differs");
  for(std::size_t n=0;n<memory.banks.size();++n)check(memory.banks[n].data==bytes(c.at("banks")[n].at("after")),name+" retained contact bytes differ");
  const auto&r=c.at("reads");if(storage.reads.size()!=r.size())std::cerr<<name<<" native reads="<<J(storage.reads).dump()<<" original reads="<<r.dump()<<'\n';check(storage.reads.size()==r.size(),name+" ordered contact read count differs native "+std::to_string(storage.reads.size())+" original "+std::to_string(r.size()));for(std::size_t n=0;n<r.size();++n)check(storage.reads[n].first==r[n].at("address")&&storage.reads[n].second==r[n].at("bytes"),name+" ordered contact read differs at "+std::to_string(n));
  const auto&w=c.at("writes");check(stores.size()==w.size(),name+" contact store count differs");for(std::size_t n=0;n<w.size();++n)check(stores[n].instruction==w[n].at("instruction")&&stores[n].address==w[n].at("address")&&hex(stores[n].before)==w[n].at("before").get<std::string>()&&hex(stores[n].written)==w[n].at("written").get<std::string>(),name+" ordered contact store differs at "+std::to_string(n));
  const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete contact FP differs native "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original "+end.dump());
  ++count;reads+=r.size();writes+=w.size();prefixes+=stopped;alignments+=alignment;for(const auto&s:stores)swaps+=s.instruction==0x3042b4;
 }
 check(count&&prefixes&&alignments&&swaps,"Contact record acceptance matrix incomplete");std::cout<<count<<" physical original contact record cases, "<<reads<<" ordered reads, "<<writes<<" stores, "<<prefixes<<" rejected/committed prefixes, "<<alignments<<" actual alignment faults, "<<swaps<<" scalar swap publications; complete banks/FP and host state passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

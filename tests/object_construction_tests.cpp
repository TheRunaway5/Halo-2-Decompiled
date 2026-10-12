#include "halo2/object_construction.hpp"
#include "halo2/object_type_construction.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
using Bytes=std::vector<std::byte>;
std::uint32_t hex(const std::string&s){return std::stoul(s,nullptr,16);}
Bytes bytes(const std::string&s){if(s.size()%2)throw std::runtime_error("odd hex");Bytes b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(hex(s.substr(i,2))));return b;}
void require(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
struct Region{std::uint32_t address;Bytes bytes;};
using Snapshot=std::map<std::string,Region>;
struct Call{ObjectConstructionCall inputs{};std::uint32_t result,entry_mxcsr,return_mxcsr;std::string xmm;Snapshot before,after;};
struct Trial{std::uint32_t id,request,frame,result,entry_mxcsr,return_mxcsr;Snapshot before,after;std::vector<Call>calls;std::vector<ObjectConstructionStore>stores;};
struct Memory {
 std::map<std::uint32_t,Bytes> literal;Snapshot current;
 std::span<std::byte> memory(std::uint32_t a,std::size_t n) {
  for(auto &[name,r]:current)if(a>=r.address&&std::uint64_t(a)+n<=std::uint64_t(r.address)+r.bytes.size())return std::span(r.bytes).subspan(a-r.address,n);
  throw std::out_of_range("missing mutable backing at "+std::to_string(a)+" size "+std::to_string(n));
 }
 std::span<const std::byte> read(std::uint32_t a,std::size_t n) {
  for(auto &[name,r]:current)if(a>=r.address&&std::uint64_t(a)+n<=std::uint64_t(r.address)+r.bytes.size())return std::span<const std::byte>(r.bytes).subspan(a-r.address,n);
  for(auto &[p,b]:literal)if(a>=p&&std::uint64_t(a)+n<=std::uint64_t(p)+b.size())return std::span<const std::byte>(b).subspan(a-p,n);
  throw std::out_of_range("missing literal backing at "+std::to_string(a)+" size "+std::to_string(n));
 }
 std::uint32_t u(std::uint32_t a,unsigned n=4){auto b=read(a,n);std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
 void compare(const Snapshot&s,const std::string&where) {
  require(s.size()==current.size(),where+" buffer count");
  for(auto &[name,r]:s){auto i=current.find(name);require(i!=current.end(),where+" missing "+name);require(i->second.address==r.address,where+" address "+name);auto &b=i->second.bytes;require(b.size()==r.bytes.size(),where+" size "+name);auto start=name=="S"?16u:0u;
   for(std::size_t j=start;j<b.size();++j)require(b[j]==r.bytes[j],where+" "+name+" offset "+std::to_string(j));
  }
 }
 void apply(const Snapshot&after){current=after;}
};
}
int main(int argc,char**argv){try {
 require(argc==2,"usage: object_construction_tests ORIGINAL_FIXTURE");std::ifstream file(argv[1]);require(bool(file),"fixture open");Memory initial;ObjectConstructionOperands operands{};std::vector<Trial>trials;Trial*t=nullptr;Call*c=nullptr;std::string line;bool comiss_source=false;
 while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;std::istringstream s(line);std::string op,a,b,d;s>>op;
  if(op=="MEM"){s>>a>>b;initial.literal.emplace(hex(a),bytes(b));}
  else if(op=="COMISS"){s>>a>>b;require(a=="000b90f9"&&b=="0f2fc8","source COMISS opcode");comiss_source=true;}
  else if(op=="OPERANDS"){s>>a>>b;operands={hex(a),std::uint16_t(hex(b))};}
  else if(op=="CASE"){trials.emplace_back();t=&trials.back();s>>a>>b>>d;t->id=hex(a);t->request=hex(b);t->frame=hex(d);s>>a;t->result=hex(a);s>>a;t->entry_mxcsr=hex(a);s>>a;t->return_mxcsr=hex(a);}
  else if(op=="CALL"){require(t,"call outside case");t->calls.emplace_back();c=&t->calls.back();s>>a>>b>>d;c->inputs.instruction=hex(a);c->inputs.entry=hex(b);c->result=hex(d);for(auto&v:c->inputs.registers){s>>a;v=hex(a);}for(auto&v:c->inputs.arguments){s>>a;v=hex(a);}s>>c->xmm>>a;c->entry_mxcsr=hex(a);s>>a;c->return_mxcsr=hex(a);}
  else if(op=="BEFORE"||op=="AFTER"||op=="CALLBEFORE"||op=="CALLAFTER"){s>>a>>b>>d;auto*target=op=="BEFORE"?&t->before:op=="AFTER"?&t->after:op=="CALLBEFORE"?&c->before:&c->after;target->emplace(a,Region{hex(b),bytes(d)});}
  else if(op=="STORE"){ObjectConstructionStore v{};s>>a;v.instruction=hex(a);s>>a;v.address=hex(a);s>>a;v.bytes=hex(a);s>>a;v.before=hex(a);s>>a;v.after=hex(a);t->stores.push_back(v);}
  else if(op=="ENDCALL")c=nullptr;else if(op=="ENDCASE")t=nullptr;else throw std::runtime_error("unknown fixture token "+op);
 }
 std::size_t calls=0,stores=0,whole=0,scalar_controls=0;
 require(comiss_source,"missing source instruction authority");
#if defined(__x86_64__) || defined(__i386__)
 for(auto bits:{0u,0x80000000u,1u,0x80000001u,0x7fffffu,0x800000u,0x3f800000u,0xbf800000u,0x7f800000u,0xff800000u,0x7fc00000u,0x7f800001u})
  for(auto initial_mxcsr:{0x1f80u,0x1fc0u,0x9f80u,0x9fc0u,0x1fa2u}) {
   auto saved=_mm_getcsr();_mm_setcsr(initial_mxcsr);unsigned char positive;
   // These three opcode bytes are checked against the complete original
   // listing by the exporter and above. Inputs are declared ISA controls,
   // not new whole-constructor world/resource outcomes.
   asm volatile("movd %1, %%xmm1\n\txorps %%xmm0, %%xmm0\n\t.byte 0x0f,0x2f,0xc8\n\tseta %0"
       :"=qm"(positive):"r"(bits):"xmm0","xmm1","cc");
   auto actual_mxcsr=_mm_getcsr();_mm_setcsr(saved);
   ObjectConstructionFloatingPointContext fp{initial_mxcsr};auto actual=source_object_construction_positive_scalar(bits,fp);
   require(actual==bool(positive)&&fp.mxcsr==actual_mxcsr,"original COMISS value/status control");++scalar_controls;
  }
#endif
 for(const auto&trial:trials) {
  Memory m=initial;m.current=trial.before;std::size_t cursor=0;std::vector<ObjectConstructionStore>observed;
  SourceGenericObjectConstructionServices v;
  v.read=[&](auto a,auto n){return m.read(a,n);};v.memory=[&](auto a,auto n){return m.memory(a,n);};
  v.base_extent=[&](std::int16_t type){auto p=m.u(0x46fad0+std::uint32_t(std::int32_t(type)*4));return std::uint16_t(m.u(p+8,2));};
  v.invoke=[&](const ObjectConstructionCall&call,ObjectConstructionFloatingPointContext &fp){require(cursor<trial.calls.size(),"unexpected lower call");auto &expected=trial.calls[cursor++];auto where="case "+std::to_string(trial.id)+" call "+std::to_string(cursor)+" entry "+std::to_string(call.entry);
   require(call.instruction==expected.inputs.instruction&&call.entry==expected.inputs.entry,where+" identity");
   for(unsigned i=0;i<call.registers.size();++i)if(call.register_mask&(1u<<i))require(call.registers[i]==expected.inputs.registers[i],where+" register "+std::to_string(i));
   for(unsigned i=0;i<call.argument_count;++i)require(call.arguments[i]==expected.inputs.arguments[i],where+" argument "+std::to_string(i));
   if(call.xmm0_zero)require(expected.xmm==std::string(32,'0'),where+" xmm0");
   require(fp.mxcsr==expected.entry_mxcsr,where+" entry mxcsr");
   m.compare(expected.before,where+" before");m.apply(expected.after);fp.mxcsr=expected.return_mxcsr;return expected.result;
  };
  ObjectConstructionFloatingPointContext fp{trial.entry_mxcsr};
  auto result=construct_source_object({0x4e78d0,0x4eb114,0x4e58c4,0x518444,trial.frame},trial.request,operands,fp,v,[&](const auto&store){if(store.address<trial.frame||std::uint64_t(store.address)+store.bytes>std::uint64_t(trial.frame)+0xc8)observed.push_back(store);});
  require(fp.mxcsr==trial.return_mxcsr,"returned mxcsr");
  require(result==trial.result,"return datum");require(cursor==trial.calls.size(),"missing lower calls");m.compare(trial.after,"case "+std::to_string(trial.id)+" final");
  require(observed==trial.stores,"retained store sequence case "+std::to_string(trial.id));calls+=cursor;stores+=observed.size();for(auto &[name,r]:trial.after)whole+=r.bytes.size();
  // Required missing lower owner stops before it can manufacture allocation.
  m.current=trial.before;v.invoke=[](const auto&,auto&)->std::uint32_t{throw std::runtime_error("unavailable owner");};bool stopped=false;try{construct_source_object({0x4e78d0,0x4eb114,0x4e58c4,0x518444,trial.frame},trial.request,operands,fp,v);}catch(const std::runtime_error&){stopped=true;}require(stopped,"missing owner must stop");
  stopped=false;try{construct_source_object({0x4e78d0,0x4eb114,0x4e58c4,0x518444,0xfffffff8},trial.request,operands,fp,v);}catch(const std::out_of_range&){stopped=true;}require(stopped,"wrapped frame must stop before memory access");
  auto real_memory=v.memory;v.memory=[&](auto a,auto n){return real_memory(a,n).first(n-1);};stopped=false;
  try{construct_source_object({0x4e78d0,0x4eb114,0x4e58c4,0x518444,trial.frame},trial.request,operands,fp,v);}catch(const std::out_of_range&){stopped=true;}require(stopped,"truncated write backing must stop");
 }
 std::cout<<"PASS "<<trials.size()<<" original generic constructors, "<<calls<<" required lower calls, "<<stores<<" retained stores, "<<whole<<" complete output bytes, "<<scalar_controls<<" original COMISS controls; private saved-register slots excluded\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

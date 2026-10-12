#include "halo2/unit_weapon_routing.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool b,const std::string &s){if(!b)throw std::runtime_error(s);}
std::uint32_t number(const std::string &s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string &s){check(s.size()%2==0,"byte extent");std::vector<std::byte>b;for(unsigned i=0;i<s.size();i+=2)b.push_back(std::byte(number(s.substr(i,2))));return b;}
using Store=SourcePhysicsContactRecordStore;
struct Leaf {std::uint32_t site,target,flags;std::array<std::uint32_t,8> before,after;std::array<std::array<std::uint32_t,4>,8>xafter;std::vector<Store> stores;};
struct Case {std::string name;std::uint32_t entry,result,return_mask{};std::int32_t esp_delta;SourcePhysicsContactRuntimeAbi abi{};SourceRuntimeMemory memory;std::vector<Store>stores;std::vector<Leaf>leaves;std::map<std::uint32_t,std::vector<std::byte>>before,after;std::optional<Leaf>stop;};
std::uint32_t read(const SourceRuntimeMemory&m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(b[i])<<(8*i);return v;}

}
int main(int argc,char **argv) {
  std::string running;
  try {
    check(argc==2,"Supply complete original weapon routing fixture");if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream input(argv[1]);std::vector<Case>cases;
    for(std::string line;std::getline(input,line);) {
      if(line.empty()||line[0]=='#')continue;std::istringstream in(line);std::string kind,t;in>>kind;
      auto n=[&]{check(bool(in>>t),"missing numeric field");return number(t);};
      if(kind=="C") {Case c;in>>c.name;c.entry=n();c.result=n();in>>c.esp_delta;for(auto&x:c.abi.gp)x=n();c.abi.eflags=2;cases.push_back(std::move(c));continue;}
      check(!cases.empty(),"missing fixture case");auto &c=cases.back();
      if(kind=="M"||kind=="A") {const auto p=n();in>>t;auto b=bytes(t);if(kind=="M"){c.memory.mount(p,b,SourceRuntimeMemory::Access::ReadWrite,"literal original weapon routing input");c.before.emplace(p,b);}else c.after.emplace(p,std::move(b));}
      else if(kind=="W"||kind=="S") {Store s;s.instruction=n();s.address=n();in>>t;s.before=bytes(t);in>>t;s.written=bytes(t);if(kind=="W")c.stores.push_back(std::move(s));else {check(!c.leaves.empty(),"missing lower owner");c.leaves.back().stores.push_back(std::move(s));}}
      else if(kind=="B") {Leaf l{};l.site=n();l.target=n();for(auto&x:l.before)x=n();c.stop=l;}
      else if(kind=="L") {Leaf l;l.site=n();l.target=n();l.flags=n();for(auto&x:l.before)x=n();for(auto&x:l.after)x=n();for(auto&lane:l.xafter)for(auto&x:lane)x=n();c.leaves.push_back(std::move(l));}
      else if(kind=="R")c.return_mask=n();
      else check(false,"unknown original fixture record");
    }
    check(cases.size()==165,"complete routing cases");std::size_t total=0,lower_count=0,typed_count=0,missing_count=0,stopped_count=0,partial_count=0;
    for(auto &c:cases) {
      running=c.name;std::size_t wi=0,li=0;const auto esp=c.abi.gp[4];
      auto observe=[&](const Store &s){check(wi<c.stores.size(),running+" extra store");const auto &expected=c.stores[wi++];if(s!=expected){std::ostringstream msg;msg<<running<<" store "<<wi<<" actual "<<std::hex<<s.instruction<<':'<<s.address<<" expected "<<expected.instruction<<':'<<expected.address;throw std::runtime_error(msg.str());}};
      // Literal lower execution belongs only to this differential harness.
      // Native production requires actual owners and never imports this file.
      SourcePhysicsContactRuntimeServices services;
      services.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &) {
        if(li==c.leaves.size()&&c.stop){check(call.instruction==c.stop->site&&call.target==c.stop->target&&abi.gp==c.stop->before,running+" required live owner boundary ABI");throw std::runtime_error("expected real owner boundary");}
        check(li<c.leaves.size(),running+" unexpected lower owner");const auto &l=c.leaves[li++];
        check(call.instruction==l.site&&call.target==l.target&&!call.tail,running+" original lower target");
        check(abi.gp==l.before,running+" original lower register/stack ABI");
        for(const auto &s:l.stores){std::vector<std::byte>b(s.before.size());memory.read(s.address,b);check(b==s.before,running+" original lower store input");memory.write(s.address,s.written);}
        abi.gp=l.after;abi.xmm=l.xafter;abi.eflags=l.flags;
      };
      if(!c.leaves.empty()||c.stop) {
        SourceRuntimeMemory probe;for(const auto &[p,b]:c.before)probe.mount(p,b,SourceRuntimeMemory::Access::ReadWrite,"original missing-owner probe");auto abi=c.abi;bool rejected=false;
        try{source_run_unit_weapon_routing_1_1(probe,c.entry,abi,{0x27f,0x1f80},{});}catch(const std::runtime_error&e){rejected=std::string(e.what()).find("callee unavailable")!=std::string::npos;}
        check(rejected,running+" absent lower owner must fail");++missing_count;
        // This is an API error-path control, separate from the original-code
        // differential. A real lower owner may mutate its ABI before failing.
        // Preserve those mutations along with its already committed stores.
        SourcePhysicsContactRuntimeAbi interrupted{};bool reached=false;
        SourcePhysicsContactRuntimeServices partial;
        partial.required_owner=[&](auto &,const auto &,auto &state,const auto &) {
          for(unsigned i=0;i<8;++i){state.gp[i]^=0x13579bdfu+i;for(unsigned lane=0;lane<4;++lane)state.xmm[i][lane]=0x3f000000u+i*4+lane;state.x87[i]=static_cast<long double>(i)+0.25L;}
          state.eflags=0x8d7u;state.fs_base=0x12340000u;state.x87_depth=3;state.x87_status=0x2100u;
          interrupted=state;reached=true;throw std::runtime_error("partial owner failure");
        };
        SourceRuntimeMemory partial_memory;for(const auto &[p,b]:c.before)partial_memory.mount(p,b,SourceRuntimeMemory::Access::ReadWrite,"partial-owner ABI control");auto partial_abi=c.abi;bool partial_stopped=false;
        try{source_run_unit_weapon_routing_1_1(partial_memory,c.entry,partial_abi,{0x27f,0x1f80},partial);}catch(const std::runtime_error&e){partial_stopped=std::string(e.what())=="partial owner failure";}
        check(reached&&partial_stopped&&partial_abi.gp==interrupted.gp&&partial_abi.xmm==interrupted.xmm&&partial_abi.eflags==interrupted.eflags&&partial_abi.fs_base==interrupted.fs_base&&partial_abi.x87==interrupted.x87&&partial_abi.x87_depth==interrupted.x87_depth&&partial_abi.x87_status==interrupted.x87_status,running+" partial lower-owner ABI must survive failure");++partial_count;
      }
      bool stopped=false;try{source_run_unit_weapon_routing_1_1(c.memory,c.entry,c.abi,{0x27f,0x1f80},services,observe);}catch(const std::runtime_error&e){if(!c.stop||std::string(e.what())!="expected real owner boundary")throw;stopped=true;}check(stopped==bool(c.stop),running+" original reached boundary");
      // Only AL is defined by request/admission; ammo owner returns void.
      if(!stopped)check((c.abi.gp[0]&c.return_mask)==(c.result&c.return_mask),running+" defined return result");
      check(c.abi.gp[4]==esp+c.esp_delta,running+" original stack return");check(wi==c.stores.size()&&li==c.leaves.size(),running+" complete stores/lower owners");
      for(const auto &[p,b]:c.after){std::vector<std::byte>a(b.size());c.memory.read(p,a);check(a==b,running+" complete original final backing");}
      total+=wi;lower_count+=li;stopped_count+=stopped;
    }
    check(missing_count>0&&partial_count==missing_count&&lower_count==0,"fail-closed routing without lower replay");
    std::cout<<cases.size()-stopped_count<<" returned original weapon routing calls and "<<stopped_count<<" exact required-owner stops, "<<total<<" ordered stores, "<<typed_count<<" supplied results, "<<lower_count<<" explicit differential lower calls; "<<missing_count<<" absent-owner rejections, "<<partial_count<<" partial-owner ABI failure controls\n";
  }catch(const std::exception&e){std::cerr<<running<<' '<<e.what()<<'\n';return 1;}
}

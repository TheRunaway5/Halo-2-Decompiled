#include "halo2/sound_driver_update.hpp"
#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
struct Stop{};
void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::uint32_t num(const std::string&s){return std::stoul(s,nullptr,16);}
std::string hex(std::uint32_t v,unsigned n=4){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(n*2)<<(v&(n==4?0xffffffffu:((1u<<(8*n))-1)));return s.str();}
std::vector<std::string> fields(const std::string&s){std::istringstream in(s);std::string v;std::vector<std::string>r;while(std::getline(in,v,'\t'))r.push_back(v);return r;}
std::vector<std::byte> decode(const std::string&s){require(s.size()%2==0,"Odd original bytes");std::vector<std::byte>b(s.size()/2);for(std::size_t k=0;k<b.size();++k)b[k]=std::byte(num(s.substr(k*2,2)));return b;}
struct Case{std::string name,operation;std::array<std::byte,4>status;SoundDriverUpdateBindings bindings;std::map<std::uint32_t,std::vector<std::byte>>before,after;std::vector<std::string>events;};
struct Count{std::size_t bytes=0,stores=0,work=0,sections=0;};
Count run(const Case&c){
 NativeSoundStorage storage({0x480d68,0xb000000,0xc000000});
 std::vector<std::pair<std::uint64_t,std::uint64_t>>ranges;for(const auto&[a,v]:c.before)ranges.emplace_back(a&~4095u,(std::uint64_t(a)+v.size()+4095)&~std::uint64_t(4095));std::ranges::sort(ranges);
 std::uint64_t lo=0,hi=0;for(auto[a,z]:ranges){if(a<=hi){hi=std::max(hi,z);continue;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);lo=a;hi=z;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);
 auto memory=[&](std::uint32_t a,std::size_t n){auto it=c.before.upper_bound(a);require(it!=c.before.begin(),"Unowned driver update address");--it;require(std::uint64_t(a)+n<=std::uint64_t(it->first)+it->second.size(),"Unowned driver update extent");return storage.memory(a,n);};
 for(const auto&[a,v]:c.before)std::ranges::copy(v,memory(a,v.size()).begin());
 Count count;std::vector<std::string>events;SoundDriverUpdateServices services;services.memory=memory;
 services.do_work=[&]{++count.work;events.push_back("CALL\trequired_DirectSoundDoWork");throw Stop{};};
 services.load_section=[&](auto address)->std::uint32_t{++count.sections;events.push_back("CALL\trequired_XLoadSection\t"+hex(address));throw Stop{};};
 auto observe=[&](const SoundDriverStartupStore&q){++count.stores;events.push_back("STORE\t"+hex(q.instruction)+'\t'+hex(q.address)+'\t'+std::to_string(q.bytes)+'\t'+hex(q.before,q.bytes)+'\t'+hex(q.after,q.bytes));};
 auto status=c.status;bool stopped=false;
 try{if(c.operation=="update")source_sound_driver_update(c.bindings,status,services,observe);else if(c.operation=="reload_effects")source_sound_driver_reload_effects(c.bindings,services,observe);else throw std::runtime_error("Unknown original owner");}catch(const Stop&){stopped=true;}
 require(stopped,"Unavailable original platform owner admitted");require(status==c.status,"Unreached status output changed");
 if(events!=c.events){std::cerr<<c.name<<" native/original boundary differs\n";throw std::runtime_error("Original ordered update events differ");}
 for(const auto&[a,v]:c.after){require(std::ranges::equal(v,memory(a,v.size())),"Original complete update output differs");count.bytes+=v.size();}
 return count;
}
}
int main(int argc,char**argv)try{
 require(argc==2,"Usage: sound_driver_update_tests <fixtures.tsv>");std::ifstream in(argv[1]);require(bool(in),"Original fixtures unavailable");std::string line;std::getline(in,line);require(line=="SOUND_DRIVER_UPDATE_STOPS_V1","Fixture schema");std::vector<Case>cases;Case c;
 while(std::getline(in,line)){auto q=fields(line);if(q.empty())continue;
  if(q[0]=="CASE"){auto status=decode(q[3]);require(status.size()==4,"Status backing size");c={q[1],q[2],{status[0],status[1],status[2],status[3]},{num(q[4]),num(q[5]),num(q[6]),num(q[7]),num(q[8]),num(q[9]),num(q[10]),num(q[11])},{},{},{}};}
  else if(q[0]=="BEFORE"||q[0]=="AFTER")(q[0]=="BEFORE"?c.before:c.after).emplace(num(q[1]),decode(q[2]));
  else if(q[0]=="CALL"||q[0]=="STORE")c.events.push_back(line);
  else if(q[0]=="ENDCASE"){cases.push_back(std::move(c));c={};}
  else throw std::runtime_error("Unknown original row");
 }
 Count total;for(const auto&c:cases){auto n=run(c);total.bytes+=n.bytes;total.stores+=n.stores;total.work+=n.work;total.sections+=n.sections;}
 require(cases.size()==16&&total.work==8&&total.sections==8&&total.stores==0,"Incomplete original stop suite");
 std::cout<<"PASS "<<cases.size()<<" original1.1/u5 driver-prefix controls; "<<total.bytes<<" whole bytes; "<<total.work<<" genuine DoWork stops; "<<total.sections<<" genuine section stops; no platform/GetStatus/voice result supplied.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

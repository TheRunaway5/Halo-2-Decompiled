#include "halo2/native_tu15_motion_forces.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;
using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Invalid fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto b:b){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
struct Memory:SourcePhysicsContactMemory{
  std::map<std::uint32_t,std::vector<std::byte>> banks;
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing motion bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing motion extent");return i;}
  void read(std::uint32_t p,std::span<std::byte>b)const override{auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
  void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t host_status(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
std::uint16_t host_control(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
void oracle(const std::filesystem::path&path){
  check(std::filesystem::exists(path),"Required TU1.5 motion original fixture absent");std::ifstream in(path);const auto f=J::parse(in);
  check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","TU1.5 motion authority changed");
  check(f.at("required_backend")=="unchanged-tu15-physical-i386-vector-stores-banks-fp-v2","Required original motion backend missing");
  check(f.at("force_sites")==85&&f.at("angular_sites")==53,"Original motion bodies changed");
  std::size_t count=0,store_count=0;
  for(const auto&c:f.at("cases")){
    const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" missing physical original verification");
    Memory m;for(const auto&b:c.at("banks"))check(m.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate motion bank");
    std::vector<SourcePhysicsContactRecordStore> stores;NativeTu15MotionForceMemory storage(m,c.at("motion"),c.at("step"),c.at("gravity"),[&](const auto&s){stores.push_back(s);});
    NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
    const auto cw=host_control(),sw=host_status();const auto mxcsr=_mm_getcsr();
    if(c.at("entry")==0x3252d0)native_tu15_apply_motion_forces(storage,fp,c.at("preserved_x87_depth"));
    else if(c.at("entry")==0x3250a0){const auto impulse=c.at("impulse").get<NativeTu15PhysicsVector>();native_tu15_apply_motion_angular_impulse(storage,impulse,fp,c.at("preserved_x87_depth"));}
    else throw std::runtime_error("Unsupported original motion entry");
    check(host_control()==cw&&host_status()==sw&&_mm_getcsr()==mxcsr,name+" leaked native platform FP state");
    const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered vector store count differs");
    for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered vector store differs at "+std::to_string(n));}
    for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained motion bytes differ");
    const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete native FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
    ++count;store_count+=stores.size();
  }
  check(count>0,"Empty original motion fixture");std::cout<<count<<" original TU1.5 motion cases, "<<store_count<<" ordered vector stores and complete physical FP passed; full solver/live closure remains open\n";
}
}
int main(int argc,char**argv){try{check(argc==2,"Supply required original TU1.5 motion fixture");oracle(argv[1]);return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

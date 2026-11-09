#include "halo2/native_tu15_object_detachment.hpp"
#include "halo2/native_tu15_object_deletion_spatial_binding.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string&s){std::vector<std::byte>b;for(unsigned i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte> file(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned original bank");std::vector<char>b((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>out(b.size());std::transform(b.begin(),b.end(),out.begin(),[](char c){return std::byte(std::uint8_t(c));});return out;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J fault;mutable unsigned fault_count{};bool log_reads{};
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Required attachment DATA address differs: unbacked native operand");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Required attachment DATA extent differs: unbacked native operand");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        if(!fault.is_null()&&!fault.value("write",false)&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};
        if(log_reads)reads.push_back(J::array({p,b.size()}));const auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};
        const auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);
    }
};
std::uint16_t status(){std::uint16_t v;asm volatile("fnstsw %0":"=a"(v));return v;}
std::uint16_t control(){std::uint16_t v;asm volatile("fnstcw %0":"=m"(v));return v;}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply owned unchanged physical attachment fixture");const auto path=std::filesystem::path(argv[1]);std::ifstream f(path);const auto fixture=J::parse(f);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("oracle")=="unchanged-tu15-physical-array-and-real-pool-v1","Pinned original TU1.5 required");
    std::size_t cases{},reads{},stores{},faults{};
    for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
        if(read_pass&&!c.value("mutations",J::array()).empty())continue;
        const auto name=c.at("name").get<std::string>();Memory m;
        for(const auto&bank:c.at("banks"))m.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));
        m.fault=c.value("fault",J{});m.log_reads=read_pass;J events=J::array();std::set<unsigned>applied;
        SourcePhysicsContactRecordObserver observe;
        if(!read_pass)observe=[&](const auto&e){events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned n{};for(const auto&change:c.value("mutations",J::array())){if(change.at("instruction")==e.instruction&&applied.insert(n).second){const auto bytes=unhex(change.at("written"));m.write(change.at("address"),bytes);}++n;}};
        const auto word=[&](std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8*n);return v;};
        NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15SpatialRemovalStats activity;
        const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{};
        try{
            const auto entry=c.at("entry").get<std::uint32_t>(),datum=c.value("incoming_eax",0u);const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
            if(entry==0xc5250u)native_tu15_detach_light_attachment(m,datum,observe,&activity);
            else if(entry==0x1d30f0u){
                if(c.value("retained_arguments",false))native_tu15_remove_spatial_chain(m,c.at("incoming_edi"),args[0],[&]{return word(0x200f008u);},observe,&activity);
                else native_tu15_remove_spatial_chain(m,c.at("incoming_edi"),args[0],args[1],observe,&activity);
            }else if(entry==0xc0f60u){
                NativeTu15ObjectDetachmentServices services;services.detach_attachment=[&](auto attachment,auto&){native_tu15_detach_light_attachment(m,attachment,observe,&activity);};
                native_tu15_detach_object_tree(m,{args[0],args[1],args[2],args[3],args[4]},fp,services,observe);
            }else throw std::runtime_error("Unqualified attachment fixture entry");
        }catch(const Fault&){failed=true;}
        check(failed==c.value("declared_fault",false),name+" completion differs");
        check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" host FP differs");check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),name+" source FP differs");
        if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));
            if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" read order differs at"+std::to_string(n)+" native="+(n<m.reads.size()?m.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();
        }else{if(events!=c.at("writes")){std::size_t n{};while(n<events.size()&&n<c.at("writes").size()&&events[n]==c.at("writes")[n])++n;throw std::runtime_error(name+" store differs at"+std::to_string(n)+" native="+(n<events.size()?events[n].dump():"end")+" original="+(n<c.at("writes").size()?c.at("writes")[n].dump():"end"));}stores+=events.size();check(applied.size()==c.value("mutations",J::array()).size(),name+" mutation phase differs");}
        for(const auto&bank:c.at("banks"))check(m.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),name+" complete bank differs");
        if(!read_pass){++cases;faults+=failed;}
    }
    check(cases>20,"Insufficient attachment/spatial physical cases");std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",reads},{"ordered_stores",stores},{"fault_prefixes",faults},{"supplied_child_results",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

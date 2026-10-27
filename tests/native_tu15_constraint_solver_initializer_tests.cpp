#include "halo2/native_tu15_constraint_solver_initializer.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid initializer fixture bytes");std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>bytes){const char*digits="0123456789abcdef";std::string out;for(auto b:bytes){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4u];out+=digits[n&15u];}return out;}
struct Fault{};struct Boundary{};
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J fault;mutable unsigned fault_count{};bool record_reads{},record_raw{};
    std::vector<SourcePhysicsContactRecordStore>raw_stores;
    auto locate(std::uint32_t p,std::size_t n)const{auto it=banks.upper_bound(p);check(it!=banks.begin(),"Missing initializer backing");--it;check(std::uint64_t(p-it->first)+n<=it->second.size(),"Missing initializer extent");return it;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{if(!fault.is_null()&&p==fault.at("address")&&out.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(record_reads)reads.push_back(J::array({p,out.size()}));auto i=locate(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>input)override{auto i=locate(p,input.size());if(record_raw)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+input.size()),std::vector<std::byte>(input.begin(),input.end())});std::copy(input.begin(),input.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
}
int main(int argc,char**argv){try{
    check((argc==2||argc==3)&&std::filesystem::exists(argv[argc-1]),"Required physical original initializer fixture absent");std::ifstream input(argv[argc-1]);const auto f=J::parse(input);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-init-clienthash-native-linux-time-import-v1","Physical initializer authority missing");
    check(f.at("body_sites")==J::array({152,125,4,22,43,1})&&f.at("clock_import")==0x80000080u,"Initializer/hash/time original closure changed");
    std::size_t count{},read_count{},write_count{},fault_count{},clock_count{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" lacks physical original authority");Memory before;
        for(const auto&b:c.at("banks"))check(before.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate initializer bank");before.fault=c.value("fault",J{});
        for(bool read_pass:{false,true}){
            auto memory=before;memory.record_reads=read_pass;const bool fault_case=!memory.fault.is_null();memory.record_raw=fault_case&&!read_pass;
            std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observe;if(!read_pass&&!fault_case)observe=[&](const auto&s){stores.push_back(s);};
            NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};bool failed{},boundary{},clock_called{};std::uint32_t result{};
            NativeTu15ConstraintSolverInitializerServices services;
            services.read_time_seconds=[&](auto&actual_fp){check(!clock_called,name+" repeated original time dependency");clock_called=true;
                if(!c.at("boundary").is_null()&&c.at("boundary").at("kind")=="time"){boundary=true;throw Boundary{};}
                const auto&t=c.at("actual_time");check(!t.is_null()&&t.at("genuine_original_return").get<bool>(),name+" lacks genuine original time return");actual_fp={t.at("fp")[0],t.at("fp")[1],t.at("fp")[2]};return t.at("seconds").template get<std::uint32_t>();};
            services.log_failure=[&](std::uint32_t message,auto&){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("kind")=="log"&&b.at("message")==message,name+" actual original logger ABI differs");boundary=true;throw Boundary{};};
            const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();
            try{if(c.at("entry")==0x326640u)result=native_tu15_constraint_solver_client_hash(memory,c.at("key_pointer"));else native_tu15_initialize_constraint_solver(memory,fp,services,observe);}catch(const Fault&){failed=true;}catch(const Boundary&){}
            check(failed==fault_case&&boundary==!c.at("boundary").is_null(),name+" initializer fault/dependency completion differs");check(clock_called==c.at("time_called").get<bool>(),name+" actual original time call differs");
            check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" initializer leaked host FP");
            if(c.at("entry")==0x326640u&&!failed)check(result==c.at("actual_hash"),name+" actual original client hash differs");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(memory.reads!=expected){std::size_t n{};while(n<memory.reads.size()&&n<expected.size()&&memory.reads[n]==expected[n])++n;throw std::runtime_error(name+" original initializer read order differs at "+std::to_string(n));}read_count+=memory.reads.size();}
            else{if(fault_case)stores=memory.raw_stores;const auto&original=c.at("writes");check(stores.size()==original.size(),name+" ordered initializer store count differs");for(std::size_t n=0;n<stores.size();++n)check((fault_case||stores[n].instruction==original[n].at("instruction"))&&stores[n].address==original[n].at("address")&&hex(stores[n].before)==original[n].at("before").get<std::string>()&&hex(stores[n].written)==original[n].at("written").get<std::string>(),name+" ordered initializer store differs");write_count+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(memory.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained initializer bytes differ");
            const auto&after=c.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" initializer FP differs");
        }
        ++count;fault_count+=!c.value("fault",J{}).is_null();clock_count+=c.at("time_called").get<bool>();
    }
    check(count>0,"Empty original initializer fixture");std::cout<<count<<" original initializer/hash cases, "<<fault_count<<" fault prefixes, "<<read_count<<" reads, "<<write_count<<" stores and "<<clock_count<<" declared genuine time calls passed; logger/time production closure remains explicit\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

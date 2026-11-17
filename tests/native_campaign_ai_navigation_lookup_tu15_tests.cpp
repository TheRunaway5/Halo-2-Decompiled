#include "halo2/native_campaign_ai_navigation_lookup_tu15.hpp"
#ifdef HALO2_NAVIGATION_PARENT_REPLAY
#include "halo2/native_campaign_ai_object_navigation_tu15.hpp"
#endif
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <xmmintrin.h>

// Exact physical-i386 navigation lookup replay, including ordered read/store
// prefixes, complete caller frames, every GP/XMM register, and FP controls.
namespace {
using namespace halo2;using Json=nlohmann::json;
constexpr std::uint32_t stack_low=0x2000000u,stack_end=0x2040000u,stop_pc=0x3000000u;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto value:data){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;}
// sparse-runs-v1: zero-filled bank of `size` with nonzero `before_runs`;
// `after_runs` are the bytes that differ after the original executed.
std::vector<std::byte> before_bytes(const Json &bank) {
    std::vector<std::byte> out(bank.at("size").get<std::size_t>());
    for(const auto &run:bank.at("before_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}
    return out;
}
std::vector<std::byte> after_bytes(const Json &bank) {
    auto out=before_bytes(bank);
    for(const auto &run:bank.at("after_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}
    return out;
}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    // Undeclared caller stack: the original's pushes and page probes land
    // here too, but the physical reference neither records nor compares it.
    mutable std::vector<std::byte> scratch=std::vector<std::byte>(stack_end-stack_low);
    mutable Json reads=Json::array();
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    bool recording{},raw_stores{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::optional<decltype(banks)::iterator> locate(std::uint32_t address,std::size_t width)const {
        auto found=banks.upper_bound(address);
        if(found!=banks.begin()){--found;if(std::uint64_t(address-found->first)+width<=found->second.size())return found;}
        if(address>=stack_low && std::uint64_t(address)+width<=stack_end)return std::nullopt;
        throw Missing{address,width};
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        const auto found=locate(address,out.size());
        if(!found){std::copy_n(scratch.begin()+(address-stack_low),out.size(),out.begin());return;}
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        std::copy_n((*found)->second.begin()+(address-(*found)->first),out.size(),out.begin());
        if(recording)reads.push_back(Json::array({address,out.size()}));
    }
    // PUSH/CALL words inside the declared frame bank.
    // The reference has no explicit store operand for them, so they neither
    // count toward a store prefix nor appear as ordered stores.
    std::vector<std::uint32_t> implicit;
    std::uint32_t sector_sp{};bool plane_published{},dominant_done{};
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        const auto found=locate(address,values.size());
        if(!found){std::copy(values.begin(),values.end(),scratch.begin()+(address-stack_low));return;}
        // 2028A0's saved words are later reused by 123210's explicit locals.
        // Identify that lifetime from the preceding plane publication, not
        // from reference values or expected results. A later projection PUSH
        // reuses those same words after the third dominant-axis store.
        const bool dominant_local=sector_sp && plane_published && !dominant_done &&
            values.size()==4 && (address==sector_sp-0x30u || address==sector_sp-0x34u || address==sector_sp-0x38u);
        const bool pushed=values.size()==4 && !dominant_local && std::ranges::find(implicit,address)!=implicit.end();
        if(!pushed && phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        auto &bank=(*found)->second;const auto at=address-(*found)->first;
        if(raw_stores && !pushed)stores.push_back({0,address,{bank.begin()+at,bank.begin()+at+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),bank.begin()+at);
        if(sector_sp && address==sector_sp-0x10u && values.size()==4)plane_published=true;
        if(dominant_local && address==sector_sp-0x38u)dominant_done=true;
    }
    // Only explicit-operand stores are ordered events in the reference;
    // PUSH/CALL words reach the banks directly. Kernels use write() for both,
    // so ordered comparison uses observer events (which carry the PC).
    explicit Memory(const Json &row){
        for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),before_bytes(bank));
        if(row.at("entry")==0x202660u)sector_sp=row.at("gp")[4];
        for(const auto &address:row.value("implicit_stack_addresses",Json::array()))implicit.push_back(address);
    }
};
long double extended(const std::string &text) {
    const auto raw=bytes(text);check(raw.size()==10,"Invalid x87 value");long double value{};std::memcpy(&value,raw.data(),10);return value;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply the independent original navigation fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Reference supplied an outcome");
    check(fixture.at("bank_encoding")=="sparse-runs-v1","Fixture bank encoding changed");
    std::size_t count{},completed{},prefixes{},reads{},stores{},register_checks{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),name+": original unqualified");
        const auto fault=row.value("fault",Json::object());
        for(const bool reading:std::array{false,true}) {
            if(reading && fault.value("phase",std::string{})=="after_write")continue;
            Memory memory(row);memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            SourcePhysicsContactRecordObserver observer;std::vector<SourcePhysicsContactRecordStore> events;unsigned publications{};
            if(!reading && !memory.raw_stores)observer=[&](const auto &event){
                if(!memory.locate(event.address,event.written.size()))return;
                events.push_back(event);if(memory.phase=="after_write" && publications++==memory.ordinal)throw Prefix{};};
            SourcePhysicsWorldRuntimeAbi abi{};
            for(unsigned n=0;n<8;++n)abi.gp[n]=row.at("gp")[n];
            for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)abi.xmm[r][k]=row.at("xmm")[r][k];
            abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
            abi.x87_status=row.at("before_fp")[1];abi.x87_depth=unsigned(row.at("live_x87").size());
            for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=extended(row.at("live_x87")[n]);
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool prefix{};
            try {
#ifdef HALO2_NAVIGATION_PARENT_REPLAY
                native_campaign_ai_object_navigation_call_tu15(memory,row.at("entry"),abi,observer);
#else
                native_campaign_ai_navigation_lookup_call_tu15(memory,row.at("entry"),abi,observer);
#endif
            }
            catch(const Prefix &){prefix=true;}
            catch(const Missing &value){std::ostringstream why;why<<name<<": missing storage "<<std::hex<<value.address<<" width "<<value.width;throw std::runtime_error(why.str());}
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
            if(!fault.empty())check(prefix,name+": did not stop at the original prefix");
            else {
                check(!prefix && row.at("completed").get<bool>() && row.at("stopped_at")==stop_pc,name+": native/original did not complete");
                for(unsigned n=0;n<8;++n)if(abi.gp[n]!=row.at("boundary_gp")[n].get<std::uint32_t>()) {
                    std::ostringstream why;why<<name<<": register "<<n<<" native="<<std::hex<<abi.gp[n]<<" original="<<row.at("boundary_gp")[n].get<std::uint32_t>();throw std::runtime_error(why.str());
                }
                for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)check(abi.xmm[r][k]==row.at("boundary_xmm")[r][k],name+": XMM differs");
                check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] && abi.floating_point->mxcsr==row.at("after_fp")[2],
                    name+": FP native="+Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr}).dump()+" original="+row.at("after_fp").dump());
                check(abi.x87_depth==row.at("live_x87").size(),name+": occupied x87 depth differs");
                for(unsigned n=0;n<abi.x87_depth;++n){const auto expected=extended(row.at("live_x87")[n]);check(std::memcmp(&abi.x87[n],&expected,10)==0,name+": occupied x87 value differs");}
                register_checks+=40;if(!reading)++completed;
            }
            for(const auto &bank:row.at("banks")) {
                const auto &actual=memory.banks.at(bank.at("address"));const auto expected=after_bytes(bank);
                if(actual!=expected) {
                    std::size_t at=0;while(at<actual.size() && actual[at]==expected[at])++at;
                    std::ostringstream why;why<<name<<": retained bytes differ at "<<std::hex<<bank.at("address").get<std::uint32_t>()+at<<" native="<<std::to_integer<unsigned>(actual[at])<<" original="<<std::to_integer<unsigned>(expected[at]);throw std::runtime_error(why.str());
                }
            }
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                if(memory.reads!=expected) {
                    std::size_t at=0;while(at<memory.reads.size() && at<expected.size() && memory.reads[at]==expected[at])++at;
                    std::cerr<<name<<": first read difference at "<<at<<" native="<<(at<memory.reads.size()?memory.reads[at].dump():"end")<<" original="<<(at<expected.size()?expected[at].dump():"end")<<'\n';
                }
                check(memory.reads==expected,name+": ordered reads differ");reads+=memory.reads.size();
            } else {
                if(memory.raw_stores)events=memory.stores;
                const auto &writes=row.at("writes");
                check(events.size()==writes.size(),name+": store count native="+std::to_string(events.size())+" original="+std::to_string(writes.size()));
                for(std::size_t n=0;n<events.size();++n) {
                    const auto &a=events[n];const auto &b=writes[n];
                    if(!((memory.raw_stores||a.instruction==b.at("instruction")) && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>())) {
                        std::ostringstream why;why<<name<<": ordered store "<<n<<" differs native pc="<<std::hex<<a.instruction<<" address="<<a.address<<" before="<<hex(a.before)<<" written="<<hex(a.written)<<" original="<<b.dump();throw std::runtime_error(why.str());
                    }
                }
                stores+=events.size();
            }
        }
        if(!fault.empty())++prefixes;++count;
    }
#ifdef HALO2_NAVIGATION_PARENT_REPLAY
    check(count && completed==count && !prefixes && fixture.at("whole_call_composition").get<bool>(),"Navigation parent composition matrix incomplete");
#else
    check(count && completed && prefixes,"Navigation lookup matrix incomplete");
#endif
    std::cout<<count<<" original navigation-lookup cases ("<<completed<<" complete, "<<prefixes<<" fault prefixes); "<<reads<<" reads; "<<stores<<" stores; "<<register_checks<<" register checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

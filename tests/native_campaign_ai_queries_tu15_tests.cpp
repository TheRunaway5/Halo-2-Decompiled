#include "halo2/native_campaign_ai_queries_tu15.hpp"
#include "halo2/native_campaign_ai_queries_tu15_binding.hpp"
#include <algorithm>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace {
using namespace halo2;
using Json=nlohmann::json;
void check(bool value,const std::string &message) { if(!value) throw std::runtime_error(message); }
std::vector<std::byte> unhex(const std::string &text) {
    std::vector<std::byte> result;
    for(std::size_t i=0;i<text.size();i+=2) result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto byte:bytes) { const auto value=std::to_integer<unsigned>(byte); result+=digits[value>>4];result+=digits[value&15]; }
    return result;
}
class Memory final : public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto locate(std::uint32_t address,std::size_t size) const {
        auto at=banks.upper_bound(address);
        if(at==banks.begin()) throw std::runtime_error("Missing native AI record backing");
        --at;
        if(std::uint64_t(address)+size>std::uint64_t(at->first)+at->second.size())
            throw std::runtime_error("Native AI record read crosses backing");
        return at;
    }
    void read(std::uint32_t address,std::span<std::byte> output) const override {
        const auto at=locate(address,output.size());
        std::copy_n(at->second.begin()+(address-at->first),output.size(),output.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> input) override {
        const auto at=locate(address,input.size());
        auto &bank=banks.at(at->first);
        std::copy(input.begin(),input.end(),bank.begin()+(address-at->first));
    }
};
void run_case(const Json &fixture,const Json &input) {
    Memory memory;
    for(const auto &bank:fixture.at("banks")) memory.banks.emplace(bank.at(0).get<unsigned>(),std::vector<std::byte>(bank.at(1).get<unsigned>()));
    for(const auto &patch:input.at("patches")) memory.write(patch.at(0),unhex(patch.at(1)));
    const auto initial_memory=memory.banks;
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=input.at("gp").get<decltype(abi.gp)>();abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();
    abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    abi.x87_depth=0;abi.x87_status=0;abi.fs_base=0x45670000;
    for(unsigned i=0;i<8;++i)abi.x87[i]=i+0.25L;
    const auto initial=abi;
    Json reads=Json::array(),stores=Json::array();bool fault=false;
    const auto fail=input.at("fail_read").is_null()?-1:input.at("fail_read").get<int>();
    const auto fail_event=fail<0?Json():input.at("reads").at(fail);
    unsigned required_calls{};
    SourcePhysicsWorldRuntimeServices services{};
    services.required_owner=[&](auto &,const auto &,auto &,const auto &) {
        ++required_calls;throw std::runtime_error("Native AI leaf delegated a missing result");
    };
    std::fenv_t environment;std::fegetenv(&environment);
    const auto exceptions=std::fetestexcept(FE_ALL_EXCEPT);
#if defined(__i386__) || defined(__x86_64__)
    const auto mxcsr=_mm_getcsr();
#endif
    try {
        native_campaign_ai_queries_tu15_run(memory,input.at("entry"),abi,services,
            [&](const auto &event) { stores.push_back(Json::array({event.instruction,event.address,hex(event.before),hex(event.written)})); },
            [&](const auto &event) {
                reads.push_back(Json::array({event.instruction,event.address,event.bytes}));
                if(!fail_event.is_null() && reads.back()==fail_event) throw std::runtime_error("Declared actual read boundary");
            });
    } catch(const std::exception &) { if(fail<0)throw;fault=true; }
    check(fault==(fail>=0),"Native AI fault prefix did not stop at its actual read");
    check(!required_calls,"Native AI leaf supplied an external outcome");
    for(const auto reg:{3u,5u,6u,7u}) check(abi.gp[reg]==initial.gp[reg],"Native AI clobbered callee-preserved state");
    if(fail<0) {
        check(abi.gp[4]==input.at("final_gp").at(4),"Native AI caller argument consumption differs");
        if(input.at("entry")==0x1be5b0u)
            check(abi.gp[0]==input.at("final_gp").at(0),"Native AI semantic behavior score differs");
    }
    check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Native AI SIMD state differs");
    check(abi.eflags==initial.eflags,"Native AI reconstructed dead CPU flags");
    Json expected_reads=Json::array(),expected_stores=Json::array();
    for(const auto &event:input.at("reads")) if(event.at(0)!=0x1be623u)expected_reads.push_back(event);
    for(const auto &event:input.at("stores")) if(event.at(0)!=0x1be5c6u)expected_stores.push_back(event);
    check(reads==expected_reads,"Native AI lazy ordered semantic reads differ");
    check(stores==expected_stores,"Native AI ordered persistent stores differ");
    auto expected=initial_memory;
    for(const auto &event:expected_stores) {
        const auto address=event.at(1).get<unsigned>();const auto data=unhex(event.at(3));
        auto at=expected.upper_bound(address);--at;
        std::copy(data.begin(),data.end(),at->second.begin()+(address-at->first));
    }
    check(memory.banks==expected,"Native AI changed untouched persistent/caller bytes");
    check(abi.x87==initial.x87 && abi.x87_depth==initial.x87_depth && abi.x87_status==initial.x87_status &&
        abi.fs_base==initial.fs_base && abi.floating_point.has_value() &&
        abi.floating_point->x87_control_word==initial.floating_point->x87_control_word &&
        abi.floating_point->mxcsr==initial.floating_point->mxcsr && !abi.active_mmx,
        "Native AI touched retained FP/FS/MMX state");
    check(std::fetestexcept(FE_ALL_EXCEPT)==exceptions,"Native integer AI changed host FP exception state");
#if defined(__i386__) || defined(__x86_64__)
    check(_mm_getcsr()==mxcsr,"Native integer AI changed host MXCSR");
#endif
    std::fesetenv(&environment);
}
}
int main(int argc,char **argv) try {
    check(argc==2,"Expected unchanged-original native AI query fixture");
    Json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong native AI authority");
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_dependency_returns").empty(),"AI original evidence supplies an outcome");
    for(const auto &input:fixture.at("cases")) {
        try {run_case(fixture,input);}
        catch(const std::exception &e) {throw std::runtime_error(input.at("name").get<std::string>()+": "+e.what());}
    }
    Memory missing;SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x201f000;
    for(const auto entry:{0x1be5b0u,0x1bae40u}) {
        bool rejected=false;try {native_campaign_ai_queries_tu15_run(missing,entry,abi);} catch(const std::exception &) {rejected=true;}
        check(rejected,"Native AI supplied missing backing");
    }
    check(!native_campaign_ai_queries_tu15_owns_entry(0x1bd1e0),"Native AI leaf claims unconverted behavior");
    const auto activity=native_campaign_ai_query_activity_tu15();
    check(activity.threat && activity.reset,"Native AI activity counters did not observe both semantic owners");
    std::cout<<"PASS "<<fixture.at("cases").size()<<" unchanged-original AI query/reset cases, public results, ordered semantic accesses/stores, preserved state, fault prefixes and untouched FP\n";
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}

#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_campaign_ai_actor_path_query_tu15.hpp"
#include "halo2/native_campaign_ai_goal_evaluation_tu15.hpp"
#include "halo2/native_campaign_ai_goal_scoring_tu15.hpp"
#include "halo2/native_campaign_ai_jump_arc_tu15.hpp"
#include "halo2/native_campaign_ai_jump_fallback_tu15.hpp"
#include "halo2/native_campaign_ai_path_influence_tu15.hpp"
#include "halo2/native_campaign_ai_path_result_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <xmmintrin.h>

// Replays the physical unchanged-i386 26A380 remainder fixture (26A712
// continuations and the scoring callees at their own entries) through the
// native owners. Every declared bank, ordered read, ordered store with its
// PC, the result register, callee-saved registers, ESP and the x87/MXCSR
// controls must match. Where the original stopped at an unported callee the
// native must refuse that exact entry after the same reads and stores.
namespace {
using namespace halo2;using Json=nlohmann::json;
constexpr std::uint32_t stack_low=0x2000000u,stack_end=0x2040000u,stop_pc=0x3000000u;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto value:data){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;}
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
Json expand_writes(const Json &writes) {
    Json out=Json::array();
    for(const auto &write:writes) {
        const auto repeat=write.value("repeat",1u);
        for(unsigned n=0;n<repeat;++n){auto copy=write;copy.erase("repeat");copy["address"]=write.at("address").get<std::uint32_t>()+n*4u;out.push_back(copy);}
    }
    return out;
}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    // Undeclared stack: pushes and private frames; never recorded.
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
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        const auto found=locate(address,values.size());
        if(!found){std::copy(values.begin(),values.end(),scratch.begin()+(address-stack_low));return;}
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        auto &bank=(*found)->second;const auto at=address-(*found)->first;
        if(raw_stores)stores.push_back({0,address,{bank.begin()+at,bank.begin()+at+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),bank.begin()+at);
    }
    void install(std::uint32_t address,std::uint32_t value) {
        for(unsigned k=0;k<4;++k)scratch.at(address+k-stack_low)=std::byte(value>>(k*8u));
    }
    explicit Memory(const Json &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),before_bytes(bank));}
};
long double extended(const std::string &text) {
    const auto raw=bytes(text);check(raw.size()==10,"Invalid x87 value");long double value{};std::memcpy(&value,raw.data(),10);return value;
}
// Fault prefixes inside composed x87 owners carry their pending quantity.
std::exception_ptr cause_of(std::exception_ptr error) {
    for(;;) {
        try{std::rethrow_exception(error);}
        catch(const NativeCampaignAiPathInfluencePendingErrorTu15 &e){error=e.cause();continue;}
        catch(const NativeCampaignAiJumpArcPendingErrorTu15 &e){error=e.cause();continue;}
        catch(const NativeCampaignAiJumpPendingErrorTu15 &e){error=e.cause();continue;}
        catch(const NativeCampaignAiPendingDistanceErrorTu15 &e){error=e.cause();continue;}
        catch(const NativeCampaignAiPathResultPendingErrorTu15 &e){error=e.cause();continue;}
        catch(...){return error;}
    }
}
SourcePhysicsWorldRuntimeAbi entry_abi(const Json &row,Memory &memory) {
    SourcePhysicsWorldRuntimeAbi abi{};
    for(unsigned n=0;n<8;++n)abi.gp[n]=row.at("gp")[n];
    for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)abi.xmm[r][k]=row.at("xmm")[r][k];
    abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=unsigned(row.at("live_x87").size());
    for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=extended(row.at("live_x87")[n]);
    std::vector<std::uint32_t> words{row.value("return_pc",stop_pc),row.at("matrix").get<std::uint32_t>(),row.at("source").get<std::uint32_t>()};
    for(const auto &word:row.at("stack_arguments"))words.push_back(word);
    for(std::size_t n=0;n<words.size();++n)memory.install(abi.gp[4]+std::uint32_t(n)*4u,words[n]);
    return abi;
}
// Entries whose original result is AL.
bool returns_al(std::uint32_t entry) {
    return entry==0x26c5a0u || entry==0x269870u || entry==0x269db0u || entry==0x269e70u;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply the independent original 26A380 remainder fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","26A380 original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"26A380 reference supplied an outcome");
    check(fixture.at("bank_encoding")=="sparse-runs-v1" && fixture.at("write_encoding")=="rep-runs-v1","26A380 fixture encoding changed");
    std::size_t count{},completed{},refused{},prefixes{},reads{},stores{},register_checks{};
    std::map<std::uint32_t,std::size_t> entries;
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),name+": original unqualified");
        const auto fault=row.value("fault",Json::object());const auto entry=row.at("entry").get<std::uint32_t>();
        const auto writes=expand_writes(row.at("writes"));
        const auto stopped_at=row.value("stopped_at",stop_pc);
        for(const bool reading:std::array{false,true}) {
            if(reading && fault.value("phase",std::string{})=="after_write")continue;
            Memory memory(row);memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.recording=reading;
            memory.raw_stores=!reading && (memory.phase=="before_read" || memory.phase=="before_write");
            SourcePhysicsContactRecordObserver observer;std::vector<SourcePhysicsContactRecordStore> events;unsigned publications{};
            if(!reading && !memory.raw_stores)observer=[&](const auto &event){
                if(!memory.locate(event.address,event.written.size()))return;
                events.push_back(event);if(memory.phase=="after_write" && publications++==memory.ordinal)throw Prefix{};};
            auto abi=entry_abi(row,memory);
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool prefix{};std::optional<std::uint32_t> refusal;
            const auto missing=[&](const Missing &value){
                std::ostringstream why;why<<name<<": unexpected missing storage "<<std::hex<<value.address<<" width "<<std::dec<<value.width;
                throw std::runtime_error(why.str());};
            try {
                if(entry==0x26a380u)native_campaign_ai_evaluate_goals_with_query_call_tu15(memory,abi,observer);
                else if(entry==0x26a712u)native_campaign_ai_evaluate_goals_resume_call_tu15(memory,abi,observer);
                else native_campaign_ai_goal_scoring_call_tu15(memory,entry,abi,observer);
            } catch(...) {
                try{std::rethrow_exception(cause_of(std::current_exception()));}
                catch(const Prefix &){prefix=true;}
                catch(const Missing &value){missing(value);}
                catch(const NativeCampaignAiPerformRequiredOwnerTu15 &value){refusal=value.entry();}
            }
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
            if(!fault.empty()) {
                check(prefix,name+": native did not stop at the original prefix");
            } else if(stopped_at!=stop_pc) {
                if(!refusal || *refusal!=stopped_at) {
                    std::ostringstream why;why<<name<<": original stopped at "<<std::hex<<stopped_at<<", native "<<(refusal?"refused "+std::to_string(*refusal):std::string(prefix?"faulted":"completed"));
                    throw std::runtime_error(why.str());
                }
                if(!reading)++refused;
            } else {
                if(prefix || refusal) {
                    std::ostringstream why;why<<name<<": native did not complete"<<(refusal?" (refused "+std::to_string(*refusal)+")":"");
                    throw std::runtime_error(why.str());
                }
                const auto &gp=row.at("boundary_gp");
                const auto expected=gp[0].get<std::uint32_t>();
                if(entry==0x26a712u || entry==0x26a380u)check(abi.gp[0]==expected,name+": goal result differs");
                if(returns_al(entry) && (abi.gp[0]&0xffu)!=(expected&0xffu)) {
                    std::ostringstream why;why<<name<<": AL native="<<std::hex<<(abi.gp[0]&0xffu)<<" original="<<(expected&0xffu);
                    throw std::runtime_error(why.str());
                }
                if(entry==0x1209c0u)check(abi.xmm[0][0]==row.at("result_bits").get<std::uint32_t>(),name+": XMM0 result differs");
                for(const unsigned n:{3u,4u,5u,6u,7u}) {
                    if(abi.gp[n]!=gp[n].get<std::uint32_t>()) {
                        std::ostringstream why;why<<name<<": register "<<n<<" native="<<std::hex<<abi.gp[n]<<" original="<<gp[n].get<std::uint32_t>();
                        throw std::runtime_error(why.str());
                    }
                }
                check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                      abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs native="+Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr}).dump()+" original="+row.at("after_fp").dump());
                register_checks+=8;if(!reading){++completed;++entries[entry];}
            }
            for(const auto &bank:row.at("banks")) {
                const auto &actual=memory.banks.at(bank.at("address"));const auto expected=after_bytes(bank);
                if(actual!=expected) {
                    std::size_t at=0;while(at<actual.size() && actual[at]==expected[at])++at;
                    std::ostringstream why;why<<name<<": retained bytes differ at "<<std::hex<<bank.at("address").get<std::uint32_t>()+at
                        <<" native="<<std::to_integer<unsigned>(actual[at])<<" original="<<std::to_integer<unsigned>(expected[at]);
                    throw std::runtime_error(why.str());
                }
            }
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                // A read-modify-write (26AB9E INC WORD) or memory-to-memory
                // MOVS (26ADFD/26AF30/26AF6D/26AF6F) faults as a whole before
                // its store: the observer records neither half, the native has
                // read the operand (INC) or the source (MOVS). Accept exactly
                // that one trailing read.
                if(memory.phase=="before_write" && memory.reads.size()==expected.size()+1u) {
                    const auto &boundary=row.at("boundary");
                    const auto site=boundary.at("instruction").get<std::uint32_t>();
                    const bool string_move=site==0x26adfdu || site==0x26af30u || site==0x26af6du || site==0x26af6fu;
                    if(memory.reads.back()==Json::array({boundary.at("address"),boundary.at("bytes")}) ||
                       (string_move && memory.reads.back()[1]==boundary.at("bytes")))memory.reads.erase(memory.reads.size()-1u);
                }
                if(memory.reads!=expected) {
                    std::size_t at=0;while(at<memory.reads.size() && at<expected.size() && memory.reads[at]==expected[at])++at;
                    std::cerr<<name<<": first read difference at "<<at<<" native="<<(at<memory.reads.size()?memory.reads[at].dump():"end")
                        <<" original="<<(at<expected.size()?row.at("reads")[at].dump():"end")<<'\n';
                }
                check(memory.reads==expected,name+": ordered reads differ");reads+=memory.reads.size();
            } else {
                if(memory.raw_stores)events=memory.stores;
                for(std::size_t n=0;n<std::max(events.size(),writes.size());++n) {
                    if(n>=events.size() || n>=writes.size()) {
                        std::ostringstream why;why<<name<<": store count differs native="<<events.size()<<" original="<<writes.size()<<" at "<<n;
                        if(n<writes.size())why<<" next original "<<writes[n].dump();
                        if(n<events.size())why<<" next native pc="<<std::hex<<events[n].instruction<<" address="<<events[n].address;
                        throw std::runtime_error(why.str());
                    }
                    const auto &a=events[n];const auto &b=writes[n];
                    if(!((memory.raw_stores||a.instruction==b.at("instruction")) && a.address==b.at("address") &&
                        hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>())) {
                        std::ostringstream why;why<<name<<": ordered store "<<n<<" differs native pc="<<std::hex<<a.instruction<<" address="<<a.address
                            <<" before="<<hex(a.before)<<" written="<<hex(a.written)<<" original="<<b.dump();
                        throw std::runtime_error(why.str());
                    }
                }
                stores+=events.size();
            }
        }
        if(!fault.empty())++prefixes;
        ++count;
    }
    check(completed && refused && prefixes && entries[0x26a380u]==24 && entries[0x26a712u] && entries[0x267a60u] && entries[0x140910u],"26A380 qualification matrix incomplete");
    std::cout<<count<<" original 26A380 remainder cases ("<<completed<<" complete: "<<entries[0x26a712u]<<" 26A712, "
        <<entries[0x267a60u]<<" 267A60, "<<entries[0x140910u]<<" 140910, "<<(completed-entries[0x26a712u]-entries[0x267a60u]-entries[0x140910u])
        <<" other callees; "<<refused<<" required-owner refusals; "<<prefixes<<" fault prefixes); "<<reads<<" reads; "<<stores<<" stores; "
        <<register_checks<<" register checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/native_campaign_ai_path_search_tu15.hpp"
#include "halo2/native_campaign_ai_jump_arc_tu15.hpp"
#include "halo2/native_campaign_ai_jump_fallback_tu15.hpp"
#include "halo2/native_campaign_ai_path_influence_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <sstream>
#include <xmmintrin.h>

// Replays the physical unchanged-i386 27B2B0/27B340/27BD30 fixture through
// the native kernels. Every declared bank, ordered read, ordered store (with
// its PC), AL/AX, callee-saved register, ESP and FP control/status result
// must match, including every read/store fault prefix.
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
// rep-runs-v1: 27B2B0's REP STOSD iterations are stored as one counted run.
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
    // Undeclared stack: the original's pushes and private frames land here,
    // and the physical reference neither records nor compares them.
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
    // The observer's argument words: written before the original's entry,
    // never recorded or counted.
    void install(std::uint32_t address,std::uint32_t value) {
        for(unsigned k=0;k<4;++k) {
            const auto byte=std::byte(value>>(k*8u));const auto at=address+k;
            auto found=banks.upper_bound(at);
            if(found!=banks.begin() && at-std::prev(found)->first<std::prev(found)->second.size())std::prev(found)->second[at-std::prev(found)->first]=byte;
            else scratch.at(at-stack_low)=byte;
        }
    }
    explicit Memory(const Json &row){for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),before_bytes(bank));}
};
// Fault prefixes inside the composed x87 owners carry their pending
// extended quantity with the actual cause.
std::exception_ptr cause_of(std::exception_ptr error) {
    for(;;) {
        try{std::rethrow_exception(error);}
        catch(const NativeCampaignAiPathInfluencePendingErrorTu15 &e){error=e.cause();continue;}
        catch(const NativeCampaignAiJumpArcPendingErrorTu15 &e){error=e.cause();continue;}
        catch(const NativeCampaignAiJumpPendingErrorTu15 &e){error=e.cause();continue;}
        catch(const NativeCampaignAiPendingDistanceErrorTu15 &e){error=e.cause();continue;}
        catch(...){return error;}
    }
}
SourcePhysicsWorldRuntimeAbi entry_abi(const Json &row,Memory &memory) {
    SourcePhysicsWorldRuntimeAbi abi{};
    for(unsigned n=0;n<8;++n)abi.gp[n]=row.at("gp")[n];
    for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)abi.xmm[r][k]=row.at("xmm")[r][k];
    abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=0;
    std::vector<std::uint32_t> words{stop_pc,row.at("matrix").get<std::uint32_t>(),row.at("source").get<std::uint32_t>()};
    for(const auto &word:row.at("stack_arguments"))words.push_back(word);
    for(std::size_t n=0;n<words.size();++n)memory.install(abi.gp[4]+std::uint32_t(n)*4u,words[n]);
    return abi;
}
void dispatch(std::uint32_t entry,Memory &memory,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(entry==0x27b2b0u)native_campaign_ai_path_search_call_tu15(memory,abi,observe);
    else if(entry==0x27b340u)native_campaign_ai_path_expand_call_tu15(memory,abi,observe);
    else if(entry==0x27bd30u)native_campaign_ai_path_neighbors_call_tu15(memory,abi,observe);
    else throw std::runtime_error("Unknown 27B2B0 fixture entry");
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply the independent original 27B2B0 fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","27B2B0 original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>() && fixture.at("stop_entries").empty(),"27B2B0 reference supplied an outcome");
    check(fixture.at("bank_encoding")=="sparse-runs-v1" && fixture.at("write_encoding")=="rep-runs-v1","27B2B0 fixture encoding changed");
    std::size_t count{},completed{},prefixes{},reads{},stores{},register_checks{},overruns{};
    std::map<std::uint32_t,std::size_t> entries;
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),name+": original unqualified");
        const auto fault=row.value("fault",Json::object());const auto entry=row.at("entry").get<std::uint32_t>();
        const auto writes=expand_writes(row.at("writes"));
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
            bool prefix{};
            const auto missing=[&](const Missing &value){
                std::ostringstream why;why<<name<<": unexpected missing storage "<<std::hex<<value.address<<" width "<<std::dec<<value.width;
                throw std::runtime_error(why.str());};
            try{dispatch(entry,memory,abi,observer);}
            catch(...) {
                try{std::rethrow_exception(cause_of(std::current_exception()));}
                catch(const Prefix &){prefix=true;}
                catch(const Missing &value){missing(value);}
            }
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
            if(!fault.empty()) {
                check(prefix,name+": native did not stop at the original prefix");
            } else {
                check(!prefix,name+": native did not complete");
                const auto &gp=row.at("boundary_gp");
                const auto mask=entry==0x27bd30u ? 0xffffu : 0xffu;
                if((abi.gp[0]&mask)!=(gp[0].get<std::uint32_t>()&mask)) {
                    std::ostringstream why;why<<name<<": result native="<<std::hex<<abi.gp[0]<<" original="<<gp[0].get<std::uint32_t>();
                    throw std::runtime_error(why.str());
                }
                for(const unsigned n:{3u,4u,5u,6u,7u})check(abi.gp[n]==gp[n].get<std::uint32_t>(),name+": preserved register or ESP differs");
                check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                      abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs native="+Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr}).dump()+" original="+row.at("after_fp").dump());
                register_checks+=7;if(!reading){++completed;++entries[entry];}
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
                // A read-modify-write (DEC/INC WORD) faults as a whole before
                // its store: the observer records neither half, the native
                // has read the operand. Accept exactly that one trailing read.
                if(memory.phase=="before_write" && memory.reads.size()==expected.size()+1u) {
                    const auto &boundary=row.at("boundary");
                    if(memory.reads.back()==Json::array({boundary.at("address"),boundary.at("bytes")}))memory.reads.erase(memory.reads.size()-1u);
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
        // 27B340's caller owns only 64 rows: the same original 27BD30 state
        // that publishes row 64 must refuse there, after rows 0..63.
        if(row.value("overrun_control",false)) {
            Memory memory(row);auto abi=entry_abi(row,memory);
            NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
            const auto rows=row.at("stack_arguments")[0].get<std::uint32_t>();
            std::vector<SourcePhysicsContactRecordStore> events;
            const SourcePhysicsContactRecordObserver observer=[&](const auto &event){if(memory.locate(event.address,event.written.size()))events.push_back(event);};
            std::optional<std::int32_t> refused;
            try{(void)native_campaign_ai_path_neighbors_tu15(memory,row.at("matrix"),row.at("source"),rows,row.at("stack_arguments")[1],abi.gp[4],64,fp,observer);}
            catch(const NativeCampaignAiPathSearchOverrunTu15 &error){refused=error.row();check(error.site()==0x27c32du,name+": overrun site");}
            check(refused && *refused==64,name+": the 64-row caller did not refuse row 64");
            std::size_t owed=0;while(owed<writes.size() && writes[owed].at("address").get<std::uint32_t>()-rows<64u*0x30u)++owed;
            check(owed<writes.size() && events.size()==owed,name+": overrun refusal published a different store prefix");
            for(std::size_t n=0;n<owed;++n)check(events[n].address==writes[n].at("address") && hex(events[n].written)==writes[n].at("written").get<std::string>(),name+": overrun prefix store differs");
            ++overruns;
        }
        if(!fault.empty())++prefixes;
        ++count;
    }
    check(completed && prefixes && entries[0x27b2b0u] && entries[0x27b340u] && entries[0x27bd30u] && overruns,"27B2B0 qualification matrix incomplete");
    std::cout<<count<<" original 27B2B0/27B340/27BD30 cases ("<<completed<<" complete: "<<entries[0x27b2b0u]<<" 27B2B0, "<<entries[0x27b340u]
        <<" 27B340, "<<entries[0x27bd30u]<<" 27BD30; "<<prefixes<<" fault prefixes; "<<overruns<<" 64-row refusal controls); "<<reads<<" reads; "
        <<stores<<" stores; "<<register_checks<<" register checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

#include "halo2/native_campaign_ai_route_tu15.hpp"
#include "halo2/native_campaign_ai_route_corridor_tu15.hpp"
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

// Replays unchanged-i386 corridor entries through the native owners.
// All retained banks and ordered stores (including PCs), saved GP/ESP,
// defined result bits, CW/SW/MXCSR match. Deep private bytes and ordered
// reads are outside this semantic whole-entry qualification contract.
// This fixture qualifies whole calls. Rejected-memory FP continuations are
// outside this cohort and are not inferred from successful execution.
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
bool silent_owner(std::uint32_t){return false;}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    // Undeclared stack: pushes and private frames are neither recorded nor compared.
    mutable std::vector<std::byte> scratch=std::vector<std::byte>(stack_end-stack_low);
    mutable Json reads=Json::array();
    // Capstone's explicit-operand metadata omits PUSH/POP. Saved words
    // remain in full-bank comparisons and restored-register assertions.
    std::vector<std::uint32_t> implicit_saved_words;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{},written{};
    bool recording{};
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
        const bool implicit=out.size()==4 && std::ranges::find(implicit_saved_words,address)!=implicit_saved_words.end();
        if(!implicit && phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        std::copy_n((*found)->second.begin()+(address-(*found)->first),out.size(),out.begin());
        if(recording && !implicit)reads.push_back(Json::array({address,out.size()}));
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        const auto found=locate(address,values.size());
        if(!found){std::copy(values.begin(),values.end(),scratch.begin()+(address-stack_low));return;}
        const bool implicit=values.size()==4 && std::ranges::find(implicit_saved_words,address)!=implicit_saved_words.end();
        if(!implicit && phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        auto &bank=(*found)->second;const auto at=address-(*found)->first;
        if(!implicit)stores.push_back({0,address,{bank.begin()+at,bank.begin()+at+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),bank.begin()+at);
    }
    void install(std::uint32_t address,std::uint32_t value) {
        for(unsigned k=0;k<4;++k) {
            const auto byte=std::byte(value>>(k*8u));const auto at=address+k;
            auto found=banks.upper_bound(at);
            if(found!=banks.begin() && at-std::prev(found)->first<std::prev(found)->second.size())std::prev(found)->second[at-std::prev(found)->first]=byte;
            else scratch.at(at-stack_low)=byte;
        }
    }
    std::uint32_t peek_word(std::uint32_t address)const {
        const auto found=locate(address,4);std::uint32_t value{};
        const auto &data=found?(*found)->second:scratch;
        const auto offset=address-(found?(*found)->first:stack_low);
        for(unsigned k=0;k<4;++k)value|=std::to_integer<unsigned>(data[offset+k])<<(k*8u);
        return value;
    }
    explicit Memory(const Json &row):implicit_saved_words(row.value("implicit_saved_words",std::vector<std::uint32_t>{})) {
        for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),before_bytes(bank));
    }
};
SourcePhysicsWorldRuntimeAbi entry_abi(const Json &row,Memory &memory) {
    SourcePhysicsWorldRuntimeAbi abi{};
    for(unsigned n=0;n<8;++n)abi.gp[n]=row.at("gp")[n];
    for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)abi.xmm[r][k]=row.at("xmm")[r][k];
    abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=0;
    std::vector<std::uint32_t> words{row.value("return_pc",stop_pc),row.at("matrix").get<std::uint32_t>(),row.at("source").get<std::uint32_t>()};
    for(const auto &word:row.at("stack_arguments"))words.push_back(word);
    for(std::size_t n=0;n<words.size();++n)memory.install(abi.gp[4]+std::uint32_t(n)*4u,words[n]);
    return abi;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply the independent original route fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","route original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"route reference supplied an outcome");
    check(fixture.at("bank_encoding")=="sparse-runs-v1","route fixture encoding changed");
    std::size_t count{},completed{},refused{},prefixes{},reads{},stores{},register_checks{},tagged{};
    std::map<std::uint32_t,std::size_t> entries,refusals,pending_stores;
    const std::map<std::uint32_t,std::uint32_t> refusal_sites{{0x2cceb0u,0x2cc182u}};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),name+": original unqualified");
        const auto fault=row.value("fault",Json::object());const auto entry=row.at("entry").get<std::uint32_t>();
        const auto stopped=row.value("stopped_at",stop_pc);
        const auto &writes=row.at("writes");
        for(const bool reading:std::array{false}) {
            if(reading && fault.value("phase",std::string{})=="after_write")continue;
            Memory memory(row);memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);memory.recording=reading;
            // Native store PCs tag the raw bank store they follow.
            // The store pass only, and not in before-read/before-write prefixes:
            // an observed store reads its before-bytes, which would shift the
            // read ordinal. Those prefixes compare raw stores without PCs.
            const bool raw_stores=memory.phase=="before_read" || memory.phase=="before_write";
            SourcePhysicsContactRecordObserver observer;
            if(!reading && !raw_stores)observer=[&](const SourcePhysicsContactRecordStore &event) {
                if(!memory.locate(event.address,event.written.size()))return;
                check(!memory.stores.empty() && memory.stores.back().address==event.address && memory.stores.back().written==event.written,
                    name+": native store event without its store");
                memory.stores.back().instruction=event.instruction;
                // After-write means the instruction has committed, including
                // an FSTP pop. Throw from its observer, not memory.write.
                if(memory.phase=="after_write" && memory.written++==memory.ordinal)throw Prefix{};
            };
            auto abi=entry_abi(row,memory);
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool prefix{};std::optional<NativeCampaignAiPerformRequiredOwnerTu15> refusal;
            std::optional<NativeCampaignAiPendingDistanceTu15> pending;
            std::optional<std::array<std::byte,10>> pending_integer;
            try{
                try{native_campaign_ai_route_corridor_call_tu15(memory,entry,abi,{},observer);}
                catch(const NativeCampaignAiPendingDistanceErrorTu15 &error) {
                    pending=error.pending();std::rethrow_exception(error.cause());
                }
                catch(const NativeCampaignAiRoutePendingIntegerErrorTu15 &error) {
                    pending_integer=error.pending().extended;std::rethrow_exception(error.cause());
                }
            }
            catch(const Prefix &){prefix=true;}
            catch(const NativeCampaignAiPerformRequiredOwnerTu15 &error){refusal=error;}
            catch(const Missing &value) {
                std::ostringstream why;why<<name<<": unexpected missing storage "<<std::hex<<value.address<<" width "<<std::dec<<value.width;
                throw std::runtime_error(why.str());
            }
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
            if(!fault.empty()) {
                check(prefix,name+": native did not stop at the original prefix");
                const auto expected_pending=row.value("pending_extended",std::string{});
                const bool pending_read=memory.phase=="before_read" && row.at("boundary").at("instruction")==0x27aa22u;
                check((bool(pending)||bool(pending_integer))==!expected_pending.empty() || (pending && pending_read),name+": pending quantity presence differs");
                if(pending && !expected_pending.empty())check(hex(pending->extended)==expected_pending,name+": pending extended distance differs");
                if(pending_integer)check(hex(*pending_integer)==expected_pending,name+": pending extended integer differs");
                if((pending||pending_integer) && !reading)++pending_stores[row.at("boundary").at("instruction").get<std::uint32_t>()];
            } else if(stopped!=stop_pc) {
                if(!refusal || refusal->entry()!=stopped) {
                    std::ostringstream why;why<<name<<": original stopped at "<<std::hex<<stopped<<", native "
                        <<(refusal?"refused at "+std::to_string(refusal->entry()):std::string(prefix?"faulted":"completed"));
                    throw std::runtime_error(why.str());
                }
                check(refusal->site()==refusal_sites.at(stopped),name+": refusal call site differs");
                check(abi.gp[4]==row.at("boundary_gp")[4],name+": refusal callee ESP differs");
                // The authenticated physical CALL's five-byte instruction
                // supplies this return word even when the private call slot
                // is not part of the retained operand-event bank.
                check(memory.peek_word(abi.gp[4])==refusal_sites.at(stopped)+5u,name+": refusal return word differs");
                check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                      abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP at the refusal differs");
                if(!reading){++refused;++refusals[stopped];}
            } else {
                check(!prefix && !refusal,name+": native did not complete"+(refusal?std::string(" (refused ")+refusal->what()+")":""));
                const auto &gp=row.at("boundary_gp");
                const auto mask=(entry==0x2cdeb0u||entry==0x2cdbc0u)?0xffu:(entry==0x2cd500u||entry==0x2cd870u||entry==0x2cba80u)?0xffffu:0u;
                if((abi.gp[0]&mask)!=(gp[0].get<std::uint32_t>()&mask)) {
                    std::ostringstream why;why<<name<<": result native="<<std::hex<<abi.gp[0]<<" original="<<gp[0].get<std::uint32_t>();
                    throw std::runtime_error(why.str());
                }
                for(const unsigned n:{3u,4u,5u,6u,7u})if(abi.gp[n]!=gp[n].get<std::uint32_t>()) {
                    std::ostringstream why;why<<name<<": preserved register or ESP "<<n<<" differs native="<<std::hex<<abi.gp[n]
                        <<" original="<<gp[n].get<std::uint32_t>();
                    throw std::runtime_error(why.str());
                }
                check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                      abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs native="+Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr}).dump()+" original="+row.at("after_fp").dump());
                register_checks+=7;if(!reading){++completed;++entries[entry];}
            }
            check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                  abi.floating_point->mxcsr==row.at("after_fp")[2],name+": boundary FP differs native="+Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr}).dump()+" original="+row.at("after_fp").dump());
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
                // A read-modify-write (27AA72's INC WORD) faults as a whole
                // before its store: accept exactly that one trailing read.
                // MOVS (the fixture's movs_sites): the observer checks the
                // destination operand first, so a before-write boundary there
                // records no source read; the native has read the source.
                if(memory.phase=="before_write" && memory.reads.size()==expected.size()+1u) {
                    const auto &boundary=row.at("boundary");
                    const auto at=boundary.at("instruction").get<std::uint32_t>();
                    const auto &movs=fixture.at("movs_sites");
                    if(memory.reads.back()==Json::array({boundary.at("address"),boundary.at("bytes")}) ||
                       (std::ranges::find(movs,Json(at))!=movs.end() && memory.reads.back()[1]==boundary.at("bytes")))
                        memory.reads.erase(memory.reads.size()-1u);
                }
                if(memory.reads!=expected) {
                    std::size_t at=0;while(at<memory.reads.size() && at<expected.size() && memory.reads[at]==expected[at])++at;
                    std::cerr<<name<<": first read difference at "<<at<<" native="<<(at<memory.reads.size()?memory.reads[at].dump():"end")
                        <<" original="<<(at<expected.size()?row.at("reads")[at].dump():"end")<<'\n';
                }
                check(memory.reads==expected,name+": ordered reads differ");reads+=memory.reads.size();
            } else {
                const auto &events=memory.stores;
                for(std::size_t n=0;n<std::max(events.size(),writes.size());++n) {
                    if(n>=events.size() || n>=writes.size()) {
                        std::ostringstream why;why<<name<<": store count differs native="<<events.size()<<" original="<<writes.size()<<" at "<<n;
                        if(n<writes.size())why<<" next original "<<writes[n].dump();
                        if(n<events.size())why<<" next native pc="<<std::hex<<events[n].instruction<<" address="<<events[n].address;
                        throw std::runtime_error(why.str());
                    }
                    const auto &a=events[n];const auto &b=writes[n];
                    const auto pc=b.at("instruction").get<std::uint32_t>();
                    // An after-write prefix stops inside the boundary store,
                    // before its PC event; the whole case checks that PC.
                    const bool boundary=memory.phase=="after_write" && n+1u==events.size();
                    const bool pc_ok=raw_stores || (a.instruction ? a.instruction==pc : silent_owner(pc) || boundary);
                    if(!(pc_ok && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() &&
                         hex(a.written)==b.at("written").get<std::string>())) {
                        std::ostringstream why;why<<name<<": ordered store "<<n<<" differs native pc="<<std::hex<<a.instruction<<" address="<<a.address
                            <<" before="<<hex(a.before)<<" written="<<hex(a.written)<<" original="<<b.dump();
                        throw std::runtime_error(why.str());
                    }
                    if(a.instruction)++tagged;
                }
                stores+=events.size();
            }
        }
        if(!fault.empty())++prefixes;
        ++count;
    }
    check(completed,"Corridor qualification matrix incomplete");
    std::cout<<count<<" original corridor cases ("<<completed<<" complete; "<<refused<<" named refusals";
    for(const auto &[at,n]:refusals)std::cout<<' '<<std::hex<<at<<std::dec<<':'<<n;
    std::cout<<"; "<<prefixes<<" fault prefixes); "<<reads<<" reads; "<<stores<<" stores ("<<tagged<<" with PCs); "
        <<register_checks<<" register checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

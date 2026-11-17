#include "halo2/native_campaign_ai_publication_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <xmmintrin.h>

// Replays the physical unchanged-i386 26C3C0/1FC540/1FC9B0/200D30 fixture
// through the native owners. Every declared bank, ordered read, ordered
// store (with its PC where the native owner reports one), the named refusal
// entry, EAX/AL, EBX/ESP/EBP/ESI/EDI and the x87 CW/SW and MXCSR must match,
// including every read/store fault prefix.
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
// 26C600 (native order-script assignment) publishes no store PCs; its
// stores must still match in order, address and bytes, from its closure.
bool silent_owner(std::uint32_t pc){return (pc>=0x26c600u && pc<0x26c740u) || (pc>=0x140d50u && pc<0x140f00u);}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<std::uint32_t,std::vector<std::byte>> banks;
    // Undeclared stack: pushes and private frames are neither recorded nor compared.
    mutable std::vector<std::byte> scratch=std::vector<std::byte>(stack_end-stack_low);
    mutable Json reads=Json::array();
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
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        std::copy_n((*found)->second.begin()+(address-(*found)->first),out.size(),out.begin());
        if(recording)reads.push_back(Json::array({address,out.size()}));
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        const auto found=locate(address,values.size());
        if(!found){std::copy(values.begin(),values.end(),scratch.begin()+(address-stack_low));return;}
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        auto &bank=(*found)->second;const auto at=address-(*found)->first;
        stores.push_back({0,address,{bank.begin()+at,bank.begin()+at+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),bank.begin()+at);
        if(phase=="after_write" && written++==ordinal)throw Prefix{};
    }
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
    check(argc==2,"Supply the independent original 26C3C0 fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","26C3C0 original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"26C3C0 reference supplied an outcome");
    check(fixture.at("bank_encoding")=="sparse-runs-v1","26C3C0 fixture encoding changed");
    const auto kind=fixture.value("fixture_kind",std::string("publication-v1"));
    check(kind=="publication-v1" || kind=="command-point-1fc720-v1","Unknown publication qualification cohort");
    const bool command_point=kind=="command-point-1fc720-v1";
    std::size_t count{},completed{},refused{},prefixes{},reads{},stores{},register_checks{},tagged{};
    std::map<std::uint32_t,std::size_t> entries,refusals;
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),name+": original unqualified");
        const auto fault=row.value("fault",Json::object());const auto entry=row.at("entry").get<std::uint32_t>();
        if(command_point)check(entry==0x1fc720u,name+": command point cohort contains another entry");
        const auto stopped=row.value("stopped_at",stop_pc);
        const auto &writes=row.at("writes");
        const auto semantic_pcs=row.value("semantic_child_store_pcs",std::vector<std::uint32_t>{});
        for(const bool reading:std::array{false,true}) {
            if(reading && (fault.value("phase",std::string{})=="after_write" || !row.value("compare_reads",true)))continue;
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
            };
            auto abi=entry_abi(row,memory);
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool prefix{};std::optional<NativeCampaignAiPerformRequiredOwnerTu15> refusal;
            try{native_campaign_ai_publication_call_tu15(memory,entry,abi,{},observer);}
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
            } else if(stopped!=stop_pc) {
                if(!refusal || refusal->entry()!=stopped) {
                    std::ostringstream why;why<<name<<": original stopped at "<<std::hex<<stopped<<", native "
                        <<(refusal?"refused at "+std::to_string(refusal->entry()):std::string(prefix?"faulted":"completed"));
                    throw std::runtime_error(why.str());
                }
                check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                      abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP at the refusal differs");
                if(!reading){++refused;++refusals[stopped];}
            } else {
                check(!prefix && !refusal,name+": native did not complete"+(refusal?std::string(" (refused ")+refusal->what()+")":""));
                const auto &gp=row.at("boundary_gp");
                const auto mask=entry==0x26c3c0u ? 0xffffffffu : 0xffu;
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
                // A read-modify-write (200A10's INC WORD) faults as a whole
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
                    const bool pc_ok=raw_stores || (a.instruction ? a.instruction==pc : row.value("semantic_child_stores",false) || std::ranges::find(semantic_pcs,pc)!=semantic_pcs.end() || silent_owner(pc) || boundary);
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
    if(command_point)check(completed && prefixes && entries[0x1fc720u] && entries.size()==1u,
        "1FC720 qualification matrix incomplete");
    else check(completed && refused && prefixes && entries[0x26c3c0u] && entries[0x200d30u] && entries[0x1fc9b0u] && entries[0x1fc540u],
        "26C3C0 qualification matrix incomplete");
    std::cout<<count<<" original "<<(command_point?"1FC720":"26C3C0/1FC540/1FC9B0/200D30")<<" cases ("<<completed<<" complete: ";
    if(command_point)std::cout<<entries[0x1fc720u]<<" 1FC720";
    else std::cout<<entries[0x26c3c0u]<<" 26C3C0, "<<entries[0x1fc540u]<<" 1FC540, "
        <<entries[0x1fc9b0u]<<" 1FC9B0, "<<entries[0x200d30u]<<" 200D30";
    std::cout<<"; "<<refused<<" named refusals";
    for(const auto &[at,n]:refusals)std::cout<<' '<<std::hex<<at<<std::dec<<':'<<n;
    std::cout<<"; "<<prefixes<<" fault prefixes); "<<reads<<" reads; "<<stores<<" stores ("<<tagged<<" with PCs); "
        <<register_checks<<" register checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

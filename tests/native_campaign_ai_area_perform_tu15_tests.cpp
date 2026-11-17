#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_campaign_ai_area_choice_tu15.hpp"
#include "halo2/native_campaign_ai_candidate_collection_tu15.hpp"
#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/native_campaign_ai_actor_path_query_tu15.hpp"
#include "halo2/native_campaign_ai_object_navigation_tu15.hpp"
#include "halo2/native_campaign_ai_biped_refresh_tu15.hpp"
#include "halo2/native_campaign_ai_goal_evaluation_tu15.hpp"
#include "halo2/native_campaign_ai_path_distance_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <xmmintrin.h>

// Replays the physical unchanged-i386 1BBEE0/1BC0A0/1ECD40/26BA30 fixture
// through the native kernels. Every declared bank, ordered read, ordered store, register,
// XMM and FP control result must match; unported callees must refuse at the
// exact CALL where the original stopped. Rows marked retained_refresh_data
// separately qualify retained banks/stores, saved GP and FP, excluding private
// query scratch, dead volatile registers and the legacy ordered-read contract.
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
struct Stopped {std::uint32_t site,entry;SourcePhysicsWorldRuntimeAbi abi;};
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
    // 1BBEE0's PUSH EBP/EBX/ESI/EDI words inside the declared frame bank.
    // The reference has no explicit store operand for them, so they neither
    // count toward a store prefix nor appear as ordered stores.
    std::vector<std::uint32_t> implicit;
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        const auto found=locate(address,values.size());
        if(!found){std::copy(values.begin(),values.end(),scratch.begin()+(address-stack_low));return;}
        const bool pushed=values.size()==4 && std::ranges::find(implicit,address)!=implicit.end();
        if(!pushed && phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        auto &bank=(*found)->second;const auto at=address-(*found)->first;
        if(raw_stores && !pushed)stores.push_back({0,address,{bank.begin()+at,bank.begin()+at+values.size()},{values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),bank.begin()+at);
    }
    // Only explicit-operand stores are ordered events in the reference;
    // PUSH/CALL words reach the banks directly. Kernels use write() for both,
    // so ordered comparison uses observer events (which carry the PC).
    explicit Memory(const Json &row){
        for(const auto &bank:row.at("banks"))banks.emplace(bank.at("address"),before_bytes(bank));
        if(row.at("entry")==0x1bbee0u) {
            const auto sp=row.at("gp")[4].get<std::uint32_t>(),frame=((sp-4u)&~7u)-0x76cu-12u;
            implicit={sp-4u,frame+8u,frame+4u,frame};
        } else {
            if(row.at("entry")==0x1ecd40u)implicit.push_back(row.at("gp")[4].get<std::uint32_t>()-4u);
            if(row.at("entry")==0x1ed700u) {
                const auto sp=row.at("gp")[4].get<std::uint32_t>();
                implicit={sp-4u,sp-8u,sp-12u,sp-16u};
            }
            if(row.at("entry")==0x2013b0u) {
                const auto sp=row.at("gp")[4].get<std::uint32_t>();
                // Saved registers, argument pushes and CALL returns of the
                // query and its character-property/point-cache callees.
                // Its explicit stack stores are byte SP+4 and word SP-4.
                for(unsigned offset=0x14;offset<=0x40;offset+=4)implicit.push_back(sp-offset);
            }
            if(row.at("entry")==0x275e90u) {
                const auto sp=row.at("gp")[4].get<std::uint32_t>();
                for(unsigned offset:{4u,8u,12u,16u,20u,24u,0x80u,0x84u,0x88u})implicit.push_back(sp-offset);
            }
            if(row.at("entry")==0x275ba0u)implicit.push_back(row.at("gp")[4].get<std::uint32_t>()-4u);
            if(row.at("entry")==0x275cb0u) {
                const auto sp=row.at("gp")[4].get<std::uint32_t>();
                for(unsigned offset=0x78;offset<=0x98;offset+=4)implicit.push_back(sp-offset);
                // Only the vehicle arm pushes at SP-9C. In the biped child
                // that address is E2300's explicit local, not an implicit word.
                const auto &sites=row.at("executed_sites");
                if(std::ranges::find(sites,Json(0x275d8bu))!=sites.end())implicit.push_back(sp-0x9cu);
            }
        }
        for(const auto &address:row.value("implicit_stack_addresses",Json::array()))implicit.push_back(address);
    }
};
long double extended(const std::string &text) {
    const auto raw=bytes(text);check(raw.size()==10,"Invalid x87 value");long double value{};std::memcpy(&value,raw.data(),10);return value;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply the independent original 1BBEE0 fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","1BBEE0 original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"1BBEE0 reference supplied an outcome");
    check(fixture.at("bank_encoding")=="sparse-runs-v1","1BBEE0 fixture bank encoding changed");
    std::size_t count{},completed{},refused{},prefixes{},reads{},stores{},register_checks{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>(),name+": original unqualified");
        const auto fault=row.value("fault",Json::object());
        const auto stopped_at=row.value("stopped_at",stop_pc);
        for(const bool reading:std::array{false,true}) {
            if(reading && row.value("retained_refresh_data",false))continue;
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
            if(row.at("entry")==0x26ba30u || row.at("entry")==0x26a380u || row.value("retained_refresh_data",false)) {
                // Direct 26BA30/26A380 entry: the observer's [return, matrix,
                // source] words plus stack_arguments in the undeclared
                // caller stack.
                std::vector<std::uint32_t> words{stop_pc,row.at("matrix").get<std::uint32_t>(),row.at("source").get<std::uint32_t>()};
                for(const auto &word:row.at("stack_arguments"))words.push_back(word);
                std::vector<std::byte> raw(words.size()*4u);
                for(std::size_t n=0;n<words.size();++n)for(unsigned k=0;k<4;++k)raw[n*4u+k]=std::byte(words[n]>>(k*8u));
                std::copy(raw.begin(),raw.end(),memory.scratch.begin()+(abi.gp[4]-stack_low));
            }
            std::optional<NativeCampaignAiBipedRefreshScratchBindingTu15> refresh_binding;
            if(row.value("retained_refresh_data",false)) {
                std::vector<NativeTu15ContactPrivateRange> retained;
                for(const auto &[base,data]:memory.banks)retained.push_back({base,std::uint32_t(data.size())});
                retained.push_back({stack_low,stack_end-stack_low});
                refresh_binding.emplace(memory,0x6000000u,retained);
            }
            NativeCampaignAiPerformServicesTu15 services;
            services.callee=[](std::uint32_t site,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &state){throw Stopped{site,entry,state};};
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            bool prefix{};std::optional<Stopped> stop;std::optional<std::uint32_t> refusal;
            const auto missing=[&](const Missing &value){
                std::ostringstream why;why<<name<<": unexpected missing storage "<<std::hex<<value.address<<" width "<<std::dec<<value.width;
                throw std::runtime_error(why.str());};
            try {
                if(row.at("entry")==0x275ba0u || row.at("entry")==0x275cb0u || row.at("entry")==0xe2300u ||
                    row.at("entry")==0xe04b0u || row.at("entry")==0xe22c0u || row.at("entry")==0x2186f0u)
                    native_campaign_ai_object_navigation_call_tu15(memory,row.at("entry"),abi,observer);
                else if(row.at("entry")==0x2013b0u || row.at("entry")==0x275e90u)
                    native_campaign_ai_actor_path_query_call_tu15(memory,row.at("entry"),abi,observer);
                else if(row.at("entry")==0x1bc0a0u)native_campaign_ai_area_perform_release_tu15(memory,abi,observer);
                else if(row.at("entry")==0x1ecd40u || row.at("entry")==0x1ed700u)
                    native_campaign_ai_character_property_call_tu15(memory,row.at("entry"),abi);
                else if(row.at("entry")==0x26ba30u)native_campaign_ai_collect_candidates_call_tu15(memory,abi,observer);
                else if(row.at("entry")==0x26a380u)native_campaign_ai_evaluate_goals_call_tu15(memory,abi,observer);
                else native_campaign_ai_area_perform_tu15(memory,abi,services,observer);
            }catch(const Prefix &){prefix=true;}
            catch(const Missing &value){missing(value);}
            catch(const Stopped &value){stop=value;}
            catch(const NativeCampaignAiPerformRequiredOwnerTu15 &value){refusal=value.entry();}
            catch(const NativeCampaignAiPathDistancePendingErrorTu15 &error){prefix=true;try{std::rethrow_exception(error.cause());}catch(const Prefix &){}catch(const Missing &value){missing(value);}}
            catch(const NativeCampaignAiAreaScorePendingErrorTu15 &error){prefix=true;try{std::rethrow_exception(error.cause());}catch(const Prefix &){}catch(const Missing &value){missing(value);}}
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
            if(!fault.empty()) {
                check(prefix,name+": native did not stop at the original prefix");
            } else if(stopped_at!=stop_pc && refusal) {
                // A leaf refusal (no transport ABI): the original stopped at the
                // same entry; banks, ordered reads and stores are compared below.
                check(*refusal==stopped_at,name+": native refused another owner");
                if(row.value("object_navigation_exact",false)) {
                    for(unsigned n=0;n<8;++n)check(abi.gp[n]==row.at("boundary_gp")[n],name+": required-owner register "+std::to_string(n)+" differs");
                    check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                          abi.floating_point->mxcsr==row.at("after_fp")[2],name+": required-owner FP differs");
                    register_checks+=8;
                }
                if(!reading)++refused;
            } else if(stopped_at!=stop_pc) {
                check(stop && stop->entry==stopped_at,name+": native did not refuse at the original required owner");
                check(row.at("boundary_gp")[4].get<std::uint32_t>()==stop->abi.gp[4],name+": required-owner stack differs");
                for(unsigned n=0;n<8;++n) {
                    if(row.at("boundary_gp")[n].get<std::uint32_t>()!=stop->abi.gp[n]) {
                        std::ostringstream why;why<<name<<": required-owner register "<<n<<" native="<<std::hex<<stop->abi.gp[n]<<" original="<<row.at("boundary_gp")[n].get<std::uint32_t>();
                        throw std::runtime_error(why.str());
                    }
                }
                check(stop->abi.floating_point->x87_control_word==row.at("after_fp")[0] && stop->abi.x87_status==row.at("after_fp")[1] &&
                      stop->abi.floating_point->mxcsr==row.at("after_fp")[2],name+": required-owner FP differs");
                // After 26B220 the 0BAD60 marker request (and 26BA30's 218B20
                // projections) own XMM scratch the native owners do not model.
                // XMM registers are caller-saved in the original ABI and 26A380
                // writes each one before reading it.
                if(row.value("xmm_exact",false) && stopped_at!=0x26ba30u && stopped_at!=0x26a380u)for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)
                    check(stop->abi.xmm[r][k]==row.at("boundary_xmm")[r][k],name+": required-owner XMM differs");
                register_checks+=8;if(!reading)++refused;
            } else {
                check(!stop && !prefix && !refusal,name+": native did not complete");
                const auto &gp=row.at("boundary_gp");
                check((abi.gp[0]&255u)==(gp[0].get<std::uint32_t>()&255u),name+": AL result differs");
                if((row.at("entry")==0x26ba30u || row.at("entry")==0x26a380u) && abi.gp[0]!=gp[0].get<std::uint32_t>()) {
                    std::ostringstream why;why<<name<<": 26BA30 count native="<<std::hex<<abi.gp[0]<<" original="<<gp[0].get<std::uint32_t>();
                    throw std::runtime_error(why.str());
                }
                for(const unsigned n:{3u,4u,5u,6u,7u})check(abi.gp[n]==gp[n].get<std::uint32_t>(),name+": preserved register differs");
                // EAX/ECX/EDX beyond AL are dead at 1B0ADB and 1B0F58; they are
                // modeled exactly where no callee owns them (inactive, 1BC0A0).
                if(row.at("entry")==0x1bc0a0u || row.at("entry")==0x1ecd40u || row.at("entry")==0x1ed700u || row.at("entry")==0x26a380u ||
                    name.rfind("inactive",0)==0 || row.value("query_scratch_exact",false))
                    for(unsigned n=0;n<3;++n) {
                        if(abi.gp[n]!=gp[n].get<std::uint32_t>()){std::ostringstream why;why<<name<<": exit register "<<n<<" native="<<std::hex<<abi.gp[n]<<" original="<<gp[n].get<std::uint32_t>();throw std::runtime_error(why.str());}
                    }
                // XMM is caller-saved scratch: exact only on paths whose
                // callees the native models it for (not 0BAD60 or 218B20).
                const auto &sites=row.at("executed_sites");
                const bool scratch_xmm=std::ranges::find(sites,Json(0x26b220u))!=sites.end();
                if(row.value("xmm_exact",false) && !scratch_xmm) {
                    for(unsigned r=0;r<8;++r)for(unsigned k=0;k<4;++k)check(abi.xmm[r][k]==row.at("boundary_xmm")[r][k],name+": exit XMM differs");
                }
                check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] &&
                      abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs native="+Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr}).dump()+" original="+row.at("after_fp").dump());
                register_checks+=6;if(!reading)++completed;
            }
            if(row.value("retained_refresh_data",false))check(Json::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})==row.at("after_fp"),name+": retained refresh FP prefix differs");
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
                // A read-modify-write instruction (2A7A0F INC WORD) faults as a
                // whole before its store: the observer records neither half.
                // The native publication reads that operand, then stores; only
                // that one trailing read of the boundary operand is accepted.
                if(memory.phase=="before_write" && memory.reads.size()==expected.size()+1u) {
                    const auto &boundary=row.at("boundary");
                    if(memory.reads.back()==Json::array({boundary.at("address"),boundary.at("bytes")}))memory.reads.erase(memory.reads.size()-1u);
                }
                if(memory.reads!=expected) {
                    std::size_t at=0;while(at<memory.reads.size() && at<expected.size() && memory.reads[at]==expected[at])++at;
                    std::cerr<<name<<": first read difference at "<<at<<" native="<<(at<memory.reads.size()?memory.reads[at].dump():"end")<<" original="<<(at<expected.size()?expected[at].dump():"end")<<'\n';
                }
                check(memory.reads==expected,name+": ordered reads differ");reads+=memory.reads.size();
            } else {
                if(memory.raw_stores)events=memory.stores;
                const auto &writes=row.at("writes");
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
    if(fixture.at("owners").size()==1 && fixture.at("owners").contains("0x1ed700"))
        check(count==374 && completed==32 && prefixes==342 && !refused,"1ED700 qualification matrix incomplete");
    else check(count && completed && refused && prefixes,"1BBEE0 qualification matrix incomplete");
    std::cout<<count<<" original AI-owner cases ("<<completed<<" complete, "<<refused<<" required-owner refusals, "<<prefixes
        <<" fault prefixes); "<<reads<<" reads; "<<stores<<" stores; "<<register_checks<<" register checks passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

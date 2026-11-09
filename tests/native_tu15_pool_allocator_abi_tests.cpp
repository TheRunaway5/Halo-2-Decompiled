#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_pool_allocator_abi.hpp"
#include "physics/runtime/native_tu15_array_allocator_abi.hpp"
#include "physics/runtime/native_tu15_stack_arena_abi.hpp"
#include "physics/runtime/native_tu15_main_heap_abi.hpp"
#include "physics/runtime/native_tu15_main_heap_allocate_abi.hpp"
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>

namespace halo2 {
using namespace physics_world_runtime_detail;
struct ObservedWorldState:WorldState{using WorldState::WorldState;bool selected_observation_pcs=true;};
inline unsigned translated_fallbacks{};
struct RequiredOwner{};
inline std::uint32_t required_entry{};
inline std::function<void(ObservedWorldState &,std::uint32_t)> observe_entry;
inline bool actual_owner_transport{};
inline std::set<std::uint32_t> declared_entry_stops;
std::uint32_t sign_extend(std::uint32_t value,unsigned width){return std::uint32_t(width==1u?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))):std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}
std::uint32_t canonical_branch(std::uint32_t value,SourcePhysicsContactImage){switch(value){case 0x232c14u:return 0x22e314u;case 0x232c1au:return 0x22e31au;case 0x232d4eu:return 0x22e44eu;case 0x232d64u:return 0x22e464u;default:return value;}}
namespace production {static void run_22e950(ObservedWorldState &);static void run_22eb20(ObservedWorldState &);static void run_148a10(ObservedWorldState &);static void run_148a70(ObservedWorldState &);static void run_22e380(ObservedWorldState &);static void run_22e420(ObservedWorldState &);static void run_22e1d0(ObservedWorldState &);static void run_22e400(ObservedWorldState &);static void run_22e430(ObservedWorldState &);static void run_149bf0(ObservedWorldState &);static void run_149c40(ObservedWorldState &);static void run_233160(ObservedWorldState &);static void run_2331a0(ObservedWorldState &);static void run_232aa0(ObservedWorldState &);static void run_232ac0(ObservedWorldState &);static void run_2e1030(ObservedWorldState &);static void run_149930(ObservedWorldState &);}
void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t,bool){
    // An oracle entry boundary is independent of whether this fixture also
    // contains a native/reference implementation of that owner. Stop before
    // recording or executing the callee, matching the physical observer.
    if(declared_entry_stops.contains(target)){required_entry=target;throw RequiredOwner{};}
    if(actual_owner_transport&&!native_raw_allocator_detail::reference_mode()&&
        std::find(native_allocator_owner_detail::owner_entries.begin(),native_allocator_owner_detail::owner_entries.end(),target)!=native_allocator_owner_detail::owner_entries.end()) {
        if(observe_entry)observe_entry(s,target);
        if(native_allocator_owner_detail::dispatch(s,target,[](auto &v,auto owner,auto site,bool tail){dispatch(v,owner,site,tail);}))return;
        throw std::runtime_error("Qualified actual allocator owner declined");
    }
    if(target==0x233250u){production::run_22e950(s);return;}
    if(target==0x233420u){production::run_22eb20(s);return;}
    if(target==0x149da0u){production::run_148a10(s);return;}
    if(target==0x149e00u){production::run_148a70(s);return;}
    if(target==0x232c80u){production::run_22e380(s);return;}
    if(target==0x232d20u){production::run_22e420(s);return;}
    if(target==0x232ad0u){production::run_22e1d0(s);return;}
    if(target==0x232d00u){production::run_22e400(s);return;}
    if(target==0x232d30u){production::run_22e430(s);return;}
    if(target==0x149bf0u){production::run_149bf0(s);return;}
    if(target==0x149c40u){production::run_149c40(s);return;}
    if(target==0x233160u){production::run_233160(s);return;}
    if(target==0x2331a0u){production::run_2331a0(s);return;}
    if(target==0x232aa0u){production::run_232aa0(s);return;}
    if(target==0x232ac0u){production::run_232ac0(s);return;}
    if(target==0x2e1030u){production::run_2e1030(s);return;}
    required_entry=target;throw RequiredOwner{};
}
#include "native_tu15_allocator_actual_owner_reference.inc"
#include "pool_allocator_abi_bodies.inc"
}
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::string difference(const J &actual,const J &expected,const std::string &path="$"){
    if(actual==expected)return {};
    if(actual.is_object()&&expected.is_object()){
        for(auto it=expected.begin();it!=expected.end();++it){if(!actual.contains(it.key()))return path+"."+it.key()+" missing in native packet";
            const auto diff=difference(actual.at(it.key()),it.value(),path+"."+it.key());if(!diff.empty())return diff;}
        for(auto it=actual.begin();it!=actual.end();++it)if(!expected.contains(it.key()))return path+"."+it.key()+" unexpected in native packet";
    }
    if(actual.is_array()&&expected.is_array()){
        for(unsigned n=0;n<std::min(actual.size(),expected.size());++n){const auto diff=difference(actual[n],expected[n],path+"["+std::to_string(n)+"]");if(!diff.empty())return diff;}
    }
    return path+" native="+actual.dump()+" original="+expected.dump();
}
std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte>out;for(unsigned i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned genuine pool bank");std::vector<char>raw((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(raw.size());for(unsigned i=0;i<raw.size();++i)b[i]=std::byte(std::uint8_t(raw[i]));return b;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();bool logging{};
    auto at(std::uint32_t p,std::size_t size)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked pool ABI operand");--i;check(std::uint64_t(p-i->first)+size<=i->second.size(),"Unbacked pool ABI extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{if(logging)reads.push_back(J::array({p,out.size()}));auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>in)override{auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply unchanged TU1.5 pool ABI fixture");const std::filesystem::path path=argv[1];std::ifstream input(path);const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Pool ABI original pin mismatch");
    unsigned cases{},stores{},reads{};
    for(const auto &c:fixture.at("cases"))for(bool read_pass:{false,true}) {
        const auto name=c.at("name").get<std::string>();Memory memory;memory.logging=read_pass;
        for(const auto &b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));
        J actual_stores=J::array();SourcePhysicsContactRecordObserver observe;
        if(!read_pass)observe=[&](const auto &e){actual_stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        SourcePhysicsContactRuntimeServices services;std::uint32_t pc{};services.instruction=[&](std::uint32_t value){pc=value;};
        services.required_owner=[](auto &,const auto &request,auto &,const auto &){required_entry=request.target;throw RequiredOwner{};};
        const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};auto &cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.service_sites=sites;
        ObservedWorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        const auto &initial=c.at("before_abi");SourcePhysicsContactRuntimeAbi abi{};abi.gp=initial.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=initial.at("eflags");
        const auto initial_xmm=unhex(initial.at("xmm"));std::memcpy(abi.xmm.data(),initial_xmm.data(),128);
        abi.x87_status=initial.at("fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{initial.at("fp")[0],initial.at("fp")[2]};
        J entry_abis=J::array();observe_entry=[&](auto &s,std::uint32_t target){
            const auto a=s.save();entry_abis.push_back(J{{"entry",target},{"abi",J{{"gp",a.gp},{"eflags",a.eflags&0xcd5u},
                {"xmm",hex(std::as_bytes(std::span(a.xmm)))},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})}}}});
        };
        state.load(abi);bool required=false;required_entry=0;actual_owner_transport=c.value("raw_owner_transport",false);
        declared_entry_stops.clear();for(const auto &entry:c.value("required_entries",J::array()))declared_entry_stops.insert(entry.get<std::uint32_t>());
        try {const auto entry=c.at("entry").get<unsigned>();
            if(actual_owner_transport&&std::find(native_allocator_owner_detail::owner_entries.begin(),native_allocator_owner_detail::owner_entries.end(),entry)!=native_allocator_owner_detail::owner_entries.end())dispatch(state,entry,0,false);
            else if(entry==0x233250u)production::run_22e950(state);else if(entry==0x233420u)production::run_22eb20(state);
            else if(entry==0x149da0u)production::run_148a10(state);else if(entry==0x149e00u)production::run_148a70(state);
            else if(entry==0x2e10a0u)production::run_2e10a0(state);else if(entry==0x2e1110u)production::run_2e1110(state);
            else if(entry==0x232c80u)production::run_22e380(state);else if(entry==0x232d20u)production::run_22e420(state);
            else if(entry==0x232ad0u)production::run_22e1d0(state);else if(entry==0x232d00u)production::run_22e400(state);else if(entry==0x232d30u)production::run_22e430(state);
            else dispatch(state,entry,0,false);}catch(const RequiredOwner&){required=true;}
        check(required==c.at("required_owner")&&required_entry==c.at("required_entry"),name+" actual required owner entry differs native="+
            J{{"required",required},{"entry",required_entry},{"pc",pc}}.dump()+" original="+
            J{{"required",c.at("required_owner")},{"entry",c.at("required_entry")},{"pc",c.at("last_instruction")}}.dump());
        const auto actual=state.save();const auto &expected=c.at("after_abi");
        check(actual.gp==expected.at("gp").get<std::array<std::uint32_t,8>>(),name+" complete GP ABI differs "+difference(J(actual.gp),expected.at("gp")));
        check((actual.eflags&0xcd5u)==(expected.at("eflags").get<unsigned>()&0xcd5u),name+" flags/DF differ native="+std::to_string(actual.eflags)+" original="+expected.at("eflags").dump());
        check(hex(std::as_bytes(std::span(actual.xmm)))==expected.at("xmm").get<std::string>(),name+" complete XMM ABI differs");
        check(J::array({actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr})==expected.at("fp"),name+" FP ABI differs");
        check(pc==c.at("last_instruction"),name+" return cursor differs");
        auto expected_entries=c.at("entry_abis");for(auto &e:expected_entries)e["abi"]["eflags"]=e["abi"]["eflags"].get<unsigned>()&0xcd5u;
        check(entry_abis==expected_entries,name+" complete callee-entry ABI differs "+difference(entry_abis,expected_entries));
        if(read_pass) {
            J expected_reads=J::array();for(const auto &r:c.at("reads"))expected_reads.push_back(J::array({r.at("address"),r.at("bytes")}));
            if(memory.reads!=expected_reads){unsigned i{};while(i<memory.reads.size()&&i<expected_reads.size()&&memory.reads[i]==expected_reads[i])++i;throw std::runtime_error(name+" read order differs at "+std::to_string(i)+" native="+(i<memory.reads.size()?memory.reads[i].dump():"end")+" original="+(i<expected_reads.size()?expected_reads[i].dump():"end"));}reads+=memory.reads.size();
        }else {check(actual_stores==c.at("writes"),name+" exact ordered stores differ");stores+=actual_stores.size();}
        for(const auto &b:c.at("banks"))check(memory.banks.at(b.at("address"))==file(path.parent_path()/b.at("after_file").get<std::string>()),name+" complete pool/stack bank differs");
        if(!read_pass)++cases;
    }
    const bool reference=std::getenv("HALO2_REFERENCE_POOL_ALLOCATOR");check(reference?translated_fallbacks>=2u*cases:translated_fallbacks==0,"Unexpected original pool body fallback");
    J actual_owners=J::object();for(const auto &[entry,count]:native_allocator_owner_detail::activity().entries)actual_owners[std::to_string(entry)]=count;
    const auto &raw=native_raw_allocator_detail::activity();
    std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",reads},{"ordered_stores",stores},{"translated_body_fallbacks",translated_fallbacks},{"native_pool_releases",native_pool_allocator_detail::activity().releases},{"native_pool_allocations",native_pool_allocator_detail::activity().allocations},{"native_heap_release_wrappers",native_main_heap_detail::activity().release_wrappers},{"native_heap_ledger_releases",native_main_heap_detail::activity().ledger_releases},{"native_heap_sized_wrappers",native_main_heap_allocation_detail::activity().sized_wrappers},{"native_heap_allocations",native_main_heap_allocation_detail::activity().allocations},{"native_heap_gap_probes",native_main_heap_allocation_detail::activity().gap_probes},{"native_raw_allocations",raw.allocations},{"native_raw_releases",raw.releases},{"native_tagged_allocations",raw.tagged_allocations},{"native_tagged_releases",raw.tagged_releases},{"native_raw_default_entries",raw.default_heaps},{"actual_owner_entries",actual_owners},{"crt_import_tails",native_allocator_owner_detail::activity().crt_import_tails}}.dump()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

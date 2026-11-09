#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_physical_epoch_abi.hpp"
#ifdef HALO2_EPOCH_WITH_OWNER_DISPATCH
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#endif
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <nlohmann/json.hpp>

namespace halo2 {
using namespace physics_world_runtime_detail;
inline std::map<std::uint32_t,unsigned> translated_fallbacks;inline std::uint32_t required_entry{};
struct RequiredOwner{};
inline std::function<void(WorldState&,std::uint32_t)> child_entry_observer;
void dispatch(WorldState &,std::uint32_t target,std::uint32_t,bool);
bool run_epoch(WorldState &s,std::uint32_t entry){
    const auto child=[](auto &v,std::uint32_t owner,std::uint32_t site,bool tail){dispatch(v,owner,site,tail);};
#ifdef HALO2_EPOCH_WITH_OWNER_DISPATCH
    return native_allocator_owner_detail::dispatch(s,entry,child);
#else
    return native_physical_epoch_detail::run(s,entry,child);
#endif
}
namespace production {
#include "physical_epoch_body.inc"
}
namespace reference_owner {
#include "physical_epoch_reference.inc"
}
void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){
#include "physical_epoch_dispatch.inc"
}
}
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte>out;for(unsigned n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);check(bool(f),"Missing owned physical-epoch bank");std::vector<char>raw((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b(raw.size());for(unsigned n=0;n<raw.size();++n)b[n]=std::byte(std::uint8_t(raw[n]));return b;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;mutable J reads=J::array();bool logging{};
    auto at(std::uint32_t p,std::size_t size)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked physical operand");--i;check(std::uint64_t(p-i->first)+size<=i->second.size(),"Unbacked physical extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>out)const override{if(logging)reads.push_back(J::array({p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte>in)override{const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);}
};
struct FaultMemory:Memory {
    mutable unsigned operations{};unsigned fail_at{};
    mutable std::vector<std::tuple<char,std::uint32_t,std::size_t>> accesses;
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        accesses.emplace_back('r',p,out.size());if(++operations==fail_at)throw std::runtime_error("injected epoch read fault");Memory::read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override{
        accesses.emplace_back('w',p,in.size());if(++operations==fail_at)throw std::runtime_error("injected epoch write fault");Memory::write(p,in);
    }
};
void canonical(std::uint32_t entry,WorldState &s){switch(entry){case 0x12f040u:reference_owner::run_12f040(s);return;
    case 0x12f0c0u:reference_owner::run_12f0c0(s);return;default:reference_owner::run_13fd10(s);return;}}
struct FaultStats {unsigned cases{},prefixes{},native_callbacks{},observer_free_cases{};};
FaultStats faults(const J &fixture,const std::filesystem::path &path){
    FaultStats stats;child_entry_observer={};
    for(const auto &c:fixture.at("cases")){
        const auto name=c.at("name").get<std::string>();
        if(!(name.ends_with("_0")&&(name.starts_with("declared_records_2_")||name.starts_with("declared_records_4_")||
            name.starts_with("declared_records_5_")||name.starts_with("saved_"))))continue;
        FaultMemory input;
        for(const auto &bank:c.at("banks")){
            const auto address=bank.at("address").get<std::uint32_t>();
            if(address==0x2000000u||address==0x4000000u||address==0x4f2d24u||address==0x4f2d34u||address==0x4f2d58u)
                input.banks.emplace(address,file(path.parent_path()/bank.at("before_file").get<std::string>()));
        }
        SourcePhysicsWorldRuntimeAbi initial{};const auto &before=c.at("before_abi");
        initial.gp=before.at("gp").get<std::array<std::uint32_t,8>>();initial.eflags=before.at("eflags");
        const auto xmm=unhex(before.at("xmm"));std::memcpy(initial.xmm.data(),xmm.data(),128);
        initial.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("fp")[0],before.at("fp")[2]};
        initial.x87_status=3u<<11u;initial.x87_depth=2;initial.x87[0]=1.25L;initial.x87[1]=-2.5L;
        for(bool observed:{false,true}){
        auto active_initial=initial;if(!observed)active_initial.eflags^=0x8d5u;
        unsigned limit{};
        for(unsigned fail=0;;++fail){
            FaultMemory expected=input,actual=input;expected.fail_at=actual.fail_at=fail;
            std::vector<SourcePhysicsContactRecordStore> es,ns;
            SourcePhysicsContactRecordObserver eo,no;if(observed){eo=[&](const auto &e){es.push_back(e);};no=[&](const auto &e){ns.push_back(e);};}
            SourcePhysicsWorldRuntimeServices er{},nr{};std::uint32_t epc{},npc{};
            er.instruction=[&](auto pc){epc=pc;};nr.instruction=[&](auto pc){npc=pc;++stats.native_callbacks;};
            const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};auto &cursor=nr.instruction_cursor;
            cursor.position=&npc;cursor.callback_type=&nr.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;
            cursor.observation_only_stores=true;cursor.service_sites=sites;
            WorldState x{expected,eo,er,SourcePhysicsContactImage::TitleUpdate15},y{actual,no,nr,SourcePhysicsContactImage::TitleUpdate15};x.load(active_initial);y.load(active_initial);
            std::string ee,ne;try{canonical(c.at("entry"),x);}catch(const std::exception &e){ee=e.what();}const auto ex=x.save();
            try{dispatch(y,c.at("entry"),0,false);}catch(const std::exception &e){ne=e.what();}const auto na=y.save();_mm_setcsr(0x1f80u);
            bool same=ee==ne&&expected.banks==actual.banks&&expected.accesses==actual.accesses&&expected.operations==actual.operations&&es==ns&&
                ex.gp==na.gp&&ex.eflags==na.eflags&&ex.xmm==na.xmm&&epc==npc&&ex.x87_depth==na.x87_depth&&ex.x87_status==na.x87_status&&
                ex.floating_point->x87_control_word==na.floating_point->x87_control_word&&ex.floating_point->mxcsr==na.floating_point->mxcsr;
            for(unsigned n=0;n<ex.x87_depth;++n)same=same&&!std::memcmp(&ex.x87[n],&na.x87[n],10);
            if(!same){std::cerr<<name<<" fault="<<fail<<" pc="<<std::hex<<epc<<'/'<<npc<<" gp="<<(ex.gp==na.gp)<<" flags="<<ex.eflags<<'/'<<na.eflags
                <<" operations="<<std::dec<<expected.operations<<'/'<<actual.operations<<" errors="<<ee<<'/'<<ne<<'\n';
                for(unsigned n=0;n<8;++n)if(ex.gp[n]!=na.gp[n])std::cerr<<"gp"<<n<<' '<<std::hex<<ex.gp[n]<<'/'<<na.gp[n]<<'\n';
                throw std::runtime_error("Physical epoch retained memory-fault prefix differs");}
            if(!fail){++stats.cases;if(!observed)++stats.observer_free_cases;limit=expected.operations;}else++stats.prefixes;
            if(fail>=limit)break;
        }
        }
    }
    return stats;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply physical-epoch fixture");const std::filesystem::path path=argv[1];std::ifstream input(path);const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Physical original pin differs");
    unsigned cases{},reads{},stores{},entries{};
    for(const auto &c:fixture.at("cases"))for(bool read_pass:{false,true}) {
        const auto name=c.at("name").get<std::string>();Memory memory;memory.logging=read_pass;
        for(const auto &bank:c.at("banks"))memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>()));
        J written=J::array();SourcePhysicsContactRecordObserver observer;
        if(!read_pass)observer=[&](const auto &e){written.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};services.instruction=[&](auto value){
            pc=value;for(const auto &site:c.value("stop_before_sites",J::array()))if(site==pc){required_entry=pc;throw RequiredOwner{};}
        };
        const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};auto &cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.service_sites=sites;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
        const auto &before=c.at("before_abi");abi.gp=before.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=before.at("eflags");
        const auto xmm=unhex(before.at("xmm"));std::memcpy(abi.xmm.data(),xmm.data(),128);abi.x87_status=before.at("fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("fp")[0],before.at("fp")[2]};state.load(abi);
        J observed_entries=J::array();child_entry_observer=[&](auto &value,auto owner){
            const auto a=value.save();observed_entries.push_back(J{{"entry",owner},{"abi",J{{"gp",a.gp},{"eflags",a.eflags&0xcd5u},
                {"xmm",hex(std::as_bytes(std::span(a.xmm)))},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})}}}});
        };
        bool required{};required_entry=0;try{dispatch(state,c.at("entry"),0,false);}catch(const RequiredOwner&){required=true;}
        check(required==c.at("required_owner")&&required_entry==c.at("required_entry"),name+" boundary differs");
        auto expected_entries=c.at("entry_abis");for(auto &entry:expected_entries)entry["abi"]["eflags"]=entry["abi"]["eflags"].get<unsigned>()&0xcd5u;
        check(observed_entries==expected_entries,name+" full child-entry ABI differs");
        const auto after=state.save();const auto &expected=c.at("after_abi");
        check(after.gp==expected.at("gp").get<std::array<std::uint32_t,8>>(),name+" complete GP differs");
        check((after.eflags&0xcd5u)==(expected.at("eflags").get<unsigned>()&0xcd5u),name+" flags/DF differ");
        check(hex(std::as_bytes(std::span(after.xmm)))==expected.at("xmm").get<std::string>(),name+" XMM differs");
        check(J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==expected.at("fp"),name+" FP differs");
        check(pc==c.at("last_instruction"),name+" published cursor differs");
        if(read_pass){
            J expected_reads=J::array();for(const auto &r:c.at("reads"))expected_reads.push_back(J::array({r.at("address"),r.at("bytes")}));
            if(memory.reads!=expected_reads){
                unsigned index{};while(index<memory.reads.size()&&index<expected_reads.size()&&memory.reads[index]==expected_reads[index])++index;
                throw std::runtime_error(name+" ordered reads differ at"+std::to_string(index)+" native="+
                    (index<memory.reads.size()?memory.reads[index].dump():"end")+" original="+
                    (index<expected_reads.size()?c.at("reads")[index].dump():"end"));
            }
            reads+=memory.reads.size();
        }
        else{
            if(written!=c.at("writes")){
                unsigned index{};while(index<written.size()&&index<c.at("writes").size()&&written[index]==c.at("writes")[index])++index;
                throw std::runtime_error(name+" ordered stores differ at"+std::to_string(index)+" native="+
                    (index<written.size()?written[index].dump():"end")+" original="+
                    (index<c.at("writes").size()?c.at("writes")[index].dump():"end"));
            }
            stores+=written.size();entries+=observed_entries.size();++cases;
        }
        for(const auto &bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),name+" whole bank differs");
    }
    const auto fault=faults(fixture,path);const bool reference=std::getenv("HALO2_REFERENCE_PHYSICAL_EPOCHS");
    check(reference?!translated_fallbacks.empty():translated_fallbacks.empty(),"Unexpected epoch translated fallback");
    check(reference||fault.native_callbacks==0,"Epoch fault gate fell back to translated instructions");
    unsigned fallback_count{};for(const auto &[entry,count]:translated_fallbacks)fallback_count+=count;
    J owner_entries=nullptr;
#ifdef HALO2_EPOCH_WITH_OWNER_DISPATCH
    const auto &native=native_physical_epoch_detail::activity().kernel;const auto &activity=native_allocator_owner_detail::activity();
    const std::array<std::pair<std::uint32_t,std::uint64_t>,3> counts{{{0x12f040u,native.object_resets},{0x12f0c0u,native.ticks},{0x13fd10u,native.page_rebases}}};
    for(const auto &[entry,count]:counts){const auto found=activity.epoch_entries.find(entry);check((found==activity.epoch_entries.end()?0:found->second)==count,"Epoch actual owner activity differs");}
    check(activity.entries.empty()&&activity.tick_entries.empty(),"Epoch proof executed a raw allocator or tick-conversion owner");owner_entries=activity.epoch_entries;
#endif
    std::cout<<J{{"passed",true},{"cases",cases},{"ordered_reads",reads},{"ordered_stores",stores},{"callee_entry_packets",entries},{"translated_body_fallbacks",fallback_count},{"translated_entries",translated_fallbacks},
        {"actual_owner_activity",owner_entries},
        {"retained_faults",J{{"normal_cases",fault.cases},{"observer_free_cases",fault.observer_free_cases},{"prefixes",fault.prefixes},{"native_instruction_callbacks",fault.native_callbacks}}}}.dump()<<'\n';return 0;

}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

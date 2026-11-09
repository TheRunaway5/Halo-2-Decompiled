#include "native_tu15_allocator_abi_fixture.hpp"
#include "physics/runtime/native_tu15_page_window_abi.hpp"
#include "halo2/native_tu15_page_allocator.hpp"
#include <iostream>
#include <set>

namespace halo2 {
using namespace physics_world_runtime_detail;
inline unsigned page_window_fallbacks{};inline bool page_window_source_route{};
namespace reference_owner {
#include "page_window_reference.inc"
}
#ifdef HALO2_PAGE_WINDOW_SOURCE_ROUTE
namespace production_source {
#include "page_window_source.inc"
}
#endif
}
namespace {
using namespace native_allocator_fixture;using halo2::physics_world_runtime_detail::WorldState;
auto &fallbacks=page_window_fallbacks;unsigned native_callbacks{},normal_callbacks{};
template<class Before>void execute(WorldState &s,Before before){
#ifdef HALO2_PAGE_WINDOW_SOURCE_ROUTE
    if(page_window_source_route){production_source::run_13fe00(s);return;}
#endif
    if(!native_page_window_detail::run(s,before)){++fallbacks;reference_owner::run_13fe00(s);}
}
struct FaultStats{unsigned cases{},memory_prefixes{},observer_prefixes{};};
FaultStats faults(const J &fixture,const std::filesystem::path &directory){
    FaultStats stats;
    for(const auto &c:fixture.at("cases")){
        if(!c.value("fault_case",false))continue;
        auto initial=abi(c);initial.x87_depth=2;initial.x87_status=3u<<11u;initial.x87[0]=1.25L;initial.x87[1]=-2.5L;
        const auto input=load(c,directory);
        for(unsigned protocol=0;protocol<3;++protocol){unsigned limit{};
            for(unsigned fail=0;;++fail){Memory expected=input,actual=input;if(protocol<2)expected.fail_at=actual.fail_at=fail;
                std::vector<SourcePhysicsContactRecordStore> es,ns;SourcePhysicsContactRecordObserver eo,no;
                if(protocol){eo=[&](const auto &e){es.push_back(e);if(protocol==2&&es.size()==fail)throw std::runtime_error("injected window observer fault");};
                    no=[&](const auto &e){ns.push_back(e);if(protocol==2&&ns.size()==fail)throw std::runtime_error("injected window observer fault");};}
                SourcePhysicsWorldRuntimeServices er{},nr{};std::uint32_t epc{},npc{};er.instruction=[&](auto pc){epc=pc;};nr.instruction=[&](auto pc){npc=pc;++native_callbacks;};cursor(nr,npc);
                WorldState x{expected,eo,er,SourcePhysicsContactImage::TitleUpdate15},y{actual,no,nr,SourcePhysicsContactImage::TitleUpdate15};x.load(initial);y.load(initial);
                std::string ee,ne;try{reference_owner::run_13fe00(x);}catch(const std::exception &e){ee=e.what();}const auto ex=x.save();
                try{execute(y,native_page_window_detail::Noop{});}catch(const std::exception &e){ne=e.what();}const auto na=y.save();_mm_setcsr(0x1f80u);
                if(ee!=ne||!same_abi(ex,na)||expected.banks!=actual.banks||expected.accesses!=actual.accesses||es!=ns||epc!=npc){
                    std::cerr<<c.at("name")<<" protocol="<<protocol<<" prefix="<<fail<<" pc="<<std::hex<<epc<<'/'<<npc<<" flags="<<ex.eflags<<'/'<<na.eflags<<'\n';throw std::runtime_error("Page window retained prefix differs");}
                if(!fail){++stats.cases;limit=protocol==2?es.size():expected.operations;}else if(protocol==2)++stats.observer_prefixes;else++stats.memory_prefixes;
                if(fail>=limit)break;
            }
        }
    }
    return stats;
}
}
int main(int argc,char **argv)try{
    check(argc==2||argc==3,"Supply page-window fixture and optional source route");page_window_source_route=argc==3;const bool reference=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATOR");const std::filesystem::path path=argv[1];std::ifstream stream(path);const auto fixture=J::parse(stream);unsigned cases{},reads{},stores{},packets{},legacy_cases{};
    for(const auto &c:fixture.at("cases"))for(bool reading:{false,true}){
        auto memory=load(c,path.parent_path());memory.logging=reading;const auto initial=abi(c);J written=J::array(),captured=J::array();SourcePhysicsContactRecordObserver observer;
        if(!reading)observer=[&](const auto &e){written.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};WorldState *current{};
        const auto before=[&](WorldState &s,auto site){captured.push_back(packet(s,site));};
        services.instruction=[&](auto site){pc=site;++normal_callbacks;if(std::find(c.at("abi_entries").begin(),c.at("abi_entries").end(),site)!=c.at("abi_entries").end())before(*current,site);};cursor(services,pc);
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;state.load(initial);execute(state,before);const auto after=state.save();
        auto entries=c.at("entry_abis");for(auto &entry:entries)entry["abi"]["eflags"]=entry["abi"]["eflags"].get<unsigned>()&0xcd5u;
        if(page_window_source_route&&!reference)check(captured.empty(),"Production page-window prefix invoked instruction callbacks");
        else if(captured!=entries){unsigned n{};while(n<captured.size()&&n<entries.size()&&captured[n]==entries[n])++n;throw std::runtime_error(c.at("name").get<std::string>()+" physical pre-I/O packet differs at"+std::to_string(n)+" native="+(n<captured.size()?captured[n].dump():"end")+" original="+(n<entries.size()?entries[n].dump():"end"));}
        const auto &expected=c.at("after_abi");check(after.gp==expected.at("gp").get<std::array<std::uint32_t,8>>()&&(after.eflags&0xcd5u)==(expected.at("eflags").get<unsigned>()&0xcd5u),"Page window normal GP/flags differ");
        check(hex(std::as_bytes(std::span(after.xmm)))==expected.at("xmm").get<std::string>()&&J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==expected.at("fp"),"Page window FP/XMM differs");check(pc==c.at("last_instruction"),"Page window final cursor differs");
        if(reading){J er=J::array();for(const auto &r:c.at("reads"))er.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==er,"Page window ordered reads differ");reads+=memory.reads.size();}
        else{check(written==c.at("writes"),"Page window ordered stores differ");stores+=written.size();packets+=captured.size();++cases;}
        for(const auto &bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),"Page window full bank differs");
        if(!reading){auto legacy=load(c,path.parent_path());auto legacy_abi=initial;native_tu15_page_window_prefer_abi(legacy,legacy_abi);check(legacy_abi.gp==after.gp&&legacy.banks==memory.banks&&legacy_abi.eflags==initial.eflags,"Shared page-window scratch ABI changed");++legacy_cases;}
    }
    const auto f=faults(fixture,path.parent_path());check(reference?fallbacks==2u*cases+f.cases+f.memory_prefixes+f.observer_prefixes:fallbacks==0,"Page window fallback differs");check(reference||(!native_callbacks&&!normal_callbacks),"Page window invoked retained instruction callbacks");
    std::cout<<J{{"passed",true},{"cases",cases},{"physical_pre_io_packets",packets},{"normal_instruction_callbacks",normal_callbacks},{"ordered_reads",reads},{"ordered_stores",stores},{"legacy_scratch_compatibility",legacy_cases},{"translated_fallbacks",fallbacks},{"faults",J{{"normal",f.cases},{"memory_prefixes",f.memory_prefixes},{"observer_prefixes",f.observer_prefixes},{"native_callbacks",native_callbacks}}}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

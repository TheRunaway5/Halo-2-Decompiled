#include "native_tu15_allocator_abi_fixture.hpp"
#include "physics/runtime/native_tu15_page_retire_abi.hpp"
#include "halo2/native_tu15_page_allocator.hpp"
#include "halo2/native_campaign_ai_resource_retirement_tu15.hpp"
#include <iostream>
#include <set>

namespace halo2 {
using namespace physics_world_runtime_detail;
inline unsigned parent_fallbacks{};inline std::map<std::uint32_t,unsigned> child_fallbacks;
inline std::set<std::uint32_t> declared_stops;inline std::uint32_t required_entry{};
struct RequiredOwner{};
inline std::function<void(WorldState&,std::uint32_t)> child_entry;
inline std::string source_route;
void publish_entry(WorldState &s,std::uint32_t target){if(child_entry)child_entry(s,target);if(declared_stops.contains(target)){required_entry=target;throw RequiredOwner{};}}
void canonical_dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);
void production_dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);
namespace canonical {
#include "page_retire_reference.inc"
}
namespace production {
#include "page_retire_production.inc"
}
#ifdef HALO2_PAGE_RETIRE_SOURCE_ROUTES
#include "page_retire_source_routes.inc"
#endif
void canonical_dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){switch(target){case 0x12fd10u:canonical::run_12fd10(s);return;case 0x16e8a0u:canonical::run_16e8a0(s);return;default:required_entry=target;throw RequiredOwner{};}}
void production_dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){switch(target){case 0x12fd10u:production::run_12fd10(s);return;case 0x16e8a0u:production::run_16e8a0(s);return;default:required_entry=target;throw RequiredOwner{};}}
}
namespace {
using namespace native_allocator_fixture;using halo2::physics_world_runtime_detail::WorldState;
unsigned parent_callbacks{},datum_callbacks{},residual_callback_sites{},duplicate_entries{};
template<class Before>void execute(WorldState &s,Before before){
#ifdef HALO2_PAGE_RETIRE_SOURCE_ROUTES
    if(source_route=="primary"){source_primary::run_140310(s);return;}
    if(source_route=="combat"){source_combat::run_140310(s);return;}
#endif
    if(!native_page_retire_detail::run(s,[](auto &v,auto target,auto site,auto tail){production_dispatch(v,target,site,tail);},before))production::run_140310(s);
}
struct FaultStats{unsigned normal{},memory_prefixes{},observer_prefixes{};};
FaultStats faults(const J &fixture,const std::filesystem::path &directory){
    FaultStats stats;child_entry={};declared_stops.clear();
    for(const auto &c:fixture.at("cases")){
        if(!c.value("fault_case",false))continue;
        if(!source_route.empty()&&!c.value("source_fault_case",false))continue;
        const auto input=load(c,directory);auto initial=abi(c);initial.x87_depth=2;initial.x87_status=3u<<11u;initial.x87[0]=1.25L;initial.x87[1]=-2.5L;
        for(unsigned protocol=0;protocol<3;++protocol){unsigned limit{};
            for(unsigned fail=0;;++fail){Memory x=input,y=input;if(protocol<2)x.fail_at=y.fail_at=fail;
                std::vector<SourcePhysicsContactRecordStore> es,ns;SourcePhysicsContactRecordObserver eo,no;
                if(protocol){eo=[&](const auto &e){es.push_back(e);if(protocol==2&&es.size()==fail)throw std::runtime_error("injected retirement observer fault");};no=[&](const auto &e){ns.push_back(e);if(protocol==2&&ns.size()==fail)throw std::runtime_error("injected retirement observer fault");};}
                SourcePhysicsWorldRuntimeServices er{},nr{};std::uint32_t epc{},npc{};er.instruction=[&](auto pc){epc=pc;};nr.instruction=[&](auto pc){npc=pc;if(pc>=0x140310u&&pc<0x14038cu)++parent_callbacks;if(pc>=0x16e8a0u&&pc<=0x16e931u)++datum_callbacks;};cursor(nr,npc);
                WorldState expected{x,eo,er,SourcePhysicsContactImage::TitleUpdate15},actual{y,no,nr,SourcePhysicsContactImage::TitleUpdate15};expected.load(initial);actual.load(initial);
                std::string ee,ne;try{canonical::run_140310(expected);}catch(const std::exception &e){ee=e.what();}const auto ex=expected.save();
                try{execute(actual,native_page_window_detail::Noop{});}catch(const std::exception &e){ne=e.what();}const auto na=actual.save();_mm_setcsr(0x1f80u);
                if(ee!=ne||!same_abi(ex,na)||x.banks!=y.banks||x.accesses!=y.accesses||es!=ns||epc!=npc){
                    std::cerr<<c.at("name")<<" protocol="<<protocol<<" prefix="<<fail<<" pc="<<std::hex<<epc<<'/'<<npc<<" flags="<<ex.eflags<<'/'<<na.eflags<<'\n';
                    for(unsigned n=0;n<8;++n)if(ex.gp[n]!=na.gp[n])std::cerr<<"gp"<<n<<' '<<ex.gp[n]<<'/'<<na.gp[n]<<'\n';throw std::runtime_error("Page retirement retained prefix differs");}
                if(!fail){++stats.normal;limit=protocol==2?es.size():x.operations;}else if(protocol==2)++stats.observer_prefixes;else++stats.memory_prefixes;
                if(fail>=limit)break;
            }
        }
    }
    return stats;
}
}
int main(int argc,char **argv)try{
    check(argc==2||argc==3,"Supply page retirement fixture and optional source route");if(argc==3)source_route=argv[2];const std::filesystem::path path=argv[1];std::ifstream input(path);const auto fixture=J::parse(input);
    const bool parent_reference=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATOR"),datum_reference=std::getenv("HALO2_REFERENCE_DATUM_DELETE");unsigned cases{},boundaries{},packets{},reads{},stores{},legacy_cases{},semantic_cases{},legacy_callback_entries{},diagnostic_callbacks{};
    for(const auto &c:fixture.at("cases"))for(bool reading:{false,true}){
        auto memory=load(c,path.parent_path());memory.logging=reading;const auto initial=abi(c);J written=J::array(),captured=J::array();bool pending_tail{};SourcePhysicsContactRecordObserver observer;
        if(!reading)observer=[&](const auto &e){written.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        declared_stops.clear();for(const auto &p:c.value("required_entries",J::array()))declared_stops.insert(p.get<unsigned>());required_entry=0;
        const auto stop=[&](auto pc){if(declared_stops.contains(pc)){required_entry=pc;throw RequiredOwner{};}};
        const auto before=[&](WorldState &s,auto pc){captured.push_back(packet(s,pc));if(pc==0x16e8a0u)pending_tail=true;stop(pc);};
        child_entry=[&](WorldState &s,auto pc){const auto current=packet(s,pc);if(pc==0x16e8a0u&&pending_tail){check(!captured.empty()&&captured.back()==current,"Refused child changed tail-entry ABI");pending_tail=false;++duplicate_entries;}else captured.push_back(current);};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};WorldState *current{};const bool diagnostic=!source_route.empty()&&!declared_stops.empty();const auto prior_parent_callbacks=parent_callbacks;
        services.instruction=[&](auto site){pc=site;if(site>=0x140310u&&site<0x14038cu){++parent_callbacks;if(std::find(c.at("parent_boundaries").begin(),c.at("parent_boundaries").end(),site)!=c.at("parent_boundaries").end())before(*current,site);}else if(site>=0x16e8a0u&&site<=0x16e931u)++datum_callbacks;else if(site>=0x12fd10u&&site<0x12fdfeu)++residual_callback_sites;};cursor(services,pc);
        if(diagnostic)services.instruction_cursor.first_trace=&diagnostic;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;state.load(initial);bool required{};try{execute(state,before);}catch(const RequiredOwner&){required=true;}if(diagnostic)diagnostic_callbacks+=parent_callbacks-prior_parent_callbacks;
        check(required==c.at("required_owner")&&required_entry==c.at("required_entry"),"Retirement actual owner boundary differs");
        auto expected_entries=c.at("entry_abis");for(auto &entry:expected_entries)entry["abi"]["eflags"]=entry["abi"]["eflags"].get<unsigned>()&0xcd5u;
        if(!source_route.empty()&&!parent_reference&&!diagnostic){
            unsigned n{};for(const auto &observed:captured){while(n<expected_entries.size()&&expected_entries[n].at("entry")!=observed.at("entry"))++n;check(n<expected_entries.size()&&observed==expected_entries[n],"Source-prefix callback entry differs");++n;}
        }else if(captured!=expected_entries){unsigned n{};while(n<captured.size()&&n<expected_entries.size()&&captured[n]==expected_entries[n])++n;throw std::runtime_error(c.at("name").get<std::string>()+" pre-I/O/callee entry differs at"+std::to_string(n)+" native="+(n<captured.size()?captured[n].dump():"end")+" original="+(n<expected_entries.size()?expected_entries[n].dump():"end"));}
        const auto after=state.save();const auto &expected=c.at("after_abi");check(after.gp==expected.at("gp").get<std::array<std::uint32_t,8>>()&&(after.eflags&0xcd5u)==(expected.at("eflags").get<unsigned>()&0xcd5u),"Retirement full return GP/flags differ");
        check(hex(std::as_bytes(std::span(after.xmm)))==expected.at("xmm").get<std::string>()&&J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==expected.at("fp"),"Retirement FP/XMM differs");check(pc==c.at("last_instruction"),c.at("name").get<std::string>()+" retirement final cursor native="+std::to_string(pc)+" original="+c.at("last_instruction").dump());
        if(reading){J er=J::array();for(const auto &r:c.at("reads"))er.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==er,"Retirement ordered reads differ");reads+=memory.reads.size();}
        else{check(written==c.at("writes"),"Retirement ordered stores differ");stores+=written.size();packets+=captured.size();++cases;boundaries+=required;}
        for(const auto &bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),"Retirement full bank differs");
        if(!reading&&!required&&c.value("scalar_compatible",false)&&!(initial.eflags&1024u)){
            child_entry={};auto semantic=load(c,path.parent_path());native_campaign_ai_resource_metadata_retire_tu15(semantic,initial.gp[7],initial.gp[3]);
            for(const auto &bank:c.at("banks"))if(bank.at("address")!=0x2000000u)check(semantic.banks.at(bank.at("address"))==memory.banks.at(bank.at("address")),"Shared semantic retirement bank differs");++semantic_cases;
            auto legacy=load(c,path.parent_path());auto a=initial;SourcePhysicsWorldRuntimeServices child_services;const auto previous_callbacks=child_fallbacks[0x12fd10u];
            SourcePhysicsContactRuntimeServices owners;owners.required_owner=[&](auto &,const auto &call,auto &value,const auto &observe){WorldState s{legacy,observe,child_services,SourcePhysicsContactImage::TitleUpdate15};s.load(value);production_dispatch(s,call.target,call.instruction,call.tail);value=s.save();};
            native_tu15_page_delete_abi(legacy,a,owners);check(a.gp==after.gp&&legacy.banks==memory.banks,"Shared legacy retirement scratch GP/banks differ");++legacy_cases;legacy_callback_entries+=child_fallbacks[0x12fd10u]-previous_callbacks;
        }
    }
    const auto physical_callback_entries=child_fallbacks[0x12fd10u]-legacy_callback_entries;
    const auto f=faults(fixture,path.parent_path());check(parent_reference?parent_fallbacks==2u*cases+f.normal+f.memory_prefixes+f.observer_prefixes:parent_fallbacks==(source_route.empty()?0u:2u*boundaries),"Retirement parent fallback differs");
    check(parent_reference||parent_callbacks==diagnostic_callbacks,"Native retirement ran parent instruction callbacks");check(datum_reference||!datum_callbacks,"Native datum closure ran instruction callbacks");
    std::cout<<J{{"passed",true},{"cases",cases},{"actual_owner_boundaries",boundaries},{"pre_io_and_entry_packets",packets},{"ordered_reads",reads},{"ordered_stores",stores},{"parent_fallbacks",parent_fallbacks},{"datum_fallbacks",child_fallbacks[0x16e8a0u]},{"physical_cohort_real_callback_entries",physical_callback_entries},{"legacy_real_callback_entries",legacy_callback_entries},{"retained_fault_real_callback_entries",child_fallbacks[0x12fd10u]-physical_callback_entries-legacy_callback_entries},{"physical_cohort_real_callback_instruction_sites",residual_callback_sites},{"semantic_shared_data_cases",semantic_cases},{"legacy_scratch_data_cases",legacy_cases},{"duplicate_entry_consistency_checks",duplicate_entries},{"faults",J{{"normal",f.normal},{"memory_prefixes",f.memory_prefixes},{"observer_prefixes",f.observer_prefixes}}}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

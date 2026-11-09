#include "native_tu15_allocator_abi_fixture.hpp"
#include "physics/runtime/native_tu15_page_allocate_abi.hpp"
#include <iostream>
#include <set>

namespace halo2 {
using namespace physics_world_runtime_detail;
inline std::set<std::uint32_t> stops;
inline std::uint32_t required_entry{};inline unsigned source_parent_fallbacks{};inline std::string source_route;
struct RequiredOwner{};
inline std::function<void(WorldState&,std::uint32_t)> publish_entry;
void page_reference_dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);
namespace page_allocate_reference {
#include "page_allocate_reference.inc"
}
#ifdef HALO2_PAGE_ALLOCATE_COPY_ROUTES
#include "page_allocate_copy_routes.inc"
#endif
void page_reference_dispatch(WorldState &s,std::uint32_t owner,std::uint32_t,bool){
    switch(owner){
    case 0x13fe50u:page_allocate_reference::run_13fe50(s);return;
    case 0x32a660u:page_allocate_reference::run_32a660(s);return;
    case 0x13fe00u:page_allocate_reference::run_13fe00(s);return;
    case 0x140310u:page_allocate_reference::run_140310(s);return;
    case 0x16e6a0u:page_allocate_reference::run_16e6a0(s);return;
    case 0x16e8a0u:page_allocate_reference::run_16e8a0(s);return;
    case 0x12fcb0u:page_allocate_reference::run_12fcb0(s);return;
    default:if(publish_entry)publish_entry(s,owner);required_entry=owner;throw RequiredOwner{};
    }
}
}
namespace {
using namespace native_allocator_fixture;
SourcePhysicsWorldRuntimeAbi page_abi(const J &c){auto value=abi(c);const auto &live=c.at("before_abi").at("x87");value.x87_depth=live.size();for(unsigned n=0;n<live.size();++n){const auto raw=unhex(live[n]);check(raw.size()==10,"Invalid extended fixture input");std::memcpy(&value.x87[n],raw.data(),10);}return value;}
J page_live(const SourcePhysicsWorldRuntimeAbi &value){J result=J::array();for(unsigned n=0;n<value.x87_depth;++n)result.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&value.x87[n]),10)));return result;}
J page_packet(const physics_world_runtime_detail::WorldState &s,std::uint32_t pc){auto result=packet(s,pc);result["abi"]["x87"]=page_live(s.save());return result;}
unsigned page_admission(const J &c,const std::filesystem::path &directory){
    using physics_world_runtime_detail::WorldState;unsigned tested{};
    for(unsigned kind=0;kind<12;++kind){auto memory=load(c,directory);const auto banks=memory.banks;std::uint32_t pc=0x76543210u;unsigned callbacks{},calls{};
        SourcePhysicsContactRecordObserver observer;if(kind==8)observer=[](const auto&){};
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++callbacks;};cursor(services,pc);
        const bool tracing=true;const std::array<unsigned,3>wrong_sites{0x38846e,0x21f590,0x33b596};
        switch(kind){case 1:services.instruction_cursor.position=nullptr;break;case 2:services.instruction_cursor.executor={};break;
        case 3:services.instruction_cursor.callback_type=nullptr;break;case 4:services.instruction_cursor.callback_type=&typeid(int);break;
        case 5:services.instruction_cursor.literal_service_sites=false;break;case 6:services.instruction_cursor.first_trace=&tracing;break;
        case 7:services.instruction_cursor.second_trace=&tracing;break;case 8:services.instruction_cursor.observation_only_stores=false;break;
        case 9:services.instruction_cursor.service_sites=std::span(wrong_sites).first(2);break;case 10:services.instruction_cursor.service_sites=wrong_sites;break;
        case 11:services.instruction_cursor={};break;default:break;}
        WorldState state{memory,observer,services,kind==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};state.load(page_abi(c));const auto initial=state.save();
        const bool accepted=native_page_allocate_detail::run(state,[&](auto&,auto,auto,bool){++calls;});
        check(!accepted&&same_abi(initial,state.save())&&memory.banks==banks&&memory.accesses.empty()&&!callbacks&&!calls&&pc==0x76543210u,"Page admission modified or accepted a refused protocol");++tested;
    }
    return tested;
}
}
#include "native_tu15_page_allocate_faults.hpp"
int main(int argc,char **argv)try{
    using namespace native_allocator_fixture;check(argc==2||argc==3,"Supply page allocation ABI fixture");const bool canonical_diagnostic=argc==3&&std::string(argv[2])=="canonical-io-diagnostic";if(argc==3&&!canonical_diagnostic)source_route=argv[2];
    const std::filesystem::path path=argv[1];std::ifstream input(path);const auto fixture=J::parse(input);
    const bool parent_reference=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATE");
    const bool any_child_reference=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATOR")||std::getenv("HALO2_REFERENCE_RESOURCE_BUSY")||std::getenv("HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE")||std::getenv("HALO2_REFERENCE_DATUM_DELETE");
    const unsigned admission=(!parent_reference&&!any_child_reference)?page_admission(fixture.at("cases").at(0),path.parent_path()):0u;
    unsigned cases{},packets{},reads{},stores{},callbacks{},parent_callbacks{},diagnostic_callbacks{},boundaries{},duplicate_entries{},mutations{};
    struct FeedbackMemory:Memory {
        const J *input{};const std::uint32_t *position{};std::set<unsigned> applied;std::function<void(char)> before_access;
        void read(std::uint32_t address,std::span<std::byte> bytes)const override{if(before_access)before_access('r');Memory::read(address,bytes);}
        void write(std::uint32_t address,std::span<const std::byte> bytes)override{
            if(before_access)before_access('w');Memory::write(address,bytes);if(!input||!position)return;unsigned n{};
            for(const auto &event:input->value("mutations",J::array())){
                if(event.at("instruction")==*position&&applied.insert(n).second){const auto at=event.at("address").get<unsigned>();const auto value=unhex(event.at("written"));const auto bank=Memory::at(at,value.size());std::copy(value.begin(),value.end(),banks.at(bank->first).begin()+at-bank->first);}++n;
            }
        }
    };
    for(const auto &c:fixture.at("cases"))for(bool reading:{false,true}){
        FeedbackMemory memory;static_cast<Memory&>(memory)=load(c,path.parent_path());memory.input=&c;memory.logging=reading;J written=J::array(),entries=J::array();stops.clear();required_entry=0;
        for(const auto &p:c.value("required_entries",J::array()))stops.insert(p.get<unsigned>());
        const bool source_diagnostic=!source_route.empty()&&!stops.empty();const auto previous_parent_callbacks=parent_callbacks;
        SourcePhysicsContactRecordObserver observer;
        if(!reading)observer=[&](const auto &e){written.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
        WorldState *current{};std::uint32_t pc{};memory.position=&pc;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto site){pc=site;++callbacks;if(site>=0x13fe50u&&site<=0x14030du)++parent_callbacks;if(((!parent_reference&&!source_diagnostic)||canonical_diagnostic)&&std::find(c.at("parent_boundaries").begin(),c.at("parent_boundaries").end(),site)!=c.at("parent_boundaries").end()&&publish_entry)publish_entry(*current,site);};cursor(services,pc);if(source_diagnostic)services.instruction_cursor.first_trace=&source_diagnostic;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;state.load(page_abi(c));
        const auto before=[&](WorldState &s,auto owner){
            if(std::find(c.at("abi_entries").begin(),c.at("abi_entries").end(),owner)==c.at("abi_entries").end())return;
            const auto entry=page_packet(s,owner);const bool io=std::find(c.at("parent_boundaries").begin(),c.at("parent_boundaries").end(),owner)!=c.at("parent_boundaries").end();
            if(io||entries.empty()||entries.back()!=entry)entries.push_back(entry);else ++duplicate_entries;
            if(stops.contains(owner)){required_entry=owner;throw RequiredOwner{};}
        };
        publish_entry=before;
        unsigned pending_peek{};
        if(!source_route.empty()&&!parent_reference&&!source_diagnostic){memory.before_access=[&](char kind){
            if(std::find(c.at("parent_boundaries").begin(),c.at("parent_boundaries").end(),pc)==c.at("parent_boundaries").end())return;
            const bool write=std::find(c.at("parent_write_boundaries").begin(),c.at("parent_write_boundaries").end(),pc)!=c.at("parent_write_boundaries").end();
            if(!write){if(kind=='r')before(state,pc);}
            else if(kind=='r'){before(state,pc);pending_peek=pc;}
            else{if(pending_peek!=pc)before(state,pc);pending_peek=0;}
        };}
        before(state,0x13fe50u);bool stopped{};
        try{
#ifdef HALO2_PAGE_ALLOCATE_COPY_ROUTES
            if(source_route=="primary")source_primary::run_13fe50(state);
            else if(source_route=="shadow")source_shadow::run_13fe50(state);
            else if(source_route=="primary_specialized")source_primary_specialized::run_13fe50(state);
            else if(source_route=="shadow_specialized")source_shadow_specialized::run_13fe50(state);
            else
#endif
            if(!native_page_allocate_detail::run(state,[](auto &s,auto owner,auto site,auto tail){page_reference_dispatch(s,owner,site,tail);},before))page_allocate_reference::run_13fe50(state);}
        catch(const RequiredOwner&){stopped=true;}
        catch(const std::exception &e){
            std::string last;if(!memory.accesses.empty()){const auto [kind,address,bytes]=memory.accesses.back();last=std::string(1,kind)+"@"+std::to_string(address)+"/"+std::to_string(bytes);}
            throw std::runtime_error(c.at("name").get<std::string>()+" native execution failed pc="+std::to_string(pc)+" access="+last+" gp="+J(state.gp).dump()+" error="+e.what());
        }
        check(stopped==c.at("required_owner")&&required_entry==c.at("required_entry"),c.at("name").get<std::string>()+" genuine child entry stop differs");
        if(source_diagnostic)diagnostic_callbacks+=parent_callbacks-previous_parent_callbacks;
        auto expected_entries=c.at("entry_abis");
        if((parent_reference||source_diagnostic)&&!canonical_diagnostic){J selected=J::array();for(const auto &entry:expected_entries)if(std::find(c.at("parent_boundaries").begin(),c.at("parent_boundaries").end(),entry.at("entry"))==c.at("parent_boundaries").end())selected.push_back(entry);expected_entries=selected;}
        if(!source_route.empty()&&!parent_reference&&!source_diagnostic){
            const bool referenced_children=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATOR")&&std::getenv("HALO2_REFERENCE_RESOURCE_BUSY")&&std::getenv("HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE");
            J selected=J::array();for(const auto &entry:expected_entries){const auto target=entry.at("entry").get<unsigned>();const bool io=std::find(c.at("parent_boundaries").begin(),c.at("parent_boundaries").end(),target)!=c.at("parent_boundaries").end();if(io||target==0x13fe50u||referenced_children)selected.push_back(entry);}expected_entries=selected;
        }
        for(auto &entry:expected_entries)entry["abi"]["eflags"]=entry["abi"]["eflags"].get<unsigned>()&0xcd5u;
        if(entries!=expected_entries){unsigned n{};while(n<entries.size()&&n<expected_entries.size()&&entries[n]==expected_entries[n])++n;
            if(canonical_diagnostic){std::cout<<J{{"canonical_physical_pre_io_agrees",false},{"case",c.at("name")},{"position",n},{"canonical",n<entries.size()?entries[n]:J()},{"original",n<expected_entries.size()?expected_entries[n]:J()}}.dump()<<'\n';return 3;}
            throw std::runtime_error(c.at("name").get<std::string>()+" complete child entry differs at "+std::to_string(n)+" native="+(n<entries.size()?entries[n].dump():"end")+" original="+(n<expected_entries.size()?expected_entries[n].dump():"end"));}
        const auto after=state.save();const auto &expected=c.at("after_abi");
        if(after.gp!=expected.at("gp").get<std::array<std::uint32_t,8>>()||(after.eflags&0xcd5u)!=(expected.at("eflags").get<unsigned>()&0xcd5u)){
            std::cerr<<c.at("name")<<" flags native="<<std::hex<<after.eflags<<" original="<<expected.at("eflags")<<'\n';
            for(unsigned n=0;n<8;++n)if(after.gp[n]!=expected.at("gp").at(n))std::cerr<<"gp"<<n<<" native="<<std::hex<<after.gp[n]<<" original="<<expected.at("gp").at(n)<<'\n';
            throw std::runtime_error("Page allocation full return GP/flags differ");}
        check(hex(std::as_bytes(std::span(after.xmm)))==expected.at("xmm").get<std::string>()&&J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==expected.at("fp"),"Page allocation FP/XMM differs");check(page_live(after)==expected.at("x87"),"Page allocation live80-bit payload differs");
        check(pc==c.at("last_instruction"),c.at("name").get<std::string>()+" final cursor differs native="+std::to_string(pc)+" original="+c.at("last_instruction").dump());
        if(reading){J er=J::array();for(const auto &r:c.at("reads"))er.push_back(J::array({r.at("address"),r.at("bytes")}));
            if(memory.reads!=er){unsigned n{};while(n<memory.reads.size()&&n<er.size()&&memory.reads[n]==er[n])++n;throw std::runtime_error(c.at("name").get<std::string>()+" read differs at "+std::to_string(n)+" native="+(n<memory.reads.size()?memory.reads[n].dump():"end")+" original="+(n<er.size()?er[n].dump():"end"));}reads+=memory.reads.size();}
        else{if(written!=c.at("writes")){unsigned n{};while(n<written.size()&&n<c.at("writes").size()&&written[n]==c.at("writes")[n])++n;throw std::runtime_error(c.at("name").get<std::string>()+" store differs at "+std::to_string(n)+" native="+(n<written.size()?written[n].dump():"end")+" original="+(n<c.at("writes").size()?c.at("writes")[n].dump():"end"));}stores+=written.size();packets+=entries.size();++cases;boundaries+=stopped;}
        check(memory.applied.size()==c.value("mutations",J::array()).size(),"Declared observer feedback not delivered");if(!reading)mutations+=memory.applied.size();
        for(const auto &bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),c.at("name").get<std::string>()+" complete caller/heap bank differs");
    }
    if(!source_route.empty())check(source_parent_fallbacks==(parent_reference?2u*cases:2u*boundaries),"Source default parent fallback differs");
    const auto a=native_page_allocate_detail::activity();ParentFaultStats faults;
    const bool child_reference=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATOR")||std::getenv("HALO2_REFERENCE_RESOURCE_BUSY")||std::getenv("HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE")||std::getenv("HALO2_REFERENCE_DATUM_DELETE");
    if(fixture.value("run_parent_faults",false)&&!parent_reference&&!child_reference&&source_route.empty())faults=page_parent_faults(fixture,path.parent_path());
    check(!faults.instruction_callbacks,"Native parent fault sweep ran translated callbacks");
    std::cout<<J{{"passed",true},{"admission_refusals",admission},{"cases",cases},{"genuine_boundaries",boundaries},{"complete_child_entry_packets",packets},{"ordered_reads",reads},{"ordered_stores",stores},{"instruction_callbacks",callbacks},{"parent_instruction_callbacks",parent_callbacks-diagnostic_callbacks},{"diagnostic_parent_instruction_callbacks",diagnostic_callbacks},{"source_parent_fallbacks",source_parent_fallbacks},{"duplicate_entry_consistency_checks",duplicate_entries},{"declared_memory_feedback_events",mutations},
        {"native_probes",a.native_probes},{"native_preferences",a.native_preferences},{"native_retirements",a.native_retirements},{"native_allocations",a.native_allocations},{"native_busy",a.native_busy},{"residual_children",a.residuals},{"parent_atomic_provider_faults",J{{"cases",faults.cases},{"provider_prefixes",faults.provider_prefixes},{"observer_prefixes",faults.observer_prefixes},{"push_provider_prefixes",faults.push_provider_prefixes},{"push_observer_prefixes",faults.push_observer_prefixes}}}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

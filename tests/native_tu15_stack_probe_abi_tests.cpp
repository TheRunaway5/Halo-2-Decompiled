#include "native_tu15_allocator_abi_fixture.hpp"
#include "physics/runtime/native_tu15_stack_probe_abi.hpp"
#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"
#ifdef HALO2_STACK_PROBE_OWNER_DISPATCH
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#endif
#include <iostream>

namespace halo2 {
using namespace physics_world_runtime_detail;
using physics_vehicle_runtime_detail::VehicleState;
inline unsigned stack_probe_fallbacks{};inline std::string stack_probe_route;
namespace reference_owner {
#include "stack_probe_reference.inc"
}
#ifdef HALO2_STACK_PROBE_COPY_ROUTES
#include "stack_probe_copy_routes.inc"
#endif
}
namespace {
using namespace native_allocator_fixture;using ProbeState=halo2::physics_vehicle_runtime_detail::VehicleState;
auto &fallbacks=stack_probe_fallbacks;unsigned native_callbacks{},normal_callbacks{};
std::uint32_t physical_pc(std::uint32_t pc){return stack_probe_route=="vehicle"?pc+0x3b70u:pc;}
void expected(ProbeState &s){
#ifdef HALO2_STACK_PROBE_COPY_ROUTES
    if(stack_probe_route=="vehicle"){vehicle_reference::run_326af0(s);return;}
#endif
    reference_owner::run_32a660(s);
}
template<class Before>void execute(ProbeState &s,Before before){
#ifdef HALO2_STACK_PROBE_COPY_ROUTES
    if(stack_probe_route=="ai"){ai_copy::run_32a660(s);return;}
    if(stack_probe_route=="shadow"){shadow_copy::run_32a660(s);return;}
    if(stack_probe_route=="player"){player_copy::run_32a660(s);return;}
    if(stack_probe_route=="vehicle"){vehicle_copy::run_326af0(s);return;}
#endif
#ifdef HALO2_STACK_PROBE_OWNER_DISPATCH
    if(stack_probe_route=="owner"){
        if(native_allocator_owner_detail::dispatch(s,0x32a660u,[](auto&,auto,auto,auto){throw std::runtime_error("Unexpected stack-probe child owner");}))return;
        ++fallbacks;reference_owner::run_32a660(s);return;
    }
#endif
    if(!native_stack_probe_detail::run(s,before)){++fallbacks;reference_owner::run_32a660(s);}
}
void feedback(Memory &memory,const J &c,std::uint32_t site){for(const auto &m:c.value("mutations",J::array()))if(m.at("instruction")==physical_pc(site)){const auto p=m.at("address").get<std::uint32_t>();const auto raw=unhex(m.at("written"));const auto bank=memory.at(p,raw.size());std::copy(raw.begin(),raw.end(),memory.banks.at(bank->first).begin()+p-bank->first);}}
// Replay declared memory feedback after the genuine saved-counter write.
// Read-only observation mode uses the Memory owner so enabling a store
// observer does not add a synthetic preimage read to the physical read list.
struct FeedbackMemory:Memory {
    const J *input{};
    void write(std::uint32_t p,std::span<const std::byte> data)override{
        Memory::write(p,data);
        if(input&&p==input->at("stack_pointer").get<std::uint32_t>()-4u)feedback(*this,*input,stack_probe_route=="vehicle"?0x326b09u:0x32a679u);
    }
};
struct FaultStats{unsigned cases{},memory_prefixes{},observer_prefixes{};};
unsigned admissions(const J &c,const std::filesystem::path &directory){
    unsigned count{};
    for(unsigned mode=0;mode<10;++mode){
        auto memory=load(c,directory);const auto initial=abi(c);SourcePhysicsWorldRuntimeServices services;std::uint32_t pc=0x12345678u;
        services.instruction=[](auto){};cursor(services,pc);bool trace=true;SourcePhysicsContactRecordObserver observer;
        static const std::array<std::uint32_t,3>wrong_sites{0x38846fu,0x21f590u,0x33b595u};
        if(mode==1)services.instruction_cursor.first_trace=&trace;
        if(mode==2)services.instruction_cursor.second_trace=&trace;
        if(mode==3)services.instruction_cursor.literal_service_sites=false;
        if(mode==4)services.instruction_cursor.position=nullptr;
        if(mode==5)services.instruction_cursor.executor=std::thread::id{};
        if(mode==6)services.instruction_cursor.callback_type=&typeid(std::nullptr_t);
        if(mode==7){observer=[](const auto&){};services.instruction_cursor.observation_only_stores=false;}
        if(mode==8)services.instruction_cursor.service_sites=wrong_sites;
        if(mode==9)services.instruction_cursor.callback_type=nullptr;
        ProbeState state{memory,observer,services,mode?SourcePhysicsContactImage::TitleUpdate15:SourcePhysicsContactImage::MapPack11};state.load(initial);const auto before=state.save();
        check(!native_stack_probe_detail::run(state),"Stack probe admitted an unqualified observation/image protocol");
        check(memory.operations==0&&pc==0x12345678u&&same_abi(before,state.save()),"Rejected stack probe changed state");++count;
    }
    return count;
}
FaultStats faults(const J &fixture,const std::filesystem::path &directory){
    FaultStats stats;
    for(const auto &c:fixture.at("cases")){
        if(!c.value("fault_case",false))continue;
        auto initial=abi(c);initial.fs_base=0x76543000u;initial.x87_depth=2;initial.x87_status=3u<<11u;initial.x87[0]=1.25L;initial.x87[1]=-2.5L;
        for(unsigned lane=0;lane<8;++lane)for(unsigned word=0;word<4;++word)initial.xmm[lane][word]=0x7fc00001u^(lane*0x03030303u+word*0x10101010u);
        const auto input=load(c,directory);
        for(unsigned protocol=0;protocol<3;++protocol){unsigned limit{};
            for(unsigned fail=0;;++fail){Memory expected=input,actual=input;if(protocol<2)expected.fail_at=actual.fail_at=fail;
                std::vector<SourcePhysicsContactRecordStore> es,ns;SourcePhysicsContactRecordObserver eo,no;
                if(protocol){eo=[&](const auto &e){es.push_back(e);feedback(expected,c,e.instruction);if(protocol==2&&es.size()==fail)throw std::runtime_error("injected stack observer fault");};
                    no=[&](const auto &e){ns.push_back(e);feedback(actual,c,e.instruction);if(protocol==2&&ns.size()==fail)throw std::runtime_error("injected stack observer fault");};}
                SourcePhysicsWorldRuntimeServices er{},nr{};std::uint32_t epc{},npc{};er.instruction=[&](auto pc){epc=pc;};nr.instruction=[&](auto pc){npc=pc;++native_callbacks;};cursor(nr,npc);
                ProbeState x{expected,eo,er,SourcePhysicsContactImage::TitleUpdate15},y{actual,no,nr,SourcePhysicsContactImage::TitleUpdate15};x.load(initial);y.load(initial);
                std::string ee,ne;try{::expected(x);}catch(const std::exception &e){ee=e.what();}const auto ex=x.save();
                try{execute(y,native_page_window_detail::Noop{});}catch(const std::exception &e){ne=e.what();}const auto na=y.save();_mm_setcsr(0x1f80u);
                if(ee!=ne||!same_abi(ex,na)||ex.fs_base!=na.fs_base||expected.banks!=actual.banks||expected.accesses!=actual.accesses||es!=ns||epc!=npc){
                    std::cerr<<c.at("name")<<" protocol="<<protocol<<" prefix="<<fail<<" pc="<<std::hex<<epc<<'/'<<npc<<" flags="<<ex.eflags<<'/'<<na.eflags<<'\n';throw std::runtime_error("Stack probe retained prefix differs");}
                if(!fail){++stats.cases;limit=protocol==2?es.size():expected.operations;}else if(protocol==2)++stats.observer_prefixes;else++stats.memory_prefixes;
                if(fail>=limit)break;
            }
        }
    }
    return stats;
}
}
int main(int argc,char **argv)try{
    check(argc==2||argc==3,"Supply stack-probe fixture and optional route");stack_probe_route=argc==3?argv[2]:"candidate";const bool reference=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATOR");const std::filesystem::path path=argv[1];std::ifstream stream(path);const auto fixture=J::parse(stream);unsigned cases{},reads{},stores{},packets{};
    for(const auto &c:fixture.at("cases"))for(bool reading:{false,true}){
        FeedbackMemory memory;static_cast<Memory&>(memory)=load(c,path.parent_path());memory.logging=reading;if(reading&&c.contains("mutations"))memory.input=&c;const auto initial=abi(c);J written=J::array(),captured=J::array();SourcePhysicsContactRecordObserver observer;
        if(!reading)observer=[&](const auto &e){written.push_back(J{{"instruction",physical_pc(e.instruction)},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});feedback(memory,c,e.instruction);};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};ProbeState *current{};
        const auto before=[&](ProbeState &s,auto site){captured.push_back(packet(s,physical_pc(site)));};
        services.instruction=[&](auto site){pc=site;++normal_callbacks;if(std::find(c.at("abi_entries").begin(),c.at("abi_entries").end(),physical_pc(site))!=c.at("abi_entries").end())before(*current,site);};cursor(services,pc);
        ProbeState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;state.load(initial);execute(state,before);const auto after=state.save();
        auto entries=c.at("entry_abis");for(auto &entry:entries)entry["abi"]["eflags"]=entry["abi"]["eflags"].get<unsigned>()&0xcd5u;
        if(stack_probe_route!="candidate"&&!reference)check(captured.empty(),"Native source/actual-owner route invoked instruction callbacks");
        else if(captured!=entries){unsigned n{};while(n<captured.size()&&n<entries.size()&&captured[n]==entries[n])++n;throw std::runtime_error(c.at("name").get<std::string>()+" physical pre-I/O packet differs at"+std::to_string(n)+" native="+(n<captured.size()?captured[n].dump():"end")+" original="+(n<entries.size()?entries[n].dump():"end"));}
        const auto &expected=c.at("after_abi");check(after.gp==expected.at("gp").get<std::array<std::uint32_t,8>>()&&(after.eflags&0xcd5u)==(expected.at("eflags").get<unsigned>()&0xcd5u),"Stack probe normal GP/flags differ");
        check(hex(std::as_bytes(std::span(after.xmm)))==expected.at("xmm").get<std::string>()&&J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==expected.at("fp"),"Stack probe FP/XMM differs");check(physical_pc(pc)==c.at("last_instruction"),"Stack probe final cursor differs");
        if(reading){J er=J::array();for(const auto &r:c.at("reads"))er.push_back(J::array({r.at("address"),r.at("bytes")}));check(memory.reads==er,c.at("name").get<std::string>()+" stack probe ordered reads differ native="+memory.reads.dump()+" original="+er.dump());reads+=memory.reads.size();}
        else{check(written==c.at("writes"),"Stack probe ordered stores differ");stores+=written.size();packets+=captured.size();++cases;}
        for(const auto &bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),"Stack probe full bank differs");
    }
    const auto f=faults(fixture,path.parent_path());const auto admission_checks=admissions(fixture.at("cases").at(0),path.parent_path());check(reference?fallbacks==2u*cases+f.cases+f.memory_prefixes+f.observer_prefixes:fallbacks==0,"Stack probe fallback differs");check(reference||(!native_callbacks&&!normal_callbacks),"Stack probe invoked retained instruction callbacks");
    J owner_activity=J::object();
#ifdef HALO2_STACK_PROBE_OWNER_DISPATCH
    const auto &a=native_allocator_owner_detail::activity();for(const auto &[entry,count]:a.probe_entries)owner_activity[std::to_string(entry)]=count;
    check(a.entries.empty()&&a.tick_entries.empty()&&a.epoch_entries.empty()&&a.bsp_entries.empty()&&a.bsp_polygon_entries.empty()&&a.delete_entries.empty(),"Stack-probe route invoked unrelated actual-owner group");
    check(stack_probe_route=="owner"&&!reference?!a.probe_entries.empty():a.probe_entries.empty(),"Stack-probe actual-owner activity differs");
#endif
    std::cout<<J{{"passed",true},{"cases",cases},{"admission_checks",admission_checks},{"physical_pre_io_packets",packets},{"normal_instruction_callbacks",normal_callbacks},{"ordered_reads",reads},{"ordered_stores",stores},{"translated_fallbacks",fallbacks},{"actual_owner_activity",owner_activity},{"faults",J{{"normal",f.cases},{"memory_prefixes",f.memory_prefixes},{"observer_prefixes",f.observer_prefixes},{"native_callbacks",native_callbacks}}}}.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

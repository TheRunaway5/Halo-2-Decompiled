#define main unused_arena_physical_main
#include "native_tu15_arena_initialize_frame_tests.cpp"
#undef main
#include "physics/runtime/native_tu15_arena_initialize_source_frame.hpp"

namespace {
using Phase=halo2::native_arena_initialize_source_detail::Phase;
thread_local Phase declared_phase{};
thread_local unsigned retained_statements{},handoffs{};
thread_local std::set<std::uint32_t>covered_handoffs;
struct ContinuationMemory:Memory {
    mutable std::function<void()>feedback;
    void read(std::uint32_t address,std::span<std::byte>out)const override{Memory::read(address,out);if(feedback)feedback();}
    void write(std::uint32_t address,std::span<const std::byte>in)override{Memory::write(address,in);if(feedback)feedback();}
};
void statement(){++retained_statements;}
bool continued(const State&s){
    const auto&c=s.services.instruction_cursor;
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
    return !s.active_mmx&&s.fp.count<=8u&&s.fp.top<8u&&(cw&0x3fu)==0x3fu&&(_mm_getcsr()&0x1f80u)==0x1f80u&&
        s.services.instruction&&c.position&&c.callback_type&&s.services.instruction.target_type()==*c.callback_type&&
        c.executor==std::this_thread::get_id()&&c.literal_service_sites&&c.service_sites.size()==3&&
        c.service_sites[0]==0x38846eu&&c.service_sites[1]==0x21f590u&&c.service_sites[2]==0x33b595u&&
        !(c.first_trace&&*c.first_trace)&&!(c.second_trace&&*c.second_trace)&&(!s.observe||c.observation_only_stores);
}
void genuine_entry_stop(State&s,std::uint32_t target,std::uint32_t site,bool tail){
    check(!tail&&(target==0x21b920u||target==0x21b990u||target==0x166a80u),"Foreign actual Arena child entry");
    if(target==0x21b920u)check(site==0x1252a4u,"Foreign reserve source site");
    static_cast<ContinuationMemory&>(s.memory).pc=target;throw ChildEntryStop{};
}
std::uint32_t native_arena_continuation_entry(State&s){
    const auto next=native_arena_initialize_source_detail::run_source(s,continued,genuine_entry_stop,
        native_arena_initialize_source_detail::Noop{},declared_phase);
    if(next){++handoffs;covered_handoffs.insert(next);}return next;
}
}
namespace arena_reference {
using namespace halo2::physics_world_runtime_detail;using WorldState=::State;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){genuine_entry_stop(s,target,site,tail);}
void run_217090(WorldState&s){genuine_entry_stop(s,0x21b990u,0x1252bau,false);}
void run_1656a0(WorldState&s){genuine_entry_stop(s,0x166a80u,0u,false);}
#include "arena_continuation_reference.inc"
}
namespace arena_candidate {
using namespace halo2::physics_world_runtime_detail;using WorldState=::State;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){genuine_entry_stop(s,target,site,tail);}
void run_217090(WorldState&s){genuine_entry_stop(s,0x21b990u,0x1252bau,false);}
void run_1656a0(WorldState&s){genuine_entry_stop(s,0x166a80u,0u,false);}
#include "arena_continuation_candidate.inc"
}
namespace {
J execute(const J&row,bool native,unsigned mutation,unsigned ordinal){
    const auto host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image value;~Restore(){native_raw_x87_detail::restore(value);}}restore{host};
    ContinuationMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
    memory.write_sites={0x125280u,0x12528eu,0x12528fu,0x12529au,0x1252a4u,0x1252adu,0x1252b8u,0x1252bau,0x1252d2u,0x1252d7u,0x1252e5u,0x1252edu,0x1252fdu,0x125308u,0x12530du,0x125316u,0x12531du,0x125321u,0x125328u,0x125330u};
    SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;State*current{};
    bool first{},second{},changed{};unsigned io{},observer_calls{};J callbacks=J::array(),observers=J::array();
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0x21f590u,0x33b596u};
    services.instruction=[&](auto pc){memory.pc=pc;if(!continued(*current))callbacks.push_back(packet(*current,pc));};
    auto&c=services.instruction_cursor;c={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    if(row.value("observed",false))observe=[&](const auto&){++observer_calls;observers.push_back(packet(*current,memory.pc));};
    State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;SourcePhysicsWorldRuntimeAbi input{};
    input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<input.x87_depth;++n){const auto bytes=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],bytes.data(),10);}state.load(input);
    memory.capture=[&]{return packet(state,memory.pc);};
    memory.feedback=[&]{
        if(changed||!mutation||io++!=ordinal)return;changed=true;
        switch(mutation){
        case 1:first=true;break;case 2:second=true;break;
        case 3:services.instruction={};break;
        case 4:services.instruction=[&](auto pc){memory.pc=pc;callbacks.push_back(packet(state,pc));};break;
        case 5:c.position=nullptr;break;case 6:c.executor={};break;
        case 7:c.literal_service_sites=false;break;case 8:c.service_sites=wrong;break;
        case 9:{const auto cw=std::uint16_t(input.floating_point->x87_control_word&~1u);asm volatile("fldcw %0"::"m"(cw));break;}
        case 10:_mm_setcsr(_mm_getcsr()&~0x80u);break;
        case 11:throw native_arena_initialize_source_detail::Continuation{0x125281u,nullptr};
        case 12:throw Memory::Fault{};
        default:throw std::runtime_error("Unknown private Arena source mutation");
        }
    };
    declared_phase=static_cast<Phase>(row.at("phase_id").get<unsigned>());const auto before_statements=retained_statements,before_handoffs=handoffs;
    bool child{},fault{},foreign{};std::string error;
    try{if(native)arena_candidate::run_125280(state);else arena_reference::run_125280(state);}
    catch(const ChildEntryStop&){child=true;}catch(const Memory::Fault&){fault=true;}
    catch(const native_arena_initialize_source_detail::Continuation&e){check(!e.owner&&e.next==0x125281u,"Foreign Arena exception identity changed");foreign=true;}
    catch(const std::exception&e){error=e.what();}
    J banks=J::array();for(const auto&[address,bytes]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(bytes)}});
    return J{{"abi",packet(state,memory.pc)},{"reads",memory.reads},{"writes",memory.writes},{"packets",memory.packets},{"callbacks",callbacks},{"observers",observers},{"banks",banks},{"child",child},{"fault",fault},{"foreign_exception",foreign},{"error",error},{"changed",changed},{"io",io},{"retained",retained_statements-before_statements},{"handoffs",handoffs-before_handoffs}};
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply declared original Arena source inputs");std::ifstream in(argv[1]);const auto fixture=J::parse(in);unsigned comparisons{},resumed{},exceptions{};
    for(const auto&row:fixture.at("cases")){
        const auto reference=execute(row,false,0,0),baseline=execute(row,true,0,0);
        auto compare=[&](unsigned mutation,unsigned ordinal){
            auto expected=execute(row,false,mutation,ordinal),actual=execute(row,true,mutation,ordinal);
            resumed+=actual.at("handoffs").get<unsigned>();exceptions+=actual.at("fault").get<bool>()||actual.at("foreign_exception").get<bool>();
            if(!mutation)check(actual.at("retained")==0,"Normal Arena candidate retained parent statements");
            for(const char*key:{"retained","handoffs"}){expected.erase(key);actual.erase(key);}
            for(const auto&[key,value]:expected.items())check(actual.at(key)==value,row.at("name").get<std::string>()+" Arena source continuation differs mutation="+std::to_string(mutation)+" ordinal="+std::to_string(ordinal)+" key="+key);
            ++comparisons;
        };
        compare(0,0);
        const auto count=reference.at("reads").size()+reference.at("writes").size()+reference.at("observers").size();
        for(unsigned mutation=1;mutation<=12;++mutation){
            std::set<std::size_t>ordinals{0,count/2,count?count-1:0};
            if(fixture.value("all_boundaries",false)&&mutation==1)for(std::size_t n=0;n<count;++n)ordinals.insert(n);
            for(auto ordinal:ordinals)compare(mutation,ordinal);
        }
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"authenticated_handoffs",resumed},{"covered_handoffs",covered_handoffs},{"original_exception_prefixes",exceptions},{"private_unbound",true},{"raw_architectural_fault_claim",false},{"supplied_child_results",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

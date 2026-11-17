// Controller/admission interface proof. The standalone friend issuers below
// are not the actual World producer; its complete-engine proof is separate.
#define main unused_arena_physical_main
#include "native_tu15_arena_initialize_frame_tests.cpp"
#undef main
#include "physics/runtime/native_tu15_arena_world_initialize_owner.hpp"

namespace world_owner_proof {
using Services=SourcePhysicsContactRuntimeServices;
namespace publication=native_arena_world_publication_detail;
struct World {
    std::uint32_t position{};bool first{},second{};
    const std::thread::id executor=std::this_thread::get_id();
    const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};
    struct Required {
        World*const world;
        void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
            SourcePhysicsContactRuntimeAbi&,const SourcePhysicsContactRecordObserver&)const{}
        static bool matches(const Services::RequiredOwner&callback,const void*owner)noexcept{
            const auto*actual=callback.target<Required>();
            return actual&&actual->world==owner&&actual->world->executor==std::this_thread::get_id()&&
                !actual->world->first&&!actual->world->second;
        }
    };
};
}
namespace halo2 {
class PhysicsLiveWorldTu15 {
public:
    template<class Call>static void publish(const SourcePhysicsGameArenaNative&arena,
        const SourcePhysicsContactMemory&bank,const world_owner_proof::Services&producer,
        const void*world,Call call){native_arena_world_publication_detail::Construction scope{arena,bank,producer,world};call();}
};
class SourcePhysicsGameArenaNative {
    const SourcePhysicsContactMemory&bank_;
    const world_owner_proof::Services lower_;
    static bool matches(const world_owner_proof::Services&joined,const void*owner,
        const SourcePhysicsGameArenaNative&arena)noexcept{
        return owner==&arena&&native_arena_world_publication_detail::Construction::qualified(arena,arena.bank_,arena.lower_)&&
            native_arena_world_publication_detail::same_instruction(joined,arena.lower_)&&
            native_required_owner_policy_detail::preserves_instruction_policy(joined);
    }
public:
    SourcePhysicsGameArenaNative(const SourcePhysicsContactMemory&bank,
        const world_owner_proof::Services&lower):bank_(bank),lower_(lower){}
    template<class Call>void publish(const SourcePhysicsContactMemory&memory,
        const world_owner_proof::Services&joined,Call call)const{
        native_arena_world_publication_detail::Borrow scope{*this,memory,joined,this,&matches};call();
    }
};
}
namespace {
thread_local unsigned statements{};
thread_local bool stop_at_first{};
struct FallbackStop{};
void statement(){++statements;if(stop_at_first)throw FallbackStop{};}
void genuine_entry_stop(State&s,std::uint32_t target,std::uint32_t site,bool tail){
    check(!tail&&target==0x21b920u&&site==0x1252a4u,"Foreign World Arena actual child entry");
    static_cast<Memory&>(s.memory).pc=target;throw ChildEntryStop{};
}
struct FeedbackMemory:Memory {
    mutable std::function<void()>feedback;
    void read(std::uint32_t address,std::span<std::byte>out)const override{Memory::read(address,out);if(feedback)feedback();}
    void write(std::uint32_t address,std::span<const std::byte>in)override{Memory::write(address,in);if(feedback)feedback();}
};
}
namespace raw_reference {
using namespace halo2::physics_world_runtime_detail;using WorldState=::State;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){genuine_entry_stop(s,target,site,tail);}
void run_217090(WorldState&){throw std::runtime_error("Unqualified whole-cold child result");}
void run_1656a0(WorldState&){throw std::runtime_error("Unqualified CRC result");}
#include "arena_world_owner_raw_reference.inc"
}
namespace prepared_reference {
using namespace halo2::physics_world_runtime_detail;using WorldState=::State;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){genuine_entry_stop(s,target,site,tail);}
void run_217090(WorldState&){throw std::runtime_error("Unqualified whole-cold child result");}
void run_1656a0(WorldState&){throw std::runtime_error("Unqualified CRC result");}
#include "arena_world_owner_prepared_reference.inc"
}
namespace raw_candidate {
using namespace halo2::physics_world_runtime_detail;using WorldState=::State;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){genuine_entry_stop(s,target,site,tail);}
void run_217090(WorldState&){throw std::runtime_error("Unqualified whole-cold child result");}
void run_1656a0(WorldState&){throw std::runtime_error("Unqualified CRC result");}
#include "arena_world_owner_raw_candidate.inc"
}
namespace prepared_candidate {
using namespace halo2::physics_world_runtime_detail;using WorldState=::State;
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){genuine_entry_stop(s,target,site,tail);}
void run_217090(WorldState&){throw std::runtime_error("Unqualified whole-cold child result");}
void run_1656a0(WorldState&){throw std::runtime_error("Unqualified CRC result");}
#include "arena_world_owner_prepared_candidate.inc"
}
namespace {
J execute(const J&row,bool candidate,bool prepared,unsigned mutation,unsigned ordinal,bool entry_refusal){
    const auto host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image value;~Restore(){native_raw_x87_detail::restore(value);}}restore{host};
    FeedbackMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
    memory.write_sites={0x125280u,0x12528eu,0x12528fu,0x12529au,0x1252a4u};
    world_owner_proof::World world;
    world_owner_proof::Services joined;State*current{};J callbacks=J::array(),observers=J::array();
    joined.instruction=[&](auto pc){memory.pc=pc;world.position=pc;callbacks.push_back(packet(*current,pc));};
    auto&cursor=joined.instruction_cursor;
    cursor={&memory.pc,&world.first,&world.second,world.sites,world.executor,&joined.instruction.target_type(),true,false};
    joined.required_owner=world_owner_proof::World::Required{&world};
    joined.required_owner_policy={&joined.required_owner.target_type(),world.executor,&world,&world_owner_proof::World::Required::matches};
    const auto producer=joined;
    SourcePhysicsGameArenaNative arena{memory,producer};
    SourcePhysicsContactRecordObserver observe;
    State state{memory,observe,joined,SourcePhysicsContactImage::TitleUpdate15};current=&state;
    SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");
    input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}state.load(input);
    memory.capture=[&]{return packet(state,memory.pc);};
    unsigned io{};bool changed{},publish_world=true,publish_borrow=true;
    constexpr std::array<std::uint32_t,3>wrong_sites{0x38846eu,0x21f590u,0x33b596u};
    const auto mutate=[&]{
        changed=true;
        switch(mutation){
        case 0:break;case 1:world.first=true;break;case 2:world.second=true;break;
        case 3:joined.instruction={};break;
        case 4:joined.instruction=[&](auto pc){memory.pc=pc;callbacks.push_back(packet(state,pc));};break;
        case 5:cursor.position=nullptr;break;case 6:cursor.executor={};break;
        case 7:cursor.literal_service_sites=false;break;case 8:cursor.service_sites=wrong_sites;break;
        case 9:{const auto cw=std::uint16_t(input.floating_point->x87_control_word&~1u);asm volatile("fldcw %0"::"m"(cw));break;}
        case 10:_mm_setcsr(_mm_getcsr()&~0x80u);break;
        case 11:throw native_arena_initialize_source_detail::Continuation{0x125281u,nullptr};
        case 12:throw Memory::Fault{};
        case 13:observe=[&](const auto&event){observers.push_back(J{{"pc",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});};break;
        case 14:joined.required_owner_policy.owner=nullptr;break;
        case 15:joined.required_owner_policy.matches=nullptr;break;
        case 16:cursor.callback_type=nullptr;break;
        case 17:publish_world=false;break;case 18:publish_borrow=false;break;
        case 19:state.image=SourcePhysicsContactImage::MapPack11;break;
        default:throw std::runtime_error("Unknown World Arena source policy");
        }
    };
    memory.feedback=[&]{if(!entry_refusal&&!changed&&mutation&&io++==ordinal)mutate();};
    if(entry_refusal&&mutation)mutate();
    stop_at_first=entry_refusal;const auto before_statements=statements;
    const auto before_activity=native_arena_world_initialize_owner_detail::activity();
    bool stopped{},fallback{},fault{},foreign{};
    const auto run=[&]{
        if(candidate){if(prepared)prepared_candidate::run_125280(state);else raw_candidate::run_125280(state);}
        else{if(prepared)prepared_reference::run_125280(state);else raw_reference::run_125280(state);}
    };
    const auto borrow=[&]{if(publish_borrow)arena.publish(memory,joined,run);else run();};
    try{if(publish_world)PhysicsLiveWorldTu15::publish(arena,memory,producer,&world,borrow);else borrow();}
    catch(const ChildEntryStop&){stopped=true;}catch(const FallbackStop&){fallback=true;}
    catch(const Memory::Fault&){fault=true;}
    catch(const native_arena_initialize_source_detail::Continuation&e){check(!e.owner&&e.next==0x125281u,"Foreign World Arena exception changed");foreign=true;}
    stop_at_first=false;
    J banks=J::array();for(const auto&[address,bytes]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(bytes)}});
    return J{{"abi",packet(state,memory.pc)},{"reads",memory.reads},{"writes",memory.writes},{"packets",memory.packets},
        {"callbacks",callbacks},{"observers",observers},{"banks",banks},{"child",stopped},{"fallback",fallback},
        {"fault",fault},{"foreign",foreign},{"changed",changed},{"io",io},
        {"retained",statements-before_statements},{"native",native_arena_world_initialize_owner_detail::activity()-before_activity}};
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply original-backed World Arena source fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    unsigned comparisons{},refusals{},native{},handoffs{};
    const auto*reference_control=std::getenv("HALO2_REFERENCE_ARENA_INITIALIZE");
    const bool forced_reference=reference_control&&*reference_control;
    for(const auto&row:fixture.at("cases"))for(bool prepared:{false,true}){
        const auto baseline=execute(row,false,prepared,0,0,false);
        const auto count=baseline.at("reads").size()+baseline.at("writes").size();
        const auto compare=[&](unsigned mutation,unsigned ordinal,bool refusal){
            auto expected=execute(row,false,prepared,mutation,ordinal,refusal),actual=execute(row,true,prepared,mutation,ordinal,refusal);
            if(refusal){check(actual.at("fallback")==true&&actual.at("native")==0,"World Arena untouched entry refusal failed");++refusals;}
            else if(prepared&&!forced_reference){check(actual.at("native")==1,"World Arena prepared source did not own entry");++native;if(mutation&&actual.at("retained").get<unsigned>())++handoffs;}
            else check(actual.at("native")==0,"World Arena raw callbacks incorrectly admitted");
            if(!mutation&&prepared&&!refusal)check(bool(actual.at("retained").get<unsigned>())==forced_reference,"World Arena default/reference retained parent differs");
            expected.erase("retained");actual.erase("retained");expected.erase("native");actual.erase("native");
            for(const auto&[key,value]:expected.items())check(actual.at(key)==value,"World Arena source continuation differs mutation="+std::to_string(mutation)+" key="+key);
            ++comparisons;
        };
        compare(0,0,false);
        for(unsigned mutation=1;mutation<=16;++mutation){
            for(const auto ordinal:std::set<std::size_t>{0,count/2,count?count-1:0})compare(mutation,ordinal,false);
            if(mutation!=11&&mutation!=12)compare(mutation,0,true);
        }
        compare(17,0,true);compare(18,0,true);compare(19,0,true);
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"untouched_refusals",refusals},
        {"prepared_native_calls",native},{"retained_handoffs",handoffs},{"raw_callbacks_native",false},{"forced_reference",forced_reference},
        {"scope","Standalone publication interface with exact authored source controller and genuine child-entry stops; actual World producer coupling has its separate full-engine proof."}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

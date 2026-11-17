#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_packed_rotation_source_owner.hpp"
namespace {std::array<unsigned,12>rotation_literals{},rotation_statements{};}
#include "packed_rotation_routes.inc"

namespace {
template<class State>J rotation_route(const J&row,unsigned variant,unsigned mode,bool retained,void(*bound)(State&),void(*reference)(State&)){
    OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    memory.write_sites={0x296197u,0x29619bu,0x2961e1u};
    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{},estimates{},providers{},observed{};
    SourcePhysicsContactRecordObserver observe=[&](const auto&){++observed;};if(mode==4||mode==5)observe={};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};bool trace=mode==1;
    if(mode!=5){services.instruction=[&](std::uint32_t pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};}
    services.reciprocal_sqrt=[&](std::uint32_t bits){++estimates;auto value=_mm_castsi128_ps(_mm_set_epi32(0,0,0,std::bit_cast<std::int32_t>(bits)));asm volatile("rsqrtss %1,%0":"=x"(value):"x"(value):"memory");return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];};
    if(mode==3)services.required_owner=[&](auto&,const auto&,auto&,const auto&){++providers;throw std::runtime_error("Childless rotation invoked an owner");};
    State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    if(mode==2)services.instruction=[&,different=1u](std::uint32_t pc){memory.pc=pc;++callbacks;(void)different;};
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    if(row.contains("active_mmx")){SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}abi.active_mmx=opaque;}
    state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};const auto native=native_packed_rotation_owner_detail::activity();const auto literal=rotation_literals[variant],statements=rotation_statements[variant];
    if(retained)reference(state);else bound(state);
    J banks=J::array();for(const auto&bank:row.at("banks"))banks.push_back(hex(memory.banks.at(bank.at("address"))));
    return J{{"abi",packet(state,memory.pc)},{"reads",memory.reads},{"writes",memory.writes},{"packets",memory.packets},{"banks",banks},{"estimates",estimates},{"providers",providers},{"observed",observed},{"callbacks",callbacks},{"native",native_packed_rotation_owner_detail::activity()-native},{"literal",rotation_literals[variant]-literal},{"statements",rotation_statements[variant]-statements}};
}
template<class State>void rotation_owner(const J&fixture,unsigned variant,unsigned mode,void(*bound)(State&),void(*reference)(State&),unsigned&comparisons,unsigned&native,unsigned&literals,unsigned&statements){
    for(const auto&row:fixture.at("cases")){
        J expected=rotation_route(row,variant,mode,true,bound,reference),actual=rotation_route(row,variant,mode,false,bound,reference);
        const bool fallback=native_packed_rotation_owner_detail::reference()||mode==1||mode==2;
        check(actual.at("native")==unsigned(!fallback)&&actual.at("literal")==unsigned(fallback)&&actual.at("estimates")==4&&actual.at("providers")==0,"Packed rotation source entry accounting differs");
        if(!fallback)check(actual.at("callbacks")==0&&actual.at("statements")==0,"Packed rotation native route retained statements");
        else check(actual.at("statements")==31,"Packed rotation reference did not execute all 31 sites");
        native+=actual.at("native").get<unsigned>();literals+=actual.at("literal").get<unsigned>();statements+=actual.at("statements").get<unsigned>();
        for(const char*key:{"callbacks","native","literal","statements"}){expected.erase(key);actual.erase(key);}
        check(actual==expected,"Packed rotation actual source route differs: "+std::to_string(variant)+" / "+row.at("name").get<std::string>());++comparisons;
    }
}
struct GuardState:physics_world_runtime_detail::WorldState{using WorldState::WorldState;bool selected_observation_pcs=true;};
void guards(){unsigned count{};for(unsigned kind=0;kind<28;++kind){
    OwnerMemory memory;SourcePhysicsContactRecordObserver observe=[](const auto&){};SourcePhysicsWorldRuntimeServices services;services.instruction=[](std::uint32_t){};services.reciprocal_sqrt=[](std::uint32_t value){return value;};std::uint32_t pc{};bool trace=true;std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&c=services.instruction_cursor;c={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};if(kind==11)c.literal_service_sites=false;
    GuardState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.fp.count=0;state.fp.top=0;state.fp.status=0;
    if(kind!=11)check(native_packed_rotation_owner_detail::admitted(state),"Packed rotation baseline guard refused");
    auto copied=c;std::array extra{sites[0],sites[1],sites[2],sites[2]};const auto image=native_raw_x87_detail::capture();
    switch(kind){case 0:state.image=SourcePhysicsContactImage::MapPack11;break;case 1:state.image=SourcePhysicsContactImage(99);break;case 2:state.fp.count=9;break;case 3:state.fp.top=8;break;case 4:state.fp.status|=0x8080;break;case 5:state.active_mmx.emplace();state.fp.count=1;break;case 6:services.instruction={};break;case 7:services.instruction=[different=1u](std::uint32_t){(void)different;};break;case 8:c.position=nullptr;break;case 9:c.executor={};break;case 10:c.literal_service_sites=false;break;case 11:c.literal_service_sites=true;break;case 12:state.instruction_cursor=&copied;break;case 13:c.callback_type=nullptr;break;case 14:sites[2]^=1;break;case 15:c.service_sites=std::span(sites).first(2);break;case 16:c.first_trace=&trace;break;case 17:c.second_trace=&trace;break;case 18:c.observation_only_stores=false;break;case 19:c.callback_type=&typeid(int);break;case 20:_mm_setcsr(image.mxcsr&~0x80u);break;case 21:services.instruction={};state.instruction_cursor=nullptr;break;case 22:c.service_sites=extra;break;case 23:{const auto cw=std::uint16_t(image.word(0)&~1u);asm volatile("fldcw %0"::"m"(cw));break;}case 24:services.reciprocal_sqrt={};break;case 25:state.selected_observation_pcs=false;break;case 26:services.instruction={};c.position=nullptr;c.literal_service_sites=false;break;case 27:state.instruction_cursor=nullptr;break;}
    const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto flags=state.flags.bits(),other=state.other_flags,depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
    const bool accepted=native_packed_rotation_owner_detail::bridge(state);native_raw_x87_detail::restore(image);
    check(!accepted&&gp==state.gp&&std::memcmp(xmm.data(),state.xmm,128)==0&&flags==state.flags.bits()&&other==state.other_flags&&depth==state.fp.count&&top==state.fp.top&&status==state.fp.status&&memory.reads.empty()&&memory.writes.empty()&&pc==0,"Packed rotation untouched refusal differs "+std::to_string(kind));++count;
    }std::cout<<J{{"untouched_live_refusals",count}}.dump()<<'\n';
}
}
int main(int argc,char**argv)try{
    const auto host=native_raw_x87_detail::capture();struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{host};
    if(argc==2&&std::string(argv[1])=="--guards"){guards();return 0;}
    check(argc==3,"Supply packed rotation route inputs and mode");std::ifstream input(argv[1]);const auto fixture=J::parse(input);const auto mode=unsigned(std::stoul(argv[2]));unsigned comparisons{},native{},literals{},statements{};
#define ROTATION_OWNER(N) rotation_owner(fixture,N,mode,&packed_rotation_route##N::bound,&packed_rotation_route##N::retained,comparisons,native,literals,statements);
    PACKED_ROTATION_OWNERS(ROTATION_OWNER)
#undef ROTATION_OWNER
    std::cout<<J{{"passed",true},{"source_variants",12},{"comparisons",comparisons},{"native_calls",native},{"retained_entries",literals},{"retained_statements",statements}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

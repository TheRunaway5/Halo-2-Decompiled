#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_packed_rotation_frame.hpp"
#include "packed_rotation_sources.inc"

namespace{
template<class SourceState>J execute_rotation_policy(const J&row,unsigned mutation,unsigned stop_after,bool native,void(*reference)(SourceState&)){
    OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    memory.write_sites={0x296197u,0x29619bu,0x2961e1u};
    SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    constexpr std::array suffix_sites{0x2961deu,0x2961e1u,0x2961e4u,0x2961e7u};
    bool trace{},returned{};unsigned estimates{},output_stores{};J events=J::array();
    SourceState*current{};
    const auto record=[&](std::uint32_t pc){
        memory.pc=pc;
        if(returned){
            check(!current->active_mmx&&current->fp.count==0&&current->fp.top==0,"Returned estimate callback saw an invented MMX file");
            if(mutation==9&&pc==0x2961deu){current->xmm[0]=_mm_set_ps(0.5f,-0.25f,0.125f,2.f);current->xmm[1]=_mm_set_ps(8.f,4.f,-2.f,1.f);}
            if(mutation==10&&pc==0x2961e1u)current->xmm[1]=_mm_set_ps(-4.f,3.f,-2.f,1.f);
            events.push_back(packet(*current,pc));
        }
    };
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;const auto&p=services.instruction_cursor;
        const bool valid=p.position&&p.callback_type&&*p.callback_type==services.instruction.target_type()&&p.executor==std::this_thread::get_id();
        const bool observed=trace||!valid||std::find(p.service_sites.begin(),p.service_sites.end(),pc)!=p.service_sites.end();
        if(observed&&returned)record(pc);
    };
    services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    if(mutation==11)observe=[&](const auto&event){if(event.instruction==0x2961e1u&&++output_stores==1){current->gp[0]+=16u;current->xmm[1]=_mm_set1_ps(17.f);}};
    SourceState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    if(row.contains("active_mmx")){
        SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");
        for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}abi.active_mmx=opaque;
    }
    state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
    services.reciprocal_sqrt=[&](std::uint32_t input){
        check(!state.active_mmx&&state.fp.count==0&&state.fp.top==0,"Estimate provider ran before source EMMS");
        auto value=_mm_castsi128_ps(_mm_set_epi32(0,0,0,std::bit_cast<std::int32_t>(input)));
        asm volatile("rsqrtss %1,%0":"=x"(value):"x"(value):"memory");const auto result=std::bit_cast<std::array<std::uint32_t,4>>(value)[0];
        if(++estimates==1){
            switch(mutation){
            case 0:break;
            case 1:services.instruction={};break;
            case 2:services.instruction=[&](std::uint32_t pc){record(pc);};break;
            case 3:trace=true;break;
            case 4:services.instruction_cursor.position=nullptr;break;
            case 5:services.instruction_cursor.callback_type=nullptr;break;
            case 6:services.instruction_cursor.executor={};break;
            case 7:services.instruction_cursor.literal_service_sites=false;break;
            case 8:services.instruction_cursor.service_sites=suffix_sites;break;
            case 9:case 10:services.instruction=[&](std::uint32_t pc){record(pc);};break;
            case 11:break;
            default:throw std::runtime_error("Unknown estimate policy mutation");
            }
            returned=true;
        }
        if(estimates==stop_after)throw Fault{};
        return result;
    };
    bool stopped{};try{if(native)native_packed_rotation_detail::run_private(state);else reference(state);}catch(const Fault&){stopped=true;}
    check(stopped==bool(stop_after)&&estimates==(stop_after?stop_after:4u),"Source estimate policy test changed real estimate count/exception");
    J banks=J::array();for(const auto&bank:row.at("banks"))banks.push_back(hex(memory.banks.at(bank.at("address"))));
    return J{{"completed",!stopped},{"abi",packet(state,memory.pc)},{"reads",memory.reads},{"writes",memory.writes},{"packets",memory.packets},{"banks",banks},{"callbacks",events},
        {"trace",trace},{"callback_empty",!services.instruction},{"cursor_position_null",!services.instruction_cursor.position},{"literal",services.instruction_cursor.literal_service_sites},
        {"current_type_matches",services.instruction_cursor.callback_type&&services.instruction.target_type()==*services.instruction_cursor.callback_type},
        {"executor_matches",services.instruction_cursor.executor==std::this_thread::get_id()},
        {"service_sites",std::vector<std::uint32_t>(services.instruction_cursor.service_sites.begin(),services.instruction_cursor.service_sites.end())}};
}
template<class SourceState>void policy_owner(const J&fixture,void(*reference)(SourceState&),unsigned&comparisons,unsigned&observations){
    for(const auto&row:fixture.at("cases"))for(unsigned mutation=0;mutation<12;++mutation)for(unsigned stop_after:{0u,2u,4u}){
        const auto expected=execute_rotation_policy(row,mutation,stop_after,false,reference);
        const auto actual=execute_rotation_policy(row,mutation,stop_after,true,reference);
        check(actual==expected,"Packed rotation source policy continuation differs: "+row.at("name").get<std::string>()+" mutation "+std::to_string(mutation)+" stop "+std::to_string(stop_after));
        observations+=actual.at("callbacks").size();++comparisons;
    }
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply actual packed rotation source-policy inputs");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
    const auto host=native_raw_x87_detail::capture();struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{host};
    unsigned comparisons{},observations{};
    policy_owner(fixture,&packed_rotation_source0::run_296180,comparisons,observations);
    policy_owner(fixture,&packed_rotation_source1::run_296180,comparisons,observations);
    policy_owner(fixture,&packed_rotation_source2::run_296180,comparisons,observations);
    std::cout<<J{{"passed",true},{"actual_source_policy_comparisons",comparisons},{"complete_source_suffix_observations",observations},{"source_owners",3},{"policy_cases",12},{"returned_suffix_register_mutations",true},{"earlier_memory_and_before_policy_readonly",true},{"default_qualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

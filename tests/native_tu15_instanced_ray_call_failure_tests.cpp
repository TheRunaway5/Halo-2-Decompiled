#define main callback_fixture_main
#include "native_tu15_instanced_ray_callback_frame_tests.cpp"
#undef main
namespace {
static void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool){throw std::runtime_error("Unprovided canonical collector outcome");}
#include "instanced_callback_forward_reference.inc"
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply retained CALL-failure fixture");std::ifstream in(argv[1]);const auto fixture=J::parse(in);unsigned cases{};
    for(const auto&row:fixture.at("cases")){
        if(row.at("phase")!="normal"||row.at("entry")!=0x24def0u||row.at("name").get<std::string>().find("_basic_")==std::string::npos)continue;
        unsigned failed_write{};bool found{};for(const auto&event:row.at("writes")){if(event.at("instruction")==0x24df14u){found=true;break;}++failed_write;}check(found,"Original memCALL write missing");
        const auto call=std::find_if(row.at("calls").begin(),row.at("calls").end(),[](const auto&packet){return packet.at("entry")==0x24df14u;});check(call!=row.at("calls").end(),"Original before-CALL ABI missing");
        const auto stack_before=call->at("gp")[4].template get<std::uint32_t>();
        J native,reference,reads,writes;std::map<std::uint32_t,std::vector<std::byte>>banks;
        for(bool raw:{true,false}){
            Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));memory.write_fault=int(failed_write);
            SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
            if(!raw)services.instruction=[&](std::uint32_t pc){memory.pc=pc;};
            WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
            incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
            for(unsigned n=0;n<incoming.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],v.data(),10);}state.load(incoming);
            auto before=[&](auto&,std::uint32_t pc){memory.pc=pc;};auto genuine=[](auto&&...){throw std::runtime_error("Unprovided raw collector outcome");};bool failed{};
            try{if(raw)native_instanced_ray_callback_detail::forward_retained(state,before,genuine);else{native_raw_x87_detail::Scope scope{state};canonical_forward(state);}}catch(const Boundary&){failed=true;}
            check(failed&&memory.pc==0x24df14u&&memory.write_count==failed_write+1u,"Provider did not fail after the genuine target read");
            const auto actual_reads=memory.architectural_reads();check(!actual_reads.empty()&&actual_reads.back().at("instruction")==0x24df14u,"CALL target read was suppressed");
            auto actual=packet(state,memory.pc);check(actual.at("gp")[4]==(raw?stack_before:stack_before-4u),"Raw atomic vs retained precommit stack policy lost");
            if(raw){native=actual;reads=actual_reads;writes=memory.writes;banks=memory.banks;}else{reference=actual;check(actual_reads==reads&&memory.writes==writes&&memory.banks==banks,"Retained CALL protocol differs outside the declared stack policy");}
        }
        reference["gp"][4]=native["gp"][4];check(reference==native,"Retained CALL ABI differs outside explicit GP4 precommit policy");++cases;
    }
    check(cases>0,"No actual forward inputs");std::cout<<J{{"passed",true},{"cases",cases},{"actual_target_read_preserved",true},{"raw_atomic_stack_commit",true},{"canonical_predecrement_policy_difference_verified",true},{"physical_post_target_write_failure_claim",false},{"supplied_child_outcomes",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

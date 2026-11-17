#include "physics/runtime/native_tu15_mopp_ray_diagnostic_frame.hpp"
#include "runtime/native_tu15_diagnostic_logger.hpp"
// Reuse the already qualified real formatter/stream reference transport and
// its exact source-copy types; no production provider result is supplied.
#define main diagnostic_stream_reference_main
#include "native_tu15_diagnostic_stream_tests.cpp"
#undef main

namespace {
struct ReturnMemory:Memory {
    const std::set<std::uint32_t>indirect_calls{0x30ae24u,0x30ae38u,0x2e30c5u};
    J attempted_stores=J::array();
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(packet_sites.contains(pc))packets.push_back(capture());
        Memory::read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        attempted_stores.push_back(J{{"instruction",pc},{"address",address},{"written",hex(in)}});
        // The read of an indirect CALL's target is its first CPU subevent.
        // Its atomic return-word write shares that same original packet.
        if(packet_sites.contains(pc)&&!indirect_calls.contains(pc))packets.push_back(capture());
        Memory::write(address,in);
    }
};
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply genuine MOPP diagnostic-return inputs");
    std::ifstream file(argv[1]);const auto fixture=J::parse(file);
    unsigned comparisons{},packets{},getters{},formatted{},buffered{},default_stops{},null_boundaries{},epilogues{},provider_failures{};
    J fault_flags=J::array();
    for(const auto&row:fixture.at("cases")){
        ReturnMemory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
        memory.banks.emplace(fixture.at("immutable_text").at("address"),unhex(fixture.at("immutable_text").at("bytes")));
        for(auto pc:fixture.at("packet_sites"))memory.packet_sites.insert(pc.get<std::uint32_t>());
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        services.instruction=[&](auto pc){memory.pc=pc;};
        bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");
        input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];
        input.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<input.x87_depth;++n){auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}state.load(input);
        memory.capture=[&]{return packet(state,memory.pc);};
        auto before=[&](auto&,auto pc){memory.pc=pc;if(pc==0x30ae42u&&row.value("null_boundary",false)){memory.packets.push_back(packet(state,pc));throw Fault{};}};
        std::function<void(State&,std::uint32_t,std::uint32_t,bool)>call;
        call=[&](auto&s,std::uint32_t target,std::uint32_t,bool){
            memory.pc=target;
            if(target==0x2e1c80u){++default_stops;throw GenuineBoundary{};}
            if(!memory.packet_sites.contains(target))memory.packets.push_back(packet(s,target));
            if(target==0x2e16d0u){++getters;native_diagnostic_logger_detail::run_private(s,before);}
            else if(target==0x2e3080u){++formatted;native_diagnostic_stream_detail::print(s,call,before);}
            else if(target==0x2e30e0u)native_diagnostic_stream_detail::flush(s,call,before);
            else if(target==0x32ba80u)retained::run_32ba80(s);
            else if(target==0x2e3db0u){++buffered;retained::run_2e3db0(s);}
            else if(target==0x2e3e70u)retained::run_2e3e70(s);
            else if(target==0x31d400u){services.instruction(target);s.pop();}
            else throw std::runtime_error("Unexpected real diagnostic owner "+std::to_string(target));
        };
        bool sink{},null{},provider_failure{};retained_child_samples=0;
        try{
            if(row.at("entry")==0x2e16d0u){memory.pc=0x2e16d0u;memory.packets.push_back(packet(state,memory.pc));native_diagnostic_logger_detail::run_private(state,before);}
            else if(row.at("entry")==0x30ae4cu){
                native_raw_x87_detail::Scope scope{state};
                auto never=[](auto&,auto,auto){throw std::runtime_error("Direct epilogue unexpectedly called a child");};
                native_mopp_ray_caller_detail::DiagnosticFrame frame{state,before,scope,never};frame.restore_frame();++epilogues;
            }
            else{
                native_raw_x87_detail::Scope scope{state};
                native_mopp_ray_caller_detail::Frame frame{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}};
                const auto entry=frame.begin();check(!entry.valid_program_class,"Expected actual invalid MOPP byte input");
                auto dispatch=[&](auto&s,auto target,auto site){call(s,target,site,false);};
                native_mopp_ray_caller_detail::DiagnosticFrame diagnostic{state,before,scope,dispatch};diagnostic.invoke();
            }
        }catch(const GenuineBoundary&){sink=true;}catch(const Fault&){null=true;++null_boundaries;}
        catch(const std::runtime_error&error){
            // This is the real Memory::at refusal from the attempted provider
            // write. Do not synthesize store success, NULL backing or a CPU
            // exception result in the native controller.
            if(!row.value("actual_write_fault",false)||memory.pc!=0x30ae42u||
               std::string_view(error.what())!="Unbacked diagnostic memory address=0 pc=3190338")throw;
            provider_failure=true;++provider_failures;
        }
        const auto name=row.at("name").get<std::string>();
        check(sink==row.value("sink_boundary",false)&&null==row.value("null_boundary",false),name+": real boundary ownership differs");
        check(provider_failure==row.value("actual_write_fault",false),name+": native provider-failure ownership differs");
        auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},
            {"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};
        if(provider_failure){
            // Compare every native execution flag to the actual CPU packet
            // immediately before its faulting instruction. Keep the complete
            // separately observed saved exception image, whose RF is set by
            // hardware, distinct from the native provider exception.
            const auto execution=row.at("pre_null_store_packet").at("eflags").get<std::uint32_t>();
            const auto saved=row.at("boundary_eflags").get<std::uint32_t>();
            check(saved==(execution|0x10000u),name+": actual CPU saved-RF relation differs");
            expected["eflags"]=execution;
            check(memory.attempted_stores.size()==memory.writes.size()+1&&
                memory.attempted_stores.back()==J{{"instruction",0x30ae42u},{"address",0u},{"written","00000000"}},name+": actual attempted NULL provider write differs");
            for(const auto&event:memory.reads)check(event.at("address")!=0u,name+": native provider invented a NULL preimage read");
            fault_flags.push_back(J{{"case",name},{"native_execution_eflags",state.save().eflags},{"cpu_before_instruction_eflags",execution},{"cpu_saved_exception_eflags",saved}});
        }
        check(packet(state,memory.pc)==expected,name+": final ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.reads==row.at("reads"),name+": ordered reads differ actual="+memory.reads.dump()+" expected="+row.at("reads").dump());
        check(memory.writes==row.at("writes"),name+": ordered stores differ");
        auto expected_calls=row.at("calls");
        if(memory.packets!=expected_calls){unsigned n{};while(n<memory.packets.size()&&n<expected_calls.size()&&memory.packets[n]==expected_calls[n])++n;
            throw std::runtime_error(name+": complete IO/callee packet differs at"+std::to_string(n)+" actual="+(n<memory.packets.size()?memory.packets[n].dump():"END")+" expected="+(n<expected_calls.size()?expected_calls[n].dump():"END"));}
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": bank differs");
        ++comparisons;packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_io_callee_packets",packets},{"real_getter_calls",getters},{"formatted_messages",formatted},{"buffered_appends",buffered},{"unresumed_default_sink_entries",default_stops},{"null_store_before_instruction_boundaries",null_boundaries},{"separately_declared_raw_epilogue_returns",epilogues},{"genuine_native_unbacked_write_refusals",provider_failures},{"actual_cpu_saved_fault_vs_execution_flags",fault_flags},{"production_binding",false},{"retained_formatter_writer_copy",true}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

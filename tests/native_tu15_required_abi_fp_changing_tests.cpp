#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_integer_fp_scope.hpp"
#include "physics/runtime/native_tu15_required_abi_fp.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "platform/native_tu15_tick_conversion.hpp"
#include "rendering/native_tu15_sphere_frustum_abi.hpp"

namespace {
struct ChangingContext{
    std::uint32_t entry{};const SourcePhysicsWorldRuntimeServices*services{};
    const SourcePhysicsContactRecordObserver*observe{};State**live{};
    SourcePhysicsWorldRuntimeAbi*reference{};bool referenced{},poison{};
    unsigned*entries{},*aliases{};
};
// Each declared provider executes the genuine existing complete numeric child,
// publishes that actual ABI, and only then optionally poisons its host file.
struct ChangingChild{
    const ChangingContext*context{};
    void operator()(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRuntimeCall&call,
        SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsContactRecordObserver&observe)const{
        const auto&c=*context;check(call.target==c.entry&&call.instruction==0x5eed0000u&&!call.tail,"FP-changing declared call descriptor differs");
        check(&observe==c.observe,"FP-changing provider wrapped observer identity");
        if(c.referenced){check(&abi==c.reference,"FP-changing provider copied reference ABI");++*c.aliases;}
        ++*c.entries;State child{memory,observe,*c.services,SourcePhysicsContactImage::TitleUpdate15};child.load(abi);
        auto*previous=*c.live;*c.live=&child;
        try{
            const bool accepted=c.entry==0x22a5c0u?native_tick_conversion_detail::convert(child):
                c.entry==0x184690u?native_sphere_frustum_abi_detail::bridge(child):false;
            check(accepted,"Genuine FP-changing child refused declared input");abi=child.save();
        }catch(...){abi=child.save();*c.live=previous;if(c.poison)hostile();throw;}
        *c.live=previous;if(c.poison)hostile();
    }
    static bool matches(const SourcePhysicsContactRuntimeServices::RequiredOwner&callback,const void*owner)noexcept{
        const auto*actual=callback.target<ChangingChild>();return actual&&actual->context==owner;
    }
};
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply unchanged original FP-changing child fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
    const auto original_host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{original_host};
    unsigned comparisons{},entries{},aliases{},new_x87{},new_mxcsr{};
    for(const auto&row:fixture.at("cases"))for(bool poison:{false,true})for(bool referenced:{false,true}){
        OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.write_sites=row.at("write_sites").get<std::set<std::uint32_t>>();
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;bool trace{};unsigned callbacks{};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction=[&](auto pc){++callbacks;memory.pc=pc;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        state.load(abi);State*live=&state;memory.capture=[&]{return packet(*live,memory.pc);};auto reference=abi;
        const auto before=state.save();const auto host=native_raw_x87_detail::capture();
        const ChangingContext context{row.at("entry"),&services,&observe,&live,&reference,referenced,poison,&entries,&aliases};
        services.required_owner=ChangingChild{&context};services.required_owner_policy={&typeid(ChangingChild),std::this_thread::get_id(),&context,&ChangingChild::matches};
        check(native_required_owner_policy_detail::preserves_instruction_policy(services),"FP-changing readonly policy witness failed");
        {
            native_integer_fp_detail::Scope owner{state};
            owner.child([&]{
                if(referenced){reference=state.save();native_required_abi_fp_detail::invoke(state,reference,[&]{services.required_owner(memory,{0x5eed0000u,row.at("entry"),false},reference,observe);});}
                else{auto actual=state.save();native_required_abi_fp_detail::invoke(state,actual,[&]{services.required_owner(memory,{0x5eed0000u,row.at("entry"),false},actual,observe);});}
            });
        }
        check(!callbacks,"FP-changing child executed retained callbacks");
        check(packet(state,row.at("expected").at("entry"))==row.at("expected"),"FP-changing full returned ABI differs: "+row.at("name").get<std::string>());
        check(memory.writes==row.at("writes"),"FP-changing child ordered writes differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),"FP-changing child bank differs");
        auto expected_host=host;const auto returned=state.save();expected_host.word(0,returned.floating_point->x87_control_word);
        expected_host.word(4,std::uint16_t((returned.x87_status&~0x3800u)|(host.word(4)&0x3800u)));expected_host.mxcsr=returned.floating_point->mxcsr;
        check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),"FP-changing host/source environment handoff differs");
        new_x87+=bool((returned.x87_status&~before.x87_status)&0x3fu);
        new_mxcsr+=bool((returned.floating_point->mxcsr&~before.floating_point->mxcsr)&0x3fu);++comparisons;
    }
    check(new_x87&&new_mxcsr,"FP-changing corpus did not produce new original x87 and MXCSR exception bits");
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"genuine_provider_entries",entries},{"declared_reference_abi_handoffs",aliases},{"new_source_x87_exception_cases",new_x87},{"new_source_mxcsr_exception_cases",new_mxcsr},{"whole_parent_or_default_qualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

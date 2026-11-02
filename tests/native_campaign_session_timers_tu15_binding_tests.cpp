#include "native_campaign_session_timers_test_support.hpp"
#include "halo2/native_campaign_session_timers_tu15_binding.hpp"
#include <cstring>
#include <set>

int main(int argc,char **argv)try{
    check(argc==2,"Supply unchanged physical-original timer caller fixture");
    std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&
        !fixture.at("supplied_original_outcomes").get<bool>(),"Timer caller reference lacks unchanged physical authority");
    std::map<std::string,std::vector<std::byte>> payloads;
    if(fixture.contains("bank_data"))for(const auto &[key,value]:fixture.at("bank_data").items())payloads.emplace(key,bytes(value));
    const auto bank_bytes=[&](const Json &bank,const char *phase){return bank.contains(phase)?bytes(bank.at(phase)):payloads.at(bank.at(std::string(phase)+"_ref"));};
    std::size_t calls{},whole{},partial{},ordered_reads{},stores{},observed{},return_controls{};
    std::set<std::uint32_t> admitted_sites;const auto initial_activity=native_campaign_session_timers_calls_tu15();
    const auto run=[&](const Json &row,bool observing,bool wrong_return,bool fail_return){
        const auto name=row.at("name").get<std::string>();
        check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": missing physical caller evidence");
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bank_bytes(bank,"before"));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("entry_gp").get<std::array<std::uint32_t,8>>();
        const auto sp=abi.gp[4];auto frame=bytes(row.at("entry_stack"));check(frame.size()==12u,name+": genuine timer frame width differs");
        if(wrong_return)for(unsigned n=0;n<4u;++n)frame[n]=std::byte(0x71000000u>>(8u*n));
        const auto found=memory.banks.upper_bound(sp);
        const bool backed=found!=memory.banks.begin()&&std::uint64_t(sp-std::prev(found)->first)+frame.size()<=std::prev(found)->second.size();
        if(backed){const auto previous=std::prev(found);std::copy(frame.begin(),frame.end(),memory.banks.at(previous->first).begin()+sp-previous->first);}
        else check(memory.banks.emplace(sp,frame).second,name+": actual timer stack overlaps another retained bank");
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        if(memory.phase=="before_return_read"||fail_return){memory.phase="before_read";memory.ordinal=row.at("core_reads").size();}
        abi.eflags=row.value("entry_eflags",0x246u);abi.fs_base=0x60200000u;
        abi.xmm=row.at("entry_xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        std::memset(abi.x87.data(),0xa5,sizeof(abi.x87));abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);check(value.size()==10u,"Timer caller x87 payload width differs");std::memcpy(&abi.x87[n],value.data(),10);}
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        const auto old_gp=abi.gp;const auto old_xmm=abi.xmm;const auto old_flags=abi.eflags,old_fs=abi.fs_base,old_depth=abi.x87_depth;
        std::array<std::byte,sizeof(abi.x87)> old_x87{};std::memcpy(old_x87.data(),abi.x87.data(),old_x87.size());
        const auto &metadata=row.at("call");const SourcePhysicsContactRuntimeCall call{metadata.at("instruction"),metadata.at("target"),metadata.at("tail")};
        check(native_campaign_session_timers_tu15_owns_entry(call.target)&&native_campaign_session_timers_tu15_owns_call(call),name+": genuine timer call rejected");
        NativeCampaignTransitionClockTu15 producer;std::size_t consumed{};unsigned attempted{};Json clock_events=Json::array();
        const auto &expected_clocks=row.at("clock_events");
        if(row.at("clock_available").get<bool>())producer=[&](std::uint32_t cell){
            if(cell!=0x60212000u)throw NativeCampaignSessionTimerClockRequiredTu15(cell);
            if(memory.phase=="clock_refresh"&&attempted++==memory.ordinal)throw Prefix{};
            check(consumed<expected_clocks.size(),name+": timer binding consumed an extra clock input");const auto &event=expected_clocks.at(consumed);
            check(event.at("kind")=="Milliseconds"&&event.at("operand")==0&&event.at("instruction")==0x33b595u&&event.at("cell")==cell&&event.at("phase")=="before_volatile_load",name+": timer binding clock authority differs");
            check(event.at("engine_reads_before").get<std::size_t>()+(observing?memory.writes.size():0u)==memory.reads.size()&&event.at("engine_stores_before")==memory.writes.size(),name+": timer binding clock phase differs");
            const auto value=event.at("value").get<std::uint32_t>();std::array<std::byte,4> raw{};for(unsigned n=0;n<4u;++n)raw[n]=std::byte(value>>(8u*n));
            check(event.at("before")==memory.external_clock_before(cell)&&event.at("written")==hex(raw),name+": timer binding clock publication differs");
            memory.external_clock_write(cell,value);clock_events.push_back(event);++consumed;
        };
        std::vector<SourcePhysicsContactRecordStore> events;SourcePhysicsContactRecordObserver observer;
        if(observing)observer=[&](const auto &event){events.push_back(event);};
        bool stopped{},required{},return_required{};HostEnvironment host;
        try{native_campaign_session_timers_tu15_run(memory,call,abi,producer,observer);}
        catch(const Prefix&){stopped=true;}
        catch(const NativeCampaignSessionTimerReturnRequiredTu15 &error){check(wrong_return&&error.target()==0x71000000u,name+": unexpected timer continuation owner");stopped=return_required=true;}
        catch(const NativeCampaignSessionTimerClockRequiredTu15 &error){check(!row.at("boundary").is_null()&&row.at("boundary").at("phase")=="required_clock_owner"&&row.at("boundary").at("cell")==error.cell,name+": timer binding clock dependency differs");stopped=required=true;}
        catch(const Missing &error){check(row.value("actual_read_fault",false)||row.value("actual_write_fault",false),name+": unexpected timer binding source fault");check(error.address==row.at("boundary").at("address")&&error.width==row.at("boundary").at("bytes"),name+": timer binding fault operand differs");stopped=true;}
        check(host.unchanged(),name+": timer binding damaged ambient live FP");
        const bool body_completed=row.at("completed").get<bool>()||fault.value("phase",std::string{})=="before_return_read";
        check(stopped==(!row.at("completed").get<bool>()||wrong_return||fail_return),name+": timer binding completion differs");
        check(return_required==wrong_return,name+": timer binding supplied an unknown continuation");
        if(body_completed)check(abi.gp[0]==row.at("boundary_gp")[0],name+": timer binding real EAX result differs");
        for(unsigned n=1;n<8u;++n)if(n!=4u)check(abi.gp[n]==old_gp[n],name+": timer binding changed held GP state");
        check(abi.gp[4]==sp+(stopped?0u:12u),name+": timer binding true RET8 transport differs");
        check(abi.xmm==old_xmm&&abi.eflags==old_flags&&abi.fs_base==old_fs&&abi.x87_depth==old_depth&&!abi.active_mmx&&std::memcmp(old_x87.data(),abi.x87.data(),old_x87.size())==0,name+": timer binding changed unrelated retained ABI");
        if(body_completed)check(abi.xmm==row.at("boundary_xmm").get<decltype(abi.xmm)>(),name+": timer binding original XMM continuation differs");
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0]&&abi.x87_status==row.at("after_fp")[1]&&abi.floating_point->mxcsr==row.at("after_fp")[2],name+": timer binding changed retained FP policy/status");
        check(memory.writes.size()==row.at("writes").size(),name+": timer binding store count differs");
        for(std::size_t n=0;n<memory.writes.size();++n){const auto &a=memory.writes[n],&b=row.at("writes")[n];check(a.at("address")==b.at("address")&&a.at("before")==b.at("before")&&a.at("written")==b.at("written"),name+": timer binding ordered store differs");}
        if(observing){check(events.size()==row.at("writes").size(),name+": timer observer count differs");for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+": timer observer publication differs");}observed+=events.size();}
        else{
            Json wanted=row.at("core_reads");for(const auto &event:row.at("return_reads"))wanted.push_back(event);
            if(fail_return&&!wanted.empty())wanted.erase(wanted.end()-1);
            check(memory.reads.size()==wanted.size(),name+": timer binding lazy read count differs");
            for(std::size_t n=0;n<wanted.size();++n){const auto &a=memory.reads[n],&b=wanted[n];const auto &value=b.contains("before")?b.at("before"):b.at("value");
                const bool ret=b.at("address")==sp;if(wrong_return&&ret)check(a.at("before")==hex(std::span(frame).first(4u)),name+": timer binding failed wrong actual RET read");
                else check(a.at("before")==value,name+": timer binding read value differs");
                check(a.at("address")==b.at("address")&&a.at("bytes")==b.at("bytes")&&a.at("stores_before")==b.at("stores_before"),name+": timer binding lazy read order differs");}
            ordered_reads+=memory.reads.size();
        }
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bank_bytes(bank,"after"),name+": timer binding retained bytes differ");
        check(clock_events==expected_clocks,name+": timer binding clock replay differs");
        check(required==(!row.at("boundary").is_null()&&row.at("boundary").at("phase")=="required_clock_owner"),name+": timer binding required clock owner bypassed");
        ++calls;whole+=!stopped;partial+=stopped;stores+=memory.writes.size();return_controls+=wrong_return||fail_return;admitted_sites.insert(call.instruction);
    };
    for(const auto &row:fixture.at("cases")){run(row,false,false,false);if(row.at("completed").get<bool>())run(row,true,false,false);}
    for(auto site:{0x77ad3u,0x77bd3u,0x77c1au,0x78204u}){
        const auto found=std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[&](const auto &row){return row.at("completed").template get<bool>()&&row.at("call").at("instruction")==site&&!row.at("writes").empty();});
        check(found!=fixture.at("cases").end(),"Timer reference lacks a mutating completed actual caller");run(*found,false,true,false);run(*found,false,false,true);
    }
    for(const auto &call:{SourcePhysicsContactRuntimeCall{0u,0x777e0u,false},{0x77ad3u,0u,false},{0x77ad3u,0x777e0u,true},{0x77ad8u,0x777e0u,false}}){
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};const auto count=native_campaign_session_timers_calls_tu15();bool rejected{};
        try{native_campaign_session_timers_tu15_run(memory,call,abi,{});}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&memory.reads.empty()&&memory.writes.empty()&&count==native_campaign_session_timers_calls_tu15(),"Unsupported timer admission touched source/counter or supplied a fallback");
    }
    check(native_campaign_session_timers_calls_tu15()==initial_activity+calls,"Timer binding entry activity differs");
    check(admitted_sites.size()==4u&&whole&&partial&&observed&&return_controls==8u,"Timer binding matrix is incomplete");
    std::cout<<calls<<" actual timer caller transports, "<<ordered_reads<<" ordered reads, "<<stores<<" stores, "<<observed<<" observed publications, "<<whole<<" complete and "<<partial<<" partial stops passed; four real callsites and true EAX/RET8 retained\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

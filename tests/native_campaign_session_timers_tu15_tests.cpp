#include "native_campaign_session_timers_test_support.hpp"
int main(int argc,char **argv)try{
    check(argc==2,"Supply unchanged physical-original session timers fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","session timers original image changed");
    check(fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"session timers reference lacks unchanged physical authority");
    check(fixture.at("external_input_format")=="actual-platform-Milliseconds-at-33B595-v1","Timer reference lacks actual platform-input capture authority");
    std::map<std::string,std::vector<std::byte>> payloads;if(fixture.contains("bank_data"))for(const auto &[hash,value]:fixture.at("bank_data").items())payloads.emplace(hash,bytes(value));
    auto bank_bytes=[&](const Json &bank,const char *phase){return bank.contains(phase)?bytes(bank.at(phase)):payloads.at(bank.at(std::string(phase)+"_ref"));};
    auto events=[&](const Json &row,const char *field){
        if(row.contains(field))return row.at(field);
        const auto &ref=row.at(std::string(field)+"_prefix");const auto &seed=fixture.at("cases").at(ref.at("seed").get<std::size_t>());
        Json result=Json::array();for(std::size_t n=0;n<ref.at("count").get<std::size_t>();++n)result.push_back(seed.at(field).at(n));return result;
    };
    std::size_t count{},reads{},writes{},whole{},prefixes{},faults{};
    for(const auto &row:fixture.at("cases")){
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": missing session timers physical authority");
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bank_bytes(bank,"before"));
        const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
        bool stopped{},required{};std::optional<std::uint32_t> returned;HostEnvironment host;
        const auto expected_clocks=events(row,"clock_events");std::size_t consumed_clocks{},attempted_clocks{};Json clock_events=Json::array();
        NativeCampaignTransitionClockTu15 producer;
        if(row.at("clock_available").get<bool>())producer=[&](std::uint32_t cell){
            // Existing native World data binding, independently authenticated
            // by the reference. A forged IAT value is not a new clock owner.
            if(cell!=0x60212000u)throw NativeCampaignSessionTimerClockRequiredTu15(cell);
            if(memory.phase=="clock_refresh"&&attempted_clocks++==memory.ordinal)throw Prefix{};
            check(consumed_clocks<expected_clocks.size(),name+": unexpected clock input consumption");
            const auto &event=expected_clocks.at(consumed_clocks);
            check(event.at("kind")=="Milliseconds"&&event.at("operand")==0&&event.at("instruction")==0x33b595u&&event.at("phase")=="before_volatile_load",name+": unauthenticated platform clock event");
            check(event.at("cell")==cell&&event.at("engine_reads_before")==memory.reads.size()&&event.at("engine_stores_before")==memory.writes.size(),name+": clock input phase differs");
            const auto value=event.at("value").get<std::uint32_t>();std::array<std::byte,4> encoded{};for(unsigned n=0;n<4;++n)encoded[n]=std::byte(value>>(8u*n));
            check(event.at("before")==memory.external_clock_before(cell)&&event.at("written")==hex(encoded),name+": platform clock publication differs");
            memory.external_clock_write(cell,event.at("value"));clock_events.push_back(event);++consumed_clocks;
        };
        try{
            check(row.at("entry")==0x777e0u,"Unknown session-timer owner");
            returned=native_campaign_session_retire_timers_value_tu15(memory,row.at("record_filter"),row.at("slot"),producer);
        }catch(const Prefix&){stopped=true;}
        catch(const NativeCampaignSessionTimerClockRequiredTu15 &owner){
            const auto &boundary=row.at("boundary");check(!boundary.is_null()&&boundary.at("phase")=="required_clock_owner",name+": unexpected required clock owner");
            check(boundary.at("instruction")==owner.instruction&&boundary.at("cell")==owner.cell,name+": required clock owner differs");stopped=true;required=true;
        }catch(const Missing &error){
            check(row.value("actual_read_fault",false)||row.value("actual_write_fault",false),name+": unexpected session timers source address "+std::to_string(error.address));
            check(error.address==row.at("boundary").at("address")&&error.width==row.at("boundary").at("bytes"),name+": session timers fault operand differs");stopped=true;++faults;
        }
        check(host.unchanged(),name+": session timers kernel damaged ambient live FP");
        check(stopped==!row.at("completed").get<bool>(),name+": session timers completion differs");
        if(!stopped)check(returned&&*returned==row.at("boundary_gp").at(0),name+": session timers continuation value differs");
        else check(!returned,name+": interrupted session timers returned a manufactured value");
        check(row.at("before_fp")==row.at("after_fp"),name+": original data-only metadata owner changed FP");
        auto expected_reads=events(row,"reads");const auto expected_writes=events(row,"writes");
        if(!row.at("boundary").is_null())for(const auto *field:{"uncommitted_copy_reads","uncommitted_reads"})if(row.at("boundary").contains(field))for(const auto &event:row.at("boundary").at(field))expected_reads.push_back(event);
        std::string first_read_difference;
        for(std::size_t n=0;n<std::min(memory.reads.size(),expected_reads.size());++n){const auto &a=memory.reads[n],&b=expected_reads[n];const auto &before=b.contains("before")?b.at("before"):b.at("value");if(a.at("address")!=b.at("address")||a.at("bytes")!=b.at("bytes")||a.at("before")!=before||a.at("stores_before")!=b.at("stores_before")){first_read_difference=" first at "+std::to_string(n)+" native="+a.dump()+" original="+b.dump();break;}}
        check(memory.reads.size()==expected_reads.size(),name+": session timers ordered read count differs native="+std::to_string(memory.reads.size())+" original="+std::to_string(expected_reads.size())+first_read_difference);
        for(std::size_t n=0;n<memory.reads.size();++n){const auto &a=memory.reads[n],&b=expected_reads[n];const auto &before=b.contains("before")?b.at("before"):b.at("value");check(a.at("address")==b.at("address")&&a.at("bytes")==b.at("bytes")&&a.at("before")==before&&a.at("stores_before")==b.at("stores_before"),name+": session timers ordered reads differ at "+std::to_string(n)+" native="+a.dump()+" original="+b.dump());}
        check(memory.writes.size()==expected_writes.size(),name+": session timers ordered store count differs");
        for(std::size_t n=0;n<memory.writes.size();++n){const auto &a=memory.writes[n],&b=expected_writes[n];check(a.at("address")==b.at("address")&&a.at("before")==b.at("before")&&a.at("written")==b.at("written"),name+": session timers ordered stores differ at "+std::to_string(n));}
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bank_bytes(bank,"after"),name+": retained session timers bytes differ");
        check(clock_events==expected_clocks,name+": recorded platform clock replay differs");
        check(required==(!row.at("boundary").is_null()&&row.at("boundary").at("phase")=="required_clock_owner"),name+": required clock owner was bypassed");
        ++count;reads+=memory.reads.size();writes+=memory.writes.size();whole+=!stopped;prefixes+=stopped;
    }
    check(count&&whole,"session timers matrix is empty");std::cout<<count<<" physical original session timers cases, "<<reads<<" ordered reads, "<<writes<<" stores, "<<prefixes<<" prefixes, "<<faults<<" actual faults, "<<whole<<" whole passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

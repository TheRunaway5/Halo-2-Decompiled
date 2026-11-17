#include "native_tu15_marker_owner_test_support.hpp"
int main(int argc,char**argv)try{
    const bool canonical=argc==3&&std::string_view(argv[1])=="--canonical";
    check(argc==2||canonical,"Supply marker-owner original fixture");std::ifstream input_file(argv[canonical?2:1]);const auto fixture=J::parse(input_file);
    unsigned comparisons{},packet_count{},child_calls{},values{};
    for(const auto&original:fixture.at("cases"))for(unsigned mode=0;mode<3;++mode){
        auto row=canonical?marker_canonical_projection(original):original;
        if(canonical)for(auto&site:row["write_sites"]){const auto pc=site.get<std::uint32_t>();if(pc>=0xbacd0u&&pc<=0xbad51u)site=pc-0x1140u;}
        const bool noisy=mode!=0;
        OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.noisy=noisy;memory.write_sites=row.at("write_sites").get<std::set<std::uint32_t>>();
        if(row.contains("fault"))memory.fault=row.at("fault");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        if(mode==2||(memory.fault&&memory.fault->at("phase")=="after_write"))observe=[&](const auto&event){
            check(event.instruction==memory.pc&&hex(event.written)==memory.writes.back().at("written").get<std::string>(),"Marker-owner observer store differs");
            if(noisy)hostile();
            if(memory.fault&&memory.fault->at("phase")=="after_write"&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};
        };
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.xmm=row.at("xmm");input.eflags=row.at("eflags");
        input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];
        input.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<input.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&input.x87[n],value.data(),10);}
        state.load(input);memory.capture=[&]{return packet(state,memory.pc);};
        auto before=[&](auto&,auto pc){memory.pc=pc;if(noisy)hostile();};
        bool stopped{};try{native_marker_owner_detail::run_private(state,[&](auto&callee,auto target,auto site){
            check(target==0xfc510u&&site==(canonical?0xb9bf9u:0xbad39u),"Unexpected marker-owner child");
            native_object_parent_mode_detail::run_private(callee,before);++child_calls;
        },before,canonical?native_marker_owner_detail::Observation::canonical:native_marker_owner_detail::Observation::actual);}catch(const Fault&){stopped=true;}
        check(stopped==row.contains("fault"),"Marker-owner completion differs");
        check(packet(state,memory.pc)==row.at("expected"),"Marker-owner final ABI differs case "+row.at("name").get<std::string>()+" actual="+packet(state,memory.pc).dump()+" expected="+row.at("expected").dump());
        check(memory.reads==row.at("reads"),"Marker-owner ordered reads differ case "+row.at("name").get<std::string>()+" actual="+memory.reads.dump()+" expected="+row.at("reads").dump());
        check(memory.writes==row.at("writes"),"Marker-owner ordered stores differ");
        if(memory.packets!=row.at("calls")){
            const auto&expected=row.at("calls");std::size_t index{};
            while(index<memory.packets.size()&&index<expected.size()&&memory.packets[index]==expected[index])++index;
            throw std::runtime_error("Marker-owner complete pre-I/O packets differ case "+row.at("name").get<std::string>()+" index="+std::to_string(index)+" actual="+(index<memory.packets.size()?memory.packets[index].dump():"missing")+" expected="+(index<expected.size()?expected[index].dump():"missing"));
        }
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),"Marker-owner banks differ");
        packet_count+=memory.packets.size();++comparisons;
        if(!noisy&&!stopped&&row.value("value_domain",true)){
            OwnerMemory value_memory;for(const auto&bank:row.at("banks"))value_memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            const auto value=native_marker_owner_detail::value([&](auto address,unsigned width){
                std::uint32_t result{};std::memcpy(&result,value_memory.at(address,width),width);return result;
            },row.at("matrix"));
            check(value==row.at("expected").at("gp")[0].get<std::uint32_t>(),"Marker-owner typed value differs");++values;
        }
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packet_count},{"genuine_native_child_calls",child_calls},{"typed_value_cases",values}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

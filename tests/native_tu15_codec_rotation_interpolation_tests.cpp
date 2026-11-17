#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_rotation_interpolation_frame.hpp"

namespace{
void packed_rotation_host(unsigned index){
    auto image=native_raw_x87_detail::capture();const auto top=index&7u;
    image.word(4,std::uint16_t(top<<11u));image.word(8,0xffffu);
    constexpr std::array values{"0000000000000080ff3f","00000000000000000080","01000000000000000000","45230100000000c0ff7f","5634020000000080ff7f"};
    for(unsigned n=0;n<8;++n){
        const auto raw=bytes(values[(index+n)%values.size()]);std::memcpy(image.x87.data()+28+n*10,raw.data(),10);
        const bool live=index%3==0?n<4:index%3==1?n%2==0:true;
        if(live){const auto shift=((top+n)&7u)*2u;const auto tag=index%3==2?0u:native_raw_x87_detail::value_tag(raw.data());image.word(8,std::uint16_t((image.word(8)&~(3u<<shift))|(tag<<shift)));}
    }
    native_raw_x87_detail::restore(image);
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply rotation interpolation original fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
    const auto original_host=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{original_host};
    unsigned comparisons{},packets{},estimates{};
    for(const auto&row:fixture.at("cases"))for(bool noisy:{false,true}){
        packed_rotation_host(comparisons);
        OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.write_sites={0x29626au,0x29627du,0x296283u,0x296287u,0x2962cfu};memory.noisy=noisy;if(row.contains("fault"))memory.fault=row.at("fault");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        unsigned callbacks{},local_estimates{};bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[&](auto pc){++callbacks;memory.pc=pc;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        if(row.contains("active_mmx")){
            SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");
            for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}abi.active_mmx=opaque;
        }
        state.load(abi);const auto host=native_raw_x87_detail::capture();memory.capture=[&]{return packet(state,memory.pc);};unsigned observed_writes{};
        if(row.value("phase",std::string{})=="after_write")observe=[&](const auto&){
            if(noisy)hostile();if(observed_writes++==row.value("native_observer_ordinal",row.at("fault").at("ordinal").get<unsigned>()))throw Fault{};
        };
        services.reciprocal_sqrt=[&](std::uint32_t input){
            check(memory.pc==0x2962c6u,"Rotation interpolation changed estimate source PC");
            if(!local_estimates)memory.packets.push_back(memory.capture());
            else check(memory.capture()==memory.packets.back(),"Rotation interpolation changed source ABI between estimate lanes");
            auto value=_mm_castsi128_ps(_mm_set_epi32(0,0,0,std::bit_cast<std::int32_t>(input)));
            asm volatile("rsqrtss %1,%0":"=x"(value):"x"(value):"memory");
            const auto result=std::bit_cast<std::array<std::uint32_t,4>>(value)[0];
            ++local_estimates;if(noisy)hostile();return result;
        };
        bool stopped{},alignment{};std::uint32_t saved_flags{};
        try{native_codec_rotation_interpolation_detail::run_private(state,[&](auto&,auto pc){memory.pc=pc;
            if(row.value("alignment_fault",false)&&pc==row.at("alignment_entry"))memory.packets.push_back(memory.capture());});}
        catch(const Fault&){stopped=true;}
        catch(const native_codec_rotation_interpolation_detail::AlignedAccessFault&fault){
            stopped=alignment=true;saved_flags=fault.saved_fault_flags;
            check(fault.instruction==row.at("alignment_entry")&&fault.address==row.at("alignment_address")&&fault.execution_flags==row.at("execution_flags_before_fault"),"Rotation interpolation alignment fault context differs");
        }
        const auto name=row.at("name").get<std::string>();check(stopped==row.contains("phase")&&alignment==row.value("alignment_fault",false),name+": rotation interpolation completion differs");
        bool estimated{};for(const auto&call:row.at("calls"))estimated|=call.at("entry")==0x2962c6u;
        check(!callbacks&&local_estimates==(estimated?4u:0u),"Rotation interpolation changed actual estimate/callback count");
        auto returned=packet(state,row.at("expected").at("entry"));if(alignment)returned["eflags"]=saved_flags;
        check(returned==row.at("expected"),name+": rotation interpolation full returned/stopped ABI differs");
        auto expected_host=host;const auto retained=state.save();expected_host.word(0,retained.floating_point->x87_control_word);
        expected_host.word(4,std::uint16_t((retained.x87_status&~0x3800u)|(host.word(4)&0x3800u)));expected_host.mxcsr=retained.floating_point->mxcsr;
        check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": rotation interpolation lost host payload/tags/TOP or source control/status");
        check(memory.reads==row.at("reads"),name+": rotation interpolation ordered reads differ");
        J writes=J::array();for(unsigned n=0;n<memory.writes.size();++n){auto event=memory.writes[n];
            if(event.at("instruction")==0x2962cfu){check(n+1<memory.writes.size(),"Missing second retained vector half");const auto&next=memory.writes[++n];
                check(next.at("instruction")==event.at("instruction")&&next.at("address").get<std::uint32_t>()==event.at("address").get<std::uint32_t>()+8u,"Retained vector half order differs");
                event["before"]=event.at("before").get<std::string>()+next.at("before").get<std::string>();event["written"]=event.at("written").get<std::string>()+next.at("written").get<std::string>();}
            writes.push_back(event);
        }
        check(writes==row.at("writes"),name+": rotation interpolation whole-vector write projection differs");
        J observed=J::array();bool vector_seen{};for(const auto&p:memory.packets){if(p.at("entry")==0x2962cfu){if(vector_seen){check(p==observed.back(),"Retained vector halves changed CPU ABI");continue;}vector_seen=true;}observed.push_back(p);}
        check(observed==row.at("calls"),name+": rotation interpolation original pre-I/O packets differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": rotation interpolation bank differs");
        estimates+=local_estimates;packets+=observed.size();++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_original_pre_io_packets",packets},{"actual_reciprocal_estimates",estimates},{"retained_vector_half_atomicity_qualified",false},{"default_qualified",false},{"private_unbound",true}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

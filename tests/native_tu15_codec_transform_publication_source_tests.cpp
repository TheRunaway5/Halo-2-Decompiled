#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_transform_publication_frame.hpp"
#include "codec_transform_publication_sources.inc"

namespace {
struct Boundary {std::uint32_t pc;};
struct SourceMemory:OwnerMemory {
    mutable bool combined_read{};
    mutable unsigned preimages{};
    mutable J preimage_packets=J::array();
    std::function<void(std::uint32_t)> after_preimage,after_write;
    static bool repeat(std::uint32_t pc){return pc==0x283fa7u;}
    static bool rmw(std::uint32_t pc){return pc==0x283fc8u||pc==0x283ff2u;}
    static bool combined(std::uint32_t pc){return repeat(pc)||rmw(pc);}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(write_sites.contains(pc)||(combined(pc)&&combined_read)) {
            preimage_packets.push_back(capture());
            if(fault&&fault->at("phase")=="before_preimage"&&preimages++==fault->at("ordinal"))throw Fault{};
            std::memcpy(out.data(),at(address,out.size()),out.size());
            if(after_preimage)after_preimage(pc);return;
        }
        OwnerMemory::read(address,out);
        if(combined(pc))combined_read=true;
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        OwnerMemory::write(address,in);
        if(combined(pc))combined_read=false;
        if(after_write)after_write(pc);
    }
};
using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
struct Result {J observations;Banks banks;bool operator==(const Result&)const=default;};

template<bool Prepared,class S>Result execute(const J& row,const J& fixture,const Banks& seed,void(*reference)(S&),
                                 bool candidate,unsigned observer_mode) {
    SourceMemory memory;memory.banks=seed;
    for(const auto& pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
    if(row.contains("fault"))memory.fault=row.at("fault");
    SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
    const auto stop=row.value("stop",0u),occurrence=row.value("repeat_occurrence",0u);
    const auto callback_stop=row.value("callback_stop",0u);
    unsigned completed{},repeat_count{};bool observer_changed{},trace{},policy_changed{};
    J repeat_packets=J::array(),observer_packets=J::array(),instruction_packets=J::array();
    const auto record_instruction=[&](std::uint32_t pc) {
        if(pc==callback_stop)throw Boundary{pc};
        if(!candidate&&pc==stop&&(!SourceMemory::repeat(pc)||repeat_count==occurrence))throw Boundary{pc};
        memory.pc=pc;
        instruction_packets.push_back(memory.capture());
        if(SourceMemory::repeat(pc)){++repeat_count;repeat_packets.push_back(memory.capture());}
    };
    services.instruction=record_instruction;
    static constexpr std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,0x33b595u};
    if(observer_mode>=4u)services.instruction_cursor={
        &memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,false};
    S state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("fp")[1];
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("fp")[0],row.at("fp")[2]};
    state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
    const auto install=[&](unsigned version){
        observer=[&,version](const SourcePhysicsContactRecordStore& event) {
            observer_packets.push_back(J{{"version",version},{"packet",memory.capture()},{"address",event.address},
                {"before",hex(event.before)},{"written",hex(event.written)}});
            if(memory.fault&&memory.fault->at("phase")=="after_write"&&completed++==memory.fault->at("ordinal"))throw Fault{};
        };
    };
    if(observer_mode!=1u)install(1u);
    memory.after_preimage=[&](std::uint32_t pc) {
        if(observer_mode==2u&&!observer_changed&&SourceMemory::rmw(pc)){observer={};observer_changed=true;}
    };
    memory.after_write=[&](std::uint32_t pc) {
        if((observer_mode==1u||observer_mode==3u)&&!observer_changed&&SourceMemory::rmw(pc)){
            install(2u);observer_changed=true;
        }
        if(observer_mode>=5u&&!policy_changed&&SourceMemory::repeat(pc)) {
            policy_changed=true;
            if(observer_mode==5u)trace=true;
            else services.instruction=[&](std::uint32_t at){record_instruction(at);};
        }
    };
    std::uint32_t continuation{};bool interrupted{};
    try {
        if(candidate)halo2::native_codec_transform_publication_detail::run_private_source_suffix<Prepared>(state,[&](S&,std::uint32_t pc) {
            const auto next=fixture.at("successors").at(std::to_string(pc)).get<std::uint32_t>();
            if(next==stop&&(!SourceMemory::repeat(pc)||repeat_count==occurrence))throw Boundary{next};
        });
        else reference(state);
    }catch(const Boundary& next){continuation=next.pc;}
    catch(const Fault&){interrupted=true;}
    check(continuation==(callback_stop?callback_stop:stop)&&interrupted==row.contains("fault"),"Wrong retained publication retirement/fault");
    const auto abi_pc=interrupted||continuation?memory.pc:0x28404au;
    return {J{{"abi",packet(state,abi_pc)},{"reads",std::move(memory.reads)},{"writes",std::move(memory.writes)},
        {"packets",std::move(memory.packets)},{"preimages",std::move(memory.preimage_packets)},
        {"observers",std::move(observer_packets)},{"repeats",std::move(repeat_packets)},{"instructions",std::move(instruction_packets)},
        {"observer_changed",observer_changed},{"observer_present",bool(observer)},
        {"live_pc",memory.pc},{"policy_changed",policy_changed},
        {"continuation",continuation},{"fault",interrupted}},std::move(memory.banks)};
}
void compare(const J& row,const Result& expected,const Result& actual) {
    if(expected==actual)return;
    std::cerr<<row.at("name")<<'\n';
    for(auto i=expected.observations.begin();i!=expected.observations.end();++i)if(i.value()!=actual.observations.at(i.key())) {
        const auto& other=actual.observations.at(i.key());
        if(i.value().is_array()&&other.is_array()) {
            std::cerr<<i.key()<<" count "<<i.value().size()<<"/"<<other.size()<<'\n';
            for(std::size_t n=0;n<std::min(i.value().size(),other.size());++n)if(i.value()[n]!=other[n]) {
                std::cerr<<"expected "<<i.value()[n].dump()<<" actual "<<other[n].dump()<<'\n';break;
            }
        }else std::cerr<<i.key()<<" expected "<<i.value().dump()<<" actual "<<other.dump()<<'\n';
    }
    throw std::runtime_error("Retained publication packets differ");
}
struct Totals {
    unsigned comparisons{},faults{},frontiers{},repeat_frontiers{},observer_feedback{},rmw_faults{};
    unsigned instruction_frontiers{},prepared_quiet_repeat{},policy_feedback{};
    J rmw_witness;
};
template<bool Prepared,class S>void run(const J& fixture,void(*reference)(S&),Totals& totals) {
    std::set<std::pair<std::string,std::uint32_t>> tested_faults;
    std::set<std::uint32_t> tested_frontiers;
    std::set<std::uint32_t> tested_instructions;
    for(const auto& row:fixture.at("cases")) {
        Banks seed;for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        for(unsigned mode=0;mode<7u;++mode) {
            const Result expected=execute<Prepared>(row,fixture,seed,reference,false,mode);
            compare(row,expected,execute<Prepared>(row,fixture,seed,reference,true,mode));++totals.comparisons;
            if(expected.observations.at("observer_changed"))++totals.observer_feedback;
            if(expected.observations.at("policy_changed"))++totals.policy_feedback;
            if constexpr(Prepared)if(mode==4u&&!expected.observations.at("repeats").empty()){
                const auto& repeats=expected.observations.at("repeats");
                check(repeats.size()%8u==0u&&repeats.back().at("gp")[1]==0u&&
                      repeats.size()==expected.observations.at("instructions").size(),
                      "Quiet prepared REP did not deliver only the direct trailing callbacks");
                ++totals.prepared_quiet_repeat;
            }
            if(mode)continue;
            auto test=[&](const J& prefix) {
                const Result stopped=execute<Prepared>(prefix,fixture,seed,reference,false,0u);
                compare(prefix,stopped,execute<Prepared>(prefix,fixture,seed,reference,true,0u));++totals.comparisons;
                return stopped;
            };
            for(const J& entry:expected.observations.at("instructions")) {
                const auto pc=entry.at("entry").get<std::uint32_t>();
                if(fixture.at("successors").contains(std::to_string(pc))||!tested_instructions.insert(pc).second)continue;
                auto prefix=row;prefix["name"]="numeric-callback-"+std::to_string(pc);prefix["callback_stop"]=pc;
                test(prefix);++totals.instruction_frontiers;
            }
            for(const auto* stream:{"reads","writes"}) {
                unsigned ordinal{};
                for(const J& access:expected.observations.at(stream)) {
                    const auto pc=access.at("instruction").get<std::uint32_t>();
                    const bool reading=std::string_view(stream)=="reads";
                    for(const auto* phase:reading?std::vector{"before_read"}:std::vector{"before_preimage","before_write","after_write"}) {
                        if(!tested_faults.emplace(phase,pc).second)continue;
                        auto prefix=row;prefix["name"]=std::string(phase)+"-"+std::to_string(pc);
                        prefix["fault"]=J{{"phase",phase},{"ordinal",ordinal}};
                        const Result stopped=test(prefix);++totals.faults;
                        if(SourceMemory::rmw(pc)&&!reading&&std::string_view(phase)!="after_write"){
                            if(!totals.rmw_faults)totals.rmw_witness=J{{"name",prefix["name"]},{"abi",stopped.observations["abi"]}};
                            ++totals.rmw_faults;
                        }
                    }
                    const auto key=std::to_string(pc);
                    if(fixture.at("successors").contains(key)&&tested_frontiers.insert(pc).second) {
                        auto prefix=row;prefix["name"]="retired-"+key;prefix["stop"]=fixture.at("successors").at(key);
                        if(SourceMemory::repeat(pc)) {
                            for(unsigned n=1;n<=8u;++n) {prefix["repeat_occurrence"]=n;test(prefix);++totals.repeat_frontiers;}
                        }else{test(prefix);++totals.frontiers;}
                    }
                    ++ordinal;
                }
            }
        }
    }
    check(tested_frontiers.size()==fixture.at("successors").size(),"Retained publication retirement set incomplete");
}
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply genuine captured publication suffix inputs");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto original=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{original};
    Totals totals;
    run<false>(fixture,&publication_source0::legacy,totals);run<true>(fixture,&publication_source1::legacy,totals);
    run<false>(fixture,&publication_source2::legacy,totals);run<true>(fixture,&publication_source3::legacy,totals);
    check(totals.rmw_faults==16u&&totals.observer_feedback>0u&&totals.prepared_quiet_repeat>0u&&totals.policy_feedback>0u,
          "Missing actual source OR, quiet REP, or observer/policy witnesses");
    std::cout<<J{{"passed",true},{"copies",4},{"comparisons",totals.comparisons},{"provider_prefixes",totals.faults},
        {"exact_retirements",totals.frontiers},{"exact_repeat_retirements",totals.repeat_frontiers},
        {"numeric_callback_prefixes",totals.instruction_frontiers},{"prepared_quiet_repeat",totals.prepared_quiet_repeat},
        {"instruction_policy_feedback",totals.policy_feedback},
        {"observer_feedback",totals.observer_feedback},{"rmw_provider_prefixes",totals.rmw_faults},
        {"first_rmw_source_fault",totals.rmw_witness}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

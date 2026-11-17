#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_transform_composition_source_frame.hpp"
#include "codec_transform_composition_sources.inc"

namespace {
struct Boundary {std::uint32_t pc;};
struct SourceMemory:OwnerMemory {
    mutable unsigned preimages{};
    mutable bool after_repeat_read{};
    mutable J preimage_packets=J::array();
    static bool repeat(std::uint32_t pc){return pc==0x12058eu||pc==0x1205a3u;}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(write_sites.contains(pc)||(repeat(pc)&&after_repeat_read)) {
            preimage_packets.push_back(capture());
            if(fault&&fault->at("phase")=="before_preimage"&&preimages++==fault->at("ordinal"))throw Fault{};
            std::memcpy(out.data(),at(address,out.size()),out.size());return;
        }
        OwnerMemory::read(address,out);
        if(repeat(pc))after_repeat_read=true;
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        OwnerMemory::write(address,in);
        if(repeat(pc))after_repeat_read=false;
    }
};
using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
struct Result {J observations;Banks banks;bool operator==(const Result&)const=default;};

template<bool Ordered,class S>
Result execute(const J& row,const J& fixture,const Banks& seed,void(*reference)(S&),void(*child)(S&),bool candidate) {
    SourceMemory memory;memory.banks=seed;
    for(const auto& pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
    if(row.contains("fault"))memory.fault=row.at("fault");
    SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
    const auto stop=row.value("stop",0u),occurrence=row.value("repeat_occurrence",0u);
    unsigned completed{},child_calls{};std::map<std::uint32_t,unsigned>repeat_counts;
    J repeat_packets=J::array();
    services.instruction=[&](std::uint32_t pc) {
        if(!candidate&&pc==stop&&(!SourceMemory::repeat(pc)||repeat_counts[pc]==occurrence))throw Boundary{pc};
        memory.pc=pc;
        if(SourceMemory::repeat(pc)){++repeat_counts[pc];repeat_packets.push_back(memory.capture());}
        if(pc==0x1201e0u)++child_calls;
    };
    S state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    if(row.contains("active_mmx")) {
        SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");
        for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}
        abi.active_mmx=opaque;
    }
    state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
    J observer_packets=J::array();
    observer=[&](const SourcePhysicsContactRecordStore& event) {
        observer_packets.push_back(J{{"packet",memory.capture()},{"address",event.address},
            {"before",hex(event.before)},{"written",hex(event.written)}});
        if(memory.fault&&memory.fault->at("phase")=="after_write"&&completed++==memory.fault->at("ordinal"))throw Fault{};
    };
    std::uint32_t continuation{};bool interrupted{};
    try {
        if(candidate) {
            namespace c=halo2::native_codec_transform_composition_detail;
            c::run_private_source<Ordered>(state,
                [&](S& s,std::uint32_t target,std::uint32_t site) {
                    check(target==0x1201e0u&&site==0x1205adu,"Wrong genuine composition child");child(s);
                },c::Noop{},[&](S&,std::uint32_t pc) {
                    const auto next=fixture.at("successors").at(std::to_string(pc)).get<std::uint32_t>();
                    if(next==stop&&(!SourceMemory::repeat(pc)||repeat_counts[pc]==occurrence))throw Boundary{next};
                });
        } else reference(state);
    }catch(const Boundary& next){continuation=next.pc;}
    catch(const Fault&){interrupted=true;}
    check(continuation==stop&&interrupted==row.contains("fault"),"Wrong retained composition retirement/fault");
    return {J{{"abi",memory.capture()},{"reads",std::move(memory.reads)},{"writes",std::move(memory.writes)},
        {"packets",std::move(memory.packets)},{"preimages",std::move(memory.preimage_packets)},
        {"observers",std::move(observer_packets)},{"repeats",std::move(repeat_packets)},
        {"child_calls",child_calls},{"continuation",continuation},{"fault",interrupted}},std::move(memory.banks)};
}
void compare(const J& row,const Result& expected,const Result& actual) {
    if(expected==actual)return;
    std::cerr<<row.at("name")<<'\n';
    for(auto i=expected.observations.begin();i!=expected.observations.end();++i)if(i.value()!=actual.observations.at(i.key())) {
        const auto& other=actual.observations.at(i.key());
        if(i.value().is_array()&&other.is_array()) {
            std::cerr<<i.key()<<" count "<<i.value().size()<<"/"<<other.size()<<'\n';
            for(std::size_t n=0;n<std::min(i.value().size(),other.size());++n)if(i.value()[n]!=other[n]){
                std::cerr<<"expected "<<i.value()[n].dump()<<" actual "<<other[n].dump()<<'\n';break;
            }
        }else std::cerr<<i.key()<<" expected "<<i.value().dump()<<" actual "<<other.dump()<<'\n';
    }
    throw std::runtime_error("Retained transform composition packets differ");
}
struct Totals {
    unsigned comparisons{},faults{},frontiers{},repeat_frontiers{},child_frontiers{},legacy_differences{},unordered_differences{};
    J first_legacy_difference,first_unordered_difference;
};
J first_difference(const J& row,const Result& expected,const Result& actual) {
    J difference={{"name",row.at("name")},{"expected_return",expected.observations.at("abi")},
                  {"actual_return",actual.observations.at("abi")}};
    const auto& a=expected.observations.at("packets");const auto& b=actual.observations.at("packets");
    for(std::size_t n=0;n<std::min(a.size(),b.size());++n)if(a[n]!=b[n]) {
        difference["first_different_pre_io"]=J::array({a[n],b[n]});break;
    }
    return difference;
}
template<class S>void run(const J& fixture,void(*ordered)(S&),void(*legacy)(S&),
                          void(*ordered_child)(S&),void(*legacy_child)(S&),Totals& totals) {
    std::set<std::pair<std::string,std::uint32_t>> tested_faults;
    std::set<std::uint32_t> tested_frontiers;
    for(const auto& row:fixture.at("cases")) {
        Banks seed;for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        const Result expected=execute<true>(row,fixture,seed,ordered,ordered_child,false);
        compare(row,expected,execute<true>(row,fixture,seed,ordered,ordered_child,true));++totals.comparisons;
        const Result old=execute<false>(row,fixture,seed,legacy,legacy_child,false);
        if(!(expected==old)) {
            if(!totals.legacy_differences)totals.first_legacy_difference=first_difference(row,expected,old);
            ++totals.legacy_differences;
        }
        const Result unordered=execute<false>(row,fixture,seed,legacy,legacy_child,true);
        if(!(old==unordered)) {
            if(!totals.unordered_differences)totals.first_unordered_difference=first_difference(row,old,unordered);
            ++totals.unordered_differences;
        }
        auto test=[&](const J& prefix){
            compare(prefix,execute<true>(prefix,fixture,seed,ordered,ordered_child,false),
                           execute<true>(prefix,fixture,seed,ordered,ordered_child,true));
            ++totals.comparisons;
        };
        for(const auto* stream:{"reads","writes"}) {
            unsigned ordinal{};
            for(const J& access:expected.observations.at(stream)) {
                const auto pc=access.at("instruction").get<std::uint32_t>();
                const bool reading=std::string_view(stream)=="reads";
                for(const auto* phase:reading?std::vector{"before_read"}:std::vector{"before_preimage","before_write","after_write"}) {
                    if(!tested_faults.emplace(phase,pc).second)continue;
                    auto prefix=row;prefix["name"]=std::string(phase)+"-"+std::to_string(pc);
                    prefix["fault"]=J{{"phase",phase},{"ordinal",ordinal}};
                    test(prefix);++totals.faults;
                }
                const auto key=std::to_string(pc);
                if(fixture.at("successors").contains(key)&&tested_frontiers.insert(pc).second) {
                    auto prefix=row;prefix["name"]="retired-"+key;prefix["stop"]=fixture.at("successors").at(key);
                    if(SourceMemory::repeat(pc)) {
                        for(unsigned occurrence=1;occurrence<=8u;++occurrence) {
                            prefix["repeat_occurrence"]=occurrence;test(prefix);++totals.repeat_frontiers;
                        }
                    }else {
                        test(prefix);++totals.frontiers;if(pc==0x1205adu)++totals.child_frontiers;
                    }
                }
                ++ordinal;
            }
        }
    }
    check(tested_frontiers.size()==fixture.at("successors").size(),"Retained composition retirement set incomplete");
}
}
int main(int argc,char** argv)try {
    check(argc==2,"Supply exact retained composition inputs");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto original=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{original};
    Totals totals;
    run(fixture,&composition_source0::ordered,&composition_source0::legacy,
        &composition_source0::ordered_product,&composition_source0::legacy_product,totals);
    run(fixture,&composition_source1::ordered,&composition_source1::legacy,
        &composition_source1::ordered_product,&composition_source1::legacy_product,totals);
    std::cout<<J{{"passed",true},{"copies",2},{"comparisons",totals.comparisons},{"provider_prefixes",totals.faults},
        {"exact_retirements",totals.frontiers},{"exact_repeat_retirements",totals.repeat_frontiers},
        {"exact_child_retirements",totals.child_frontiers},{"unmodified_source_differences",totals.legacy_differences},
        {"unordered_view_differences_from_unmodified_source",totals.unordered_differences},
        {"first_unmodified_source_difference",totals.first_legacy_difference},
        {"first_unordered_view_difference",totals.first_unordered_difference}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

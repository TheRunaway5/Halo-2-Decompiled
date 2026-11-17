#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_inverse_transform_source_frame.hpp"
#include "codec_inverse_transform_sources.inc"

namespace {
struct Boundary {std::uint32_t pc;};
struct SourceMemory:OwnerMemory {
    mutable unsigned preimages{};
    mutable J preimage_packets=J::array();
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(write_sites.contains(pc)) {
            preimage_packets.push_back(capture());
            if(fault&&fault->at("phase")=="before_preimage"&&preimages++==fault->at("ordinal"))throw Fault{};
        }
        OwnerMemory::read(address,out);
    }
};
using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
struct Result {J observations;Banks banks;};
template<class S>Result execute(const J& row,const Banks& seed,void(*reference)(S&),unsigned route) {
    SourceMemory memory;memory.banks=seed;
    memory.write_sites={0x120748u,0x120749u,0x120758u,0x12075du,0x120763u,0x120769u,0x120779u,
        0x12081du,0x120840u,0x120862u,0x120894u,0x1208a2u,0x1208b0u,0x1208c6u,0x1208d4u,0x1208e2u,0x1208e7u};
    if(row.contains("fault"))memory.fault=row.at("fault");
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    const auto stop=row.value("stop",0u);
    unsigned callbacks{},completed{};bool trace{};
    services.instruction=[&](std::uint32_t pc){if(pc==stop)throw Boundary{pc};memory.pc=pc;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
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
    J completed_writes=J::array();
    observer=[&](const SourcePhysicsContactRecordStore& event) {
        completed_writes.push_back(J{{"packet",memory.capture()},{"address",event.address},
            {"before",hex(event.before)},{"written",hex(event.written)}});
        if(memory.fault&&memory.fault->at("phase")=="after_write"&&completed++==memory.fault->at("ordinal"))throw Fault{};
    };
    std::uint32_t continuation{};bool interrupted{};
    try {
        namespace transform=halo2::native_codec_inverse_transform_detail;
        if(route==1u)continuation=transform::run_private_source<true>(state,[&](auto&,auto next){return next!=stop;});
        else if(route==2u)continuation=transform::run_private_source<false>(state,[&](auto&,auto next){return next!=stop;});
        else reference(state);
    } catch(const Boundary& next) {continuation=next.pc;}
    catch(const Fault&) {interrupted=true;}
    check((route?!callbacks:callbacks>0)&&continuation==stop&&interrupted==row.contains("fault"),
          "Inverse transform source failed exact callback/retirement route");
    return {J{{"abi",memory.capture()},{"reads",std::move(memory.reads)},{"writes",std::move(memory.writes)},
        {"packets",std::move(memory.packets)},{"preimages",std::move(memory.preimage_packets)},
        {"observers",std::move(completed_writes)},{"continuation",continuation},{"fault",interrupted}},std::move(memory.banks)};
}
bool equal(const Result& left,const Result& right){return left.observations==right.observations&&left.banks==right.banks;}
void compare(const J& row,const Result& expected,const Result& actual) {
    if(equal(expected,actual))return;
    std::cerr<<row.at("name")<<'\n';
    for(auto it=expected.observations.begin();it!=expected.observations.end();++it)if(it.value()!=actual.observations.at(it.key())) {
        const auto& right=actual.observations.at(it.key());
        if(it.value().is_array()&&right.is_array()) {
            for(std::size_t n=0;n<std::min(it.value().size(),right.size());++n)if(it.value()[n]!=right[n]) {
                std::cerr<<it.key()<<'['<<n<<"] expected "<<it.value()[n].dump().substr(0,2600)
                         <<" actual "<<right[n].dump().substr(0,2600)<<'\n';break;
            }
        } else std::cerr<<it.key()<<" expected "<<it.value().dump().substr(0,2600)<<" actual "<<right.dump().substr(0,2600)<<'\n';
    }
    throw std::runtime_error("Inverse transform ordered retained source differs");
}
struct Totals {unsigned comparisons{},faults{},frontiers{},legacy_differences{},unordered_view_differences{};J first_legacy;};
template<class S>void run(const J& fixture,void(*ordered)(S&),void(*legacy)(S&),Totals& totals) {
    std::set<std::pair<std::string,std::uint32_t>> faults;
    std::set<std::uint32_t> stops;
    const auto& successors=fixture.at("successors");
    for(const auto& row:fixture.at("cases")) {
        Banks seed;for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        const Result expected=execute(row,seed,ordered,0u);
        compare(row,expected,execute(row,seed,ordered,1u));++totals.comparisons;
        const Result old=execute(row,seed,legacy,0u);
        if(!equal(expected,old)) {
            if(!totals.legacy_differences) {
                auto first=J{{"name",row.at("name")},{"ordered_return",expected.observations.at("abi")},{"unmodified_return",old.observations.at("abi")}};
                const auto& before=expected.observations.at("packets");const auto& after=old.observations.at("packets");
                for(std::size_t n=0;n<std::min(before.size(),after.size());++n)if(before[n]!=after[n]) {
                    first["first_different_pre_io"]=J::array({before[n],after[n]});break;
                }
                totals.first_legacy=std::move(first);
            }
            ++totals.legacy_differences;
        }
        totals.unordered_view_differences+=!equal(old,execute(row,seed,legacy,2u));
        for(const auto* stream:{"reads","writes"}) {
            unsigned ordinal{};
            for(const J& access:expected.observations.at(stream)) {
                const auto pc=access.at("instruction").get<std::uint32_t>();
                const bool read=std::string_view(stream)=="reads";
                for(const auto* phase:read?std::vector{"before_read"}:std::vector{"before_preimage","before_write","after_write"}) {
                    if(!faults.emplace(phase,pc).second)continue;
                    auto prefix=row;prefix["name"]=std::string(phase)+"-"+std::to_string(pc);
                    prefix["fault"]=J{{"phase",phase},{"ordinal",ordinal}};
                    compare(prefix,execute(prefix,seed,ordered,0u),execute(prefix,seed,ordered,1u));
                    ++totals.faults;++totals.comparisons;
                }
                const auto key=std::to_string(pc);
                if(successors.contains(key)&&stops.insert(pc).second) {
                    auto prefix=row;prefix["name"]="retirement-"+std::to_string(pc);prefix["stop"]=successors.at(key);
                    compare(prefix,execute(prefix,seed,ordered,0u),execute(prefix,seed,ordered,1u));
                    ++totals.frontiers;++totals.comparisons;
                }
                ++ordinal;
            }
        }
    }
    check(stops.size()==successors.size(),"Inverse transform source retirement set incomplete");
}
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply real retained inverse-transform fixtures");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto host=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{host};
    Totals totals;
    run(fixture,&inverse_source0::ordered,&inverse_source0::legacy,totals);
    run(fixture,&inverse_source1::ordered,&inverse_source1::legacy,totals);
    std::cout<<J{{"passed",true},{"copies",2},{"comparisons",totals.comparisons},{"provider_prefixes",totals.faults},
        {"exact_retirements",totals.frontiers},{"unmodified_source_differences",totals.legacy_differences},
        {"unordered_view_differences_from_unmodified_source",totals.unordered_view_differences},
        {"first_unmodified_source_difference",totals.first_legacy}}.dump()<<'\n';return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}

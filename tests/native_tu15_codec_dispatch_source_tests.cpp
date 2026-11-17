#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_dispatch_source_frame.hpp"
#include "codec_dispatch_sources.inc"

namespace {
struct Prefix {std::uint32_t pc;};
struct TailEntry {std::uint32_t pc,target;};
struct FeedbackMemory:Memory {
    std::function<void(std::uint32_t)> feedback;
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        Memory::read(address,out);if(feedback)feedback(address);
    }
};
using Banks=std::map<std::uint32_t,std::vector<std::byte>>;
struct Result {J observations;Banks banks;};

template<class S>Result execute(const J& row,unsigned mode,const Banks& seed,const J& transfers,void(*reference)(S&),bool candidate) {
    FeedbackMemory memory;memory.banks=seed;
    if(row.contains("fault"))memory.fault=row.at("fault");
    SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
    const auto entry=row.at("entry").get<std::uint32_t>();
    const auto stop=row.value("stop",0u);
    unsigned callbacks{};bool trace{},changed{};
    services.instruction=[&](std::uint32_t pc) {
        if(pc==stop)throw Prefix{pc};
        memory.pc=pc;++callbacks;
        const auto key=std::to_string(pc);
        if(transfers.contains(key))throw TailEntry{pc,transfers.at(key)};
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    S state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
    const auto ret=entry+(entry==0x283570u?0x36u:0x6cu);
    memory.feedback=[&](std::uint32_t address) {
        if(changed)return;
        if(mode==1&&address==0x510f80u) {
            // MOV AL must combine the provider's returned byte with the live
            // upper EAX, and the pending branch still uses the live wide flags.
            state.gp[0]=0x8a73f100u;state.flags.zf=!state.flags.zf;changed=true;
        } else if(mode==2&&address==0x510f5cu) {
            state.gp[0]=0x9abcdef0u;state.flags.zf=!state.flags.zf;changed=true;
        } else if(mode==3&&memory.pc==ret) {
            state.gp[4]+=4u;changed=true;
        } else if(mode==4&&memory.pc==entry) {
            changed=true;throw halo2::native_codec_copy_source_detail::Continuation{0x12345678u,nullptr};
        }
    };
    halo2::native_codec_dispatch_detail::SourceResult result{};
    bool interrupted{},foreign{};
    try {
        if(candidate)result=halo2::native_codec_dispatch_detail::run_private_source(state,entry,[&](auto&,auto next){return next!=stop;});
        else {reference(state);result.transfer={ret,0u};}
    } catch(const TailEntry& next) {result.transfer={next.pc,next.target};}
    catch(const Prefix& next) {result.continuation=next.pc;}
    catch(const Fault&) {interrupted=true;}
    catch(const halo2::native_codec_copy_source_detail::Continuation& error) {
        check(error.pc==0x12345678u&&!error.owner,"Selector foreign exception identity changed");foreign=true;
    }
    check(candidate?!callbacks:callbacks>0,"Unexpected selector retained callback route");
    check(result.continuation==stop&&interrupted==row.contains("fault")&&foreign==(mode==4),"Selector did not reach the declared exact source boundary");
    if(mode)check(changed,"Selector declared provider feedback was not reached");
    return {J{{"abi",memory.capture()},{"reads",std::move(memory.reads)},{"packets",std::move(memory.packets)},
        {"site",result.transfer.instruction},{"target",result.transfer.target},{"continuation",result.continuation},
        {"fault",interrupted},{"foreign",foreign}},std::move(memory.banks)};
}
struct Totals {unsigned comparisons{},prefixes{},frontiers{},feedback{},tail_entries{};};
template<class S>void compare(const J& fixture,std::uint32_t entry,unsigned route,void(*reference)(S&),Totals& totals) {
    const auto& transfers=fixture.at("transfers").at(std::to_string(entry));
    for(const auto& row:fixture.at("cases")) {
        if(row.at("entry")!=entry)continue;
        Banks seed;for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        std::vector<unsigned> modes{0u};
        if(!row.contains("fault")&&!row.contains("stop")) {
            if(row.at("variant_loads")==true)modes.insert(modes.end(),{1u,2u});
            if(row.at("returns")==true)modes.push_back(3u);
            modes.push_back(4u);
        }
        for(auto mode:modes) {
            const auto expected=execute(row,mode,seed,transfers,reference,false);
            const Result actual=execute(row,mode,seed,transfers,reference,true);
            if(expected.observations!=actual.observations||expected.banks!=actual.banks) {
                std::cerr<<row.at("name")<<" retained source route "<<route<<" mode "<<mode<<'\n';
                for(auto it=expected.observations.begin();it!=expected.observations.end();++it)
                    if(it.value()!=actual.observations.at(it.key()))
                        std::cerr<<it.key()<<" expected "<<it.value().dump().substr(0,2200)<<" actual "<<actual.observations.at(it.key()).dump().substr(0,2200)<<'\n';
                throw std::runtime_error("Codec selector retained source differs");
            }
            ++totals.comparisons;totals.prefixes+=row.contains("fault");totals.frontiers+=row.contains("stop");
            totals.feedback+=mode!=0;totals.tail_entries+=actual.observations.at("target").get<std::uint32_t>()!=0;
        }
    }
}
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply genuine retained selector source inputs");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto host=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{host};
    Totals totals;
#define SELECTOR_ROUTE(N,E) compare(fixture,E,N,&selector_source##N::entry,totals);
    SELECTOR_SOURCE_ROUTES(SELECTOR_ROUTE)
#undef SELECTOR_ROUTE
    std::cout<<J{{"passed",true},{"source_copies",SELECTOR_SOURCE_COUNT},{"comparisons",totals.comparisons},
        {"read_prefixes",totals.prefixes},{"retired_prefixes",totals.frontiers},{"live_feedback",totals.feedback},
        {"true_tail_boundaries",totals.tail_entries},{"descendant_results_supplied",false}}.dump()<<'\n';return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}

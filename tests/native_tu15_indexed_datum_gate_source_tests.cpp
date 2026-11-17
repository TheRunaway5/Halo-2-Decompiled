#include "gameplay/objects/native_tu15_indexed_datum_gate_source_frame.hpp"
#include "halo2/death_startup_runtime.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"
#include <functional>
#include <type_traits>

namespace {
using namespace halo2;
using namespace halo2::object_property_source_test;
using namespace halo2::native_indexed_datum_gate_detail;

// The validator copies the complete states and retained function bodies from
// the authenticated source modules, including ParticleFrame's actual resolver.
#include "indexed_datum_source_owners.inc"

struct SourceMemory final : mopp_ray_frame_test::Memory {
    std::function<J()> packet;
    J trace=J::array();
    int stop=-1;
    bool alias_feedback{};
    mutable bool pending_feedback{};
    std::function<void()> feedback;

    void mark(const char* kind,std::uint32_t instruction,std::uint32_t address=0,
              unsigned width=0,std::span<const std::byte> before={},
              std::span<const std::byte> after={}) {
        trace.push_back(J::array({kind,instruction,pc,address,width,hex(before),hex(after),packet()}));
        if(int(trace.size())-1==stop)throw Boundary{};
    }
    void read(std::uint32_t address,std::span<std::byte> output) const override {
        auto& self=const_cast<SourceMemory&>(*this);
        self.mark("read",pc,address,output.size());
        const auto bank=at(address,output.size());
        std::copy_n(bank->second.begin()+address-bank->first,output.size(),output.begin());
        if(alias_feedback&&pending_feedback){pending_feedback=false;feedback();}
    }
    void write(std::uint32_t address,std::span<const std::byte> value) override {
        mark("write",pc,address,value.size(),{},value);
        const auto bank=at(address,value.size());
        std::copy(value.begin(),value.end(),banks.at(bank->first).begin()+address-bank->first);
    }
};

template<class State> J snapshot(const State& state) {
    const auto a=state.save();J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)
        live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    J mmx;
    if constexpr(requires{a.mmx;})mmx=a.mmx;
    return J::array({a.gp,a.eflags,a.xmm,a.fs_base,a.floating_point->x87_control_word,
        a.x87_status,a.floating_point->mxcsr,std::move(live),std::move(mmx)});
}

struct Run {
    J trace,abi;
    Banks banks;
    std::uint32_t cursor;
    bool stopped;
    friend bool operator==(const Run&,const Run&)=default;
};

template<class Owner,bool Literal> Run run(const Seed& seed,unsigned mode,bool native,int stop,
                                           std::set<std::uint32_t>& sites) {
    using State=typename Owner::State;
    SourceMemory memory;memory.banks=seed.banks;memory.stop=stop;
    SourcePhysicsContactRuntimeServices services;
    const SourceUnitAnimationRuntimeServices animation;
    const SourcePhysicsContactRecordObserver observe=mode==1?SourcePhysicsContactRecordObserver{}:
        SourcePhysicsContactRecordObserver{[&](const auto& event){
            memory.mark("observer",event.instruction,event.address,event.written.size(),event.before,event.written);
            if(mode==5&&event.instruction==0x3eb21u)memory.pending_feedback=true;
        }};
    if(mode!=1)services.instruction=[&](std::uint32_t pc){
        memory.mark("instruction",pc);memory.pc=pc;if(!native)sites.insert(pc);
    };
    static constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    const bool trace=mode==3;
    if(mode==2||mode==3)
        services.instruction_cursor={&memory.pc,&trace,nullptr,service_sites,
            std::this_thread::get_id(),&services.instruction.target_type(),true};
    memory.alias_feedback=mode==4||mode==5;
    memory.pending_feedback=mode==4;
    const auto outer=host_fp();
    Run result;
    {
        const physics_step_detail::FpScope scope{*seed.abi.floating_point};
        State s=[&]() -> State {
            if constexpr(Owner::map_pack)return {memory,observe,services,animation};
            else return {memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        }();
        if constexpr(Owner::map_pack){
            SourceUnitAnimationRuntimeAbi abi;
            static_cast<SourcePhysicsContactRuntimeAbi&>(abi)=seed.abi;
            abi.mmx={1,2,3,4,0xfedcba9876543210u,6,7,8};
            s.load(abi);
        }else s.load(seed.abi);
        memory.packet=[&]{return snapshot(s);};
        memory.feedback=[&]{s.gp[2]^=1u;};
        result.stopped=false;
        try {
            if(native)run_source_frame<Literal,Owner::addresses>(s);
            else Owner::template retained<Literal>(s);
        }catch(const Boundary&){result.stopped=true;}
        result.abi=snapshot(s);
    }
    check(outer==host_fp(),"3EB10 source changed host FP controls");
    result.trace=std::move(memory.trace);result.banks=std::move(memory.banks);
    result.cursor=memory.pc;return result;
}

struct Counts {unsigned returns{},stops{},packets{},profiles{};};
template<class Owner,bool Literal=false> void compare(const J& rows,Counts& totals) {
    std::set<std::uint32_t> sites;
    for(unsigned row=0;row<rows.size();++row){
        const Seed seed{rows[row]};
        for(unsigned mode=0;mode<6;++mode){
            if(mode==2&&!Literal)continue;
            const auto describe=[&](int stop) {
                return std::string(Owner::label)+(Literal?" prepared":" raw")+
                    " row "+std::to_string(row)+" mode "+std::to_string(mode)+" event "+std::to_string(stop);
            };
            const auto expected=run<Owner,Literal>(seed,mode,false,-1,sites);
            check(!expected.stopped&&run<Owner,Literal>(seed,mode,true,-1,sites)==expected,
                  "3EB10 retained source differs: "+describe(-1));
            ++totals.returns;totals.packets+=expected.trace.size();
            if(row<9&&(mode==0||mode==2||mode>=4))
                for(unsigned n=0;n<expected.trace.size();++n){
                    const auto prefix=run<Owner,Literal>(seed,mode,false,int(n),sites);
                    check(prefix.stopped&&run<Owner,Literal>(seed,mode,true,int(n),sites)==prefix,
                          "3EB10 retained source differs: "+describe(int(n)));
                    ++totals.stops;totals.packets+=prefix.trace.size();
                }
        }
    }
    check(sites.size()==25,"3EB10 retained source callback coverage incomplete");
    ++totals.profiles;
}
} // namespace

int main(int argc,char** argv)try {
    check(argc==2,"Supply 3EB10 source fixtures for both images");
    std::ifstream in(argv[1]);const auto rows=J::parse(in);
    Counts counts;
    compare<PrimaryCurrent>(rows.at("tu15"),counts);
    compare<PrimaryCurrent,true>(rows.at("tu15"),counts);
    compare<PrimaryFresh>(rows.at("tu15"),counts);
    compare<PrimaryFresh,true>(rows.at("tu15"),counts);
    compare<ParticleCurrent>(rows.at("tu15"),counts);
    compare<ParticleCurrent,true>(rows.at("tu15"),counts);
    compare<ParticleFresh>(rows.at("tu15"),counts);
    compare<ParticleFresh,true>(rows.at("tu15"),counts);
    compare<DeathCurrent>(rows.at("mappack11"),counts);
    compare<DeathFresh>(rows.at("mappack11"),counts);
    std::cout<<J{{"passed",true},{"complete_source_returns",counts.returns},
        {"complete_source_packets",counts.packets},{"source_failure_prefixes",counts.stops},
        {"profiles",counts.profiles},{"covered_sites_per_profile",25}}.dump()<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

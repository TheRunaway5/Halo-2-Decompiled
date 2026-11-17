#include "gameplay/objects/native_tu15_object_table_ratio_source_frame.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

using namespace halo2;
using namespace halo2::object_property_source_test;
namespace original {
#include "object_table_ratio_reference.inc"
}
namespace {
template<unsigned Route> struct Profile {
    using Current=original::current::WorldState;
    using Fresh=original::fresh::WorldState;
    static constexpr auto label="Object table ratio";
    static std::string fixture_name(std::uint32_t) {throw std::runtime_error("Unexpected symbol service in 24B430");}
    static std::uint32_t fixture_key(std::uint32_t key) {return key;}
    template<bool Literal> static void call_native(auto& state,unsigned variant) {
        using namespace native_object_table_ratio_detail;
        if constexpr(Route==0)run_source_frame<Literal>(state);
        else run_mode_owner<Literal>(state,[&](auto& child){
            if constexpr(Route==1)run_source_frame<Literal>(child);
            else original::run_leaf(child,variant);
        });
    }
    static void run_native(auto& state,unsigned variant) {
        if(variant&2u)call_native<true>(state,variant);else call_native<false>(state,variant);
    }
    static void run_reference(auto& state,unsigned variant) {
        if constexpr(Route==0)original::run_leaf(state,variant);else original::run_parent(state,variant);
    }
    static void after_instruction(WorldState& state,std::uint32_t site,int feedback) {
        if(feedback==1&&site==0x24b481u)state.flags.zf=!state.flags.zf;
    }
    static void after_read(const auto& memory,std::uint32_t,std::span<std::byte> value) {
        if(memory.feedback==2&&*memory.cursor==0x24b48cu){
            const std::uint16_t count=19;
            std::memcpy(value.data(),&count,2);
        }
    }
    static void after_write(auto& memory,std::uint32_t,std::span<const std::byte>) {
        if(memory.feedback==4&&*memory.cursor==0x165602u)*memory.observer=*memory.collector;
    }
    static void after_observer(auto&,const auto&) {}
};

template<unsigned Route> J compare(const J& rows) {
    using P=Profile<Route>;
    const std::array modes{Mode{},Mode{true},Mode{false,false,false,true},
        Mode{false,false,false,false,1},Mode{false,false,false,false,2},Mode{false,false,false,false,4}};
    std::size_t normals{},prefixes{},packets{},real_child_entries{},real_child_returns{};
    for(std::size_t n=0;n<rows.size();++n){
        const Seed seed{rows[n]};
        for(unsigned variant=0;variant<4;++variant)for(std::size_t mode=0;mode<modes.size();++mode){
            const auto name=std::to_string(Route)+"/"+std::to_string(n)+"/"+std::to_string(variant)+"/"+std::to_string(mode);
            const auto expected=run<P>(seed,modes[mode],variant,false);
            check(!expected.stopped,"Unexpected retained 24B430 return");
            equal<P>(run<P>(seed,modes[mode],variant,true),expected,name);
            for(const auto& event:expected.trace)if(event.kind==Mark::instruction){
                real_child_entries+=event.pc==0x24b430u;
                real_child_returns+=event.pc==0x165607u;
            }
            ++normals;packets+=expected.trace.size();
            if(n<16)for(std::size_t stop=0;stop<expected.trace.size();++stop){
                const auto stopped=run<P>(seed,modes[mode],variant,false,int(stop));
                check(stopped.stopped,"Expected a retained 24B430 stop");
                equal<P>(run<P>(seed,modes[mode],variant,true,int(stop)),stopped,name+"/stop"+std::to_string(stop));
                ++prefixes;packets+=stopped.trace.size();
            }
        }
    }
    check(Coverage<P>::sites.size()==(Route==0?34u:42u),"24B430/1655F0 callback coverage incomplete");
    if constexpr(Route!=0)check(real_child_entries&&real_child_entries==real_child_returns,"1655F0 genuine child route was not exercised");
    return J{{"normal",normals},{"prefixes",prefixes},{"complete_packets",packets},{"sites",Coverage<P>::sites.size()},
             {"observed_child_entries",real_child_entries},{"observed_child_returns",real_child_returns}};
}
}
int main(int argc,char** argv)try {
    check(argc==2,"Supply 24B430 source cases");std::ifstream in(argv[1]);const auto rows=J::parse(in);
    std::cout<<J{{"passed",true},{"leaf",compare<0>(rows.at("leaf"))},
                 {"native_parent_child",compare<1>(rows.at("parent"))},
                 {"native_parent_retained_child",compare<2>(rows.at("parent"))},
                 {"production_bound",false}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

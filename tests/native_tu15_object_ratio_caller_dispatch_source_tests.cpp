#include "gameplay/objects/native_tu15_object_ratio_caller_dispatch_source_frame.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

using namespace halo2;
using namespace halo2::object_property_source_test;
using native_object_ratio_caller_detail::Route;
namespace original {
#include "object_ratio_caller_dispatch_reference.inc"
}
namespace {
struct Profile {
    using Current=original::current::WorldState;
    using Fresh=original::fresh::WorldState;
    static constexpr auto label="Object ratio initial dispatch";
    static inline std::optional<Route> route;
    static std::string fixture_name(std::uint32_t key) {return original::fixture_name(key);}
    static std::uint32_t fixture_key(std::uint32_t key) {return original::fixture_key(key);}
    static void run_native(auto& state,unsigned variant) {
        route=variant&2u?native_object_ratio_caller_detail::run_source_dispatch<true>(state):
                        native_object_ratio_caller_detail::run_source_dispatch<false>(state);
    }
    static void run_reference(auto& state,unsigned variant) {route=original::run_reference(state,variant);}
    static void after_instruction(WorldState& state,std::uint32_t pc,int feedback) {
        if(feedback==1&&pc==0x104711u)state.flags.zf=!state.flags.zf;
    }
    static void after_read(const auto& memory,std::uint32_t,std::span<std::byte> value) {
        if(memory.feedback==2&&*memory.cursor==0x104706u){
            const std::uint32_t address=0x61003040u;std::memcpy(value.data(),&address,4);
        }
    }
    static void after_write(auto& memory,std::uint32_t,std::span<const std::byte>) {
        if(memory.feedback==4&&*memory.cursor==0x10470au)*memory.observer=*memory.collector;
    }
    static void after_observer(auto&,const auto&) {}
};
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply private 1046C0 initial dispatch source cases");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    const std::array modes{Mode{},Mode{true},Mode{false,true},Mode{true,true},Mode{false,true,true},
        Mode{false,false,false,true},Mode{false,false,false,false,1},Mode{false,false,false,false,2},
        Mode{false,false,false,false,4}};
    std::set<Route> routes;
    std::size_t normals{},prefixes{},packets{};
    for(std::size_t n=0;n<rows.size();++n){
        const Seed seed{rows[n]};
        for(unsigned variant=0;variant<4;++variant)for(std::size_t mode=0;mode<modes.size();++mode){
            const auto name=std::to_string(n)+"/"+std::to_string(variant)+"/"+std::to_string(mode);
            Profile::route.reset();const auto expected=run<Profile>(seed,modes[mode],variant,false);
            const auto route=Profile::route;
            check(!expected.stopped&&route,"Missing actual retained 1046C0 continuation");
            if(!mode)check(static_cast<std::uint32_t>(*route)==rows[n].at("continuation").get<std::uint32_t>(),
                           "Actual retained original-key continuation changed");
            Profile::route.reset();equal<Profile>(run<Profile>(seed,modes[mode],variant,true),expected,name);
            check(Profile::route==route,std::string(Profile::label)+" retained source differs: continuation "+name);
            routes.insert(*route);++normals;packets+=expected.trace.size();
            if(n<12)for(std::size_t stop=0;stop<expected.trace.size();++stop){
                Profile::route.reset();const auto prefix=run<Profile>(seed,modes[mode],variant,false,int(stop));
                check(prefix.stopped&&!Profile::route,"Expected a retained 1046C0 stop");
                equal<Profile>(run<Profile>(seed,modes[mode],variant,true,int(stop)),prefix,name+"/stop"+std::to_string(stop));
                check(!Profile::route,"A stopped 1046C0 prefix published a continuation");
                ++prefixes;packets+=prefix.trace.size();
            }
        }
    }
    check(routes.size()==6&&Coverage<Profile>::sites.size()==32,"1046C0 initial dispatch coverage incomplete");
    std::cout<<J{{"passed",true},{"normal",normals},{"prefixes",prefixes},{"complete_packets",packets},
        {"sites",Coverage<Profile>::sites.size()},{"continuations",routes.size()},{"production_bound",false}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

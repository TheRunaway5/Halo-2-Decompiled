#include "gameplay/objects/native_tu15_object_ratio_caller_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_definition_mode_query_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_record_ratio_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_table_ratio_source_frame.hpp"
#include "gameplay/weapons/native_tu15_object_parent_mode.hpp"
#include <cstdlib>
#include "native_tu15_object_property_source_test_harness.hpp"

using namespace halo2;
using namespace halo2::object_property_source_test;
namespace original {
struct ChildEntry {
    std::uint32_t target,site;
    friend bool operator==(const ChildEntry&,const ChildEntry&)=default;
};
static std::optional<ChildEntry> entered;
static void frontier(auto&,std::uint32_t target,std::uint32_t site=0) {
    entered=ChildEntry{target,site};
    throw Boundary{};
}
static bool returning,literal;
static std::vector<ChildEntry> returned;
template<bool Literal,class State> void actual_child(State& s,std::uint32_t target) {
    switch(target){
    case 0x107140u:native_object_record_ratio_detail::run_source_frame<Literal,true>(s);return;
    case 0x165590u:native_object_definition_mode_query_detail::run_source_frame<Literal>(s);return;
    case 0x1655f0u:
        native_object_table_ratio_detail::run_mode_owner<Literal>(s,[](auto& state){
            native_object_table_ratio_detail::run_source_frame<Literal>(state);
            returned.push_back({0x24b430u,0x165602u});
        });
        return;
    case 0xfc510u:
        native_object_parent_mode_detail::run_private(s,[](auto& state,std::uint32_t pc){
            if(state.services.instruction)state.services.instruction(pc);
        });
        return;
    }
    throw std::runtime_error("Unexpected 1046C0 child owner");
}
static void call_child(auto& s,std::uint32_t target,std::uint32_t site=0) {
    if(!returning)frontier(s,target,site);
    if(literal)actual_child<true>(s,target);
    else actual_child<false>(s,target);
    returned.push_back({target,site});
}
#include "object_ratio_caller_reference.inc"
}
namespace {
struct Children {
    void record_ratio(auto& s) {original::call_child(s,0x107140u);}
    void object_parent_mode(auto& s) {original::call_child(s,0xfc510u);}
    void combat(auto& s,std::uint32_t target,std::uint32_t pc) {original::call_child(s,target,pc);}
};
struct Profile {
    using Current=original::current::WorldState;
    using Fresh=original::fresh::WorldState;
    static constexpr auto label="Object ratio caller";
    static inline bool ordered;
    static std::string fixture_name(std::uint32_t key) {return original::fixture_name(key);}
    static std::uint32_t fixture_key(std::uint32_t key) {return original::fixture_key(key);}
    static void run_native(auto& state,unsigned variant) {
        original::literal=(variant&2u)!=0;
        Children calls;
        using namespace native_object_ratio_caller_detail;
        if(ordered){
            if(variant&2u)run_source_frame<true,true>(state,calls);
            else run_source_frame<false,true>(state,calls);
        }else if(variant&2u)run_source_frame<true,false>(state,calls);
        else run_source_frame<false,false>(state,calls);
    }
    static void run_reference(auto& state,unsigned variant) {
        original::literal=(variant&2u)!=0;
        original::run_reference(state,variant,ordered);
    }
    static void after_instruction(WorldState&,std::uint32_t,int) {}
    static void after_read(const auto&,std::uint32_t,std::span<std::byte>) {}
    static void after_write(auto& memory,std::uint32_t,std::span<const std::byte>) {
        if(memory.feedback==4&&*memory.cursor==0x10470au)*memory.observer=*memory.collector;
    }
    static void after_observer(auto&,const auto&) {}
};
}

int main(int argc,char** argv) try {
    check(argc==2,"Supply private 1046C0 source fixtures");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    const std::array modes{Mode{},Mode{true},Mode{false,true},Mode{false,true,true},
        Mode{false,false,false,true},Mode{false,false,false,false,4}};
    const auto mode=std::getenv("HALO2_TEST_1046C0_RETURN_CHILDREN");
    original::returning=mode&&*mode;
    std::size_t normals{},child_entries{},prefixes{},packets{},child_returns{};
    std::set<std::uint32_t> returned_targets;
    std::set<std::uint32_t> child_targets;
    for(bool ordered:{false,true}){
        Profile::ordered=ordered;
        for(std::size_t n=0;n<rows.size();++n){
            const Seed seed{rows[n]};
            for(unsigned variant=0;variant<4;++variant)for(std::size_t m=0;m<modes.size();++m){
                const auto name=rows[n].at("name").get<std::string>()+"/"+std::to_string(ordered)+"/"+
                    std::to_string(variant)+"/"+std::to_string(m);
                original::entered.reset();original::returned.clear();
                const auto reference=run<Profile>(seed,modes[m],variant,false);
                const auto target=original::entered;
                const auto returned=original::returned;
                check(reference.stopped==bool(target),"Unexpected retained 1046C0 stop");
                original::entered.reset();original::returned.clear();
                equal<Profile>(run<Profile>(seed,modes[m],variant,true),reference,name);
                check(original::entered==target&&original::returned==returned,
                    std::string(Profile::label)+" retained source differs: child sequence "+name);
                child_returns+=returned.size();
                for(auto entry:returned)returned_targets.insert(entry.target);
                if(target){++child_entries;child_targets.insert(target->target);}else ++normals;
                packets+=reference.trace.size();
                if(n%37==0&&m==0)for(std::size_t stop=0;stop<reference.trace.size();++stop){
                    original::entered.reset();original::returned.clear();
                    const auto prefix=run<Profile>(seed,modes[m],variant,false,int(stop));
                    const auto partial=original::returned;
                    check(prefix.stopped&&!original::entered,"Missing exact retained source stop");
                    original::returned.clear();
                    equal<Profile>(run<Profile>(seed,modes[m],variant,true,int(stop)),prefix,name+"/stop"+std::to_string(stop));
                    check(!original::entered&&original::returned==partial,
                        std::string(Profile::label)+" retained source differs: stopped child sequence");
                    ++prefixes;packets+=prefix.trace.size();
                }
            }
        }
    }
    check((original::returning?returned_targets.size()==5:child_targets.size()>=3)&&normals&&prefixes,
          "Missing complete, returning-child or child-boundary source routes");
    std::cout<<J{{"passed",true},{"normal",normals},{"child_entries",child_entries},{"prefixes",prefixes},
        {"complete_packets",packets},{"observed_source_sites",Coverage<Profile>::sites.size()},
        {"child_targets",child_targets},{"child_returns",child_returns},{"returned_targets",returned_targets},
        {"actual_live_world",false},{"production_bound",false},{"child_results_supplied",false}}.dump()<<'\n';
    return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

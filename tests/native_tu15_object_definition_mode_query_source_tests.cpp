#include "gameplay/objects/native_tu15_object_definition_mode_query_source_frame.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

using namespace halo2;
using namespace halo2::object_property_source_test;
namespace original {
#include "object_definition_mode_query_reference.inc"
}
namespace {
struct Profile {
    using Current=original::current::WorldState;
    using Fresh=original::fresh::WorldState;
    static constexpr auto label="Object definition mode query";
    static std::string fixture_name(std::uint32_t) {throw std::runtime_error("Unexpected symbol service in 165590");}
    static std::uint32_t fixture_key(std::uint32_t key) {return key;}
    static void run_native(auto&state,unsigned variant) {
        if(variant&2u)native_object_definition_mode_query_detail::run_source_frame<true>(state);
        else native_object_definition_mode_query_detail::run_source_frame<false>(state);
    }
    static void run_reference(auto&state,unsigned variant) {original::run_reference(state,variant);}
    static void after_instruction(WorldState&state,std::uint32_t site,int feedback) {
        if(feedback==1&&(site==0x1655a3u||site==0x1655ddu))state.flags.zf=!state.flags.zf;
    }
    static void after_read(const auto&memory,std::uint32_t,std::span<std::byte>value) {
        if(memory.feedback==2&&*memory.cursor==0x1655d5u){
            const std::uint16_t word=1;
            std::memcpy(value.data(),&word,2);
        }
    }
    static void after_write(auto&memory,std::uint32_t,std::span<const std::byte>) {
        if(memory.feedback==4&&*memory.cursor==0x165596u)*memory.observer=*memory.collector;
    }
    static void after_observer(auto&memory,const auto&event) {
        if(memory.feedback==3&&event.instruction==0x165596u){
            const auto bank=memory.at(event.address,4);const std::uint32_t value=0x89abcdefu;
            auto&data=memory.banks.at(bank->first);
            std::memcpy(data.data()+event.address-bank->first,&value,4);
        }
    }
};
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply 165590 source cases");std::ifstream in(argv[1]);const auto rows=J::parse(in);
    const std::array modes{Mode{},Mode{true},Mode{false,false,false,true},
        Mode{false,false,false,false,1},Mode{false,false,false,false,2},
        Mode{false,false,false,false,3},Mode{false,false,false,false,4}};
    std::size_t normals{},prefixes{},packets{};
    for(std::size_t n=0;n<rows.size();++n){
        const Seed seed{rows[n]};
        for(unsigned variant=0;variant<4;++variant)for(std::size_t mode=0;mode<modes.size();++mode){
            const auto name=std::to_string(n)+"/"+std::to_string(variant)+"/"+std::to_string(mode);
            const auto expected=run<Profile>(seed,modes[mode],variant,false);
            check(!expected.stopped,"Unexpected retained 165590 return");
            equal<Profile>(run<Profile>(seed,modes[mode],variant,true),expected,name);
            ++normals;packets+=expected.trace.size();
            if(n<8)for(std::size_t stop=0;stop<expected.trace.size();++stop){
                const auto stopped=run<Profile>(seed,modes[mode],variant,false,int(stop));
                check(stopped.stopped,"Expected a retained 165590 stop");
                equal<Profile>(run<Profile>(seed,modes[mode],variant,true,int(stop)),stopped,
                               name+"/stop"+std::to_string(stop));
                ++prefixes;packets+=stopped.trace.size();
            }
        }
    }
    check(Coverage<Profile>::sites.size()==23,"165590 callback coverage incomplete");
    std::cout<<J{{"passed",true},{"normal",normals},{"prefixes",prefixes},{"complete_packets",packets},
                 {"sites",Coverage<Profile>::sites.size()},{"production_bound",false}}.dump()<<'\n';
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

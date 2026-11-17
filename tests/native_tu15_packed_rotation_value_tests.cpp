#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <random>
namespace packed_rotation_predecessor{
std::array<float,4>frontend_grip_rotation(std::array<std::int16_t,4>,float);
std::array<float,4>frontend_grip_rotation(std::array<std::int16_t,4>);
}
namespace packed_rotation_candidate{
std::array<float,4>frontend_grip_rotation(std::array<std::int16_t,4>,float);
std::array<float,4>frontend_grip_rotation(std::array<std::int16_t,4>);
}
int main()try{
    const auto initial=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{initial};
    std::mt19937 rng{0x29618028};unsigned comparisons{},rejections{};
    constexpr std::array seeds{0u,0x80000000u,0xbf800000u,0x3f800000u,0x3d123456u,1u,0x7f800000u,0x7fc12345u,0x7f812345u};
    constexpr std::array denormal_modes{0u,0x40u,0x8000u,0x8040u};
    for(unsigned ordinal=0;ordinal<512;++ordinal){
        std::array<std::int16_t,4>input{};for(auto&v:input)v=std::bit_cast<std::int16_t>(std::uint16_t(rng()));
        if(ordinal%16==0)input.fill(0);if(ordinal%16==1)input={-32768,32767,-1,1};
        for(unsigned precision:{0x7fu,0x27fu,0x37fu})for(unsigned rc=0;rc<4;++rc)for(unsigned seed_index=0;seed_index<=seeds.size();++seed_index){
            auto baseline=initial;baseline.word(0,std::uint16_t(precision|(rc<<10u)));baseline.word(4,std::uint16_t((ordinal&0x473fu)|((ordinal%8u)<<11u)));baseline.word(8,0xffffu);
            baseline.mxcsr=0x1f80u|(rc<<13u)|(ordinal&0x3fu)|denormal_modes[(ordinal/4)%4];
            std::array<float,4>outputs[2]{};std::string errors[2];native_raw_x87_detail::Image after[2];
            for(unsigned mode=0;mode<2;++mode){
                native_raw_x87_detail::restore(baseline);
                try{
                    if(seed_index==seeds.size())outputs[mode]=mode?packed_rotation_candidate::frontend_grip_rotation(input):packed_rotation_predecessor::frontend_grip_rotation(input);
                    else{const auto seed=std::bit_cast<float>(seeds[seed_index]);outputs[mode]=mode?packed_rotation_candidate::frontend_grip_rotation(input,seed):packed_rotation_predecessor::frontend_grip_rotation(input,seed);}
                }catch(const FormatError&error){errors[mode]=error.what();}
                after[mode]=native_raw_x87_detail::capture();
            }
            check(errors[0]==errors[1],"Shared packed-rotation value changed predecessor error");
            check(std::bit_cast<std::array<std::uint32_t,4>>(outputs[0])==std::bit_cast<std::array<std::uint32_t,4>>(outputs[1]),"Shared packed-rotation value changed predecessor result");
            check(native_raw_x87_detail::equivalent(after[0],after[1]),"Shared packed-rotation value changed predecessor FP environment");
            rejections+=!errors[0].empty();++comparisons;
        }
    }
    std::cout<<J{{"passed",true},{"predecessor_comparisons",comparisons},{"matching_rejections",rejections},{"standalone_fragment_comparison",true},{"public_runtime_route_qualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}

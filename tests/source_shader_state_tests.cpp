#include "halo2/source_shader_state.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct HostRounding {int before=std::fegetround();~HostRounding(){std::fesetround(before);}};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply whole original render-state extern fixture");
    std::ifstream file(argv[1]);check(bool(file),"Original state-extern corpus missing");
    const auto corpus=nlohmann::json::parse(file);
    HostRounding rounding;
    std::size_t cases=0,writes=0;
    for(const int host:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
        check(std::fesetround(host)==0,"Host rounding control unavailable");
        for(const auto &row:corpus.at("cases")) {
            const auto index=row.at("state").get<unsigned>();
            std::array<std::uint32_t,256> states;states.fill(0xdeadbeefu);
            states.at(index)=row.at("first").at("before_state_value");
            std::array<float,256> scalars{};
            std::array<ShaderVector,256> colors{};
            const auto source=row.at("source").get<unsigned>();
            scalars.at(source)=std::bit_cast<float>(row.at("scalar_bits").get<std::uint32_t>());
            for(unsigned axis=0;axis<3;++axis)
                colors.at(source)[axis]=std::bit_cast<float>(row.at("color_bits").at(axis).get<std::uint32_t>());
            const ShaderRemap mapping{std::uint16_t(row.at("table_index").get<unsigned>()|
                row.at("provider_byte").get<unsigned>()<<8),row.at("ignored_byte2"),std::uint8_t(source)};
            for(const char *phase:{"first","repeated_same_value"}) {
                check(states[index]==row.at(phase).at("before_state_value"),"Original state composition initial value differs");
                const auto output=remap_source_shader_states(states,scalars,colors,std::span(&mapping,1),row.at("fpcw"));
                check(output.size()==1 && output[0].index==index &&
                    output[0].value==row.at(phase).at("after_state_value") && states[index]==output[0].value,
                    "Native state write differs from whole original1A170");
                check(row.at(phase).at("ordered_writes").size()==1 &&
                    row.at(phase).at("ordered_writes").at(0).at("state")==output[0].index &&
                    row.at(phase).at("ordered_writes").at(0).at("value")==output[0].value,
                    "Native state write differs from actual original device-writer input");
                for(unsigned n=0;n<states.size();++n)if(n!=index)
                    check(states[n]==0xdeadbeefu,"State extern touched another state word");
                ++writes;
            }
            ++cases;
        }
        for(const auto &row:corpus.at("ordered_cases")) {
            std::array<std::uint32_t,256> states{};
            states[75]=row.at("result").at("before_state_value");
            std::array<float,256> scalars{};std::array<ShaderVector,256> colors{};
            for(const auto &[index,bits]:row.at("scalars_bits").items())
                scalars.at(std::stoul(index))=std::bit_cast<float>(bits.get<std::uint32_t>());
            for(const auto &[index,bits]:row.at("colors_bits").items())for(unsigned axis=0;axis<3;++axis)
                colors.at(std::stoul(index))[axis]=std::bit_cast<float>(bits.at(axis).get<std::uint32_t>());
            std::vector<ShaderRemap> mappings;
            for(const auto &value:row.at("mappings")) {
                const auto hex=value.get<std::string>();std::array<unsigned,4> bytes;
                for(unsigned n=0;n<4;++n)bytes[n]=std::stoul(hex.substr(n*2,2),nullptr,16);
                mappings.push_back({std::uint16_t(bytes[0]|bytes[1]<<8),std::uint8_t(bytes[2]),std::uint8_t(bytes[3])});
            }
            const auto output=remap_source_shader_states(states,scalars,colors,mappings,row.at("fpcw"));
            const auto &expected=row.at("result").at("ordered_writes");
            check(output.size()==expected.size(),"Native ordered state write count differs");
            for(unsigned n=0;n<output.size();++n)
                check(output[n].index==expected[n].at("state") && output[n].value==expected[n].at("value"),
                    "Native overlapping state writes differ from original order");
            check(states[75]==row.at("result").at("after_state_value"),"Native final ordered state word differs");
        }
    }
    check(cases==7392*4 && corpus.at("ordered_cases").size()==8,"Original state-extern coverage changed");
    std::array<std::uint32_t,256> states{};const ShaderRemap invalid{0x0107,0,0};
    bool rejected=false;try{(void)remap_source_shader_states(states,{}, {},std::span(&invalid,1),0x027f);}catch(const FormatError&){rejected=true;}
    check(rejected,"Unadmitted descriptor row reached adjacent texture-state data");
    std::cout<<corpus.at("cases").size()<<" whole original state cases and repeated equal writes passed under four host rounding modes ("
        <<writes<<" native writes)\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

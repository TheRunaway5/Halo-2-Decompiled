#include "halo2/source_hud_messages.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
std::string hex(const SourceHudMessages &bank) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(const auto&r:bank.rows)for(const auto b:r){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}
    return out;
}
}
int main(int argc,char **argv)try {
    if(argc!=2)return 2;nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    std::array<SourceHudMessages,4> views;std::uint8_t sequence{};unsigned index{},compared{};
    for(const auto &input:fixture.at("inputs")) {
        std::u16string text;for(const auto c:input.at("utf16"))text.push_back(char16_t(c.get<unsigned>()));
        const auto view=input.at("view").get<std::uint32_t>();
        if(view!=0xffffffffu)source_hud_message_submit(views.at(view),sequence,input.at("tick"),text);
        for(const auto &sample:fixture.at("submissions"))if(sample.at("input")==index) {
            check(hex(views[0])==sample.at("bank").at("rows_hex").get<std::string>(),"Native queue differs from whole unchanged TU1.5 submission");
            check(hex(views[3])==sample.at("view3").at("rows_hex").get<std::string>(),"Native queue changed another local recipient");
            check(sequence==sample.at("bank").at("next_sequence"),"Native message sequence differs at wrap");
            const auto lines=source_hud_message_lines(views[0],input.at("tick"),1,1000,1,4);
            const auto sorted=sample.at("bank").at("sorted_hex").get<std::string>();
            std::vector<std::u16string> expected;
            for(unsigned row=0;row<4;++row) {
                const auto byte=[&](unsigned at){return std::stoul(sorted.substr((row*0x88+at)*2,2),nullptr,16);};
                if(!byte(0x82))continue;std::u16string text;
                for(unsigned i=0;i<63;++i){const auto c=byte(4+i*2)|(byte(5+i*2)<<8);if(!c)break;text+=char16_t(c);}
                expected.push_back(std::move(text));
            }
            check(lines.size()==expected.size(),"Native sorted message count differs from original qsort");
            for(unsigned i=0;i<lines.size();++i)check(lines[i].text==expected[i],"Native message order differs from original comparator");
            validate_source_hud_messages(views[0]);++compared;
        }
        ++index;
    }
    unsigned fades{};
    for(const auto &sample:fixture.at("fades")) {
        const auto scalar=[&](const char *name){return std::bit_cast<float>(sample.at(name).get<std::uint32_t>());};
        const auto actual=source_hud_message_alpha(scalar("age_bits"),scalar("hold_bits"),scalar("fade_bits"),scalar("opacity_bits"));
        check(std::bit_cast<std::uint32_t>(actual)==sample.at("alpha_bits"),"Native fade differs from original instructions and original pow");++fades;
    }
    auto invalid=views[0];invalid.rows[0][0x82]=std::byte{2};bool rejected{};
    try{validate_source_hud_messages(invalid);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"Malformed native message was accepted");
    const auto colors=fixture.at("color_bytes").get<std::string>();
    for(bool elite:{false,true}) {
        const auto actual=source_hud_message_color(elite);
        for(unsigned channel=0;channel<3;++channel) {
            std::uint32_t expected{};
            for(unsigned n=0;n<4;++n)expected|=std::stoul(colors.substr(((elite?16u:0u)+channel*4u+n)*2u,2),nullptr,16)<<(n*8u);
            check(std::bit_cast<std::uint32_t>(actual[channel])==expected,"Native HUD text color differs from original readonly data");
        }
    }
    std::cout<<"PASS "<<index<<" original queue inputs, "<<compared<<" full queue/sort comparisons and "<<fades<<" exact original fade cases\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

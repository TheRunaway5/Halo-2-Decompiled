#include "halo2/native_grenade_wire.hpp"
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
}
int main()try {
    std::set<unsigned> valid;
    for(unsigned frag=0;frag<=4;++frag)for(unsigned plasma=0;plasma<=4;++plasma)
        for(int selected=-1;selected<=1;++selected) {
            const std::array<std::uint8_t,2> counts{std::uint8_t(frag),std::uint8_t(plasma)};
            const auto encoded=encode_native_grenades(counts,std::int8_t(selected));
            check(valid.insert(encoded).second,"Distinct original grenade inventories alias one wire byte");
            const auto decoded=decode_native_grenades(encoded);
            check(decoded.counts==counts && decoded.selected==selected,"Original grenade inventory lost data in transport");
        }
    check(encode_native_grenades({4,4},1)==0xa4u,"Four-grenade inventory changed its declared protocol103 representation");
    unsigned rejected{},invalid_counts{};
    for(unsigned byte=0;byte<=255;++byte) {
        bool accepted{};
        try{(void)decode_native_grenades(std::uint8_t(byte));accepted=true;}catch(const FormatError&){++rejected;}
        check(accepted==valid.contains(byte),"Wire decoder admitted a reserved grenade inventory");
    }
    for(unsigned count=5;count<=255;++count)for(unsigned type=0;type<2;++type) {
        std::array<std::uint8_t,2> counts{};counts[type]=std::uint8_t(count);bool refused{};
        try{(void)encode_native_grenades(counts,0);}catch(const FormatError&){refused=true;++invalid_counts;}
        check(refused,"Wire encoder truncated an invalid grenade inventory");
    }
    for(const int selected:{-128,-2,2,127}) {
        bool refused{};try{(void)encode_native_grenades({4,4},std::int8_t(selected));}catch(const FormatError&){refused=true;}
        check(refused,"Wire encoder aliased an invalid grenade selection");
    }
    check(valid.size()==75 && rejected==181 && invalid_counts==502,"Grenade wire test coverage changed");
    std::cout<<"PASS original inventories=75 reserved bytes=181 invalid count inputs=502 invalid selectors=4\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

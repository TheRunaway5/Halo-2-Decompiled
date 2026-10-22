#include "halo2/source_hud_messages.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
using Row=std::array<std::byte,0x88>;
unsigned byte(const Row&r,unsigned at){return std::to_integer<unsigned>(r[at]);}
std::uint32_t word(const Row&r,unsigned at) {
    std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=byte(r,at+n)<<(n*8);return value;
}
void word(Row&r,unsigned at,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)r[at+n]=std::byte(value>>(n*8));
}
float age(const Row&r,std::uint32_t tick,float dt) {
    return float(std::bit_cast<std::int32_t>(tick-word(r,0)))*dt;
}
std::u16string text(const Row&r) {
    std::u16string out;
    for(unsigned i=0;i<63;++i) {
        const auto c=byte(r,4+i*2)|(byte(r,5+i*2)<<8);
        if(!c)break;out.push_back(char16_t(c));
    }
    return out;
}
}
void source_hud_message_submit(SourceHudMessages &bank,std::uint8_t &sequence,
    std::uint32_t tick,std::u16string_view input) {
    if(input.empty() || input.front()==0)return;
    unsigned selected=0;auto oldest=std::numeric_limits<std::int32_t>::max();
    for(unsigned i=0;i<4;++i) {
        if(!byte(bank.rows[i],0x82)){selected=i;break;}
        const auto time=std::bit_cast<std::int32_t>(word(bank.rows[i],0));
        if(time<oldest){oldest=time;selected=i;}
    }
    auto&r=bank.rows[selected];bool ended{};
    //1425A0 passes62 to_wcsncpy and writes the63rd terminator. Preserve
    //the original zero padding and untouched nonliteral row fields.
    for(unsigned i=0;i<63;++i) {
        const auto c=i<62 && i<input.size() && !ended?std::uint16_t(input[i]):0u;
        ended|=c==0;r[4+i*2]=std::byte(c);r[5+i*2]=std::byte(c>>8);
    }
    word(r,0,tick);r[0x82]=std::byte{1};r[0x83]=std::byte(sequence++);
    r[0x84]=r[0x85]=std::byte{};
}
void source_hud_messages_expire(SourceHudMessages &bank,std::uint32_t tick,float dt,float hold,float fade) {
    for(auto&r:bank.rows)if(byte(r,0x82) && hold+fade<=age(r,tick,dt)) {
        word(r,0,0xffffffffu);r[0x82]=std::byte{};
    }
}
float source_hud_message_alpha(float age,float hold,float fade,float opacity) {
    if(!(age>hold))return opacity;
    const auto fraction=std::clamp(1.f-(age-hold)/fade,0.f,1.f);
    // Original readonly DOUBLE46A378, then complete37CDE8 pow and FSTP.
    constexpr double exponent=std::bit_cast<double>(std::uint64_t{0x3ffe666660000000ull});
    return float(double(opacity)*std::pow(double(fraction),exponent));
}
std::vector<SourceHudMessageLine> source_hud_message_lines(const SourceHudMessages &bank,
    std::uint32_t tick,float dt,float hold,float fade,unsigned maximum,float opacity) {
    std::vector<const Row*> rows;for(const auto&r:bank.rows)if(byte(r,0x82))rows.push_back(&r);
    //25AC7D compares signed DWORD tick differences, then the numeric BYTE
    //serials. Its sequence wrap is deliberately not a signed-byte delta.
    std::stable_sort(rows.begin(),rows.end(),[](const Row*a,const Row*b) {
        const auto delta=std::bit_cast<std::int32_t>(word(*b,0)-word(*a,0));
        return delta?delta<0:byte(*a,0x83)>byte(*b,0x83);
    });
    std::vector<SourceHudMessageLine> result;
    for(const auto*r:rows) {
        if(result.size()>=maximum)break;
        const auto alpha=source_hud_message_alpha(age(*r,tick,dt),hold,fade,opacity);
        if(alpha>0)result.push_back({text(*r),alpha});
    }
    return result;
}
void validate_source_hud_messages(const SourceHudMessages&bank) {
    for(const auto&r:bank.rows) {
        if(byte(r,0x82)>1 || byte(r,0x84) || byte(r,0x85))
            throw std::invalid_argument("Invalid native literal HUD message flags");
        if(byte(r,0x82) && (byte(r,0x80)||byte(r,0x81)))
            throw std::invalid_argument("Native literal HUD message is not terminated");
    }
}
std::array<float,3> source_hud_message_color(bool elite) {
    // English TU1.5 readonly4751E4/4751F4; verified in the original fixture.
    const std::array<std::uint32_t,3> values=elite?
        std::array{0x3f4ececfu,0x3f0f8f90u,0x3f5ededfu}:
        std::array{0x3eeaeaebu,0x3f3ababbu,0x3f800000u};
    return {std::bit_cast<float>(values[0]),std::bit_cast<float>(values[1]),std::bit_cast<float>(values[2])};
}
}

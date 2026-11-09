#include "halo2/native_campaign_bsp_player_reset_tu15.hpp"

#include <array>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Bank {
    SourcePhysicsContactMemory &memory_;
    const SourcePhysicsContactRecordObserver &observe_;
public:
    Bank(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe)
        :memory_(memory),observe_(observe) {}
    std::uint32_t read(std::uint32_t address,unsigned width=4) const {
        std::array<std::byte,4> bytes{};
        memory_.read(address,std::span(bytes).first(width));
        std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8u*n);
        return value;
    }
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width) {
        std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        if(observe_) {
            SourcePhysicsContactRecordStore event{pc,address,
                std::vector<std::byte>(width),{bytes.begin(),bytes.begin()+width}};
            memory_.read(address,event.before);
            memory_.write(address,std::span(bytes).first(width));
            observe_(event);
        } else memory_.write(address,std::span(bytes).first(width));
    }
};

// Both source loops re-read the high-water mark and bit vector after
// processing a selected row; a hole never reads its player payload.
template<class Visit>
void visit_players(Bank &bank,std::uint32_t pool,const Visit &visit) {
    std::uint32_t index=0;
    for(;;) {
        if(std::bit_cast<std::int32_t>(index)<0)return;
        const auto end=std::bit_cast<std::int32_t>(bank.read(pool+0x38u));
        if(std::bit_cast<std::int32_t>(index)>=end)return;
        const auto bitmap=bank.read(pool+0x48u);
        while(!(bank.read(bitmap+(index>>5)*4u)&(1u<<(index&31)))) {
            ++index;
            if(std::bit_cast<std::int32_t>(index)>=end)return;
        }
        const auto stride=bank.read(pool+0x24u);
        const auto player=bank.read(pool+0x44u)+index*stride;
        if(!player)return;
        visit(player);
        ++index;
    }
}
}

void native_campaign_bsp_reset_visibility_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    Bank bank(memory,observe);
    const auto game=bank.read(0x4f3218u);
    constexpr std::array offsets{0x1138u,0x1178u,0x11b8u};
    constexpr std::array sites{0x13a835u,0x13a842u,0x13a84fu};
    for(unsigned mask=0;mask<offsets.size();++mask)
        for(unsigned n=0;n<16;++n)bank.write(sites[mask],game+offsets[mask]+4u*n,0,4);
    const auto selected=bank.read(0x474c24u,2);
    bank.write(0x13a857u,game+2u,selected,2);
}

void native_campaign_bsp_reset_players_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    Bank bank(memory,observe);
    const auto context=bank.read(0x4f54f0u);
    const auto pending=bank.read(context+0xa4u,2);
    const auto pool=bank.read(0x4f54f4u);
    if(pending!=0xffffu)
        throw std::runtime_error("TU1.5 BSP player transition requires native 14E1F4 relocation");
    visit_players(bank,pool,[&](auto player){bank.write(0x14e5dbu,player+0x2au,0xffffu,2);});

    // 14E5BC calls 154E70 after the final cluster store.
    const auto status=bank.read(0x4f54f0u);
    const auto status_pool=bank.read(0x4f54f4u);
    bank.write(0x154e80u,status+4u,1,1);
    bank.write(0x154e84u,status+5u,0,1);
    visit_players(bank,status_pool,[&](auto player) {
        if(bank.read(player+0x2cu)!=0xffffffffu)
            bank.write(0x154ed8u,status+4u,0,1);
        else if(bank.read(player+2u,1)&8u)
            bank.write(0x154ee4u,status+4u,0,1);
        else
            bank.write(0x154eeau,status+5u,1,1);
    });
}
}

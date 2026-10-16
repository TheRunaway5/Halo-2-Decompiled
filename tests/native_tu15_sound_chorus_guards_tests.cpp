#include "halo2/native_tu15_sound_chorus.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2::tu15_sound_chorus;
static bool same(const std::optional<ActiveState>&a,const std::optional<ActiveState>&b){
    if(a.has_value()!=b.has_value())return false;
    if(!a)return true;
    return a->cursor==b->cursor&&a->cached_end==b->cached_end&&a->bindings.audio.data()==b->bindings.audio.data()&&a->bindings.modulation.data()==b->bindings.modulation.data()&&a->bindings.output.data()==b->bindings.output.data();
}
int main(){unsigned checked=0;
    for(unsigned test=0;test<46;test++){
        Controls c{Word24(0x54),{},AddressingMode::linear,0,400};auto p=authored_parameters();State s{Word24(2),std::nullopt};
        std::array<Word24,33>a{},m{},o{};std::array<Word24,433>h{};std::array<SampleTrace,32>t{};
        Bindings b{std::span<const Word24,32>(a.data(),32),std::span<const Word24,32>(m.data(),32),std::span<Word24,32>(o.data(),32)};
        if(test<11){constexpr unsigned mask[]={1u<<10,1u<<11,3u<<10,1u<<20,1u<<17,1u<<14,1u<<18,1u<<12,1u<<13,1u<<15,1u<<16};c.entry_status=Word24(c.entry_status.bits|mask[test]);}
        if(test>=11&&test<21)c.addressing[(test-11)/2]=(test-11)%2?AddressingMode::reverse_carry:AddressingMode::modulo;
        if(test==21)c.live_begin_offset=1;
        if(test==22)c.live_span=399;
        if(test>=23&&test<=25)s.flags=Word24(3);
        if(test==24)s.active=ActiveState{b,401,400};
        if(test==25)s.active=ActiveState{b,0,399};
        if(test==26)c.entry_status.bits=0x1000000;
        if(test==27)p.gain.bits=0x1000000;
        if(test==28)p.depth.bits=0x1000000;
        if(test==29)s.flags.bits=0x1000000;
        if(test==30)a[31].bits=0x1000000;
        if(test==31)m[31].bits=0x1000000;
        if(test==32)o[31].bits=0x1000000;
        if(test==33)h[400].bits=0x1000000;
        if(test==34||test==35)b.modulation=std::span<const Word24,32>(a.data()+(test==35),32);
        if(test==36||test==37)b.output=std::span<Word24,32>(a.data()+(test==37),32);
        if(test==38||test==39)b.output=std::span<Word24,32>(m.data()+(test==39),32);
        if(test==40)b.audio=std::span<const Word24,32>(h.data(),32);
        if(test==41)b.modulation=std::span<const Word24,32>(h.data(),32);
        if(test==42)b.output=std::span<Word24,32>(h.data(),32);
        if(test==43){p.depth=Word24(0x7fffff);m.fill(Word24(0x800000));}
        if(test==44)c.state_relative=AddressingMode::modulo;
        if(test==45)c.state_relative=AddressingMode::reverse_carry;
        auto old_p=p;auto old_flags=s.flags;auto old_active=s.active;auto old_a=a;auto old_m=m;auto old_o=o;auto old_h=h;auto old_t=t;
        bool rejected=false;try{process(c,p,s,b,std::span<Word24,401>(h.data(),401),t);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected||p!=old_p||s.flags!=old_flags||!same(s.active,old_active)||a!=old_a||m!=old_m||o!=old_o||h!=old_h||t!=old_t)throw std::runtime_error("Unsupported input mutated caller objects");
        checked++;
    }
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<checked<<"}\n";
}

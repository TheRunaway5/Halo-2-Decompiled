#include "halo2/cache.hpp"
#include "halo2/kernels.hpp"
#include "halo2/original_data.hpp"
#include "halo2/scene.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
bool same(float a,float b) {return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(const ScreenEffects &a,const ScreenEffects &b) {
    return same(a.fade_color.x,b.fade_color.x) && same(a.fade_color.y,b.fade_color.y) &&
        same(a.fade_color.z,b.fade_color.z) && same(a.fade,b.fade) &&
        same(a.letterbox,b.letterbox) && same(a.hud,b.hud);
}
}
int main(int argc,char **argv)try {
    if(argc!=2 && argc!=3)throw FormatError("Supply TU15_XBE [--hold-render]");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const bool held=argc==3 && std::string(argv[2])=="--hold-render";
    check(argc==2 || held,"Unknown screen interpolation control");
    LevelContent content;content.original_data=OriginalData::load_xbe(argv[1]);
    const auto sample=[&](const Presentation &p,float f){return held?p.effects():p.effects(f);};
    unsigned boundaries{},subframes{},moving{};
    for(int duration:{1,15,30,75})for(bool out:{false,true}) {
        Presentation p(content);p.fade({.25f,.5f,.75f},out,duration);
        p.letterbox(true,false);p.hud(0,duration);
        for(int tick=0;tick<duration+3;++tick) {
            const auto before=p.effects();
            check(same(sample(p,0),before),"Fraction zero changed a completed source screen value");
            for(float f:{1.f/240,1.f/120,.125f,.25f,.5f,.75f,1.f}) {
                const auto s=sample(p,f);
                const auto phase=std::clamp((float(tick)+f)/duration,0.f,1.f);
                const auto curve=fade_curve_fraction(phase,content.original_data->fade_curve);
                const auto alpha=out?curve:1.f-curve;
                check(same(s.fade,alpha),"Screen fade remains capped or departs from the actual Xbox curve");
                check(std::abs(s.letterbox-std::clamp((float(tick)+f)/30,0.f,1.f))<1e-7f &&
                    std::abs(s.hud-(1.f-phase))<1e-7f,"Letterbox or HUD opacity remains tick-stepped");
                check(same(p.effects(),before),"Render sampling advanced source screen state");
                if(!same(s.fade,before.fade) || !same(s.letterbox,before.letterbox))++moving;
                ++subframes;
            }
            const auto end=sample(p,1);p.tick();
            check(same(end,p.effects()),"Screen interpolation jumps at an authored tick boundary");
            ++boundaries;
        }
    }
    check(moving>300,"Fractional screen samples lost their moving-curve coverage");
    Presentation cuts(content);cuts.fade({1,0,0},false,75);
    for(int i=0;i<23;++i)cuts.tick();
    cuts.fade({0,0,1},true,30);
    const auto fresh=sample(cuts,.5f);
    check(fresh.fade_color.z==1 && fresh.fade_color.x==0 &&
        same(fresh.fade,fade_curve_fraction(.5f/30,content.original_data->fade_curve)),
        "New fade command blended with the departed fade/color");
    for(bool out:{false,true}) {
        cuts.fade({.1f,.2f,.3f},out,0);cuts.letterbox(out,true);cuts.hud(out?0:1,0);
        check(same(sample(cuts,.5f),cuts.effects()) && same(sample(cuts,1),cuts.effects()),
            "Immediate screen command was smoothed across its cut");
    }
    cuts.fade({},false,30);cuts.letterbox(true,false);cuts.hud(0,30);
    for(int i=0;i<11;++i)cuts.tick();
    const Presentation saved=cuts;const auto expected=sample(cuts,.375f);
    for(int i=0;i<50;++i)cuts.tick();cuts.fade({1,1,1},true,0);cuts.restore(saved);
    check(same(sample(cuts,.375f),expected),"Screen restore borrowed the discarded timeline");
    const auto before=cuts.effects();
    for(float invalid:{-1.f,1.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        bool failed{};try{(void)cuts.effects(invalid);}catch(const FormatError &){failed=true;}
        check(failed && same(cuts.effects(),before),"Invalid render fraction mutated or entered a screen transition");
    }
    std::cout<<"PASS actual Xbox fade table: "<<subframes<<" read-only subframes, "<<boundaries
        <<" exact tick boundaries, HUD/letterbox, immediate cuts, fade replacement and restore\n";
    std::cout<<"LIMIT native fractional curve presentation; complete transitions/capture order and Xbox pixels remain separate\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

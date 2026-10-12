#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/multiplayer.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {void check(bool value,const char *why){if(!value)throw FormatError(why);}}
int main(int argc,char **argv)try {
    check(argc==5,"Supply MAP SHARED XBE ORIGINAL_FIXTURE");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
    Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    const auto content=cache.load_content(0,true);
    const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),
        [](const auto &entry){return entry.second.weapon_type==2;});
    check(weapon!=content.weapons.end(),"Missing actual Needler");
    auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=weapon->first;
    rules.score_limit=rules.time_ticks=0;
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    const auto id=match.join("Needler projectile publication");std::array<Controls,16> controls{};
    for(unsigned tick=0;tick<90;++tick)match.step(controls);
    bool saw_flight{};unsigned births{};std::uint32_t auxiliary{};
    auto streams=match.source_effect_random(auxiliary);
    for(unsigned tick=0;tick<20;++tick) {
        const auto before=streams.primary;const auto loaded=match.unit(id).weapon.loaded;
        controls[id].fire=true;match.step(controls);
        if(match.unit(id).weapon.loaded<loaded) {
            auto expected=before;
            // The existing native spread consumes two draws before FC2E0's
            // two constructor draws. This checks the real call site; source
            // spread/creation descriptor parity is not asserted here.
            for(unsigned draw=0;draw<4;++draw)expected=expected*0x19660du+0x3c6ef35fu;
            check(streams.primary==expected,"Live needle omitted its original constructor RNG writes");
            ++births;
        }
        const auto published=match.state().weapon_projectiles;
        const auto presentation=match.simulation().presentation_projectiles();
        check(published.size()==presentation.size(),"Match omitted live needle publication");
        for(const auto &projectile:presentation) {
            saw_flight=true;
            check(projectile.traveled>0,"Live needle publishes zero traveled distance to its authored color function");
            const auto replica=std::find_if(published.begin(),published.end(),[&](const auto &p){
                return p.datum==projectile.datum && p.definition==projectile.definition;
            });
            check(replica!=published.end() && replica->traveled==projectile.traveled,
                "Match publication lost actual needle owner or traveled distance");
        }
    }
    check(saw_flight&&births>1,"Actual match produced insufficient needles for publication check");
    check(weapon->second.needler_projectile.has_value(),
        "Needler lacks its original projectile constructor owner");
    const auto &definition=*weapon->second.needler_projectile;
    std::ifstream fixture(argv[4]);check(bool(fixture),"Missing original constructor fixture");
    unsigned cases{};
    for(std::string line;std::getline(fixture,line);) {
        if(line.empty()||line[0]=='#')continue;
        std::array<std::uint32_t,14> words;std::istringstream row(line);
        for(auto &word:words)check(bool(row>>std::hex>>word),"Truncated original constructor row");
        auto rng=words[0];const auto f=[&](unsigned i){return std::bit_cast<float>(words[i]);};
        const auto birth=source_needler_projectile_birth(definition,{1,0,0},{f(2),f(3),f(4)},30,rng);
        const std::array actual{birth.timer_step,birth.arming_step,birth.speed_scale,
            birth.angular_velocity,birth.guidance_rate,birth.speed,
            birth.velocity.x,birth.velocity.y,birth.velocity.z};
        check(rng==words[1],"Needler constructor changed original RNG write count/order");
        for(unsigned i=0;i<actual.size();++i)
            if(std::bit_cast<std::uint32_t>(actual[i])!=words[5+i])
                throw FormatError("Needler constructor differs from original at row "+
                    std::to_string(cases)+" field "+std::to_string(i));
        ++cases;
    }
    check(cases==200,"Original Needler constructor cases omitted");
    std::cout<<"PASS "<<cases<<" original Needler constructor cases: exact timer, guidance, speed, velocity and two source RNG writes\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

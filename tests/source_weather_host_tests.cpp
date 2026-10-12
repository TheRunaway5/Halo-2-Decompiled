#include "halo2/simulation.hpp"
#include <bit>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
std::uint32_t word(float value){return std::bit_cast<std::uint32_t>(value);}
}
int main(int argc,char **argv){try {
    if(argc!=2)throw std::runtime_error("original Backwash cache required");
    Cache cache(argv[1]);const auto content=cache.load_content(0,true);
    if(cache.spawns().empty())throw std::runtime_error("Backwash has no authored spawn");
    Simulation simulation(content,0,cache.spawns().front());
    simulation.start_level();simulation.step({});
    require(simulation.scripts().diagnostics().empty(),"authored Backwash startup host fault");
    // The actual compiled startup commands are weather_start(0), then
    // weather_change_intensity(0,.3). The retained instruction fixture covers
    // their zero-duration clamp and these exact resulting float words.
    const auto startup=simulation.weather();
    require(startup.stopped==0 && startup.transition==3 &&
            word(startup.current)==0x3f800000u && word(startup.target)==0x3e99999au &&
            word(startup.rate)==0xc5dac000u,"Backwash commands lost original arguments/order");
    simulation.advance_weather_view(300.125,1);
    require(word(simulation.weather().current)==word(startup.current),"secondary view advances weather");
    simulation.advance_weather_view(300.125,0);
    require(word(simulation.weather().current)==word(startup.current),"first view invents elapsed weather time");
    simulation.advance_weather_view(300.158333333,0);
    require(word(simulation.weather().current)==0x3e99999au &&
            simulation.weather().transition==0,"render-time Backwash startup fade did not finish");
    simulation.request_checkpoint();simulation.step({});
    require(simulation.has_checkpoint(),"weather checkpoint unavailable");
    // The checkpoint owns the 24-byte retained record. Render-clock globals
    // are outside it;398D0 imports the restored record at the next source view0.
    for(unsigned tick=0;tick<20000 && simulation.weather().transition!=2 &&
            word(simulation.weather().target)==0x3e99999au;++tick)
        simulation.step({});
    require(simulation.scripts().diagnostics().empty(),"Backwash weather continuation fault");
    require(simulation.weather().transition==2 ||
            word(simulation.weather().target)!=0x3e99999au,
            "authored Backwash continuation never changes weather");
    simulation.advance_weather_view(360.2,0);
    require(word(simulation.weather().current)!=0x3e99999au,
            "weather did not diverge before checkpoint restore");
    simulation.request_revert();simulation.step({});
    require(simulation.checkpoint_restores()==1,"weather checkpoint not restored");
    simulation.advance_weather_view(360.2,0);
    require(word(simulation.weather().current)==0x3e99999au &&
            word(simulation.weather().target)==0x3e99999au &&
            simulation.weather().transition==0,"source retained weather record not restored");
    std::cout<<"Original Backwash startup commands, first-view clock and retained weather checkpoint passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

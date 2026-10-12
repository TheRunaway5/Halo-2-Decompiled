#include "halo2/source_game_time.hpp"
#include "halo2/source_weather.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using Words=std::array<std::uint32_t,9>;
float scalar(std::uint32_t value){return std::bit_cast<float>(value);}
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
void ownership_checks() {
    using namespace halo2;
    SourceGameTime bank;source_game_time_initialize(bank,30);auto *time=&bank;
    SourceGameTimeContext context{true,true,0,30,false};
    double realtime=0.;SourceWeatherClock weather_clock;SourceWeatherState weather;
    source_weather_change(weather,1.f,.3f);
    for(unsigned frame=0;frame<180;++frame) {
        auto step=source_game_time_advance(time,context,1.f/60.f);
        const auto consumed=source_game_time_consume(time,step,[]{return false;},[&]{++time->tick;});
        require(consumed==step.ticks,"admitted ticks were dropped");
        source_world_time_accumulate(realtime,step.elapsed);
        source_weather_view_tick(weather,weather_clock,realtime,0);
        const auto clock=std::bit_cast<std::array<std::uint32_t,2>>(weather_clock);
        for(std::int16_t slot=1;slot<4;++slot)
            require(!source_weather_view_tick(weather,weather_clock,realtime,slot),"secondary view admitted shared clock");
        require(clock==std::bit_cast<std::array<std::uint32_t,2>>(weather_clock),"split-screen altered shared clock");
    }
    require(bank.tick==90 && weather.current==.3f,"governor did not feed weather transition");
    bank.paused=1;const auto before=realtime;
    auto step=source_game_time_advance(time,context,.1f);
    source_world_time_accumulate(realtime,step.elapsed);
    require(step.ticks==0 && realtime==before,"pause advanced weather time");
    bank.paused=0;step=source_game_time_advance(time,context,1.f);
    require(step.ticks==2,"original ordinary tick cap absent");
    source_game_time_consume(time,step,[]{return true;},[]{throw std::runtime_error("aborted tick ran");});
    require(step.elapsed==0.f,"pre-tick abort admitted weather time");
    context.simulation_mode=2;bool rejected=false;
    try{source_game_time_advance(time,context,.01f);}catch(const std::invalid_argument &){rejected=true;}
    require(rejected,"missing network producer received a default budget");
    // Fresh map owner resets independently of elapsed render time or a former match tick.
    source_game_time_initialize(bank,30);realtime=0.;weather_clock={};
    require(bank.tick==0 && bank.leftover_ticks==0.f && realtime==0.,"new map inherited time");
}
}
int main(int argc,char **argv){try {
    // The instruction oracle uses original round-nearest PC53 and SSE state.
    std::fesetround(FE_TONEAREST);_mm_setcsr(0x1f80);
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    const std::uint16_t cw=0x027f;asm volatile("fldcw %0"::"m"(cw));
#endif
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream fixture;
    if(!probe) {
        if(argc!=2)throw std::runtime_error("original game-time fixture required");
        fixture.open(argv[1]);
        if(!fixture)throw std::runtime_error("original game-time fixture unavailable");
        ownership_checks();
    }
    auto &stream=probe?std::cin:fixture;std::string line;unsigned cases=0;
    while(std::getline(stream,line)) {
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);row>>std::hex;std::array<std::uint32_t,31> input{};std::uint64_t clock{};
        for(auto &value:input)if(!(row>>value))throw std::runtime_error("incomplete game-time input");
        if(!(row>>clock))throw std::runtime_error("incomplete game-time clock");
        Words words{},replacement{};std::copy_n(input.begin()+1,9,words.begin());
        std::copy_n(input.begin()+10,9,replacement.begin());
        auto bank=std::bit_cast<halo2::SourceGameTime>(words);
        auto other=std::bit_cast<halo2::SourceGameTime>(replacement);auto *time=&bank;
        const halo2::SourceGameTimeContext context{bool(input[19]),bool(input[20]),
            std::bit_cast<std::int8_t>(std::uint8_t(input[21])),
            std::bit_cast<std::int16_t>(std::uint16_t(input[22])),bool(input[23])};
        unsigned network_calls=0,stop_calls=0,tick_calls=0;std::int32_t consumed=0;
        halo2::SourceGameTimeStep step{scalar(input[24]),std::bit_cast<std::int32_t>(input[25])};
        double realtime=std::bit_cast<double>(clock);
        const auto network=[&] {
            ++network_calls;return halo2::SourceGameTimeBudget{std::bit_cast<std::int32_t>(input[27]),
                bool(input[28]),input[29]?&other:&bank};
        };
        const auto stop=[&]{return std::int32_t(stop_calls++)==std::bit_cast<std::int32_t>(input[30]);};
        const auto tick=[&]{++tick_calls;time->tick=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(time->tick)+1u);};
        switch(input[0]) {
        case 0:halo2::source_game_time_initialize(bank,context.configured_tick_rate);break;
        case 1:halo2::source_game_time_speed(bank,context.configured_tick_rate,scalar(input[24]));break;
        case 2:halo2::source_game_time_speed_ramp(bank,context.configured_tick_rate,
            scalar(input[24]),scalar(input[25]),scalar(input[26]));break;
        case 3:case 6:
            step=halo2::source_game_time_advance(time,context,scalar(input[24]),network);
            if(input[0]==3)break;
            [[fallthrough]];
        case 4:
            consumed=halo2::source_game_time_consume(time,step,stop,tick);
            if(input[0]==4)break;
            [[fallthrough]];
        case 5:halo2::source_world_time_accumulate(realtime,step.elapsed);break;
        default:throw std::runtime_error("unknown game-time operation");
        }
        words=std::bit_cast<Words>(bank);replacement=std::bit_cast<Words>(other);
        std::array<std::uint32_t,26> output{};std::copy(words.begin(),words.end(),output.begin());
        std::copy(replacement.begin(),replacement.end(),output.begin()+9);
        output[18]=std::bit_cast<std::uint32_t>(step.elapsed);output[19]=std::bit_cast<std::uint32_t>(step.ticks);
        output[20]=std::bit_cast<std::uint32_t>(consumed);output[21]=network_calls;
        output[22]=stop_calls;output[23]=tick_calls;
        const auto bits=std::bit_cast<std::uint64_t>(realtime);output[24]=std::uint32_t(bits);output[25]=std::uint32_t(bits>>32);
        if(probe){for(auto value:output)std::cout<<std::hex<<value<<' ';std::cout<<'\n';}
        else for(unsigned field=0;field<output.size();++field) {
            std::uint32_t expected;
            if(!(row>>expected) || expected!=output[field])throw std::runtime_error("original game time differs: case "+
                std::to_string(cases)+" field "+std::to_string(field));
        }
        std::string extra;if(row>>extra)throw std::runtime_error("trailing game-time fixture data");++cases;
    }
    if(!cases)throw std::runtime_error("empty game-time fixture");
    if(!probe)std::cout<<cases<<" original game-time cases and governed weather-clock ownership passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

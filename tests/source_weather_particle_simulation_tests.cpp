#include "halo2/source_weather_particle_simulation.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <xmmintrin.h>

using namespace halo2;
int main(int argc,char **argv) {try {
    if(argc!=2)throw std::runtime_error("weather particle simulation fixture required");
    std::ifstream file(argv[1]);if(!file)throw std::runtime_error("weather particle simulation fixture unavailable");
    std::string line;std::size_t cases=0,checked=0,guards=0,rsqrt_pairs=0,draws=0;
    while(std::getline(file,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;
        auto next=[&]{std::uint32_t v;if(!(row>>v))throw std::runtime_error("weather particle fixture truncated");return v;};
        auto bank=[&](std::size_t n){std::vector<std::uint32_t> v(n);for(auto &a:v)a=next();return v;};
        auto seed=next();const auto elapsed=next(),intensity=next(),portal=next(),view_count=next(),fcw=next(),mxcsr=next(),count=next();
        auto system=bank(35),view=bank(61),weather=bank(44),render=bank(std::size_t(count)*5),state=bank(std::size_t(count)*8);
        const auto exclusion_count=next();
        std::vector<SourceWeatherExclusionVolume> volumes;
        std::vector<std::vector<std::array<float,4>>> planes(exclusion_count);
        for(std::uint32_t i=0;i<exclusion_count;++i) {
            SourceWeatherExclusionVolume v;
            for(auto &a:v.center)a=std::bit_cast<float>(next());v.radius=std::bit_cast<float>(next());
            v.plane_count=std::bit_cast<std::int32_t>(next());
            if(v.plane_count<0 || v.plane_count>1024)throw std::runtime_error("fixture plane count unsupported");
            planes[i].resize(std::size_t(v.plane_count));
            for(auto &p:planes[i])for(auto &a:p)a=std::bit_cast<float>(next());
            v.planes=planes[i];volumes.push_back(v);
        }
        struct Isa {std::uint32_t index,operand,answer;};
        const auto isa_count=next();std::vector<Isa> isa(isa_count);
        for(auto &v:isa){v.index=next();v.operand=next();v.answer=next();}
        const auto old_system=system,old_render=render,old_state=state;const auto old_seed=seed;
        std::size_t consumed=0;
        SourceWeatherParticleSimulationInput input{std::as_bytes(std::span(view)),std::as_bytes(std::span(weather)),volumes,
            std::bit_cast<std::int16_t>(std::uint16_t(view_count)),std::bit_cast<float>(elapsed),
            std::bit_cast<float>(intensity),std::bit_cast<float>(portal),std::uint16_t(fcw),mxcsr,
            [&](std::size_t index,std::uint32_t operand){
                if(consumed>=isa.size() || index!=isa[consumed].index || operand!=isa[consumed].operand)
                    throw std::runtime_error("source RSQRT operands differ at call "+std::to_string(consumed));
                return isa[consumed++].answer;
            }};
        source_weather_advance_particle_elapsed(std::as_writable_bytes(std::span(system)),input.elapsed);
        const auto result=source_weather_step_ready_particles(std::as_bytes(std::span(system)),
            std::as_writable_bytes(std::span(render)),std::as_writable_bytes(std::span(state)),input,seed);
        if(next()!=seed || next()!=unsigned(result.source_AL) || next()!=result.auxiliary_draws || next()!=result.rsqrt_calls || consumed!=isa.size())
            throw std::runtime_error("weather particle return/RNG/ISA counts differ");
        rsqrt_pairs+=consumed;draws+=result.auxiliary_draws;
        for(auto *output:{&system,&render,&state})for(std::size_t i=0;i<output->size();++i) {
            const auto expected=next();
            if(expected!=(*output)[i])throw std::runtime_error("weather particle bank differs case "+std::to_string(cases)+" bank "+
                (output==&system?"system":output==&render?"render":"state")+" word "+std::to_string(i)+" expected "+std::to_string(expected)+" actual "+std::to_string((*output)[i]));
            ++checked;
        }
        checked+=4;
        std::string extra;if(row>>extra)throw std::runtime_error("weather particle fixture trailing data");
        // Unsupported ownership/ISA/control state is rejected before any mutation.
        for(unsigned kind=0;kind<(cases==0?18u:8u);++kind) {
            auto s=old_system,r=old_render,t=old_state;auto aux=old_seed;auto invalid=input;
            if(kind==0)invalid.rsqrt={};
            if(kind==1)invalid.x87_control=0x027f;
            if(kind==2)invalid.mxcsr|=0x8000;
            if(kind==3)invalid.elapsed=std::bit_cast<float>(0x7fc12345u);
            if(kind==4)invalid.view=invalid.view.first(243);
            if(kind==5)invalid.weather=invalid.weather.first(175);
            if(kind==6)invalid.portal_distance=std::bit_cast<float>(0x7f800000u);
            if(kind==8)invalid.rsqrt=[](std::size_t,std::uint32_t){return 0u;};
            if(kind==9)invalid.rsqrt=[](std::size_t,std::uint32_t)->std::uint32_t{throw std::runtime_error("explicit ISA producer failure");};
            if(kind==10)t[0]=0x7f7fffffu;
            if(kind==11)invalid.rsqrt=[](std::size_t,std::uint32_t){return 0x7f800000u;};
            if(kind==12){s[32]=0x7f7fffffu;invalid.elapsed=std::bit_cast<float>(0x7f7fffffu);}
            if(kind==13)invalid.view=std::as_bytes(std::span(r)).first(244);
            const auto host_control=_mm_getcsr();
            if(kind==15)_mm_setcsr(host_control|0x8000u);
            if(kind==16)invalid.rsqrt=[&](std::size_t,std::uint32_t){_mm_setcsr(host_control|0x40u);return isa[0].answer;};
            if(kind==17)_mm_setcsr((host_control&~0x6000u)|0x2000u);
            const auto before_s=s,before_r=r,before_t=t;
            bool rejected=false;
            try {source_weather_step_ready_particles(std::as_writable_bytes(std::span(s)).first(kind==7?139:140),
                kind==14?std::as_writable_bytes(std::span(t)).first(r.size()*4):std::as_writable_bytes(std::span(r)),
                std::as_writable_bytes(std::span(t)),invalid,aux);}
            catch(const std::exception&){rejected=true;}
            _mm_setcsr(host_control);
            if(!rejected || s!=before_s || r!=before_r || t!=before_t || aux!=old_seed)
                throw std::runtime_error("weather particle pre-mutation guard failed");
            ++guards;
        }
        ++cases;
    }
    if(!((cases==1 && checked==13351) || (cases==46 && checked==33306)))throw std::runtime_error("actual weather particle corpus incomplete");
    std::cout<<cases<<" complete ready38450 calls (1actual,remainingcontrolled); "<<checked<<" native words exact; "
        <<rsqrt_pairs<<" explicitRSQRT pairs; "<<draws<<" auxiliary draws; "<<guards<<" ownership/control/failed-producer guards\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

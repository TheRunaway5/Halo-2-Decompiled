#include "halo2/source_scenery_idle_phase.hpp"
#include "halo2/model.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::array<std::uint32_t,7> words(const SourceSceneryIdlePhaseState &s){return {
    s.delay,s.status,s.playback_flags,std::bit_cast<std::uint32_t>(s.playback_rate),
    std::bit_cast<std::uint32_t>(s.frame),s.crossed_events,s.next_events};}
SourceSceneryIdlePhaseState state(const std::array<std::uint32_t,7>& w){
    return {std::bit_cast<float>(w[4]),std::bit_cast<float>(w[3]),std::uint8_t(w[0]),
        std::uint8_t(w[1]),std::uint16_t(w[2]),std::uint16_t(w[5]),std::uint16_t(w[6])};}
template<class F>void rejected(F action){bool failed=false;try{action();}catch(const FormatError&){failed=true;}require(failed,"unproved idle path was admitted");}
std::filesystem::path map_path(const std::string &name){auto p=std::filesystem::path("assets/disc/maps")/(name+".map");
    if(!std::filesystem::exists(p))p=std::filesystem::path("assets/map-pack/maps")/(name+".map");return p;}
}
int main(int argc,char **argv){try{
    if(argc==2&&std::string_view(argv[1])=="--probe") {
        int frames;std::uint32_t step;std::array<std::uint32_t,7> w;
        while(std::cin>>frames>>step){for(auto&v:w)std::cin>>v;require(bool(std::cin),"truncated phase probe");
            SourceSceneryIdlePhaseDefinition d{0,0,std::int16_t(frames)};auto s=state(w);
            source_scenery_idle_phase_step(s,d,std::bit_cast<float>(step));
            const auto result=words(s);for(unsigned i=0;i<result.size();++i)std::cout<<(i?" ":"")<<result[i];std::cout<<'\n';}
        return 0;
    }
    constexpr auto step=std::bit_cast<float>(0x3d088889u);
    std::ifstream input("tests/fixtures/retail_scenery_idle_phase.tsv");require(bool(input),"missing original idle phase fixture");
    std::unique_ptr<Cache> cache;std::string previous,name;unsigned owners=0,samples=0,loops=0;
    while(input>>name){int placement,frames;std::uint32_t definition,graph,index;unsigned sample_count;
        std::array<std::uint32_t,7> initial;input>>placement>>definition>>graph>>index>>frames;
        for(auto &w:initial)input>>w;input>>sample_count;require(bool(input),"truncated idle constructor fixture");
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));previous=name;}
        const auto clip=cache->load_initialized_object_idle(definition);require(clip.has_value(),"source idle missing");
        const auto d=cache->load_source_scenery_idle_phase_definition(*clip);
        require(d.graph==graph&&d.animation==index&&d.frames==frames,"source idle phase metadata differs");
        auto s=source_scenery_idle_phase_initial(d,step);require(words(s)==initial,"original initialized phase bank differs");
        unsigned current=0;
        for(unsigned i=0;i<sample_count;++i){unsigned tick;std::array<std::uint32_t,7> expected;input>>tick;
            for(auto&w:expected)input>>w;require(bool(input),"truncated idle phase sample");
            require(tick>current,"idle fixture does not advance");
            while(current<tick){source_scenery_idle_phase_step(s,d,step);if(s.status&4u)++loops;++current;}
            require(words(s)==expected,"original initialized idle phase step differs");++samples;
        }
        require(current==1000,"idle fixture is not a thousand-step lifetime");++owners;
    }
    require(input.eof()&&owners==42&&samples==798,"incomplete original idle phase fixture");
    SourceSceneryIdlePhaseDefinition d{0,0,100};
    for(unsigned i=0;i<3;++i){auto changed=d;changed.event_counts[i]=1;rejected([&]{source_scenery_idle_phase_initial(changed,step);});}
    for(unsigned i=0;i<2;++i){auto changed=d;(i?changed.next_sibling:changed.previous_sibling)=0;rejected([&]{source_scenery_idle_phase_initial(changed,step);});}
    auto changed=d;changed.loop_frame=1;rejected([&]{source_scenery_idle_phase_initial(changed,step);});
    changed=d;changed.frames=1;rejected([&]{source_scenery_idle_phase_initial(changed,step);});
    rejected([&]{source_scenery_idle_phase_initial(d,-1);});
    std::ifstream steps("tests/fixtures/retail_scenery_idle_phase_step.tsv");
    require(bool(steps),"missing original controller-step fixture");unsigned step_cases=0;int frame_count;
    while(steps>>frame_count){std::uint32_t dt;std::array<std::uint32_t,7> initial_step,expected;
        steps>>dt;for(auto&w:initial_step)steps>>w;for(auto&w:expected)steps>>w;
        require(bool(steps),"truncated controller-step fixture");
        SourceSceneryIdlePhaseDefinition definition{0,0,std::int16_t(frame_count)};
        auto current=state(initial_step);source_scenery_idle_phase_step(current,definition,std::bit_cast<float>(dt));
        require(words(current)==expected,"original controller-step gates/arithmetic differ");++step_cases;
    }
    require(steps.eof()&&step_cases==201,"incomplete controller-step fixture");
    std::cout<<owners<<" original initialized SCEN controllers, "<<samples<<" frozen samples, "<<loops
             <<" exact loops and "<<step_cases<<" controller-step fixtures passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

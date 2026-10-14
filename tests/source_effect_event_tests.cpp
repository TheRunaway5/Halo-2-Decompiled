#include "halo2/source_effect_event.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
using Json=nlohmann::json;
float scalar(const Json &v){return std::bit_cast<float>(v.get<std::uint32_t>());}
std::uint32_t word(float v){return std::bit_cast<std::uint32_t>(v);}
Json replay(const Json &c) {
    const auto &v=c.at("state");
    SourceEffectEventState s{std::uint16_t(v[0]),std::int16_t(v[1]),std::int16_t(v[2]),
        scalar(v[3]),scalar(v[4]),scalar(v[5])};
    std::vector<SourceEffectEventDefinition> events;
    for(const auto &e:c.at("events"))events.push_back({scalar(e[0]),scalar(e[1]),scalar(e[2]),scalar(e[3]),scalar(e[4])});
    const auto state=[&] {return Json::array({s.flags,s.event,s.previous,word(s.elapsed),word(s.target),word(s.field_68)});};
    Json trace=Json::array();
    const auto record=[&](const char *kind,Json args=Json::array()) {
        trace.push_back({{"kind",kind},{"state",state()},{"args",args}});
    };
    const SourceEffectEventServices services{
        [&]{record("sync");},[&]{record("begin");},
        [&](float before,float interval){record("particles",Json::array({word(before),word(interval)}));},
        [&]{record("after");},[&]{record("retire");s.flags|=c.at("retire_flags").get<unsigned>();return c.at("retire_result").get<bool>();}};
    std::uint32_t random=c.at("random");bool result=false;int next=-999;
    if(c.at("action")=="select")result=source_effect_event_select(s,events,c.at("index"),random,services);
    else if(c.at("action")=="next")next=source_effect_event_next(s,events,c.at("loop"),random);
    else result=source_effect_event_advance(s,events,c.at("loop"),scalar(c.at("seconds")),random,services);
    return {{"state",state()},{"random",random},{"result",result},{"next",next},{"trace",trace}};
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=2)throw std::runtime_error("Supply original effect event fixture bank");
        std::ifstream file(argv[1]);if(!file)throw std::runtime_error("Missing original effect event fixtures");
        std::string line;unsigned cases{};
        while(std::getline(file,line)) {
            const auto fixture=Json::parse(line);
            if(replay(fixture.at("input"))!=fixture.at("original"))
                throw std::runtime_error("Original effect event mismatch at case "+std::to_string(cases));
            ++cases;
        }
        if(cases!=5917)throw std::runtime_error("Incomplete original effect event fixture bank");
        std::cout<<"PASS "<<cases<<" original effect clock/RNG/service traces\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

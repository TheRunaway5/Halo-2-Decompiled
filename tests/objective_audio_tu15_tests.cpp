#include "halo2/source_announcer_events_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;using Json=nlohmann::json;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string& value) {
    need(!(value.size()&1),"Odd original byte bank");std::vector<std::byte> out;
    for(unsigned n=0;n<value.size();n+=2)out.push_back(std::byte(std::stoul(value.substr(n,2),nullptr,16)));return out;
}
std::uint32_t word(std::span<const std::byte> bytes,unsigned at,unsigned size=4) {
    need(at<=bytes.size() && size<=bytes.size()-at,"Original bank extent");std::uint32_t result{};
    for(unsigned n=0;n<size;++n)result|=std::to_integer<std::uint32_t>(bytes[at+n])<<(n*8);return result;
}
VistaAnnouncerEvent event(const Json& row) {
    VistaAnnouncerEvent result;for(unsigned n=0;n<8;++n)result.words[n]=row.at(n);result.data=row.at(8);return result;
}
VistaAnnouncerResponse response(const Json& row) {
    const auto raw=unhex(row);need(raw.size()==0xa8,"Original response size");VistaAnnouncerResponse result;
    std::copy(raw.begin(),raw.end(),result.begin());return result;
}
VistaAnnouncerEventState bank(const std::string& hex) {
    const auto raw=unhex(hex);need(raw.size()==0xa0,"Original Xbox queue/recent extent");VistaAnnouncerEventState result;
    for(unsigned n=0;n<5;++n) {
        const auto at=n*12;auto& slot=result.queue.slots[n];slot.sound=word(raw,at);slot.voice=word(raw,at+4);
        slot.active=std::uint8_t(word(raw,at+8,1));slot.retained_byte=std::uint8_t(word(raw,at+9,1));
        slot.delay=std::bit_cast<std::int16_t>(std::uint16_t(word(raw,at+10,2)));
    }
    result.queue.count=word(raw,0x9c);
    for(unsigned n=0;n<4;++n) {
        const auto at=0x3c+n*24;result.recent[n].hash=word(raw,at);result.recent[n].tick=word(raw,at+20);
        for(unsigned cue=0;cue<4;++cue)result.recent[n].sounds[cue]=word(raw,at+4+cue*4);
    }
    return result;
}
}
int main(int argc,char** argv)try {
    need(argc==2,"Supply original TU1.5 audio reference");Json fixture;std::ifstream(argv[1])>>fixture;
    need(fixture.at("schema")=="H2_OBJECTIVE_AUDIO_TU15_V1" &&
        fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("supplied_service_results").empty(),"Audio reference authority differs");
    unsigned recipients{},selections{},queues{};
    for(const auto& row:fixture.at("recipients")) {
        std::vector<VistaAnnouncerResponse> rows;for(const auto& raw:row.at("rows"))rows.push_back(response(raw));
        const auto actual=source_announcer_recipient_tu15(rows,event(row.at("event")),
            {row.at("player"),1,row.at("membership"),0});
        need(bool(actual)!=row.at("expected").is_null(),"Original Xbox recipient admission differs");
        if(actual)need(actual->ordinal==row.at("expected"),"Original Xbox audience/response ordering differs");++recipients;
    }
    for(const auto& row:fixture.at("selections")) {
        std::vector<SourceAnnouncerAlternativeTu15> alternatives;
        for(const auto& raw:row.at("alternatives"))alternatives.push_back({raw.at("flags"),raw.at("sounds"),raw.at("weight")});
        auto random=row.at("seed").get<std::uint32_t>();const auto& expected=row.at("expected");
        const auto actual=source_announcer_response_tu15(response(row.at("response")),alternatives,
            row.at("language"),row.at("rate"),random,{row.at("fp").at(0),row.at("fp").at(1)});
        need(actual.sound==expected.at("sound") && actual.delay_ticks==expected.at("delay") &&
            actual.announcer==expected.at("announcer") && random==expected.at("seed"),"Original Xbox sound, weighting, language, delay or RNG differs");++selections;
    }
    for(const auto& row:fixture.at("queues")) {
        auto actual=bank(row.at("initial"));const auto expected=bank(row.at("expected"));const auto sound=row.at("sound").get<std::uint32_t>();
        if(source_announcer_remember_tu15(actual,event(row.at("event")),sound,row.at("tick"),row.at("rate"),{0x27f,0x1f80}) && sound!=0xffffffffu)
            vista_announcer_queue_enqueue(actual.queue,sound,row.at("delay"));
        need(actual.queue==expected.queue && actual.recent==expected.recent,"Original Xbox queue/recent publication differs");++queues;
    }
    need(recipients==375 && selections==72 && queues==36,"Original Xbox audio cohort is incomplete");
    std::cout<<"PASS "<<recipients<<" original recipient selections, "<<selections<<" weighted/language/delay choices, "<<queues<<" complete queue/recent banks\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

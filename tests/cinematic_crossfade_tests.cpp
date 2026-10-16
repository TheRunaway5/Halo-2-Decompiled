#include "halo2/cinematic_crossfade.hpp"
#include "halo2/campaign_frame_clock.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool condition,const std::string &message){if(!condition)throw std::runtime_error(message);}
float number(const nlohmann::json &value){return std::bit_cast<float>(value.get<std::uint32_t>());}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"Original TU1.5 crossfade fixture required");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("lifecycle").size()==408 && fixture.at("programs").size()==298 &&
        fixture.at("elapsed").size()==60,"Incomplete original crossfade fixture");
    CinematicCrossfadeState state;unsigned index=0;
    for(const auto &row:fixture.at("lifecycle")) {
      const auto op=row.at("op").get<std::string>();const double time=row.at("time");
      if(op=="start")state.screen_start(row.at("value").get<std::uint32_t>()!=0);
      else if(op=="stop")state.screen_stop();
      else if(op=="set")state.set(number(row.at("value")),time);
      else if(op=="update")state.update(time);
      else if(op=="frame") {
        state.update(time);const auto frame=state.frame();
        bool captured=false;unsigned programs=0;
        for(const auto &event:row.at("events")) {
          if(event.at("kind")=="capture")captured=true;
          if(event.at("kind")=="program") {
            check(frame.composite.has_value(),"Original crossfade composite was dropped at "+std::to_string(index)+
                " alpha="+std::to_string(state.alpha)+" strength="+std::to_string(state.output_strength));
            for(unsigned i=0;i<60;++i)check(frame.composite->words[i]==event.at("words").at(i).get<std::uint32_t>(),
                "Original frame program mismatch at "+std::to_string(index)+" word "+std::to_string(i));
            ++programs;
          }
        }
        check(captured==frame.capture && programs==unsigned(frame.composite.has_value()),
            "Original crossfade capture/composite scheduling mismatch at "+std::to_string(index));
        if(captured)state.capture_complete();
      } else throw std::runtime_error("Unknown fixture action");
      const auto &expected=row.at("state");
      check(state.active==(expected.at("active").get<unsigned>()!=0) &&
          state.initialized==(expected.at("initialized").get<unsigned>()!=0) &&
          state.pending==(expected.at("pending").get<unsigned>()!=0) &&
          state.capture==(expected.at("capture").get<unsigned>()!=0) &&
          bits(state.alpha)==expected.at("output").at(0).get<std::uint32_t>() &&
          bits(state.output_strength)==expected.at("output").at(1).get<std::uint32_t>() &&
          bits(state.strength)==expected.at("control").at(0).get<std::uint32_t>() &&
          bits(state.start)==expected.at("control").at(1).get<std::uint32_t>() &&
          bits(state.end)==expected.at("control").at(2).get<std::uint32_t>(),
          "Original crossfade state mismatch at "+std::to_string(index));
      ++index;
    }
    for(const auto &row:fixture.at("programs")) {
      const auto program=cinematic_crossfade_program(number(row.at("alpha")),number(row.at("strength")));
      unsigned count=0;
      for(const auto &event:row.at("events"))if(event.at("kind")=="program") {
        check(program.has_value(),"Original crossfade program was dropped");
        for(unsigned i=0;i<60;++i)check(program->words[i]==event.at("words").at(i).get<std::uint32_t>(),
            "Original crossfade alpha/program mismatch at "+std::to_string(index));
        ++count;
      }
      check(count==unsigned(program.has_value()),"Spurious native crossfade composite");++index;
    }
    for(const auto &row:fixture.at("elapsed")) {
      const auto actual=accepted_campaign_frame_elapsed(number(row.at("seconds")),row.at("requested"),row.at("completed"));
      check(bits(actual)==row.at("result").get<std::uint32_t>(),"Original render-clock elapsed correction mismatch");
    }
    check(fixture.at("target").at(4)==0x11229u && fixture.at("target").at(32)==640 &&
        fixture.at("target").at(33)==480,"Original crossfade capture format/extent changed");
    std::cout<<"408 exact TU1.5 crossfade state/order actions, 298 pixel programs, 60 render-clock corrections\n";
  } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}

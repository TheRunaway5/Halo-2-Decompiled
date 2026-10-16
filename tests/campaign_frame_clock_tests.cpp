#include "halo2/campaign_frame_clock.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
int main(int argc,char **argv) {
  if(argc!=2)throw std::runtime_error("TU1.5 frame-clock fixture required");
  std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
  if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
      fixture.at("cases").size()!=224)throw std::runtime_error("Incomplete original frame-clock fixture");
  unsigned count=0;
  const auto check=[&](CampaignFrameClock &clock,const auto &row) {
    const auto actual=clock.advance(std::bit_cast<float>(row["seconds"].template get<std::uint32_t>()),row["cinematic"],row["paused"]);
    if(actual.ticks!=row["ticks"].template get<int>() ||
        std::bit_cast<std::uint32_t>(actual.elapsed)!=row["elapsed"].template get<std::uint32_t>() ||
        std::bit_cast<std::uint32_t>(actual.fraction)!=row["fraction"].template get<std::uint32_t>())
      throw std::runtime_error("Original frame-clock mismatch at "+std::to_string(count));
    ++count;return actual.ticks;
  };
  for(const auto &row:fixture["cases"]) {
    const float before=std::bit_cast<float>(row["before"].get<std::uint32_t>());
    CampaignFrameClock clock(before);
    check(clock,row);
  }
  for(const auto &sequence:fixture["sequences"]) {
    CampaignFrameClock clock;int ticks=0;
    for(const auto &row:sequence["rows"])ticks+=check(clock,row);
    if(ticks!=sequence["total_ticks"].get<int>())throw std::runtime_error("Frame schedule lost original ticks");
    if(sequence["name"].get<std::string>().starts_with("one-second-") && ticks!=30)
      throw std::runtime_error("Slow cinematic rendering changed the authored one-second timeline");
  }
  std::cout<<count<<" exact TU1.5 frame-clock samples, including 1..120 fps cinematic sequences\n";
}

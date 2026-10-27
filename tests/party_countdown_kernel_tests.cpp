#include "halo2/frontend_matchmaking_progress.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Provide unchanged TU1.5 party-countdown controls");
  std::ifstream file(argv[1]);nlohmann::json fixture;file>>fixture;
  if(fixture.at("source_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
    throw std::runtime_error("Party-countdown controls lost their original TU1.5 authority");
  unsigned cases{};
  for(const auto &row:fixture.at("cases")) {
    const auto kind=row.at("kind").get<std::string>();
    if(kind=="press") {
      if(matchmaking_lobby_countdown_press(row.at("seconds"))!=row.at("value").get<int>())
        throw std::runtime_error("Native countdown press differs from unchanged original block");
    } else if(kind=="timer") {
      if(matchmaking_lobby_countdown_due(row.at("seconds"),row.at("elapsed").get<std::uint32_t>())!=row.at("published").get<bool>())
        throw std::runtime_error("Native countdown cadence differs from unchanged original block");
    } else continue;
    ++cases;
  }
  std::cout<<"PASS "<<cases<<" unchanged TU1.5 native party-countdown controls\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

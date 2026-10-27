#include "halo2/original_pcr_intro.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
int main(int argc,char **argv)try {
  if(argc!=2)return 1;
  std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
  if(fixture.at("source_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
    throw std::runtime_error("PCR intro requires the actual TU1.5 authority");
  unsigned count{};
  for(const auto &row:fixture.at("cases")) {
    const auto actual=halo2::original_pcr_intro_color(row.at("prior_stats"),row.at("opened"),row.at("now"),
        row.at("x87_control_word"),row.at("mxcsr"));
    if(row.at("color").is_null()?actual.has_value():!actual || *actual!=row.at("color").get<std::uint32_t>())
      throw std::runtime_error("PCR intro differs from unchanged23A0F3 at control"+std::to_string(count));
    ++count;
  }
  if(count!=128)throw std::runtime_error("PCR intro source controls are incomplete");
  std::cout<<"PASS"<<count<<" original PCR intro black/fade/wrap/prior-bypass controls\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

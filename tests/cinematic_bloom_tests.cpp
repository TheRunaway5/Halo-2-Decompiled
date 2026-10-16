#include "halo2/bloom.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
int main(int argc,char **argv) {
  if(argc!=2)throw std::runtime_error("TU1.5 fixture path required");
  std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
  if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
      fixture.at("cases").size()!=284)throw std::runtime_error("Incomplete original TU1.5 bloom fixture");
  unsigned count=0;
  const auto real=[](const auto &value){return std::bit_cast<float>(value.template get<std::uint32_t>());};
  for(const auto &row:fixture.at("cases")) {
    const auto parameters=cinematic_bloom_parameters(real(row["input"][0]),real(row["input"][1]),
        row["cinematic"],{row["enabled"],real(row["override"][0]),real(row["override"][1])});
    if(std::bit_cast<std::uint32_t>(parameters.threshold)!=row["effective"][0].get<std::uint32_t>() ||
        std::bit_cast<std::uint32_t>(parameters.strength)!=row["effective"][1].get<std::uint32_t>())
      throw std::runtime_error("Original cinematic bloom parameter mismatch at "+std::to_string(count));
    const auto actual=menu_bloom(parameters.threshold,parameters.strength);
    unsigned i=0;
    for(const auto *program:{&actual.threshold,&actual.blur,&actual.composite})for(auto word:program->words)
      if(word!=row["programs"][i++].get<std::uint32_t>())
        throw std::runtime_error("Original cinematic bloom program mismatch at "+std::to_string(count));
    for(unsigned j=0;j<5;++j)if(std::bit_cast<std::uint32_t>(actual.radii[j])!=row["radii"][j].get<std::uint32_t>())
      throw std::runtime_error("Original cinematic bloom schedule mismatch");
    ++count;
  }
  std::cout<<count<<" TU1.5 bloom overrides, 51120 exact program words and 1420 blur radii\n";
}

#include "halo2/source_camouflage_context.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char** argv){try{
  if(argc!=2)throw std::runtime_error("Supply original camouflage render-context fixture");
  std::ifstream file(argv[1]);nlohmann::json fixture;file>>fixture;unsigned count=0;
  for(const auto& version:fixture.at("versions").items()){
    const auto& results=version.value().at("render");
    for(unsigned i=0;i<fixture.at("render_inputs").size();++i){
      const auto& input=fixture.at("render_inputs")[i];const auto& objects=input.at("objects");
      std::vector<unsigned> reads;
      const auto raw=[&](std::uint32_t datum)->std::optional<SourceCamouflageObjectView>{
        const auto slot=datum&0xffffu;if(slot>=objects.size())throw std::runtime_error("Unbacked original object projection");
        const auto& object=objects[slot];if(object.value("null",false))return {};
        return SourceCamouflageObjectView{object.at("type"),object.value("parent",0xffffffffu)};
      };
      SourceCamouflageContextServices services;
      services.initial=[&](std::uint32_t datum)->std::optional<SourceCamouflageObjectView>{
        if(datum==0xffffffffu)return {};
        const auto slot=datum&0xffffu;if(slot>=objects.size())throw std::runtime_error("Unbacked initial original object");
        const auto salt=objects[slot].value("salt",0x8aaau);
        if(!salt || (datum>>16)!=salt)return {};
        return raw(datum);
      };
      services.parent=raw;services.primary=[&](std::uint32_t datum){const auto slot=datum&0xffffu;
        reads.push_back(slot);return std::bit_cast<float>(objects.at(slot).value("primary_bits",0u));};
      const auto actual=source_object_camouflage(input.at("datum"),services);
      if(std::bit_cast<std::uint32_t>(actual)!=results[i].at("scalar_bits") ||
          nlohmann::json(reads)!=results[i].at("primary_read_slots"))
        throw std::runtime_error(version.key()+" source camouflage scalar/read mismatch case "+std::to_string(i));
      ++count;
    }
  }
  if(count!=156)throw std::runtime_error("Incomplete original scalar-context coverage");
  std::cout<<count<<" whole original scalar results and selected primary reads passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

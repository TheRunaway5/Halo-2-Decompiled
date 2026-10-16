#include "halo2/scene.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
int main(int argc,char **argv) {
  if(argc!=2)throw std::runtime_error("TU1.5 object-function fixture required");
  std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
  if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
    throw std::runtime_error("Object-function fixture has the wrong reference version");
  LevelContent content;ScenarioObject object;object.type=6;object.definition=42;object.name=0;
  content.objects.push_back(object);content.object_names.push_back({"object",6,0});
  unsigned count=0;
  for(const auto &row:fixture.at("cases")) {
    SceneObjects scene(content,0);const auto handle=scene.create(0,true);
    scene.set_function_variable(handle,0x0c001b89,0,0,0);
    scene.set_function_variable(handle,0x0c001b89,1,std::bit_cast<float>(row["frames"].get<std::uint32_t>()),10);
    for(const auto &sample:row["samples"]) {
      const auto value=scene.function_variable(handle,0x0c001b89,sample[0]);
      if(!value || std::bit_cast<std::uint32_t>(*value)!=sample[1].get<std::uint32_t>())
        throw std::runtime_error("Original TU1.5 object shader interpolation mismatch");
      ++count;
    }
    scene.clear_function_variable(handle,0x0c001b89);
    if(scene.function_variable(handle,0x0c001b89,10))throw std::runtime_error("Cleared override still masks the authored function");
    scene.set_function_variable(handle,123,1,0,0);scene.destroy(handle);
    const auto replacement=scene.create(0);
    if(replacement==handle || scene.function_variable(replacement,123,0))
      throw std::runtime_error("Retired generation leaked a shader override");
  }
  if(count!=99)throw std::runtime_error("Incomplete original object-function sample bank");
  std::cout<<count<<" exact TU1.5 shader function samples; clear and generation retirement passed\n";
}

#include "halo2/vista_loading_assets.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message) {
  if (!value) throw std::runtime_error(message);
}
std::vector<std::byte> read(const std::filesystem::path& path) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  check(bool(input),"missing original D3DX readback");
  const auto length=input.tellg();
  check(length>=0 && length<=64*1024*1024,"invalid original readback length");
  std::vector<std::byte> result(static_cast<std::size_t>(length));
  input.seekg(0);
  check(bool(input.read(reinterpret_cast<char*>(result.data()),std::streamsize(result.size()))),
        "incomplete original D3DX readback");
  return result;
}
}
int main(int argc,char** argv) {
  try {
    if (argc!=3 || !std::filesystem::exists(std::filesystem::path(argv[2])/"report.json"))
      return 77;
    const auto assets=halo2::load_vista_loading_assets(argv[1]);
    std::ifstream report_file(std::filesystem::path(argv[2])/"report.json");
    const auto report=nlohmann::json::parse(report_file);
    unsigned compared{};
    std::size_t pixels{};
    for (const auto& original:report.at("textures")) {
      if (!original.contains("index")) continue;
      const unsigned index=original.at("index"),pool=original.at("pool");
      check(index<12 && pool<2,"invalid original readback index");
      const auto& image=assets[index];
      const auto& texture=image.texture;
      check(image.source_width==original.at("source_width") &&
            image.source_height==original.at("source_height"),"source geometry dimensions changed");
      check(texture.width==original.at("width") && texture.height==original.at("height") &&
            texture.format==11 && texture.mips.size()==1,"actual D3DX allocation changed");
      const auto filename="texture-"+std::string(index<10?"0":"")+std::to_string(index)+
                          "-pool"+std::to_string(pool)+".bgra";
      const auto raw=read(std::filesystem::path(argv[2])/filename);
      check(raw.size()==texture.pixels.size(),"actual D3DX payload length changed");
      for (std::size_t i=0;i<raw.size();i+=4) {
        check(texture.pixels[i]==raw[i+2] && texture.pixels[i+1]==raw[i+1] &&
              texture.pixels[i+2]==raw[i],"original decoded RGB changed");
        check(texture.pixels[i+3]==(original.at("format")==21?raw[i+3]:std::byte{255}),
              "A8/X8 sampled alpha differs from original format");
        ++pixels;
      }
      ++compared;
    }
    check(compared==24,"incomplete original memory-pool comparison");
    // The source picture and actual D3DX allocation intentionally differ.
    check(assets[0].source_width==1280 && assets[0].texture.width==2048 &&
          assets[6].source_width==1044 && assets[6].texture.width==2048,
          "source/allocated dimensions were conflated");
    std::cout<<"PASS12 Vista loading images, both real D3DX pools, "<<pixels
             <<" exact sampled pixels; independent source/allocation dimensions\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr<<error.what()<<'\n';
    return 1;
  }
}

#include "halo2/particle_vertex.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

int main(int argc,char **argv) {
  try {
    using namespace halo2;
    const bool compare=argc==5 && std::string_view(argv[2])=="--compare";
    if(argc!=4 && !compare)throw std::runtime_error("map/input/output or map/--compare/input/retail required");
    if(compare && (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]) ||
        !std::filesystem::exists(argv[4])))return 77;
    Cache cache(argv[1]);std::ifstream input(argv[compare?3:2],std::ios::binary),expected;
    std::ofstream output;
    if(compare)expected.open(argv[4],std::ios::binary);
    else output.open(argv[3],std::ios::binary);
    if(!input || (compare?!expected:!output))throw std::runtime_error("particle selection fixture missing");
    std::uint32_t count{};
    if(!input.read(reinterpret_cast<char*>(&count),4))throw std::runtime_error("short selection count");
    for(unsigned i=0;i<count;++i) {
      std::array<std::uint32_t,3> args;
      if(!input.read(reinterpret_cast<char*>(args.data()),sizeof(args)))throw std::runtime_error("short selection fixture");
      const auto selected=cache.load_particle_vertex_selection(args[0],args[1],args[2]);
      const std::array<std::uint32_t,5> header{selected.template_datum,selected.pass_datum,
        selected.vertex_datum,selected.classification,std::uint32_t(selected.program.size())};
      if(compare) {
        std::array<std::uint32_t,5> original;
        if(!expected.read(reinterpret_cast<char*>(original.data()),sizeof(original)) || original!=header)
          throw std::runtime_error("particle declaration selection differs from original executable");
        std::vector<std::byte> code(selected.program.size());
        if(!expected.read(reinterpret_cast<char*>(code.data()),code.size()) || code!=selected.program)
          throw std::runtime_error("selected particle program differs from original executable");
      } else {
        output.write(reinterpret_cast<const char*>(header.data()),sizeof(header));
        output.write(reinterpret_cast<const char*>(selected.program.data()),selected.program.size());
        if(!output)throw std::runtime_error("cannot save particle shader selection");
      }
    }
    if(input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra particle selection fixtures");
    if(compare && expected.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra original shader selection records");
    std::cout<<count<<" original particle shader selections\n";
    return 0;
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

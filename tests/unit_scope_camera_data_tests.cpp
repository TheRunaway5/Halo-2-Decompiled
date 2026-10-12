#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) {
  try {
    if(argc<3)throw std::runtime_error("Supply frozen original constants and owned update executable(s)");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    nlohmann::json report;std::ifstream(argv[1])>>report;
    const auto &fields=report.at("fields_in_native_struct_order");
    if(fields.size()!=24)throw std::runtime_error("Wrong original scope constant export");
    for(int image=2;image<argc;++image) {
      const auto data=halo2::OriginalData::load_xbe(argv[image]);
      if(!data->unit_scope_camera_constants)throw std::runtime_error("Missing verified scope image import");
      const auto &c=*data->unit_scope_camera_constants;
      const std::array<float,23> actual{
        c.default_fov,c.packet_minimum,c.packet_maximum,c.render_minimum,c.render_maximum,
        c.magnified_minimum,c.magnified_maximum,c.one,c.packet_multiplier,c.transition_seconds,
        c.frame_dt_multiplier,c.half,c.three,c.five,c.fifteen,c.six,c.ten,c.twenty,c.two,c.four,
        c.twelve,c.seven,c.acceleration_cap};
      const auto compare=[&](unsigned index,std::uint64_t value,unsigned size) {
        const auto bytes=fields.at(index).at("bits").get<std::string>();
        if(bytes.size()!=size*2)throw std::runtime_error("Truncated original operand export");
        for(unsigned byte=0;byte<size;++byte)
          if(((value>>(8*byte))&255)!=std::stoul(bytes.substr(byte*2,2),nullptr,16))
            throw std::runtime_error("Actual scope import differs at "+fields.at(index).at("field").get<std::string>());
      };
      for(unsigned i=0;i<actual.size();++i)compare(i,std::bit_cast<std::uint32_t>(actual[i]),4);
      compare(23,std::bit_cast<std::uint64_t>(c.reciprocal_numerator),8);
    }
    std::cout<<"All23 binary32 scope operands and the original binary64 numerator import exactly\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}

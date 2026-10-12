#include "halo2/unit_zoom.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc,char **argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply frozen original zoom HUD fixtures");
    std::ifstream in(argv[1]);if(!in)return 77;
    unsigned rows=0;
    for(std::string line;std::getline(in,line);) {
      if(line.empty())continue;
      std::istringstream row(line);
      std::string name,mask;
      int current,present,count,n,expected_current,expected_binocular;
      if(!(row>>name>>current>>present>>count>>n))throw std::runtime_error("Malformed original HUD input row");
      std::vector<std::int16_t> modes;
      for(int i=0;i<n;++i) {
        int mode;if(!(row>>mode))throw std::runtime_error("Missing original magazine mode");
        modes.push_back(std::int16_t(mode));
      }
      if(!(row>>expected_current>>expected_binocular>>mask))throw std::runtime_error("Malformed original HUD output row");
      halo2::ZoomWeaponView weapon{{0,std::int16_t(count),0.0f,0.0f},modes};
      const auto state=halo2::source_zoom_hud_status(std::int8_t(current),present?&weapon:nullptr);
      if(state.current!=expected_current || state.binocular!=expected_binocular ||
          halo2::source_zoom_hud_mask(state)!=std::stoul(mask,nullptr,16))
        throw std::runtime_error("Original HUD status/mask mismatch in "+name);
      ++rows;
    }
    if(rows!=309)throw std::runtime_error("Incomplete frozen HUD suite");
    std::cout<<rows<<" original zoom HUD status/mask cases matched\n";
    return 0;
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

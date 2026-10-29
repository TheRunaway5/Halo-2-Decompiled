#include "halo2/source_object_emblem.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply unchanged TU1.5 player color fixture");
  std::ifstream input(argv[1]);
  if(!input)throw FormatError("Missing TU1.5 player color fixture");
  std::vector<std::array<std::uint32_t,3>> palette,teams;
  unsigned cases{};
  for(std::string line;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string kind;row>>kind;
    if(kind=="palette" || kind=="team") {
      auto &bank=kind=="palette"?palette:teams;
      unsigned index;std::array<std::uint32_t,3> rgb;
      row>>index>>std::hex>>rgb[0]>>rgb[1]>>rgb[2];
      if(index!=bank.size())throw FormatError("Unordered original player color palette");
      bank.push_back(rgb);
    } else {
      if(kind.size()!=8 || palette.size()!=18 || teams.size()!=9)
        throw FormatError("Incomplete original player color inputs");
      SourcePlayerConfiguration profile{};
      for(unsigned i=0;i<4;++i)profile[0x40+i]=std::uint8_t(std::stoul(kind.substr(i*2,2),nullptr,16));
      int team,neutral;row>>team>>neutral;
      const auto colors=source_player_appearance_colors(profile,std::int16_t(team),neutral,palette,teams);
      for(const auto &rgb:colors)for(const float value:{rgb.x,rgb.y,rgb.z}) {
        std::uint32_t expected;row>>std::hex>>expected;
        if(std::bit_cast<std::uint32_t>(value)!=expected)
          throw FormatError("Player armor color differs from unchanged TU1.5 830C0");
      }
      const auto emblem=source_player_emblem_colors(profile,palette);
      for(unsigned channel=0;channel<2;++channel)
        if(std::bit_cast<std::array<std::uint32_t,3>>(emblem[channel])!=
           std::bit_cast<std::array<std::uint32_t,3>>(colors[channel+2]))
          throw FormatError("Team color override changed independent emblem colors");
      ++cases;
    }
    if(!row)throw FormatError("Truncated original player color fixture");
  }
  if(cases!=328)throw FormatError("Original player color coverage changed");
  std::cout<<"PASS "<<cases<<" unchanged TU1.5 color cases: every team, FFA, neutral, signed palette and emblem ownership\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}

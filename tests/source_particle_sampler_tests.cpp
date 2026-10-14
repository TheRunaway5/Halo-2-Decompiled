#include "halo2/particle_material.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv) {try {
  if(argc!=3)throw FormatError("Supply original maps and original sampler records");
  std::ifstream file(argv[2]);if(!file)return 77;std::string line;unsigned cases{};
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string map;std::uint32_t prior;std::array<std::uint32_t,4> expected;
    if(!(row>>map>>std::hex>>prior))throw FormatError("bad sampler reference row");
    for(auto &word:expected)if(!(row>>word))throw FormatError("short original sampler state");
    Cache cache(std::filesystem::path(argv[1])/(map+".map"));
    const auto tag=cache.query_tag({.group=TagGroup::from_string("prt3"),.name="effects\\generic\\distortion\\distortion_ripple",.require_address=true});
    if(!tag)throw FormatError("authored sampler particle missing");
    const auto material=cache.load_particle_material(tag->datum,15,0,0);
    const std::vector<SourceParticleSamplerWrite> writes{{1,22,0x40a00000},{1,23,0},{1,25,0},{1,25,0x40a00000}};
    if(material.sampler_writes!=writes || material.textures[1])throw FormatError("original authored sampler write order or deferred external binding differs");
    SourceParticleSamplerBank bank;for(auto &stage:bank)stage.fill(prior);
    const auto before=bank;source_apply_particle_sampler_writes(bank,material.sampler_writes);
    if(bank[1]!=expected || bank[0]!=before[0] || bank[2]!=before[2] || bank[3]!=before[3])
      throw FormatError("native sampler bank differs from whole original18540");
    const auto matrix=source_particle_bump_matrix(bank,1);
    const std::array<unsigned,4> swizzle{0,1,3,2};
    for(unsigned i=0;i<4;++i)if(std::bit_cast<std::uint32_t>(matrix[i])!=expected[swizzle[i]])
      throw FormatError("SDK method to NV2A uniform ordering differs");
    auto invalid=writes;invalid.push_back({4,22,0});const auto retained=bank;bool rejected{};
    try{source_apply_particle_sampler_writes(bank,invalid);}catch(const FormatError&){rejected=true;}
    if(!rejected || bank!=retained)throw FormatError("invalid sampler writes changed retained state");
    ++cases;
  }
  if(cases!=6)throw FormatError("incomplete original sampler reference bank");
  std::cout<<"PASS "<<cases<<" whole-original authored sampler cases, ordered duplicates, retained state, unrelated stages and NV2A uniform ordering\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

#include "halo2/particle_model.hpp"
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=4)throw FormatError("Supply MAP INPUTS OUTPUTS");
  Cache cache(argv[1]);const auto model=load_particle_model(cache,0xe95b07d6u);
  std::ifstream input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);
  const auto read=[&](auto &v){input.read(reinterpret_cast<char*>(&v),sizeof(v));if(!input)throw FormatError("Truncated model fixture");};
  std::uint32_t count{};read(count);
  for(unsigned n=0;n<count;++n) {
    ParticleSpriteInput p;read(p.values);read(p.position);read(p.direction);read(p.scale);std::uint16_t random{};read(random);
    ParticleValueCache values{p.values,0xffffffffu};ParticleValueSources sources;
    const auto s=particle_model_submission(model,p,random,values,sources,*cache.shader_function_tables());
    output.write(reinterpret_cast<const char*>(s.transform.data()),sizeof(s.transform));
    const std::array<std::uint32_t,2> span{s.first,s.count};output.write(reinterpret_cast<const char*>(span.data()),sizeof(span));
  }
  if(!output)throw FormatError("Could not write model outputs");
  std::cout<<count<<" authored PRTM submissions\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

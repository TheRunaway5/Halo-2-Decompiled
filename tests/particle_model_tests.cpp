#include "halo2/particle_model.hpp"
#include <fstream>
#include <iostream>
#include <string_view>
using namespace halo2;
int main(int argc,char **argv)try {
  const bool mcc=argc==6&&std::string_view(argv[4])=="native-mcc";
  if(argc!=4&&!mcc)throw FormatError("Supply MAP INPUTS OUTPUTS [native-mcc DATUM]");
  Cache cache(argv[1]);
  if(mcc&&!cache.is_mcc())throw FormatError("TU1.5 model validation requires prepared MCC content");
  unsigned long selected=0xe95b07d6u;
  if(mcc){std::size_t end{};selected=std::stoul(argv[5],&end,0);if(end!=std::string_view(argv[5]).size()||selected>0xfffffffful)throw FormatError("Invalid complete MCC particle model datum");}
  const auto model=load_particle_model(cache,std::uint32_t(selected));
  std::ifstream input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);
  const auto read=[&](auto &v){input.read(reinterpret_cast<char*>(&v),sizeof(v));if(!input)throw FormatError("Truncated model fixture");};
  std::uint32_t count{};read(count);
  for(unsigned n=0;n<count;++n) {
    ParticleSpriteInput p;read(p.values);read(p.position);read(p.direction);read(p.scale);std::uint16_t random{};read(random);
    ParticleValueCache values{p.values,0xffffffffu};ParticleValueSources sources;
    if(mcc){read(sources.camera_forward);read(sources.camera_up);}
    const auto s=particle_model_submission(model,p,random,values,sources,*cache.shader_function_tables());
    output.write(reinterpret_cast<const char*>(s.transform.data()),sizeof(s.transform));
    const std::array<std::uint32_t,2> span{s.first,s.count};output.write(reinterpret_cast<const char*>(span.data()),sizeof(span));
  }
  if(!output)throw FormatError("Could not write model outputs");
  std::cout<<count<<" authored PRTM submissions\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
